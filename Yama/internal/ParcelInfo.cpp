

#include "ParcelInfo.h"

#include "general.h"


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

YmConst _ym::ItemInfo::queryConst(const ConstInfo& sym) const noexcept {
    for (YmWord i = 0; i < consts.size(); i++) {
        if (consts[i] == sym) {
            return YmConst(i);
        }
    }
    return YM_NO_CONST;
}

YmConst _ym::ItemInfo::pullConst(ConstInfo sym) {
    if (YmConst index = queryConst(sym); index != YM_NO_CONST) {
        return index;
    }
    if (consts.size() > YmWord(YM_MAX_CONST)) {
        _ym::Global::raiseErr(
            YmErrCode_MaxConstsLimit,
            "Cannot add constant symbol to {} (LID {}); would exceed {} limit!",
            localName,
            lid,
            YM_MAX_CONST);
        return YM_NO_CONST;
    }
    consts.push_back(std::move(sym));
    return YmConst(consts.size() - 1);
}

YmWord _ym::ParcelInfo::items() const noexcept {
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

_ym::ItemInfo* _ym::ParcelInfo::item(YmLID lid) noexcept {
    return
        lid < YmLID(items())
        ? &_items[lid]
        : nullptr;
}

const _ym::ItemInfo* _ym::ParcelInfo::item(YmLID lid) const noexcept {
    return
        lid < YmLID(items())
        ? &_items[lid]
        : nullptr;
}

std::optional<YmLID> _ym::ParcelInfo::addItem(std::string localName, YmKind kind) {
    if (item(localName)) {
        _ym::Global::raiseErr(
            YmErrCode_ItemNameConflict,
            "Cannot add item \"{}\"; name conflict!",
            localName);
        return std::nullopt;
    }
    const YmLID lid((YmLID)items());
    _items.push_back(ItemInfo{
        .lid = lid,
        .localName = localName,
        .kind = kind,
        });
    // NOTE: Move localName into _lookup to avoid heap alloc.
    _lookup.try_emplace(std::move(localName), lid);
    return lid;
}

YmConst _ym::ParcelInfo::pullConst(YmLID item, ConstInfo sym) {
    if (auto found = this->item(item)) {
        return found->pullConst(std::move(sym));
    }
    _ym::Global::raiseErr(
        YmErrCode_ItemNotFound,
        "Cannot add constant symbol; item with LID {} not found!",
        item);
    return YM_NO_CONST;
}

