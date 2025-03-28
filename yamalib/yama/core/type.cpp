

#include "type.h"

#include "kind-features.h"
#include "callsig.h"
#include "const_table.h"


yama::internal::type_mem yama::internal::get_type_mem(type x) noexcept {
    return x._mem;
}

yama::type::type(const internal::type_instance& instance) noexcept
    : _mem(instance._mem) {}

bool yama::type::complete() const noexcept {
    return _mem->stubs == 0;
}

yama::str yama::type::fullname() const noexcept {
    return _mem->fullname;
}

yama::kind yama::type::kind() const noexcept {
    return _mem->kind;
}

yama::const_table yama::type::consts() const noexcept {
    return const_table(_mem);
}

std::optional<yama::ptype> yama::type::ptype() const noexcept {
    return _mem->info->ptype();
}

std::optional<yama::callsig> yama::type::callsig() const noexcept {
    const auto ptr = _mem->info->callsig();
    return
        ptr
        ? std::make_optional(yama::callsig(*ptr, consts()))
        : std::nullopt;
}

std::optional<yama::call_fn> yama::type::call_fn() const noexcept {
    return _mem->info->call_fn();
}

size_t yama::type::max_locals() const noexcept {
    return _mem->info->max_locals();
}

std::string yama::type::fmt() const {
    return fullname().fmt();
}

yama::type::type(internal::type_mem mem) noexcept 
    : _mem(mem) {}

