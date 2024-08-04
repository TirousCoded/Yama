

#pragma once


#include <cstdint>
#include <limits>

#include <taul/unicode.h>


namespace yama {


    // the below five types are used to represent the values of primitive
    // scalar values found in the Yama language

    using int_t = int64_t;
    using uint_t = uint64_t;
    using float_t = double;
    using bool_t = bool;
    using char_t = taul::unicode_t;


    static_assert(sizeof(int_t) == sizeof(int64_t)); // make sure range is correct
    static_assert(sizeof(uint_t) == sizeof(int64_t)); // make sure range is correct
    static_assert(sizeof(float_t) == sizeof(int64_t)); // make sure double precision IEEE 754
    static_assert(sizeof(bool_t) == sizeof(int8_t));
    static_assert(sizeof(char_t) == sizeof(int32_t)); // make sure range is correct
}

