

#include "type.h"

#include "kind-features.h"
#include "callsig.h"
#include "const_table.h"

#include "../internals/type_instance.h"


yama::internal::type_mem yama::internal::get_type_mem(type x) noexcept {
    return x._mem;
}

yama::type yama::internal::create_type(const type_instance& x) noexcept {
    return type(x);
}

const yama::type_info& yama::type::info() const noexcept {
    return *_mem->info;
}

yama::str yama::type::fullname() const noexcept {
    return _mem->fullname;
}

yama::kind yama::type::kind() const noexcept {
    return _mem->kind;
}

yama::const_table yama::type::consts() const noexcept {
    return *this;
}

std::optional<yama::ptype> yama::type::ptype() const noexcept {
    return info().ptype();
}

std::optional<yama::callsig> yama::type::callsig() const noexcept {
    const auto ptr = info().callsig();
    return
        ptr
        ? std::make_optional(yama::callsig(*ptr, consts()))
        : std::nullopt;
}

std::optional<yama::call_fn> yama::type::call_fn() const noexcept {
    return info().call_fn();
}

size_t yama::type::max_locals() const noexcept {
    return info().max_locals();
}

std::string yama::type::fmt() const {
    return fullname().fmt();
}

yama::type::type(const internal::type_instance& instance) noexcept
    : _mem(instance._mem) {}

