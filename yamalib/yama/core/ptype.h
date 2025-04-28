

#pragma once


#include <cstdint>
#include <string>
#include <format>
#include <iostream>

#include "macros.h"


namespace yama {


    // ptype defines the different kinds of primitive
    // values a Yama primitive type can encapsulate

    // the usage of '0' below is to avoid conflicts w/ C++ keywords

    enum class ptype : uint8_t {
        none,
        int0,
        uint,
        float0,
        bool0,
        char0,
        type,

        num,        // this is not a valid type of primitive
    };

    constexpr size_t ptypes = size_t(ptype::num);


    std::string fmt_ptype(ptype x);
}


YAMA_SETUP_FORMAT(yama::ptype, yama::fmt_ptype(x));

