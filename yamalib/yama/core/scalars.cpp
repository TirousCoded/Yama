

#include "scalars.h"

#include <array>
#include <format>

#include <taul/strings.h>

#include "asserts.h"


std::string yama::fmt_int_dec(int_t x) {
    return internal::fmt_int_generic(x, 10, taul::digit, "");
}

std::string yama::fmt_int_hex(int_t x, bool uppercase_hex) {
    return internal::fmt_int_generic(x, 16, internal::get_hex_digits(uppercase_hex), "0x");
}

std::string yama::fmt_int_bin(int_t x) {
    return internal::fmt_int_generic(x, 2, "01", "0b");
}

std::string yama::fmt_int(int_t x, int_fmt fmt, bool uppercase_hex) {
    if (fmt == int_fmt::dec)        return fmt_int_dec(x);
    else if (fmt == int_fmt::hex)   return fmt_int_hex(x, uppercase_hex);
    else if (fmt == int_fmt::bin)   return fmt_int_bin(x);
    else                            YAMA_DEADEND;
    return {};
}

std::string yama::fmt_uint_dec(uint_t x) {
    return internal::fmt_uint_generic(x, 10, taul::digit, "", true);
}

std::string yama::fmt_uint_hex(uint_t x, bool uppercase_hex) {
    return internal::fmt_uint_generic(x, 16, internal::get_hex_digits(uppercase_hex), "0x", true);
}

std::string yama::fmt_uint_bin(uint_t x) {
    return internal::fmt_uint_generic(x, 2, "01", "0b", true);
}

std::string yama::fmt_uint(uint_t x, int_fmt fmt, bool uppercase_hex) {
    if (fmt == int_fmt::dec)        return fmt_uint_dec(x);
    else if (fmt == int_fmt::hex)   return fmt_uint_hex(x, uppercase_hex);
    else if (fmt == int_fmt::bin)   return fmt_uint_bin(x);
    else                            YAMA_DEADEND;
    return {};
}

std::string yama::fmt_float(float_t x) {
    // fmt_float basically just wraps std::format
    return std::format("{}", x);
}

std::string yama::fmt_bool(bool_t x) {
    return
        x
        ? "true"
        : "false";
}

std::string yama::fmt_char(char_t x, bool uppercase_hex, bool escape_quotes, bool escape_dbl_quotes, bool escape_backslashes) {
    //   \0              <- Null
    //   \a				<- Bell (Alert)
    //   \b				<- Backspace
    //   \f				<- Form Feed
    //   \n				<- New Line
    //   \r				<- Carriage Return
    //   \t				<- Horizontal Tab
    //   \v				<- Vertical Tab
    //   \'				<- Single Quotation
    //   \"				<- Double Quotation
    //   \\				<- Backslash
    //   \xhh			<- Hex Literal (8-bit)
    //   \uhhhh          <- Hex Literal (16-bit)
    //   \Uhhhhhhhh      <- Hex Literal (32-bit)
    if (x == '\0') {
        return "\\0";
    }
    else if (x == '\a') {
        return "\\a";
    }
    else if (x == '\b') {
        return "\\b";
    }
    else if (x == '\f') {
        return "\\f";
    }
    else if (x == '\n') {
        return "\\n";
    }
    else if (x == '\r') {
        return "\\r";
    }
    else if (x == '\t') {
        return "\\t";
    }
    else if (x == '\v') {
        return "\\v";
    }
    else if (x == '\'') {
        return
            escape_quotes
            ? "\\'"
            : "\'";
    }
    else if (x == '\"') {
        return
            escape_dbl_quotes
            ? "\\\""
            : "\"";
    }
    else if (x == '\\') {
        return
            escape_backslashes
            ? "\\\\"
            : "\\";
    }
    else if (taul::is_visible_ascii(x)) {
        return std::format("{}", char(x));
    }
    else if (x <= char_t(0xff)) {
        const auto h = internal::resolve_hex<2>(x, uppercase_hex);
        return std::format("\\x{}{}", h[0], h[1]);
    }
    else if (x <= char_t(0xffff)) {
        const auto h = internal::resolve_hex<4>(x, uppercase_hex);
        return std::format("\\u{}{}{}{}", h[0], h[1], h[2], h[3]);
    }
    else if (x <= char_t(0x10ffff)) {
        const auto h = internal::resolve_hex<8>(x, uppercase_hex);
        return std::format("\\U{}{}{}{}{}{}{}{}", h[0], h[1], h[2], h[3], h[4], h[5], h[6], h[7]);
    }
    else return "?";
}

