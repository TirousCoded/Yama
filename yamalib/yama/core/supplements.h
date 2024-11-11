

#pragma once


#include "type_info.h"


namespace yama {


    // make_supplements generates a vector of utility fns type_info(s) for useful
    // fns which make Yama a lot more usable given its current spartan state

    // these fn impls have not been unit tested, and are temporary, w/ plans to
    // remove make_supplements in the future

    std::vector<type_info> make_supplements();
}

