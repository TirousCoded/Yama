

#include "type.h"

#include "kind-features.h"


bool yama::type::complete() const noexcept {
    return _mem->stubs == 0;
}

yama::str yama::type::fullname() const noexcept {
    return _mem->fullname;
}

yama::links_view yama::type::links() const noexcept {
    return links_view{ _mem };
}

yama::kind yama::type::kind() const noexcept {
    return _mem->kind;
}

std::optional<yama::ptype> yama::type::ptype() const noexcept {
    return _mem->info->ptype();
}

std::optional<yama::callsig> yama::type::callsig() const noexcept {
    const auto ptr = _mem->info->callsig();
    return
        ptr
        ? std::make_optional(yama::callsig(*ptr, links()))
        : std::nullopt;
}

std::optional<yama::call_fn> yama::type::call_fn() const noexcept {
    return _mem->info->call_fn();
}

size_t yama::type::locals() const noexcept {
    return _mem->info->locals();
}

std::string yama::type::fmt() const {
    return fullname().fmt();
}

yama::type::type(internal::type_mem mem) noexcept 
    : _mem(std::move(mem)) {}

