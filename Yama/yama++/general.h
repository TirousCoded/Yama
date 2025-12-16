

#pragma once


#include <optional>

#include "../yama/asserts.h"
#include "meta.h"


namespace ym {


    template<Dereferenceable T>
    inline auto&& deref(T&& x) noexcept {
        ymAssert((bool)x);
        return *x;
    }

    // NOTE: See https://www.youtube.com/watch?v=0yJk5yfdih0 for details on the problem
    //       which retopt exists to try and solve.

    // Uses std::move to wrap x in a std::optional.
    // This exists to help optimize cases where a value needs to be returned such that it's wrapped in a std::optional.
    template<typename T>
    constexpr std::optional<T> retopt(T&& x) noexcept {
        return std::optional<T>(std::move(x));
    }

    // NOTE: See https://www.youtube.com/watch?v=et1fjd8X1ho for details.

    template<typename... Callable>
    struct Visitor : Callable... {
        using Callable::operator()...;
    };
}

