

#include "ParcelInfo.h"

#include "general.h"
#include "../yama++/general.h"


bool _ym::checkRefSymbol(const std::string& symbol, std::string_view msg) {
    if (_ym::Global::refSymIsLegal(symbol)) {
        return true;
    }
    _ym::Global::raiseErr(
        YmErrCode_IllegalRefSym,
        "{}; reference symbol \"{}\" is illegal!",
        (std::string)msg,
        symbol);
    return false;
}

bool _ym::checkCallable(const ItemInfo& item, std::string_view msg) {
    bool result = ymKind_IsCallable(item.kind);
    if (!result) {
        Global::raiseErr(
            YmErrCode_NonCallableItem,
            "{}; item {} (index {}) is non-callable!",
            (std::string)msg,
            item.localName,
            item.index);
    }
    return result;
}

bool _ym::ItemInfo::returnTypeIsSelf() const noexcept {
    return
        returnType
        ? consts[*returnType].as<RefInfo>().sym == "Self"
        : false;
}

bool _ym::ItemInfo::paramTypeIsSelf(YmParamIndex index) const noexcept {
    return
        size_t(index) < params.size()
        ? consts[params[index].type].as<RefInfo>().sym == "Self"
        : false;
}

bool _ym::ItemInfo::isOwner() const noexcept {
    return !memberName();
}

std::optional<std::string_view> _ym::ItemInfo::ownerName() const noexcept {
    const auto [owner, member] = split_s<YmChar>(localName, "::");
    return
        !member.empty() // If empty, then no owner/member division.
        ? std::make_optional(owner)
        : std::nullopt;
}

std::optional<std::string_view> _ym::ItemInfo::memberName() const noexcept {
    const auto [owner, member] = split_s<YmChar>(localName, "::");
    return
        !member.empty() // If empty, then no owner/member division.
        ? std::make_optional(member)
        : std::nullopt;
}

const _ym::ParamInfo* _ym::ItemInfo::queryParam(YmParamIndex index) const noexcept {
    return
        size_t(index) < params.size()
        ? &params[size_t(index)]
        : nullptr;
}

const _ym::ParamInfo* _ym::ItemInfo::queryParam(const std::string& localName) const noexcept {
    // NOTE: Due to strict cap of YM_MAX_PARAMS it should be fine to do an O(n) search.
    for (const auto& param : params) {
        if (param.name == localName) return &param;
    }
    return nullptr;
}

YmMembers _ym::ItemInfo::memberCount() const noexcept {
    return (YmUInt16)membersByIndex.size();
}

std::optional<YmParamIndex> _ym::ItemInfo::addParam(std::string name, std::string paramTypeSymbol) {
    if (!checkRefSymbol(paramTypeSymbol, "Cannot add parameter")) {
        return std::nullopt;
    }
    if (!checkCallable(*this, "Cannot add parameter")) {
        return std::nullopt;
    }
    if (queryParam(name)) {
        Global::raiseErr(
            YmErrCode_ParamNameConflict,
            "Cannot add parameter; name \"{}\" already taken!",
            name);
        return std::nullopt;
    }
    if (params.size() >= size_t(YM_MAX_PARAMS)) {
        Global::raiseErr(
            YmErrCode_MaxParamsLimit,
            "Cannot add parameter; would exceed {} limit!",
            YM_MAX_PARAMS);
        return std::nullopt;
    }
    params.push_back(ParamInfo{
        .index = YmParamIndex(params.size()),
        .name = std::move(name),
        .type = consts.pullRef(std::move(paramTypeSymbol)).value(),
        });
    return params.back().index;
}

std::optional<YmRef> _ym::ItemInfo::addRef(std::string symbol) {
    if (!checkRefSymbol(symbol, "Cannot add reference")) {
        return std::nullopt;
    }
    if (auto result = consts.pullRef(std::move(symbol), size_t(YmRef(-1)))) {
        return (YmRef)result.value();
    }
    _ym::Global::raiseErr(
        YmErrCode_InternalError,
        "Cannot add reference; internal failure!");
    return std::nullopt;
}

void _ym::ItemInfo::attemptSetupAsMember(ParcelInfo& parcel) {
    if (isOwner()) {
        return;
    }
    if (auto owner = parcel.item((std::string)ownerName().value())) {
        // Bind this->owner to a ref to our new owner.
        this->owner = consts.pullRef(owner->fullnameForRef()).value();
        // Pull ref constant of *this for our owner to be setup w/.
        auto ref = owner->consts.pullRef(fullnameForRef()).value();
        // Bind ref to by-index/name lookup in *owner.
        owner->membersByIndex.push_back(ref);
        owner->membersByName.try_emplace((std::string)memberName().value(), std::move(ref)); // Move ref.
    }
}

std::string _ym::ItemInfo::fullnameForRef() const {
    return std::format("{}:{}", (std::string)refSymHere, (std::string)localName);
}

bool _ym::ParcelInfo::verify() const {
    bool success = true;
    for (const auto& item : _items) {
        continue; // TODO: Add verif. checks when needed.
        success = false;
    }
    return success;
}

size_t _ym::ParcelInfo::items() const noexcept {
    return _items.size();
}

