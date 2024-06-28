

#pragma once


#include <taul/str.h>


namespace yama {


    // yama will make use of TAUL's immutable string type str as its
    // own main string type

    // throughout the API, little quality-of-life things are intended
    // to be provided here-and-there to help the end-user avoid having
    // to stray too far from the std::string-based way of doing things

    using str = taul::str;

    namespace string_literals {
        using namespace taul::string_literals;
    }


    // some useful concepts

    template<typename T>
    concept hashable = requires(T a)
    {
        { std::hash<T>{}(a) } -> std::convertible_to<size_t>;
    };
}

