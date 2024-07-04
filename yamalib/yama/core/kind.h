

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

    constexpr size_t kind_count = size_t(kind::num);


    std::string fmt_kind(kind x);
}


YAMA_SETUP_FORMAT(yama::kind, yama::fmt_kind(x));

