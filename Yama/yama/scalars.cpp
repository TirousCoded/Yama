

#include "scalars.h"

#include <array>

#include <taul/unicode.h>

#include "../yama/asserts.h"
#include "../yama++/Safe.h"
#include "../internal/scalar-utils.h"


size_t ymMeasureInt(YmInt x, YmIntFmt fmt) {
    if (fmt == YmIntFmt_Dec)        return _ym::measureIntGeneric(x, 10, 0);
    else if (fmt == YmIntFmt_Hex)   return _ym::measureIntGeneric(x, 16, 2);
    else if (fmt == YmIntFmt_Bin)   return _ym::measureIntGeneric(x, 2, 2);
    else                            YM_DEADEND;
    return size_t{};
}

size_t ymMeasureUInt(YmUInt x, YmIntFmt fmt) {
    if (fmt == YmIntFmt_Dec)        return _ym::measureUIntGeneric(x, 10, 0, true);
    else if (fmt == YmIntFmt_Hex)   return _ym::measureUIntGeneric(x, 16, 2, true);
    else if (fmt == YmIntFmt_Bin)   return _ym::measureUIntGeneric(x, 2, 2, true);
    else                            YM_DEADEND;
    return size_t{};
}

size_t ymMeasureFloat(YmFloat x) {
    return std::formatted_size("{}", x);
}

size_t ymMeasureBool(YmBool x) {
    return bool(x) ? 4 : 5;
}

size_t ymMeasureRune(YmRune x, YmBool escapeQuotes, YmBool escapeDblQuotes, YmBool escapeBackslashes) {
    static_assert(taul::units_required(taul::utf8, U'�') == 3); // Guarantee
    const auto escapeSeqChars = std::u32string_view(U"\0\a\b\f\n\r\t\v", 9);
    if (x == U'\'')                             return escapeQuotes ? 2 : 1;        // ' or \'
    else if (x == U'\"')                        return escapeDblQuotes ? 2 : 1;     // " or \"
    else if (x == U'\\')                        return escapeBackslashes ? 2 : 1;   /* \ or \\ */
    else if (taul::in_set(escapeSeqChars, x))   return 2;                           // \x
    else if (taul::is_visible_ascii(x))         return 1;                           // x
    else if (x <= YmRune(0xff))                 return 4;                           // \xXX
    else if (x <= YmRune(0xffff))               return 6;                           // \uXXXX
    else if (x <= YmRune(0x10ffff))             return 10;                          // \UXXXXXXXX
    else                                        return 6;                           // \uFFFD (aka. �)
    //else                                        return 3;                           // � (aka. U+FFFD)
}

static ym::Safe<YmChar> _allocBuffForFmt(size_t size, YmChar* writeTo) {
    return
        writeTo
        ? ym::Safe(writeTo)
        : ym::Safe<YmChar>(std::malloc(size + 1)); // '+ 1' for null-terminator.
}

const YmChar* ymFmtInt(YmInt x, YmBool uppercaseHex, YmIntFmt fmt, YmChar* writeTo) {
    size_t size = ymMeasureInt(x, fmt);
    ym::Safe<YmChar> result = _allocBuffForFmt(size, writeTo);
    if (fmt == YmIntFmt_Dec)        _ym::fmtIntGeneric(std::span<YmChar>(result.get(), size), x, 10, taul::digit, "");
    else if (fmt == YmIntFmt_Hex)   _ym::fmtIntGeneric(std::span<YmChar>(result.get(), size), x, 16, _ym::hexDigits(uppercaseHex), "0x");
    else if (fmt == YmIntFmt_Bin)   _ym::fmtIntGeneric(std::span<YmChar>(result.get(), size), x, 2, "01", "0b");
    else                            YM_DEADEND;
    result[size] = '\0'; // Null-Terminator
    return result;
}

const YmChar* ymFmtUInt(YmUInt x, YmBool uppercaseHex, YmIntFmt fmt, YmChar* writeTo) {
    size_t size = ymMeasureUInt(x, fmt);
    ym::Safe<YmChar> result = _allocBuffForFmt(size, writeTo);
    if (fmt == YmIntFmt_Dec)        _ym::fmtUIntGeneric(std::span<YmChar>(result.get(), size), x, 10, taul::digit, "", true);
    else if (fmt == YmIntFmt_Hex)   _ym::fmtUIntGeneric(std::span<YmChar>(result.get(), size), x, 16, _ym::hexDigits(uppercaseHex), "0x", true);
    else if (fmt == YmIntFmt_Bin)   _ym::fmtUIntGeneric(std::span<YmChar>(result.get(), size), x, 2, "01", "0b", true);
    else                            YM_DEADEND;
    result[size] = '\0'; // Null-Terminator
    return result;
}

