

#pragma once


#include <utility>

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
    concept hashable_type = 
        requires (const T v)
    {
        { std::hash<T>{}(v) } -> std::convertible_to<size_t>;
    };

    template<typename To, typename From>
    concept convertible_from =
        std::convertible_to<From, To>;

    template<typename T, typename... Args>
    concept callable_type =
        requires (T f, Args&&... args)
    {
        f(std::forward<Args>(args)...);
    };

    template<typename T, typename Returns, typename... Args>
    concept callable_r_type =
        requires (T f, Args&&... args)
    {
        { f(std::forward<Args>(args)...) } -> std::convertible_to<Returns>;
    };
}

