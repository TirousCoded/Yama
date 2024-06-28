

#include "kind.h"

#include "asserts.h"


std::string yama::fmt_kind(kind x) {
    static_assert(kind_count == 2);
    std::string result{};
    switch (x) {
    case kind::primitive:   result = "primitive";   break;
    case kind::function:    result = "function";    break;
    default:                YAMA_DEADEND;           break;
    }
    return result;
}
