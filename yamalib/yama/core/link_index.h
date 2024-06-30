

#pragma once


#include <cstddef>
#include <string>
#include <concepts>

#include "general.h"


namespace yama {


    // link_index is used to communicate when an index value is intended to 
    // refer specifically to an entry in a type's link (symbol) table

    using link_index = size_t;


    // NOTE: link_formatter has no relation to std::formatter

    // link_formatter defines a movable/copyable function object which 
    // can take in a link_index and resolve its yama::str fullname as 
    // a formatted string

    // link_formatter is expected to be cheap to copy around

    // link_formatter usage is designed around use a template argument, 
    // rather than a function pointer, as the former is able to handle 
    // things like lambdas w/ captured values

    template<typename T>
    concept link_formatter =
        std::movable<T> &&
        std::copyable<T> &&
        callable_r_type<T, std::string, link_index>;

    template<link_formatter Formatter>
    inline std::string fmt_link(link_index index, Formatter fmt) {
        return fmt(index);
    }
}

