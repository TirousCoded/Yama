

#include "type_data.h"


yama::str yama::type_data::fullname() const noexcept {
    return _info->fullname;
}

std::span<const yama::linksym> yama::type_data::linksyms() const noexcept {
    return std::span(_info->linksyms);
}

const std::optional<yama::callsig_info>& yama::type_data::callsig() const noexcept {
    return _info->callsig;
}

yama::kind yama::type_data::kind() const noexcept {
    return _kind;
}

bool yama::type_data::verified() const noexcept {
    return _other->verified;
}

