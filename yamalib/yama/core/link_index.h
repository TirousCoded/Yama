

#pragma once


#include <cstddef>
#include <string>
#include <concepts>

#include "general.h"


namespace yama {


    // link_index is used to communicate when an index value is intended to 
    // refer specifically to an entry in a type's link (symbol) table

    using link_index = size_t;
}

