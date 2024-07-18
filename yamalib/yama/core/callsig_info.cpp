

#include "callsig_info.h"


bool yama::callsig_info::verify_indices(std::span<const linksym> linksyms) const noexcept {
    for (const auto& I : params) {
        if (I >= linksyms.size()) return false;
    }
    if (ret >= linksyms.size()) return false;
    return true;
}

bool yama::callsig_info::operator==(const callsig_info& other) const noexcept {
    return
        params == other.params &&
        ret == other.ret;
}

std::string yama::callsig_info::fmt(std::span<const linksym> linksyms) const {
    std::string params_s{};
    bool first = true;
    for (const auto& I : params) {
        if (!first) {
            params_s += ", ";
        }
        params_s += _fmt_index(linksyms, I);
        first = false;
    }
    std::string ret_s = _fmt_index(linksyms, ret);
    return std::format("fn({}) -> {}", params_s, ret_s);
}

std::string yama::callsig_info::_fmt_index(std::span<const linksym> linksyms, link_index index) const {
    return
        index < linksyms.size()
        ? linksyms[index].fullname.fmt()
        : std::format("<out-of-bounds({})>", index);
}

