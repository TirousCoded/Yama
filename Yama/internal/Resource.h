

#pragma once


#include <concepts>


namespace _ym {


    template<typename T>
    concept Resource =
        requires (const std::remove_cvref_t<T> v)
    {
        typename T::Name;
        { v.getName() } noexcept -> std::convertible_to<const typename T::Name&>;
    };
}

