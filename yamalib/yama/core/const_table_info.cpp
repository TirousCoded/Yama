

#include "const_table_info.h"


std::string yama::int_const_info::fmt(const const_table_info&) const {
    return std::format("({}) {}", int_const, fmt_int(v));
}

std::string yama::uint_const_info::fmt(const const_table_info&) const {
    return std::format("({}) {}", uint_const, fmt_uint(v));
}

std::string yama::float_const_info::fmt(const const_table_info&) const {
    return std::format("({}) {}", float_const, fmt_float(v));
}

std::string yama::bool_const_info::fmt(const const_table_info&) const {
    return std::format("({}) {}", bool_const, fmt_bool(v));
}

std::string yama::char_const_info::fmt(const const_table_info&) const {
    return std::format("({}) {}", char_const, fmt_char(v));
}

yama::kind yama::primitive_type_const_info::kind() const noexcept {
    return yama::kind::primitive;
}

std::string yama::primitive_type_const_info::fmt(const const_table_info&) const {
    return std::format("({}) {}", primitive_type_const, fullname);
}

yama::kind yama::function_type_const_info::kind() const noexcept {
    return yama::kind::function;
}

std::string yama::function_type_const_info::fmt(const const_table_info& consts) const {
    return std::format("({}) {} [{}]", function_type_const, fullname, callsig.fmt(consts));
}

std::string yama::const_table_info::fmt_info(const info& x, const const_table_info& consts) {
    static_assert(const_types == 7);
    std::string result{};
    switch (yama::const_type(x.index())) {
    case int_const:             result = std::get<int_const_info>(x).fmt(consts);               break;
    case uint_const:            result = std::get<uint_const_info>(x).fmt(consts);              break;
    case float_const:           result = std::get<float_const_info>(x).fmt(consts);             break;
    case bool_const:            result = std::get<bool_const_info>(x).fmt(consts);              break;
    case char_const:            result = std::get<char_const_info>(x).fmt(consts);              break;
    case primitive_type_const:  result = std::get<primitive_type_const_info>(x).fmt(consts);    break;
    case function_type_const:   result = std::get<function_type_const_info>(x).fmt(consts);     break;
    default:                    YAMA_DEADEND;                                                   break;
    }
    return result;
}

std::optional<yama::const_type> yama::const_table_info::const_type(const_t x) const noexcept {
    return
        x < consts.size()
        ? std::make_optional(yama::const_type(consts[x].index()))
        : std::nullopt;
}

std::optional<yama::kind> yama::const_table_info::kind(const_t x) const noexcept {
    static_assert(const_types == 7);
    const auto t = const_type(x);
    if (t == primitive_type_const)      return std::make_optional(get<primitive_type_const>(x)->kind());
    else if (t == function_type_const)  return std::make_optional(get<function_type_const>(x)->kind());
    else                                return std::nullopt;
}

std::optional<yama::str> yama::const_table_info::fullname(const_t x) const noexcept {
    static_assert(const_types == 7);
    const auto t = const_type(x);
    if (t == primitive_type_const)      return std::make_optional(get<primitive_type_const>(x)->fullname);
    else if (t == function_type_const)  return std::make_optional(get<function_type_const>(x)->fullname);
    else                                return std::nullopt;
}

const yama::callsig_info* yama::const_table_info::callsig(const_t x) const noexcept {
    static_assert(const_types == 7);
    const auto t = const_type(x);
    if (t == function_type_const)   return &get<function_type_const>(x)->callsig;
    else                            return nullptr;
}

std::string yama::const_table_info::fmt_const(const_t x) const {
    return
        x < consts.size()
        ? fmt_info(consts[x], *this)
        : std::format("<illegal({})>", x);
}

std::string yama::const_table_info::fmt_type_const(const_t x) const {
    const auto fullname = this->fullname(x);
    if (!fullname)                              return std::format("<illegal({})>", x);
    else if (!is_type_const(*const_type(x)))    return std::format("<illegal({})>", x);
    else                                        return fullname->fmt();
}

std::string yama::const_table_info::fmt(const char* tab) const {
    YAMA_ASSERT(tab);
    std::string result{};
    result += std::format("const_table_info ({} consts)", consts.size());
    for (const_t i = 0; i < size(); i++) {
        result += std::format("\n{}[{}] {}", tab, i, fmt_const(i));
    }
    return result;
}

yama::const_table_info& yama::const_table_info::add_int(int_t v) {
    int_const_info a{
        .v = v,
    };
    consts.push_back(a);
    return *this;
}

yama::const_table_info& yama::const_table_info::add_uint(uint_t v) {
    uint_const_info a{
        .v = v,
    };
    consts.push_back(a);
    return *this;
}

yama::const_table_info& yama::const_table_info::add_float(float_t v) {
    float_const_info a{
        .v = v,
    };
    consts.push_back(a);
    return *this;
}

yama::const_table_info& yama::const_table_info::add_bool(bool_t v) {
    bool_const_info a{
        .v = v,
    };
    consts.push_back(a);
    return *this;
}

yama::const_table_info& yama::const_table_info::add_char(char_t v) {
    char_const_info a{
        .v = v,
    };
    consts.push_back(a);
    return *this;
}

yama::const_table_info& yama::const_table_info::add_primitive_type(str fullname) {
    primitive_type_const_info a{
        .fullname = fullname,
    };
    consts.push_back(a);
    return *this;
}

yama::const_table_info& yama::const_table_info::add_function_type(str fullname, callsig_info callsig) {
    function_type_const_info a{
        .fullname = fullname,
        .callsig = std::move(callsig),
    };
    consts.push_back(a);
    return *this;
}

