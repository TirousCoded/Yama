

#include "kind.h"

#include "asserts.h"


std::string yama::fmt_kind(kind x) {
    static_assert(kinds == 3);
    std::string result{};
    switch (x) {
    case kind::primitive:   result = "primitive";   break;
    case kind::function:    result = "function";    break;
    //case kind::method:      result = "method";      break;
    case kind::struct0:     result = "struct";      break;
    default:                YAMA_DEADEND;           break;
    }
    return result;
}

