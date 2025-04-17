

#include "callsig.h"

#include "type.h"


yama::callsig::callsig(const callsig_info& info, const_table consts) 
    : _info(info), 
    _consts(consts) {}

const yama::callsig_info& yama::callsig::info() const noexcept {
    return *_info;
}

size_t yama::callsig::params() const noexcept {
    return info().params.size();
}

std::optional<yama::type> yama::callsig::param_type(size_t index) const noexcept {
    return
        index < info().params.size()
        ? _consts.type(info().params[index])
        : std::nullopt;
}

std::optional<yama::type> yama::callsig::return_type() const noexcept {
    return _consts.type(info().ret);
}

bool yama::callsig::operator==(const callsig& other) const noexcept {
    if (params() != other.params()) return false;
    for (size_t i = 0; i < params(); i++) {
        if (param_type(i) != other.param_type(i)) return false;
    }
    if (return_type() != other.return_type()) return false;
    return true;
}

std::string yama::callsig::fmt() const {
    std::string params_s{};
    bool first = true;
    for (size_t i = 0; i < params(); i++) {
        if (!first) {
            params_s += ", ";
        }
        const auto t = param_type(i);
        params_s += t ? t->fullname().fmt() : "<unknown>";
        first = false;
    }
    const auto t = return_type();
    std::string ret_s = t ? t->fullname().fmt() : "<unknown>";
    return std::format("fn({}) -> {}", params_s, ret_s);
}