std::optional<yama::parsed_int> yama::parse_int(std::string_view x) {
    parsed_int result{ .v = 0, .bytes = 0 }; // our eventual result
    bool negated = false; // if encountered a '-'
    
    auto ctx = internal::parse_ctx::make(x);

    // incorporate_[dec|hex|bin] incorporates digit into the final result value and does overflow check
    const auto incorporate_dec =
        [&](char_t digit) {
        const auto v = (int_t)taul::where_in_set(taul::digit_u32, digit);
        YAMA_ASSERT(v < 10);
        const auto old_v = result.v;
        result.v *= 10;
        result.v += !negated ? v : -v;
        if (!negated) {
            if (result.v < old_v) { // detect overflow
                result.overflow = true;
            }
        }
        else {
            if (result.v > old_v) { // detect underflow
                result.underflow = true;
            }
        }
        };
    const auto incorporate_hex =
        [&](char_t digit) {
        auto v = (int_t)taul::where_in_set(taul::hex_uppercase_u32, digit);
        if (v == 16) {
            v = (int_t)taul::where_in_set(taul::hex_lowercase_u32, digit);
        }
        YAMA_ASSERT(v < 16);
        const auto old_v = result.v;
        result.v *= 16;
        result.v += !negated ? v : -v;
        if (!negated) {
            if (result.v < old_v) { // detect overflow
                result.overflow = true;
            }
        }
        else {
            if (result.v > old_v) { // detect underflow
                result.underflow = true;
            }
        }
        };
    const auto incorporate_bin =
        [&](char_t digit) {
        const auto v = (int_t)taul::where_in_set(U"01", digit);
        YAMA_ASSERT(v < 2);
        const auto old_v = result.v;
        result.v *= 2;
        result.v += !negated ? v : -v;
        if (!negated) {
            if (result.v < old_v) { // detect overflow
                result.overflow = true;
            }
        }
        else {
            if (result.v > old_v) { // detect underflow
                result.underflow = true;
            }
        }
        };

    if (ctx.expect(U'-')) { // check for negation
        negated = true;
    }
    bool reasonable = false; // if has needed minimum chars to declare parse successful
    bool decimal = true; // signals we're parsing decimal, not hex or binary
    // consuming leading '0' won't affect result even if no 'x' or 'b' ahead
    if (const auto a = ctx.expect(U'0')) { // check for '0x' or '0b'
        if (const auto b = ctx.expect(U'x')) { // parse hex
            decimal = false; // indicate shouldn't perform decimal parsing
            if (const auto a = ctx.expect_set(U"0123456789abcdefABCDEF")) { // first may not be '_'
                reasonable = true;
                incorporate_hex(a->cp);
            }
            while (true) { // parse digits
                bool need_digit = false; // if we find a '_', but no digit after it, fail parse
                if (const auto a = ctx.expect(U'_')) { // check for '_'
                    need_digit = true;
                }
                if (const auto a = ctx.expect_set(U"0123456789abcdefABCDEF")) { // next digit
                    incorporate_hex(a->cp);
                }
                else if (need_digit) { // if needed digit, but couldn't find one, fail parse
                    return std::nullopt;
                }
                else break; // if no digit, but also didn't fail due to it, then we're done
            }
        }
        else if (const auto b = ctx.expect(U'b')) { // parse binary
            decimal = false; // indicate shouldn't perform decimal parsing
            if (const auto a = ctx.expect_set(U"01")) { // first may not be '_'
                reasonable = true;
                incorporate_bin(a->cp);
            }
            while (true) { // parse digits
                bool need_digit = false; // if we find a '_', but no digit after it, fail parse
                if (const auto a = ctx.expect(U'_')) { // check for '_'
                    need_digit = true;
                }
                if (const auto a = ctx.expect_set(U"01")) { // next digit
                    incorporate_bin(a->cp);
                }
                else if (need_digit) { // if needed digit, but couldn't find one, fail parse
                    return std::nullopt;
                }
                else break; // if no digit, but also didn't fail due to it, then we're done
            }
        }
        else { // detect leading '0' of decimal
            reasonable = true; // even if below doesn't find any digits, parse still succeeds
        }
    }
    if (decimal) { // parse decimal
        if (const auto a = ctx.expect_set(taul::digit_u32)) { // first may not be '_'
            reasonable = true;
            incorporate_dec(a->cp);
        }
        while (true) { // parse digits
            bool need_digit = false; // if we find a '_', but no digit after it, fail parse
            if (const auto a = ctx.expect(U'_')) { // check for '_'
                need_digit = true;
            }
            if (const auto a = ctx.expect_set(taul::digit_u32)) { // next digit
                incorporate_dec(a->cp);
            }
            else if (need_digit) { // if needed digit, but couldn't find one, fail parse
                return std::nullopt;
            }
            else break; // if no digit, but also didn't fail due to it, then we're done
        }
    }
    if (!reasonable) {
        return std::nullopt;
    }
    result.bytes = ctx.offset; // can't forget this part
#if 0
    std::cerr << std::format("{{ .v=={}, .bytes=={}, .overflow=={}, .underflow=={} }}\n", result.v, result.bytes, result.overflow, result.underflow);
#endif
    return std::make_optional(result);
}

