

#include "ptype.h"

#include "asserts.h"


std::string yama::fmt_ptype(ptype x) {
    static_assert(ptypes == 6);
    std::string result{};
    switch (x) {
    case ptype::none:       result = "none";    break;
    case ptype::int0:       result = "int";     break;
    case ptype::uint:       result = "uint";    break;
    case ptype::float0:     result = "float";   break;
    case ptype::bool0:      result = "bool";    break;
    case ptype::char0:      result = "char";    break;
    default:                YAMA_DEADEND;       break;
    }
    return result;
}

