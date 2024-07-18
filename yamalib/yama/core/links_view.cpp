

#include "links_view.h"

#include "type.h"


size_t yama::links_view::size() const noexcept {
    return _mem->length();
}

std::optional<yama::type> yama::links_view::link(link_index index) const noexcept {
    if (index >= _mem.elems().size()) return std::nullopt;
    auto value = _mem.elems()[index];
    return
        value
        ? std::make_optional(type(internal::type_mem::from_anon_ref(value)))
        : std::nullopt;
}

std::optional<yama::type> yama::links_view::operator[](link_index index) const noexcept {
    return link(index);
}

