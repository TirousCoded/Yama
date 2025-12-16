

#include "scalar-utils.h"

#include "../yama/asserts.h"
#include "../yama++/print.h"


std::optional<taul::decode_result> _ym::ScalarParser::checkAny() {
    return decoder.peek();
}

std::optional<taul::decode_result> _ym::ScalarParser::check(YmChar v) {
    if (const auto r = checkAny(); r && r->cp == v) {
        return r;
    }
    return std::nullopt;
}

std::optional<taul::decode_result> _ym::ScalarParser::checkSet(std::u32string_view vs) {
    if (const auto r = checkAny(); r && taul::in_set(vs, r->cp)) {
        return r;
    }
    return std::nullopt;
}

std::optional<taul::decode_result> _ym::ScalarParser::expectAny() {
    if (const auto r = checkAny()) {
        offset += r->bytes;
        decoder.skip();
        return r;
    }
    return std::nullopt;
}

std::optional<taul::decode_result> _ym::ScalarParser::expect(YmChar v) {
    if (const auto r = check(v)) {
        offset += r->bytes;
        decoder.skip();
        return r;
    }
    return std::nullopt;
}

std::optional<taul::decode_result> _ym::ScalarParser::expectSet(std::u32string_view vs) {
    if (const auto r = checkSet(vs)) {
        offset += r->bytes;
        decoder.skip();
        return r;
    }
    return std::nullopt;
}

_ym::ScalarParser _ym::ScalarParser::make(std::string_view input) {
    return ScalarParser{
        .input = input,
        .offset = 0,
        .decoder = taul::decoder<char>(taul::utf8, input),
    };
}

size_t _ym::measureIntGeneric(YmInt x, YmUInt base, size_t prefix_len) {
    return
        x >= 0
        ? measureUIntGeneric((YmUInt)x, base, prefix_len, false)
        : measureUIntGeneric((YmUInt)-x, base, prefix_len, false) + 1;
}

size_t _ym::measureUIntGeneric(YmUInt x, YmUInt base, size_t prefix_len, bool u_suffix) {
    size_t result = 0;
    do {
        result++;
        x -= x % base;
        x /= base;
    } while (x > 0);
    if (u_suffix) {
        result++;
    }
    return result + prefix_len;
}

void _ym::fmtIntGeneric(std::span<YmChar> buff, YmInt x, YmUInt base, std::string_view base_s, std::string_view prefix) {
    if (x >= 0) {
        fmtUIntGeneric(buff, (YmUInt)x, base, base_s, prefix, false);
    }
    else {
        buff[0] = '-';
        fmtUIntGeneric(buff.subspan(1), (YmUInt)-x, base, base_s, prefix, false);
    }
}

void _ym::fmtUIntGeneric(std::span<YmChar> buff, YmUInt x, YmUInt base, std::string_view base_s, std::string_view prefix, bool u_suffix) {
    ymAssert(base == base_s.length());
    auto rIt = buff.rbegin();
    auto rOut = [&](YmChar x) {
#if 0
        ym::println("rOut({})", x);
#endif
        ymAssert(rIt != buff.rend());
        *rIt = x;
        std::advance(rIt, 1);
        };
    if (u_suffix) {
        rOut('u');
    }
    do {
        YmUInt d = x % base;
        rOut(base_s[d]);
        x -= d;
        x /= base;
    } while (x > 0);
    for (size_t i = 0; i < prefix.length(); i++) {
        rOut(prefix[prefix.length() - 1 - i]);
    }
}

