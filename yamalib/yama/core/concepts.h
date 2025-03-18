

#pragma once


#include <utility>


namespace yama {


    template<typename T>
    concept hashable_type =
        requires (const T v)
    {
        { std::hash<T>{}(v) } noexcept -> std::convertible_to<size_t>;
    };


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


    template<typename T>
    concept basic_lockable_type =
        requires (T v)
    {
        v.lock();
        v.unlock();
    };

    template<typename T>
    concept lockable_type =
        requires (T v)
    {
        requires basic_lockable_type<T>;
        { v.try_lock() } -> std::convertible_to<bool>;
    };
}

