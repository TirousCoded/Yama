

#include "type_data.h"


yama::type_data::type_data(type_data&& other) noexcept 
    : _info(std::move(other._info)), 
    _kind(std::move(other._kind)) {}

yama::type_data& yama::type_data::operator=(type_data&& other) noexcept {
    if (&other == this) return *this;
    _info = std::move(other._info);
    _kind = std::move(other._kind);
    return *this;
}

yama::str yama::type_data::fullname() const noexcept {
    return _info->fullname;
}

std::span<const yama::linksym> yama::type_data::linksyms() const noexcept {
    return std::span(_info->linksyms);
}

yama::kind yama::type_data::kind() const noexcept {
    return _kind;
}