_ym::ScalarParseResult<YmInt> _ym::parseInt(std::string_view input) {
    ScalarParseResult<YmInt> result{ .status = YmParseStatus_Success, .output = 0, .bytes = 0 }; // Our eventual result.
    bool negated = false; // If encountered a '-'.
    
    auto ctx = ScalarParser::make(input);

    // incorporate_[dec|hex|bin] incorporates digit into the final result value and does overflow check.
    const auto incorporate_dec =
        [&](YmRune digit) {
        const auto v = (YmInt)taul::where_in_set(taul::digit_u32, digit);
        ymAssert(v < 10);
        const auto old_v = result.output;
        result.output *= 10;
        result.output += !negated ? v : -v;
        if (!negated && result.output < old_v) result.status = YmParseStatus_Overflow;
        if (negated && result.output > old_v) result.status = YmParseStatus_Underflow;
        };
    const auto incorporate_hex =
        [&](YmRune digit) {
        auto v = (YmInt)taul::where_in_set(taul::hex_uppercase_u32, digit);
        if (v == 16) {
            v = (YmInt)taul::where_in_set(taul::hex_lowercase_u32, digit);
        }
        ymAssert(v < 16);
        const auto old_v = result.output;
        result.output *= 16;
        result.output += !negated ? v : -v;
        if (!negated && result.output < old_v) result.status = YmParseStatus_Overflow;
        if (negated && result.output > old_v) result.status = YmParseStatus_Underflow;
        };
    const auto incorporate_bin =
        [&](YmRune digit) {
        const auto v = (YmInt)taul::where_in_set(U"01", digit);
        ymAssert(v < 2);
        const auto old_v = result.output;
        result.output *= 2;
        result.output += !negated ? v : -v;
        if (!negated && result.output < old_v) result.status = YmParseStatus_Overflow;
        if (negated && result.output > old_v) result.status = YmParseStatus_Underflow;
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
            if (const auto a = ctx.expectSet(U"0123456789abcdefABCDEF")) { // first may not be '_'
                reasonable = true;
                incorporate_hex(a->cp);
            }
            while (true) { // parse digits
                bool need_digit = false; // if we find a '_', but no digit after it, fail parse
                if (const auto a = ctx.expect(U'_')) { // check for '_'
                    need_digit = true;
                }
                if (const auto a = ctx.expectSet(U"0123456789abcdefABCDEF")) { // next digit
                    incorporate_hex(a->cp);
                }
                else if (need_digit) { // if needed digit, but couldn't find one, fail parse
                    return ScalarParseResult<YmInt>{}; // Fail
                }
                else break; // if no digit, but also didn't fail due to it, then we're done
            }
        }
        else if (const auto b = ctx.expect(U'b')) { // parse binary
            decimal = false; // indicate shouldn't perform decimal parsing
            if (const auto a = ctx.expectSet(U"01")) { // first may not be '_'
                reasonable = true;
                incorporate_bin(a->cp);
            }
            while (true) { // parse digits
                bool need_digit = false; // if we find a '_', but no digit after it, fail parse
                if (const auto a = ctx.expect(U'_')) { // check for '_'
                    need_digit = true;
                }
                if (const auto a = ctx.expectSet(U"01")) { // next digit
                    incorporate_bin(a->cp);
                }
                else if (need_digit) { // if needed digit, but couldn't find one, fail parse
                    return ScalarParseResult<YmInt>{}; // Fail
                }
                else break; // if no digit, but also didn't fail due to it, then we're done
            }
        }
        else { // detect leading '0' of decimal
            reasonable = true; // even if below doesn't find any digits, parse still succeeds
        }
    }
    if (decimal) { // parse decimal
        if (const auto a = ctx.expectSet(taul::digit_u32)) { // first may not be '_'
            reasonable = true;
            incorporate_dec(a->cp);
        }
        while (true) { // parse digits
            bool need_digit = false; // if we find a '_', but no digit after it, fail parse
            if (const auto a = ctx.expect(U'_')) { // check for '_'
                need_digit = true;
            }
            if (const auto a = ctx.expectSet(taul::digit_u32)) { // next digit
                incorporate_dec(a->cp);
            }
            else if (need_digit) { // if needed digit, but couldn't find one, fail parse
                return ScalarParseResult<YmInt>{}; // Fail
            }
            else break; // if no digit, but also didn't fail due to it, then we're done
        }
    }
    if (!reasonable) {
        return ScalarParseResult<YmInt>{}; // Fail
    }
    result.bytes = ctx.offset; // Can't forget this part.
    return result;
}

