

#pragma once


#include <cstdint>
#include <string>
#include <format>
#include <iostream>

#include "macros.h"


namespace yama {


    // TODO: later on, when we add structs being able to be not default initializable,
    //       we'll need to revise our code (such as w/ ctx cmd/bcode instr default_init)
    //       which presumes that they always are
    //
    //       also, be sure to update compilation-tests.cpp, in 'general', 'struct decl'
    //       and 'default init expr' sections

    // yama::kind defines the different *kinds of* types in Yama

    enum class kind : uint8_t {
        primitive,
        function,
        struct0,

        num,        // this is not a valid kind of type
    };

    constexpr size_t kinds = size_t(kind::num);


    std::string fmt_kind(kind x);
}


YAMA_SETUP_FORMAT(yama::kind, yama::fmt_kind(x));