const YmChar* ymFmtFloat(YmFloat x, YmChar* writeTo) {
    size_t size = ymMeasureFloat(x);
    ym::Safe<YmChar> result = _allocBuffForFmt(size, writeTo);
    std::format_to_n(result, size, "{}", x);
    result[size] = '\0'; // Null-Terminator
    return result;
}

const YmChar* ymFmtBool(YmBool x) {
    return bool(x) ? "true" : "false";
}

const YmChar* ymFmtRune(YmRune x, YmBool uppercaseHex, YmBool escapeQuotes, YmBool escapeDblQuotes, YmBool escapeBackslashes, YmChar* writeTo) {
    size_t size = ymMeasureRune(x, escapeQuotes, escapeDblQuotes, escapeBackslashes);
    ym::Safe<YmChar> result = _allocBuffForFmt(size, writeTo);
    const auto escapeSeqChars = std::u32string_view(U"\0\a\b\f\n\r\t\v", 9);
    if (x == U'\'') {
        if (escapeQuotes) {
            result[0] = '\\';
            result[1] = '\'';
        }
        else {
            result[0] = '\'';
        }
    }
    else if (x == U'\"') {
        if (escapeDblQuotes) {
            result[0] = '\\';
            result[1] = '\"';
        }
        else {
            result[0] = '\"';
        }
    }
    else if (x == U'\\') {
        if (escapeBackslashes) {
            result[0] = '\\';
            result[1] = '\\';
        }
        else {
            result[0] = '\\';
        }
    }
    else if (size_t where = taul::where_in_set(escapeSeqChars, x); where < escapeSeqChars.size()) {
        result[0] = '\\';
        result[1] = "0abfnrtv"[where];
    }
    else if (taul::is_visible_ascii(x)) {
        result[0] = (YmChar)x;
    }
    else if (x <= YmRune(0xff)) {
        const auto h = _ym::resolveHex<2>(x, uppercaseHex);
        std::format_to_n(result, size, "\\x{}{}", h[0], h[1]);
    }
    else if (x <= YmRune(0xffff)) {
        const auto h = _ym::resolveHex<4>(x, uppercaseHex);
        std::format_to_n(result, size, "\\u{}{}{}{}", h[0], h[1], h[2], h[3]);
    }
    else if (x <= YmRune(0x10ffff)) {
        const auto h = _ym::resolveHex<8>(x, uppercaseHex);
        std::format_to_n(result, size, "\\U{}{}{}{}{}{}{}{}", h[0], h[1], h[2], h[3], h[4], h[5], h[6], h[7]);
    }
    else {
        if (uppercaseHex) {
            std::format_to_n(result, size, "\\uFFFD");
        }
        else {
            std::format_to_n(result, size, "\\ufffd");
        }
    }
    result[size] = '\0'; // Null-Terminator
    return result;
}

YmParseStatus ymParseInt(const YmChar* input, YmInt* output, size_t* bytes) {
    if (!input) return YmParseStatus_Failure;
    auto result = _ym::parseInt(input);
    if (output) *output = result.output;
    if (bytes) *bytes = result.bytes;
    return result.status;
}

YmParseStatus ymParseUInt(const YmChar* input, YmUInt* output, size_t* bytes, YmBool ignoreU) {
    if (!input) return YmParseStatus_Failure;
    auto result = _ym::parseUInt(input, ignoreU);
    if (output) *output = result.output;
    if (bytes) *bytes = result.bytes;
    return result.status;
}

YmParseStatus ymParseFloat(const YmChar* input, YmFloat* output, size_t* bytes) {
    if (!input) return YmParseStatus_Failure;
    auto result = _ym::parseFloat(input);
    if (output) *output = result.output;
    if (bytes) *bytes = result.bytes;
    return result.status;
}

YmParseStatus ymParseBool(const YmChar* input, YmBool* output, size_t* bytes) {
    if (!input) return YmParseStatus_Failure;
    auto result = _ym::parseBool(input);
    if (output) *output = result.output;
    if (bytes) *bytes = result.bytes;
    return result.status;
}

YmParseStatus ymParseRune(const YmChar* input, YmRune* output, size_t* bytes) {
    if (!input) return YmParseStatus_Failure;
    auto result = _ym::parseRune(input);
    if (output) *output = result.output;
    if (bytes) *bytes = result.bytes;
    return result.status;
}