_ym::ItemInfo* _ym::ParcelInfo::item(const std::string& localName) noexcept {
    const auto it = _lookup.find(localName);
    return
        it != _lookup.end()
        ? &_items[it->second]
        : nullptr;
}

const _ym::ItemInfo* _ym::ParcelInfo::item(const std::string& localName) const noexcept {
    const auto it = _lookup.find(localName);
    return
        it != _lookup.end()
        ? &_items[it->second]
        : nullptr;
}

_ym::ItemInfo* _ym::ParcelInfo::item(YmItemIndex index) noexcept {
    return
        index < YmItemIndex(items())
        ? &_items[index]
        : nullptr;
}

const _ym::ItemInfo* _ym::ParcelInfo::item(YmItemIndex index) const noexcept {
    return
        index < YmItemIndex(items())
        ? &_items[index]
        : nullptr;
}

std::optional<YmItemIndex> _ym::ParcelInfo::addItem(
    std::string localName,
    YmKind kind,
    std::optional<std::string> returnTypeSymbol,
    std::optional<CallBhvrCallbackInfo> callBehaviour) {
    if (item(localName)) {
        Global::raiseErr(
            YmErrCode_ItemNameConflict,
            "Cannot add item; name \"{}\" already taken!",
            localName);
        return std::nullopt;
    }
    ItemInfo newItem{
        .index = (YmItemIndex)items(),
        .localName = localName,
        .kind = kind,
    };
    if (returnTypeSymbol) {
        ymAssert(ymKind_IsCallable(newItem.kind));
        // TODO: This error msg is *clunky*, improve it.
        if (!checkRefSymbol(*returnTypeSymbol, "Cannot add item; invalid return type symbol")) {
            return std::nullopt;
        }
        newItem.returnType = newItem.consts.pullRef(std::move(*returnTypeSymbol));
    }
    newItem.callBehaviour = callBehaviour;
    // NOTE: Make sure error checks stay ABOVE mutations of *this.
    _items.push_back(std::move(newItem));
    auto& result = _items.back();
    // NOTE: Move localName into _lookup to avoid heap alloc.
    _lookup.try_emplace(std::move(localName), result.index);
    result.attemptSetupAsMember(*this);
    return result.index;
}

std::optional<YmItemIndex> _ym::ParcelInfo::addItem(
    YmItemIndex owner,
    std::string memberName,
    YmKind kind,
    std::optional<std::string> returnTypeSymbol,
    std::optional<CallBhvrCallbackInfo> callBehaviour) {
    auto ownerItemPtr = item(owner);
    if (!ownerItemPtr) {
        // TODO: Maybe look into adding more context to this error msg.
        Global::raiseErr(
            YmErrCode_ItemNotFound,
            "Cannot add item; no owner item found at index {}!",
            owner);
        return std::nullopt;
    }
    auto& ownerItem = ym::deref(ownerItemPtr);
    if (!ymKind_HasMembers(ownerItem.kind)) {
        Global::raiseErr(
            YmErrCode_ItemCannotHaveMembers,
            "Cannot add item; owner {} is a {} which cannot have members!",
            ownerItem.localName,
            ymFmtKind(ownerItem.kind));
        return std::nullopt;
    }
    if (kind == YmKind_Method) {
        ymAssert(callBehaviour.has_value());
        bool isMethodReq = callBehaviour.value().fn == methodReqCallBhvr;
        bool ownerIsProtocol = ownerItem.kind == YmKind_Protocol;
        if (!isMethodReq && ownerIsProtocol) {
            Global::raiseErr(
                YmErrCode_ProtocolItem,
                "Cannot add regular method to {} item {}!",
                ymFmtKind(ownerItem.kind),
                ownerItem.localName);
            return std::nullopt;
        }
        else if (isMethodReq && !ownerIsProtocol) {
            Global::raiseErr(
                YmErrCode_NonProtocolItem,
                "Cannot add method req. to {} item {}!",
                ymFmtKind(ownerItem.kind),
                ownerItem.localName);
            return std::nullopt;
        }
    }
    return addItem(
        std::format("{}::{}", ownerItem.localName, memberName),
        kind,
        std::move(returnTypeSymbol),
        callBehaviour);
}

std::optional<YmParamIndex> _ym::ParcelInfo::addParam(YmItemIndex item, std::string name, std::string paramTypeSymbol) {
    auto info = _expectItem(item, "Cannot add parameter");
    return
        info
        ? info->addParam(std::move(name), std::move(paramTypeSymbol))
        : std::nullopt;
}

std::optional<YmRef> _ym::ParcelInfo::addRef(YmItemIndex item, std::string symbol) {
    auto info = _expectItem(item, "Cannot add reference");
    return
        info
        ? info->addRef(std::move(symbol))
        : std::nullopt;
}

_ym::ItemInfo* _ym::ParcelInfo::_expectItem(YmItemIndex index, std::string_view msg) {
    if (_ym::ItemInfo* result = item(index)) {
        return result;
    }
    _ym::Global::raiseErr(
        YmErrCode_ItemNotFound,
        "{}; no item found at index {}!",
        (std::string)msg,
        index);
    return nullptr;
}

