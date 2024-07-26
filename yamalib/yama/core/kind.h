

#pragma once


#include <cstdint>
#include <string>
#include <format>
#include <iostream>

#include "macros.h"


namespace yama {


    // yama::kind defines the different *kinds of* types in Yama

    enum class kind : uint8_t {
        primitive,
        function,

        num,        // this is not a valid kind of type
    };

    constexpr size_t kinds = size_t(kind::num);


    std::string fmt_kind(kind x);


    // TODO: uses_callsig has not been unit tested

    // uses_callsig returns if the kind of type x is defined w/
    // or w/out a call signature

    constexpr bool uses_callsig(kind x) noexcept {
        static_assert(kinds == 2);
        switch (x) {
        case kind::primitive:   return false;   break;
        case kind::function:    return true;    break;
        default:                return bool{};  break;
        }
    }


    // TODO: is_canonical has not been unit tested

    // is_canonicalized returns if objects of types of kind x 
    // are 'canonical' objects

    constexpr bool is_canonical(kind x) noexcept {
        static_assert(kinds == 2);
        switch (x) {
        case kind::primitive:   return true;    break;
        case kind::function:    return true;    break;
        default:                return bool{};  break;
        }
    }
}


YAMA_SETUP_FORMAT(yama::kind, yama::fmt_kind(x));

