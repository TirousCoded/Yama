

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

yama::links_view yama::type::links() const noexcept {
    return links_view{ _mem };
}

std::span<const yama::linksym> yama::type::linksyms() const noexcept {
    return _mem->data.linksyms();
}

bool yama::type::operator==(const type& other) const noexcept {
    return _mem == other._mem;
}

yama::type::type(internal::type_mem mem) noexcept 
    : _mem(std::move(mem)) {}
