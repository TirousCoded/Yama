

#include "kind.h"

#include "asserts.h"


std::string yama::fmt_kind(kind x) {
    static_assert(kinds == 2);
    std::string result{};
    switch (x) {
    case kind::primitive:   result = "primitive";   break;
    case kind::function:    result = "function";    break;
    default:                YAMA_DEADEND;           break;
    }
    return result;
}

bool yama::uses_callsig(kind x) noexcept {
    static_assert(kinds == 2);
    switch (x) {
    case kind::primitive:   return false;   break;
    case kind::function:    return true;    break;
    }
    YAMA_DEADEND;
    return {};
}

