

#pragma once


#include "meta.h"


namespace ym {


    // TODO: Below hashing stuff hasn't been unit tested.

    constexpr size_t hashCombine(size_t a, size_t b) noexcept {
        // NOTE: Got this from cppreference.
        return a ^ (b << 1);
    }

    template<Hashable T>
    inline size_t hash(const T& x) noexcept {
        return std::hash<T>{}(x);
    }

    template<Hashable Arg, Hashable... Args>
    inline size_t hash(const Arg& arg, const Args&... args) noexcept {
        return hashCombine(hash(arg), hash(args...));
    }
}