std::optional<yama::parsed_uint> yama::parse_uint(std::string_view x) {
    parsed_uint result{ .v = 0, .bytes = 0 }; // our eventual result

    auto ctx = internal::parse_ctx::make(x);

    // incorporate_[dec|hex|bin] incorporates digit into the final result value and does overflow check
    const auto incorporate_dec =
        [&](char_t digit) {
        const auto v = (uint_t)taul::where_in_set(taul::digit_u32, digit);
        YAMA_ASSERT(v < 10);
        const auto old_v = result.v;
        result.v *= 10;
        result.v += v;
        if (result.v < old_v) { // detect overflow
            result.overflow = true;
        }
        };
    const auto incorporate_hex =
        [&](char_t digit) {
        auto v = (uint_t)taul::where_in_set(taul::hex_uppercase_u32, digit);
        if (v == 16) {
            v = (uint_t)taul::where_in_set(taul::hex_lowercase_u32, digit);
        }
        YAMA_ASSERT(v < 16);
        const auto old_v = result.v;
        result.v *= 16;
        result.v += v;
        if (result.v < old_v) { // detect overflow
            result.overflow = true;
        }
        };
    const auto incorporate_bin =
        [&](char_t digit) {
        const auto v = (uint_t)taul::where_in_set(U"01", digit);
        YAMA_ASSERT(v < 2);
        const auto old_v = result.v;
        result.v *= 2;
        result.v += v;
        if (result.v < old_v) { // detect overflow
            result.overflow = true;
        }
        };

    bool reasonable = false; // if has needed minimum chars to declare parse successful
    bool decimal = true; // signals we're parsing decimal, not hex or binary
    // consuming leading '0' won't affect result even if no 'x' or 'b' ahead
    if (const auto a = ctx.expect(U'0')) { // check for '0x' or '0b'
        if (const auto b = ctx.expect(U'x')) { // parse hex
            decimal = false; // indicate shouldn't perform decimal parsing
            if (const auto a = ctx.expect_set(U"0123456789abcdefABCDEF")) { // first may not be '_'
                reasonable = true;
                incorporate_hex(a->cp);
            }
            while (true) { // parse digits
                bool need_digit = false; // if we find a '_', but no digit after it, fail parse
                if (const auto a = ctx.expect(U'_')) { // check for '_'
                    need_digit = true;
                }
                if (const auto a = ctx.expect_set(U"0123456789abcdefABCDEF")) { // next digit
                    incorporate_hex(a->cp);
                }
                else if (need_digit) { // if needed digit, but couldn't find one, fail parse
                    return std::nullopt;
                }
                else break; // if no digit, but also didn't fail due to it, then we're done
            }
        }
        else if (const auto b = ctx.expect(U'b')) { // parse binary
            decimal = false; // indicate shouldn't perform decimal parsing
            if (const auto a = ctx.expect_set(U"01")) { // first may not be '_'
                reasonable = true;
                incorporate_bin(a->cp);
            }
            while (true) { // parse digits
                bool need_digit = false; // if we find a '_', but no digit after it, fail parse
                if (const auto a = ctx.expect(U'_')) { // check for '_'
                    need_digit = true;
                }
                if (const auto a = ctx.expect_set(U"01")) { // next digit
                    incorporate_bin(a->cp);
                }
                else if (need_digit) { // if needed digit, but couldn't find one, fail parse
                    return std::nullopt;
                }
                else break; // if no digit, but also didn't fail due to it, then we're done
            }
        }
        else { // detect leading '0' of decimal
            reasonable = true; // even if below doesn't find any digits, parse still succeeds
        }
    }
    if (decimal) { // parse decimal
        if (const auto a = ctx.expect_set(taul::digit_u32)) { // first may not be '_'
            reasonable = true;
            incorporate_dec(a->cp);
        }
        while (true) { // parse digits
            bool need_digit = false; // if we find a '_', but no digit after it, fail parse
            if (const auto a = ctx.expect(U'_')) { // check for '_'
                need_digit = true;
            }
            if (const auto a = ctx.expect_set(taul::digit_u32)) { // next digit
                incorporate_dec(a->cp);
            }
            else if (need_digit) { // if needed digit, but couldn't find one, fail parse
                return std::nullopt;
            }
            else break; // if no digit, but also didn't fail due to it, then we're done
        }
    }
    if (!ctx.expect(U'u')) { // if no final 'u', fail parse
        return std::nullopt;
    }
    if (!reasonable) {
        return std::nullopt;
    }
    result.bytes = ctx.offset; // can't forget this part
#if 0
    std::cerr << std::format("{{ .v=={}, .bytes=={}, .overflow=={}, .underflow=={} }}\n", result.v, result.bytes, result.overflow, result.underflow);
#endif
    return std::make_optional(result);
}

