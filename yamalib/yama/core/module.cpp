

#include "module.h"


yama::module yama::internal::create_module(const module_info* ptr) {
    return yama::module(ptr);
}

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

