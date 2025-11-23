

#include "YmItem.h"

#include "general.h"


std::string_view YmItem::path() const noexcept {
    const auto [path, localName] = _ym::split_s<YmChar>(fullname, ":");
    ymAssert(!localName.empty());
    return path;
}

std::string_view YmItem::localName() const noexcept {
    const auto [path, localName] = _ym::split_s<YmChar>(fullname, ":");
    ymAssert(!localName.empty());
    return localName;
}

std::span<const _ym::Const> YmItem::consts() const noexcept {
    return std::span(_consts);
}

void YmItem::resolveConsts() {
    ymAssert(_consts.empty());
    _consts.reserve(info->consts.size());
    for (size_t i = 0; i < info->consts.size(); i++) {
        // TODO: This won't work later when ConstInfo and Const aren't
        //       aliases of the same type anymore.
        _consts.push_back(info->consts[i]);
    }
}

