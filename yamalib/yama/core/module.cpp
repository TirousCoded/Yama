

#include "module.h"


yama::module_info yama::module::info() const {
    return deref_assert(_ptr);
}

size_t yama::module::size() const noexcept {
    return deref_assert(_ptr).size();
}

bool yama::module::contains(const str& name) const noexcept {
    return deref_assert(_ptr).contains(name);
}

yama::module::module(const module_info* ptr)
    : _ptr(ptr) {
    YAMA_ASSERT(ptr);
}

