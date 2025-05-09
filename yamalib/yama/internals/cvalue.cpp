

#include "cvalue.h"


bool yama::internal::cvalue::is(const ctype& x) const noexcept {
    return t == x;
}

bool yama::internal::cvalue::is(kind x) const noexcept {
    return t.kind() == x;
}

std::optional<yama::internal::ctype> yama::internal::cvalue::to_type() const noexcept {
    return is(kind::function) ? std::make_optional(t) : as<ctype>();
}

std::string yama::internal::cvalue::fmt() const {
    auto _fmt_val = [&]() -> std::string {
        std::string result{};
        if (const auto v = as<stateless_t>())   result = "n/a";
        else if (const auto v = as<int_t>())    result = fmt_int(*v);
        else if (const auto v = as<uint_t>())   result = fmt_uint(*v);
        else if (const auto v = as<float_t>())  result = fmt_float(*v);
        else if (const auto v = as<bool_t>())   result = fmt_bool(*v);
        else if (const auto v = as<char_t>())   result = fmt_char(*v);
        else if (const auto v = as<ctype>())    result = v->fullname().fmt();
        else                                    YAMA_DEADEND;
        return result;
        };
    return std::format("{} ({})", t.fullname(), _fmt_val());
}

yama::internal::cvalue yama::internal::cvalue::none_v(ctypesys_local& types) {
    return cvalue{ .t = types.none_type(), .v = stateless_t{} };
}

yama::internal::cvalue yama::internal::cvalue::int_v(int_t x, ctypesys_local& types) {
    return cvalue{ .t = types.int_type(), .v = x };
}

yama::internal::cvalue yama::internal::cvalue::uint_v(uint_t x, ctypesys_local& types) {
    return cvalue{ .t = types.uint_type(), .v = x };
}

yama::internal::cvalue yama::internal::cvalue::float_v(float_t x, ctypesys_local& types) {
    return cvalue{ .t = types.float_type(), .v = x };
}

yama::internal::cvalue yama::internal::cvalue::bool_v(bool_t x, ctypesys_local& types) {
    return cvalue{ .t = types.bool_type(), .v = x };
}

yama::internal::cvalue yama::internal::cvalue::char_v(char_t x, ctypesys_local& types) {
    return cvalue{ .t = types.char_type(), .v = x };
}

yama::internal::cvalue yama::internal::cvalue::type_v(ctype x, ctypesys_local& types) {
    return cvalue{ .t = types.type_type(), .v = x };
}

yama::internal::cvalue yama::internal::cvalue::fn_v(ctype x) {
    return cvalue{ .t = x, .v = stateless_t{} };
}