_ym::ScalarParseResult<YmUInt> _ym::parseUInt(std::string_view input, bool ignoreU) {
    ScalarParseResult<YmUInt> result{ .status = YmParseStatus_Success, .output = 0, .bytes = 0 }; // our eventual result

    auto ctx = ScalarParser::make(input);

    // incorporate_[dec|hex|bin] incorporates digit into the final result value and does overflow check
    const auto incorporate_dec =
        [&](YmRune digit) {
        const auto v = (YmUInt)taul::where_in_set(taul::digit_u32, digit);
        ymAssert(v < 10);
        const auto old_v = result.output;
        result.output *= 10;
        result.output += v;
        if (result.output < old_v) result.status = YmParseStatus_Overflow;
        };
    const auto incorporate_hex =
        [&](YmRune digit) {
        auto v = (YmUInt)taul::where_in_set(taul::hex_uppercase_u32, digit);
        if (v == 16) {
            v = (YmUInt)taul::where_in_set(taul::hex_lowercase_u32, digit);
        }
        ymAssert(v < 16);
        const auto old_v = result.output;
        result.output *= 16;
        result.output += v;
        if (result.output < old_v) result.status = YmParseStatus_Overflow;
        };
    const auto incorporate_bin =
        [&](YmRune digit) {
        const auto v = (YmUInt)taul::where_in_set(U"01", digit);
        ymAssert(v < 2);
        const auto old_v = result.output;
        result.output *= 2;
        result.output += v;
        if (result.output < old_v) result.status = YmParseStatus_Overflow;
        };

    bool reasonable = false; // if has needed minimum chars to declare parse successful
    bool decimal = true; // signals we're parsing decimal, not hex or binary
    // consuming leading '0' won't affect result even if no 'x' or 'b' ahead
    if (const auto a = ctx.expect(U'0')) { // check for '0x' or '0b'
        if (const auto b = ctx.expect(U'x')) { // parse hex
            decimal = false; // indicate shouldn't perform decimal parsing
            if (const auto a = ctx.expectSet(U"0123456789abcdefABCDEF")) { // first may not be '_'
                reasonable = true;
                incorporate_hex(a->cp);
            }
            while (true) { // parse digits
                bool need_digit = false; // if we find a '_', but no digit after it, fail parse
                if (const auto a = ctx.expect(U'_')) { // check for '_'
                    need_digit = true;
                }
                if (const auto a = ctx.expectSet(U"0123456789abcdefABCDEF")) { // next digit
                    incorporate_hex(a->cp);
                }
                else if (need_digit) { // if needed digit, but couldn't find one, fail parse
                    return ScalarParseResult<YmUInt>{}; // Fail
                }
                else break; // if no digit, but also didn't fail due to it, then we're done
            }
        }
        else if (const auto b = ctx.expect(U'b')) { // parse binary
            decimal = false; // indicate shouldn't perform decimal parsing
            if (const auto a = ctx.expectSet(U"01")) { // first may not be '_'
                reasonable = true;
                incorporate_bin(a->cp);
            }
            while (true) { // parse digits
                bool need_digit = false; // if we find a '_', but no digit after it, fail parse
                if (const auto a = ctx.expect(U'_')) { // check for '_'
                    need_digit = true;
                }
                if (const auto a = ctx.expectSet(U"01")) { // next digit
                    incorporate_bin(a->cp);
                }
                else if (need_digit) { // if needed digit, but couldn't find one, fail parse
                    return ScalarParseResult<YmUInt>{}; // Fail
                }
                else break; // if no digit, but also didn't fail due to it, then we're done
            }
        }
        else { // detect leading '0' of decimal
            reasonable = true; // even if below doesn't find any digits, parse still succeeds
        }
    }
    if (decimal) { // parse decimal
        if (const auto a = ctx.expectSet(taul::digit_u32)) { // first may not be '_'
            reasonable = true;
            incorporate_dec(a->cp);
        }
        while (true) { // parse digits
            bool need_digit = false; // if we find a '_', but no digit after it, fail parse
            if (const auto a = ctx.expect(U'_')) { // check for '_'
                need_digit = true;
            }
            if (const auto a = ctx.expectSet(taul::digit_u32)) { // next digit
                incorporate_dec(a->cp);
            }
            else if (need_digit) { // if needed digit, but couldn't find one, fail parse
                return ScalarParseResult<YmUInt>{}; // Fail
            }
            else break; // if no digit, but also didn't fail due to it, then we're done
        }
    }
    if (!ignoreU && !ctx.expect(U'u')) { // if !ignoreU, and no final 'u', fail parse
        return ScalarParseResult<YmUInt>{}; // Fail
    }
    if (!reasonable) {
        return ScalarParseResult<YmUInt>{}; // Fail
    }
    result.bytes = ctx.offset; // can't forget this part
    return result;
}

