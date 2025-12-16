

#pragma once


#include <memory>
#include <span>
#include <concepts>

#include "../yama/scalars.h"


namespace _ym {


    // TODO: HAStruct impl is NOT finished yet.


    template<typename T>
    concept HAStructHeader =
        std::copy_constructible<T> &&
        std::move_constructible<T> &&
        std::destructible<T> &&
        std::is_move_assignable_v<T> &&
        requires (T v)
    {
        // Queries array size.
        { v.size() } noexcept -> std::convertible_to<size_t>;
    };

    template<typename T>
    concept HAStructElement =
        std::default_initializable<T> &&
        std::copy_constructible<T> &&
        std::destructible<T>;


    // TODO: The concept of HAStruct(s) being accessed via pointers to their header, w/
    //       HAStruct basically being a static class, differs from ha_struct, and hasn't
    //       been fully fleshed out yet.

    template<HAStructHeader T, HAStructElement Element>
    class HAStructHeaderInterface {
    public:
        HAStructHeaderInterface() = default;


        //
    };


    // HAStruct (Header/Array Struct) is used to help define types composed of a header
    // section, and then an array of elements where the array's length is defined at
    // initialization using runtime information.
    //
    // The header type is required to provide a method 'size' which specifies the length
    // of the array at runtime.
    //
    // HAStruct is designed such that pointers to an HAStruct are aliased as pointers
    // to their header, w/ the header implementing HAStructHeaderInterface which implements
    // into the header an interface methods used to access the array.
    template<HAStructHeader Header, HAStructElement Element>
    class HAStruct final {
    public:
        // Requiring 'alignof(Header) == alignof(Elem)' makes this impl a lot easier, as
        // we can know for 100% certain that the memory address immediately following
        // the header will be the address of the first element.
        static_assert(alignof(Header) == alignof(Element));

        // the memory block for the ha_struct will be an array of these 'Units', which
        // satisfy 'sizeof(Unit) == alignof(Unit) == alignof(Header)'.
        struct alignas(alignof(Header)) Unit final {
            YmUInt8 v[sizeof(alignof(Header))] = { 0 };
        };

        static_assert(sizeof(Unit) == alignof(Header));
        static_assert(alignof(Unit) == alignof(Header));
        static_assert(std::is_trivially_copyable_v<Unit>);

        // The number of units per header.
        static constexpr size_t unitsPerHeader = sizeof(Header) / sizeof(Unit);
        // The number of units per element.
        static constexpr size_t unitsPerElement = sizeof(Element) / sizeof(Unit);


        //


    private:

    };
}