std::optional<yama::parsed_float> yama::parse_float(std::string_view x) {
#if 0
    std::cerr << std::format("input: {}\n", (std::string)x);
#endif

    parsed_float result{ .v = 0.0, .bytes = 0 }; // our eventual result

    auto ctx = internal::parse_ctx::make(x);

    const auto digit_v =
        [](char_t x) -> long double {
        return (long double)taul::where_in_set(taul::digit_u32, x);
        };

    const bool negated = ctx.expect(U'-').has_value(); // check for negation

    if (ctx.expect(U'i')) { // inf
        if (ctx.expect(U'n') && ctx.expect(U'f')) {
            result.v = inf;
        }
        else return std::nullopt;
    }
    else if (ctx.expect(U'n')) { // nan
        if (ctx.expect(U'a') && ctx.expect(U'n')) {
            result.v = NAN;
        }
        else return std::nullopt;
    }
    else { // regular
        std::optional<long double> integer, fraction, exponent;

        if (const auto a = ctx.expect_set(taul::digit_u32)) { // build integer part of mantissa, if any
            integer = std::make_optional<long double>(0.0);
            *integer = digit_v(a->cp);
            while (true) { // process other digits + underscores
                bool need_digit = false; // if we find a '_', we NEED a digit after, or fail parse
                if (ctx.expect(U'_')) {
                    need_digit = true;
                }
                if (const auto b = ctx.expect_set(taul::digit_u32)) {
                    *integer = *integer * 10.0 + digit_v(b->cp);
                }
                else if (need_digit) return std::nullopt; // no digit, needed one, so fail parse
                else break; // exit loop when ran out of digits
            }
        }

        if (ctx.expect(U'.')) { // build fraction part of mantissa, if any
            fraction = std::make_optional<long double>(0.0);
            long double helper = 1.0; // helps incorporate digits into *fraction
            bool past_first_digit = false; // fail parse if '_' before first digit
            while (true) { // process digits + underscores
                bool need_digit = false; // if we find a '_', we NEED a digit after, or fail parse
                if (ctx.expect(U'_')) {
                    need_digit = true;
                    if (!past_first_digit) return std::nullopt; // '_' before first digit, fail parse
                }
                if (const auto a = ctx.expect_set(taul::digit_u32)) {
                    helper /= 10;
                    *fraction += digit_v(a->cp) * helper;
                    past_first_digit = true;
                }
                else if (need_digit) return std::nullopt; // no digit, needed one, so fail parse
                else break; // exit loop when ran out of digits
            }
        }

        if (ctx.expect(U'e')) { // build exponent, if any
            exponent = std::make_optional<long double>(0.0);
            bool negated_exp = false;
            if (ctx.expect(U'-')) { // check for negation
                negated_exp = true;
            }
            else if (ctx.expect(U'+')) {} // skip '+'
            bool past_first_digit = false; // fail parse if '_' before first digit
            while (true) { // process digits + underscores
                bool need_digit = false; // if we find a '_', we NEED a digit after, or fail parse
                if (ctx.expect(U'_')) {
                    need_digit = true;
                    if (!past_first_digit) return std::nullopt; // '_' before first digit, fail parse
                }
                if (const auto b = ctx.expect_set(taul::digit_u32)) {
                    *exponent = *exponent * 10.0 + digit_v(b->cp);
                    past_first_digit = true;
                }
                else if (need_digit) return std::nullopt; // no digit, needed one, so fail parse
                else break; // exit loop when ran out of digits
            }
            if (negated_exp) { // negate exponent
                *exponent = -*exponent;
            }
        }

        if (!integer && !fraction) { // failure if have neither integer nor fraction part
            return std::nullopt;
        }

#if 0
        if (integer) std::cerr << std::format("*integer=={}\n", *integer);
        if (fraction) std::cerr << std::format("*fraction=={}\n", *fraction);
        if (exponent) std::cerr << std::format("*exponent=={}\n", *exponent);
#endif

        const long double mantissa = integer.value_or(0.0) + fraction.value_or(0.0); // calc mantissa
#if 0
        std::cerr << std::format("mantissa=={}\n", mantissa);
#endif
        const long double value = // calc final value
            mantissa != 0.0 // std::pow can't handle first arg being 0.0
            ? mantissa * std::pow(10.0, exponent.value_or(0.0))
            : mantissa;
#if 0
        std::cerr << std::format("value=={}\n", value);
#endif

        if (std::isinf(value)) { // mark overflow/underflow if inf
            if (!negated) {
                result.overflow = true;
#if 0
                std::cerr << std::format("overflow!\n");
#endif
            }
            else {
                result.underflow = true;
#if 0
                std::cerr << std::format("underflow!\n");
#endif
            }
        }

        result.v = (float_t)value; // propagate value
    }

    if (negated) { // apply negation
        result.v = -result.v;
    }

    result.bytes = ctx.offset; // can't forget this part
#if 0
    std::cerr << std::format("{{ .v=={}, .bytes=={}, .overflow=={}, .underflow=={} }}\n", result.v, result.bytes, result.overflow, result.underflow);
#endif
    return std::make_optional(result);
}