_ym::ScalarParseResult<YmFloat> _ym::parseFloat(std::string_view input) {
    ScalarParseResult<YmFloat> result{ .status = YmParseStatus_Success, .output = 0.0, .bytes = 0 }; // our eventual result

    auto ctx = ScalarParser::make(input);

    const auto digit_v =
        [](YmRune x) -> long double {
        return (long double)taul::where_in_set(taul::digit_u32, x);
        };

    const bool negated = ctx.expect(U'-').has_value(); // check for negation

    if (ctx.expect(U'i')) { // inf
        if (ctx.expect(U'n') && ctx.expect(U'f')) {
            result.output = YM_INF;
        }
        else return ScalarParseResult<YmFloat>{}; // Fail
    }
    else if (ctx.expect(U'n')) { // nan
        if (ctx.expect(U'a') && ctx.expect(U'n')) {
            // TODO: Haven't re-added NaNs yet, but old impl did, so we'll just keep this code.
            result.output = NAN;
        }
        else return ScalarParseResult<YmFloat>{}; // Fail
    }
    else { // regular
        std::optional<long double> integer, fraction, exponent;

        if (const auto a = ctx.expectSet(taul::digit_u32)) { // build integer part of mantissa, if any
            integer = std::make_optional<long double>(0.0);
            *integer = digit_v(a->cp);
            while (true) { // process other digits + underscores
                bool need_digit = false; // if we find a '_', we NEED a digit after, or fail parse
                if (ctx.expect(U'_')) {
                    need_digit = true;
                }
                if (const auto b = ctx.expectSet(taul::digit_u32)) {
                    *integer = *integer * 10.0 + digit_v(b->cp);
                }
                else if (need_digit) return ScalarParseResult<YmFloat>{}; // no digit, needed one, so fail parse
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
                    if (!past_first_digit) return ScalarParseResult<YmFloat>{}; // '_' before first digit, fail parse
                }
                if (const auto a = ctx.expectSet(taul::digit_u32)) {
                    helper /= 10;
                    *fraction += digit_v(a->cp) * helper;
                    past_first_digit = true;
                }
                else if (need_digit) return ScalarParseResult<YmFloat>{}; // no digit, needed one, so fail parse
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
                    if (!past_first_digit) return ScalarParseResult<YmFloat>{}; // '_' before first digit, fail parse
                }
                if (const auto b = ctx.expectSet(taul::digit_u32)) {
                    *exponent = *exponent * 10.0 + digit_v(b->cp);
                    past_first_digit = true;
                }
                else if (need_digit) return ScalarParseResult<YmFloat>{}; // no digit, needed one, so fail parse
                else break; // exit loop when ran out of digits
            }
            if (negated_exp) { // negate exponent
                *exponent = -*exponent;
            }
        }

        if (!integer && !fraction) { // failure if have neither integer nor fraction part
            return ScalarParseResult<YmFloat>{}; // Fail
        }

#if 0
        if (integer) ym::println("*integer=={}\n", *integer);
        if (fraction) ym::println("*fraction=={}\n", *fraction);
        if (exponent) ym::println("*exponent=={}\n", *exponent);
#endif

        const long double mantissa = integer.value_or(0.0) + fraction.value_or(0.0); // calc mantissa
#if 0
        ym::println("mantissa=={}\n", mantissa);
#endif
        const long double value = // calc final value
            mantissa != 0.0 // std::pow can't handle first arg being 0.0
            ? mantissa * std::pow(10.0, exponent.value_or(0.0))
            : mantissa;
#if 0
        ym::println("value=={}\n", value);
#endif

        if (std::isinf(value)) { // mark overflow/underflow if inf
            if (!negated) {
                result.status = YmParseStatus_Overflow;
#if 0
                ym::println("overflow!\n");
#endif
            }
            else {
                result.status = YmParseStatus_Underflow;
#if 0
                ym::println("underflow!\n");
#endif
            }
        }

        result.output = (YmFloat)value; // propagate value
    }

    if (negated) { // apply negation
        result.output = -result.output;
    }

    result.bytes = ctx.offset; // can't forget this part
    return result;
}

_ym::ScalarParseResult<YmBool> _ym::parseBool(std::string_view input) {
    if (input.substr(0, 4) == "true")           return ScalarParseResult<YmBool>{ .status = YmParseStatus_Success, .output = YM_TRUE, .bytes = 4 };
    else if (input.substr(0, 5) == "false")     return ScalarParseResult<YmBool>{ .status = YmParseStatus_Success, .output = YM_FALSE, .bytes = 5 };
    else                                        return ScalarParseResult<YmBool>{};
}

