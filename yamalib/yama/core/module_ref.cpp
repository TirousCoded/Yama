

#include "module_ref.h"

#include "../internals/imported_module.h"


yama::module_ref yama::internal::create_module(const imported_module& im) {
    return yama::module_ref(im);
}

const yama::module& yama::module_ref::info() const noexcept {
    return *_m;
}

yama::mid_t yama::module_ref::id() const noexcept {
    return _id;
}

size_t yama::module_ref::count() const noexcept {
    return info().count();
}

bool yama::module_ref::exists(const str& name) const noexcept {
    return info().exists(name);
}

yama::module_ref::module_ref(const internal::imported_module& im)
    : _m(*im.m),
    _id(im.id) {}