std::optional<yama::parsed_bool> yama::parse_bool(std::string_view x) {
    if (x.substr(0, 4) == "true")       return std::make_optional(parsed_bool{ .v = true, .bytes = 4 });
    else if (x.substr(0, 5) == "false") return std::make_optional(parsed_bool{ .v = false, .bytes = 5 });
    else                                return std::nullopt;
}

std::optional<yama::parsed_char> yama::parse_char(std::string_view x) {
#if 0
    std::cerr << std::format("input: {}\n", (std::string)x);
#endif

    parsed_char result{ .v = 0, .bytes = 0 }; // our eventual result

    auto ctx = internal::parse_ctx::make(x);

    // resolve_c helps parse 8/16/32-bit escape seqs
    auto resolve_c =
        [](std::optional<taul::decode_result> c) -> std::optional<size_t> {
        if (!c) {
            return std::nullopt;
        }
        size_t r = taul::where_in_set(taul::hex_uppercase_u32, c->cp);
        if (r == 16) {
            r = taul::where_in_set(taul::hex_lowercase_u32, c->cp);
        }
        return
            r < 16
            ? std::make_optional(r)
            : std::nullopt;
        };

    const auto a = ctx.expect_any(); // get our char, or '\\' of escape seq
    if (!a) {
        return std::nullopt;
    }
    if (a->cp != U'\\') { // handle non-escape seq
        result.v = a->cp;
    }
    else { // handle escape seq
        const auto b = ctx.expect_any();
        if (!b) {
            return std::nullopt;
        }
        if (b->cp == U'0')          result.v = U'\0';
        else if (b->cp == U'a')     result.v = U'\a';
        else if (b->cp == U'b')     result.v = U'\b';
        else if (b->cp == U'f')     result.v = U'\f';
        else if (b->cp == U'n')     result.v = U'\n';
        else if (b->cp == U'r')     result.v = U'\r';
        else if (b->cp == U't')     result.v = U'\t';
        else if (b->cp == U'v')     result.v = U'\v';
        else if (b->cp == U'\'')    result.v = U'\'';
        else if (b->cp == U'"')     result.v = U'"';
        else if (b->cp == U'\\')    result.v = U'\\';
        else if (b->cp == U'x') { // 8-bit hex escape seq, or literalize
            const auto old_ctx = ctx; // save old ctx to restore if we find hex seq to be invalid
            const auto v0 = resolve_c(ctx.expect_set(U"0123456789abcdefABCDEF"));
            const auto v1 = resolve_c(ctx.expect_set(U"0123456789abcdefABCDEF"));
            if (v0 && v1) {
                result.v += (char_t)*v0;
                result.v *= 16;
                result.v += (char_t)*v1;
            }
            else { // literalize
                ctx = old_ctx; // erase effect of above ctx.expect_set
                result.v = b->cp;
            }
        }
        else if (b->cp == U'u') { // 16-bit hex escape seq, or literalize
            const auto old_ctx = ctx; // save old ctx to restore if we find hex seq to be invalid
            const auto v0 = resolve_c(ctx.expect_set(U"0123456789abcdefABCDEF"));
            const auto v1 = resolve_c(ctx.expect_set(U"0123456789abcdefABCDEF"));
            const auto v2 = resolve_c(ctx.expect_set(U"0123456789abcdefABCDEF"));
            const auto v3 = resolve_c(ctx.expect_set(U"0123456789abcdefABCDEF"));
            if (v0 && v1 && v2 && v3) {
                result.v += (char_t)*v0;
                result.v *= 16;
                result.v += (char_t)*v1;
                result.v *= 16;
                result.v += (char_t)*v2;
                result.v *= 16;
                result.v += (char_t)*v3;
            }
            else { // literalize
                ctx = old_ctx; // erase effect of above ctx.expect_set
                result.v = b->cp;
            }
        }
        else if (b->cp == U'U') { // 32-bit hex escape seq, or literalize
            const auto old_ctx = ctx; // save old ctx to restore if we find hex seq to be invalid
            const auto v0 = resolve_c(ctx.expect_set(U"0123456789abcdefABCDEF"));
            const auto v1 = resolve_c(ctx.expect_set(U"0123456789abcdefABCDEF"));
            const auto v2 = resolve_c(ctx.expect_set(U"0123456789abcdefABCDEF"));
            const auto v3 = resolve_c(ctx.expect_set(U"0123456789abcdefABCDEF"));
            const auto v4 = resolve_c(ctx.expect_set(U"0123456789abcdefABCDEF"));
            const auto v5 = resolve_c(ctx.expect_set(U"0123456789abcdefABCDEF"));
            const auto v6 = resolve_c(ctx.expect_set(U"0123456789abcdefABCDEF"));
            const auto v7 = resolve_c(ctx.expect_set(U"0123456789abcdefABCDEF"));
            if (v0 && v1 && v2 && v3 && v4 && v5 && v6 && v7) {
                result.v += (char_t)*v0;
                result.v *= 16;
                result.v += (char_t)*v1;
                result.v *= 16;
                result.v += (char_t)*v2;
                result.v *= 16;
                result.v += (char_t)*v3;
                result.v *= 16;
                result.v += (char_t)*v4;
                result.v *= 16;
                result.v += (char_t)*v5;
                result.v *= 16;
                result.v += (char_t)*v6;
                result.v *= 16;
                result.v += (char_t)*v7;
            }
            else { // literalize
                ctx = old_ctx; // erase effect of above ctx.expect_set
                result.v = b->cp;
            }
        }
        else { // literalize
            result.v = b->cp;
        }
    }
    result.bytes = ctx.offset; // can't forget this part
#if 0
    std::cerr << std::format("{{ .v=={}, .bytes=={}, .overflow=={}, .underflow=={} }}\n", fmt_char(result.v), result.bytes, result.overflow, result.underflow);
#endif
    return std::make_optional(result);
}

