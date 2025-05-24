

#pragma once


#include <cstdint>
#include <limits>


namespace yama {


    // TODO: if we ever make a C rewrite of Yama, we could reimpl newtop_t via a preprocessor
    //       definition called something like 'yama_newtop' which expands to '(-1)', which in
    //       turn can then be converted into a 8-bit or 32-bit uint of the max value of the
    //       particular type in question


    // in our system 'newtop' is a special register pseudo-index used to signal to the
    // system that it should push a new register to the local object stack, rather than
    // writing to an existing one

    // in yama::context, newtop corresponds to the max size_t value, however in our bcode,
    // newtop instead corresponds to the max uint8_t value

    // rather than the confusion of having two different 'newtop' constants, we'll define
    // this newtop_t struct, then have it be able to implicit cast into size_t and uint8_t,
    // at which point it resolves to the expected value for that context

    
    // take note that newtop's corresponding size_t/uint8_t values are only special in
    // certain circumstances, w/ them outside those circumstances otherwise being treated
    // like any other register index


    struct newtop_t final {
        constexpr operator size_t() const noexcept { return std::numeric_limits<size_t>::max(); }
        constexpr operator uint8_t() const noexcept { return std::numeric_limits<uint8_t>::max(); }
    };


    constexpr newtop_t newtop = newtop_t{};


    static_assert(
        []() constexpr -> bool {
            constexpr size_t a = newtop;
            constexpr uint8_t b = newtop;
            return
                a == std::numeric_limits<size_t>::max() &&
                b == std::numeric_limits<uint8_t>::max();
        }());
}

