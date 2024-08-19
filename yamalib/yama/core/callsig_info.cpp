

#include "callsig_info.h"

#include "const_table_info.h"


bool yama::callsig_info::operator==(const callsig_info& other) const noexcept {
    return
        params == other.params &&
        ret == other.ret;
}

std::string yama::callsig_info::fmt(const const_table_info& consts) const {
    std::string params_s{};
    bool first = true;
    for (const auto& I : params) {
        if (!first) {
            params_s += ", ";
        }
        params_s += consts.fmt_type_const(I);
        first = false;
    }
    std::string ret_s = consts.fmt_type_const(ret);
    return std::format("fn({}) -> {}", params_s, ret_s);
}

