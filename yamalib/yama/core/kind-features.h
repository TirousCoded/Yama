

#pragma once


#include "kind.h"


namespace yama {


    // these functions are used to discern the different features
    // a given kind of type does/doesn't have


    static_assert(kinds == 2);

    constexpr bool is_primitive(kind x) noexcept { return x == kind::primitive; }
    constexpr bool is_function(kind x) noexcept { return x == kind::function; }

    // is_canonicalized returns if objects of types of kind x 
    // are 'canonicalized' objects

    constexpr bool is_canonical(kind x) noexcept {
        static_assert(kinds == 2);
        switch (x) {
        case kind::primitive:   return true;    break;
        case kind::function:    return true;    break;
        default:                return bool{};  break;
        }
    }

    // is_callable returns if objects of types of kind x are
    // 'callable' objects

    // being callable means a type's objects may be used as
    // call objects for calls, and that the type MUST be defined
    // w/ a call signature

    // non-callable types MUST NOT be defined w/ a call signature

    constexpr bool is_callable(kind x) noexcept {
        static_assert(kinds == 2);
        switch (x) {
        case kind::primitive:   return false;   break;
        case kind::function:    return true;    break;
        default:                return bool{};  break;
        }
    }
}

