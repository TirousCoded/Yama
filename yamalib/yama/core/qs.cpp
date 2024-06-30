

#include "qs.h"

#include "asserts.h"


std::string yama::fmt_qtype(qtype x) {
    static_assert(qtype_count == 2);
    std::string result{};
    switch (x) {
    case qtype::type_data:  result = "type_data";   break;
    case qtype::type:       result = "type";        break;
    default:                YAMA_DEADEND;           break;
    }
    return result;
}

