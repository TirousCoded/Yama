

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
            "Cannot add constant symbol to {} (index {}); would exceed {} limit!",
            localName,
            index,
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

std::optional<YmItemIndex> _ym::ParcelInfo::addItem(std::string localName, YmKind kind) {
    if (item(localName)) {
        _ym::Global::raiseErr(
            YmErrCode_ItemNameConflict,
            "Cannot add item \"{}\"; name conflict!",
            localName);
        return std::nullopt;
    }
    const YmItemIndex index((YmItemIndex)items());
    _items.push_back(ItemInfo{
        .index = index,
        .localName = localName,
        .kind = kind,
        });
    // NOTE: Move localName into _lookup to avoid heap alloc.
    _lookup.try_emplace(std::move(localName), index);
    return index;
}

YmConst _ym::ParcelInfo::pullConst(YmItemIndex item, ConstInfo sym) {
    if (sym.is<RefConstInfo>() && !Global::fullnameIsLegal(sym.as<RefConstInfo>().sym)) {
        Global::raiseErr(
            YmErrCode_IllegalFullname,
            "Cannot add constant symbol; fullname \"{}\" is illegal!",
            sym.as<RefConstInfo>().sym);
        return YM_NO_CONST;
    }
    if (auto found = this->item(item)) {
        return found->pullConst(std::move(sym));
    }
    Global::raiseErr(
        YmErrCode_ItemNotFound,
        "Cannot add constant symbol; item at index {} not found!",
        item);
    return YM_NO_CONST;
}

