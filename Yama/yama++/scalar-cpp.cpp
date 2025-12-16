

#include "scalar.h"

#include "Safe.h"


std::string ym::fmt(YmInt x, bool uppercaseHex, YmIntFmt fmt) {
    std::string result{};
    result.resize(ymMeasureInt(x, fmt));
    ymFmtInt(x, uppercaseHex, fmt, result.data());
    return result;
}

std::string ym::fmt(YmUInt x, bool uppercaseHex, YmIntFmt fmt) {
    std::string result{};
    result.resize(ymMeasureUInt(x, fmt));
    ymFmtUInt(x, uppercaseHex, fmt, result.data());
    return result;
}

std::string ym::fmt(YmFloat x) {
    std::string result{};
    result.resize(ymMeasureFloat(x));
    ymFmtFloat(x, result.data());
    return result;
}

std::string ym::fmt(YmBool x) {
    return std::string(Safe(ymFmtBool(x)));
}

std::string ym::fmt(YmRune x, bool uppercaseHex, bool escapeQuotes, bool escapeDblQuotes, bool escapeBackslashes) {
    std::string result{};
    result.resize(ymMeasureRune(x, escapeQuotes, escapeDblQuotes, escapeBackslashes));
    ymFmtRune(x, uppercaseHex, escapeQuotes, escapeDblQuotes, escapeBackslashes, result.data());
    return result;
}

