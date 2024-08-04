

#include "type.h"


bool yama::type::complete() const noexcept {
    return _mem->stubs == 0;
}

yama::str yama::type::fullname() const noexcept {
    return _mem->fullname;
}

yama::kind yama::type::kind() const noexcept {
    return _mem->kind;
}

std::optional<yama::callsig> yama::type::callsig() const noexcept {
    const auto& info = _mem->data.callsig();
    const auto links_v = links();
    return
        info
        ? std::make_optional(yama::callsig(*info, links_v))
        : std::nullopt;
}

std::optional<yama::ptype> yama::type::ptype() const noexcept {
    return
        kind() == kind::primitive
        ? std::make_optional(_mem->data.info<primitive_info>().ptype)
        : std::nullopt;
}

size_t yama::type::max_locals() const noexcept {
    return
        kind() == kind::function
        ? _mem->data.info<function_info>().max_locals
        : 0;
}

std::optional<yama::call_fn> yama::type::call_fn() const noexcept {
    return
        kind() == kind::function
        ? std::make_optional(_mem->data.info<function_info>().cf)
        : std::nullopt;
}

yama::links_view yama::type::links() const noexcept {
    return links_view{ _mem };
}

std::string yama::type::fmt() const {
    return fullname().fmt();
}

yama::type::type(internal::type_mem mem) noexcept 
    : _mem(std::move(mem)) {}