const char* yama::internal::get_hex_digits(bool uppercase_hex) noexcept {
    return
        uppercase_hex
        ? taul::hex_uppercase
        : taul::hex_lowercase;
}

std::string yama::internal::fmt_int_generic(int_t x, uint_t base, const char* base_s, const char* prefix) {
    const uint_t xx =
        x >= 0
        ? uint_t(x)
        : uint_t(-x);
    std::string result = fmt_uint_generic(xx, base, base_s, prefix, false);
    return
        x >= 0
        ? result
        : '-' + result;
}

std::string yama::internal::fmt_uint_generic(uint_t x, uint_t base, const char* base_s, const char* prefix, bool u_suffix) {
    YAMA_ASSERT(prefix);
    std::string result{};
    YAMA_DEREF_SAFE(base_s) {
        YAMA_ASSERT(base == taul::strlen(base_s));
        do {
            const uint_t d = x % base;
            result = base_s[d] + result;
            x -= d;
            x /= base;
        } while (x > 0);
    }
    if (u_suffix) {
        result += 'u';
    }
    return prefix + result;
}

std::optional<taul::decode_result> yama::internal::parse_ctx::check_any() {
    return decoder.peek();
}

std::optional<taul::decode_result> yama::internal::parse_ctx::check(char_t v) {
    if (const auto r = check_any(); r && r->cp == v) {
        return r;
    }
    else return std::nullopt;
}

std::optional<taul::decode_result> yama::internal::parse_ctx::check_set(std::u32string_view vs) {
    if (const auto r = check_any(); r && taul::in_set(vs, r->cp)) {
        return r;
    }
    else return std::nullopt;
}

std::optional<taul::decode_result> yama::internal::parse_ctx::expect_any() {
    if (const auto r = check_any()) {
        offset += r->bytes;
        decoder.skip();
        return r;
    }
    else return std::nullopt;
}

std::optional<taul::decode_result> yama::internal::parse_ctx::expect(char_t v) {
    if (const auto r = check(v)) {
        offset += r->bytes;
        decoder.skip();
        return r;
    }
    else return std::nullopt;
}

std::optional<taul::decode_result> yama::internal::parse_ctx::expect_set(std::u32string_view vs) {
    if (const auto r = check_set(vs)) {
        offset += r->bytes;
        decoder.skip();
        return r;
    }
    else return std::nullopt;
}

yama::internal::parse_ctx yama::internal::parse_ctx::make(std::string_view input) {
    return parse_ctx{
        .input = input,
        .offset = 0,
        .decoder = taul::decoder<char>(taul::utf8, input)
    };
}

