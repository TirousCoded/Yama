

#include "scalar.h"

#include "Safe.h"


std::string ym::fmt(YmInt x, bool uppercaseHex, YmIntFmt fmt) {
    std::string result{};
    result.resize(ymInt_Measure(x, fmt));
    ymInt_Fmt(x, uppercaseHex, fmt, result.data());
    return result;
}

std::string ym::fmt(YmUInt x, bool uppercaseHex, YmIntFmt fmt) {
    std::string result{};
    result.resize(ymUInt_Measure(x, fmt));
    ymUInt_Fmt(x, uppercaseHex, fmt, result.data());
    return result;
}

std::string ym::fmt(YmFloat x) {
    std::string result{};
    result.resize(ymFloat_Measure(x));
    ymFloat_Fmt(x, result.data());
    return result;
}

std::string ym::fmt(YmBool x) {
    return std::string(Safe(ymBool_Fmt(x)));
}

std::string ym::fmt(YmRune x, bool uppercaseHex, bool escapeQuotes, bool escapeDblQuotes, bool escapeBackslashes) {
    std::string result{};
    result.resize(ymRune_Measure(x, escapeQuotes, escapeDblQuotes, escapeBackslashes));
    ymRune_Fmt(x, uppercaseHex, escapeQuotes, escapeDblQuotes, escapeBackslashes, result.data());
    return result;
}

