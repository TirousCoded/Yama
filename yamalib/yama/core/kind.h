

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

    bool uses_callsig(kind x) noexcept;
}


YAMA_SETUP_FORMAT(yama::kind, yama::fmt_kind(x));

