

#include "type.h"


size_t yama::type::links_view::size() const noexcept {
    return _mem->length();
}

std::optional<yama::type> yama::type::links_view::link(link_index index) const noexcept {
    if (index >= _mem.elems().size()) return std::nullopt;
    auto value = _mem.elems()[index];
    return
        value
        ? std::make_optional(type(internal::type_mem::from_anon_ref(value)))
        : std::nullopt;
}

std::optional<yama::type> yama::type::links_view::operator[](link_index index) const noexcept {
    return link(index);
}

yama::type::type(type&& other) noexcept 
    : _mem(std::move(other._mem)) {}

yama::type& yama::type::operator=(type&& other) noexcept {
    if (this == &other) return *this;
    _mem = std::move(other._mem);
    return *this;
}

bool yama::type::complete() const noexcept {
    return _mem->stubs == 0;
}

yama::str yama::type::fullname() const noexcept {
    return _mem->fullname;
}

yama::kind yama::type::kind() const noexcept {
    return _mem->kind;
}

yama::type::links_view yama::type::links() const noexcept {
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

