

#include "util.h"


std::string yama::internal::fmt_string_literal(const str& x, char_t err) {
    std::string result{};
    size_t pos = 0;
    while (pos < x.length()) {
        if (auto parsed = parse_char(x.substr(pos))) {
            result += fmt_char(parsed->v);
            pos += parsed->bytes;
        }
        else {
            result += fmt_char(err);
            pos++; // avoid infinite loop
        }
    }
    return result;
}

