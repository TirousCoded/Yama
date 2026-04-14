

#include "PTableManager.h"


std::optional<const ym::Safe<YmType>*> _ym::PTableManager::fetch(YmType& proto, YmType& boxed) const noexcept {
    if (auto it = _ptables.find(_mkKey(proto, boxed)); it != _ptables.end()) {
        return it->second.data();
    }
    return std::nullopt;
}

std::optional<const ym::Safe<YmType>*> _ym::PTableManager::load(YmType& proto, YmType& boxed) {
    if (auto result = fetch(proto, boxed)) {
        return result;
    }
    return _generate(proto, boxed);
}

std::optional<const ym::Safe<YmType>*> _ym::PTableManager::_generate(YmType& proto, YmType& boxed) {
    ymAssert(!fetch(proto, boxed));
    if (!boxed.conforms(proto)) {
        return std::nullopt; // If doesn't conform, abort.
    }
    std::vector<ym::Safe<YmType>> ptable{};
    ptable.reserve(proto.members());
    for (YmMemberIndex i = 0; i < proto.members(); i++) {
        auto& localName = ym::deref(proto.member(i)).localName();
        const auto [_, memberName] = _ym::split_s<YmChar>(localName, "::");
        // TODO: This std::string alloc is suboptimal.
        ptable.push_back(ym::Safe(boxed.member((std::string)memberName)));
    }
    return _ptables.try_emplace(_mkKey(proto, boxed), std::move(ptable)).first->second.data();
}

_ym::PTableManager::_Key _ym::PTableManager::_mkKey(YmType& proto, YmType& boxed) noexcept {
    return std::make_pair(ym::Safe(proto), ym::Safe(boxed));
}

