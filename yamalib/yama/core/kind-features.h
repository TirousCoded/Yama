

#pragma once


#include "kind.h"


namespace yama {


    // these functions are used to discern the different features
    // a given kind of type does/doesn't have


    static_assert(kinds == 4);

    constexpr bool is_primitive(kind x) noexcept { return x == kind::primitive; }
    constexpr bool is_function(kind x) noexcept { return x == kind::function; }
    constexpr bool is_method(kind x) noexcept { return x == kind::method; }
    constexpr bool is_struct(kind x) noexcept { return x == kind::struct0; }

    // is_canonicalized returns if objects of types of kind x 
    // are 'canonicalized' objects

    constexpr bool is_canonical(kind x) noexcept {
        static_assert(kinds == 4);
        switch (x) {
        case kind::primitive:   return true;
        case kind::function:    return true;
        case kind::method:      return true;
        case kind::struct0:     return false;
        default:                return bool{};
        }
    }

    // is_member returns if types of kind x are member types

    constexpr bool is_member(kind x) noexcept {
        static_assert(kinds == 4);
        switch (x) {
        case kind::primitive:   return false;
        case kind::function:    return false;
        case kind::method:      return true;
        case kind::struct0:     return false;
        default:                return bool{};
        }
    }

    // TODO: when we add things like callable struct types, we'll likely
    //       need to replace is_callable below w/ something more nuanced

    // is_callable returns if objects of types of kind x are
    // 'callable' objects

    // being callable means a type's objects may be used as
    // call objects for calls, and that the type MUST be defined
    // w/ a call signature

    // non-callable types MUST NOT be defined w/ a call signature

    constexpr bool is_callable(kind x) noexcept {
        static_assert(kinds == 4);
        switch (x) {
        case kind::primitive:   return false;
        case kind::function:    return true;
        case kind::method:      return true;
        case kind::struct0:     return false;
        default:                return bool{};
        }
    }
}

