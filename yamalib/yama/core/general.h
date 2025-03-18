

#pragma once


#include <utility>
#include <iterator>

#include <taul/str.h>


namespace yama {


    // useful signed size_t

    using ssize_t = std::make_signed_t<size_t>;


    // yama will make use of TAUL's immutable string type str as its
    // own main string type

    // throughout the API, little quality-of-life things are intended
    // to be provided here-and-there to help the end-user avoid having
    // to stray too far from the std::string-based way of doing things

    using str = taul::str;

    namespace string_literals {
        using namespace taul::string_literals;
    }
}