_ym::ScalarParseResult<YmRune> _ym::parseRune(std::string_view input) {
    ScalarParseResult<YmRune> result{ .status = YmParseStatus_Success, .output = 0, .bytes = 0 }; // our eventual result

    auto ctx = ScalarParser::make(input);

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

    const auto a = ctx.expectAny(); // get our char, or '\\' of escape seq
    if (!a) {
        return ScalarParseResult<YmRune>{}; // Fail
    }
    if (a->cp != U'\\') { // handle non-escape seq
        result.output = a->cp;
    }
    else { // handle escape seq
        const auto b = ctx.expectAny();
        if (!b) {
            return ScalarParseResult<YmRune>{}; // Fail
        }
        if (b->cp == U'0')          result.output = U'\0';
        else if (b->cp == U'a')     result.output = U'\a';
        else if (b->cp == U'b')     result.output = U'\b';
        else if (b->cp == U'f')     result.output = U'\f';
        else if (b->cp == U'n')     result.output = U'\n';
        else if (b->cp == U'r')     result.output = U'\r';
        else if (b->cp == U't')     result.output = U'\t';
        else if (b->cp == U'v')     result.output = U'\v';
        else if (b->cp == U'\'')    result.output = U'\'';
        else if (b->cp == U'"')     result.output = U'"';
        else if (b->cp == U'\\')    result.output = U'\\';
        else if (b->cp == U'x') { // 8-bit hex escape seq, or literalize
            const auto old_ctx = ctx; // save old ctx to restore if we find hex seq to be invalid
            const auto v0 = resolve_c(ctx.expectSet(U"0123456789abcdefABCDEF"));
            const auto v1 = resolve_c(ctx.expectSet(U"0123456789abcdefABCDEF"));
            if (v0 && v1) {
                result.output += (YmRune)*v0;
                result.output *= 16;
                result.output += (YmRune)*v1;
            }
            else { // literalize
                ctx = old_ctx; // erase effect of above ctx.expect_set
                result.output = b->cp;
            }
        }
        else if (b->cp == U'u') { // 16-bit hex escape seq, or literalize
            const auto old_ctx = ctx; // save old ctx to restore if we find hex seq to be invalid
            const auto v0 = resolve_c(ctx.expectSet(U"0123456789abcdefABCDEF"));
            const auto v1 = resolve_c(ctx.expectSet(U"0123456789abcdefABCDEF"));
            const auto v2 = resolve_c(ctx.expectSet(U"0123456789abcdefABCDEF"));
            const auto v3 = resolve_c(ctx.expectSet(U"0123456789abcdefABCDEF"));
            if (v0 && v1 && v2 && v3) {
                result.output += (YmRune)*v0;
                result.output *= 16;
                result.output += (YmRune)*v1;
                result.output *= 16;
                result.output += (YmRune)*v2;
                result.output *= 16;
                result.output += (YmRune)*v3;
            }
            else { // literalize
                ctx = old_ctx; // erase effect of above ctx.expect_set
                result.output = b->cp;
            }
        }
        else if (b->cp == U'U') { // 32-bit hex escape seq, or literalize
            const auto old_ctx = ctx; // save old ctx to restore if we find hex seq to be invalid
            const auto v0 = resolve_c(ctx.expectSet(U"0123456789abcdefABCDEF"));
            const auto v1 = resolve_c(ctx.expectSet(U"0123456789abcdefABCDEF"));
            const auto v2 = resolve_c(ctx.expectSet(U"0123456789abcdefABCDEF"));
            const auto v3 = resolve_c(ctx.expectSet(U"0123456789abcdefABCDEF"));
            const auto v4 = resolve_c(ctx.expectSet(U"0123456789abcdefABCDEF"));
            const auto v5 = resolve_c(ctx.expectSet(U"0123456789abcdefABCDEF"));
            const auto v6 = resolve_c(ctx.expectSet(U"0123456789abcdefABCDEF"));
            const auto v7 = resolve_c(ctx.expectSet(U"0123456789abcdefABCDEF"));
            if (v0 && v1 && v2 && v3 && v4 && v5 && v6 && v7) {
                result.output += (YmRune)*v0;
                result.output *= 16;
                result.output += (YmRune)*v1;
                result.output *= 16;
                result.output += (YmRune)*v2;
                result.output *= 16;
                result.output += (YmRune)*v3;
                result.output *= 16;
                result.output += (YmRune)*v4;
                result.output *= 16;
                result.output += (YmRune)*v5;
                result.output *= 16;
                result.output += (YmRune)*v6;
                result.output *= 16;
                result.output += (YmRune)*v7;
            }
            else { // literalize
                ctx = old_ctx; // erase effect of above ctx.expect_set
                result.output = b->cp;
            }
        }
        else { // literalize
            result.output = b->cp;
        }
    }
    result.bytes = ctx.offset; // can't forget this part
    return result;
}

