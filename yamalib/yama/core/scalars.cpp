

#include "scalars.h"

#include <format>


std::string yama::fmt_int(int_t x) {
    return std::format("{}", x);
}

std::string yama::fmt_uint(uint_t x) {
    return std::format("{}", x);
}

std::string yama::fmt_float(float_t x) {
    return std::format("{}", x);
}

std::string yama::fmt_bool(bool_t x) {
    return std::format("{}", x);
}

std::string yama::fmt_char(char_t x) {
    return taul::fmt_unicode(x);
}

