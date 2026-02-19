

#include "PathBindings.h"


std::shared_ptr<YmParcel> _ym::PathBindings::get(const Spec& path) const {
    path.assertPath();
    if (auto it = _bindings.find(path); it != _bindings.end()) {
        return it->second;
    }
    return nullptr;
}

void _ym::PathBindings::set(const Spec& path, std::shared_ptr<YmParcel> x) {
    ymAssert(x != nullptr);
    _bindings[path] = std::move(x); // Overwrite existing, if any.
}

void _ym::PathBindings::reset() noexcept {
    _bindings.clear();
}

