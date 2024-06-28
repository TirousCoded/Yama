

#pragma once


#include <cstdint>
#include <memory>
#include <span>
#include <concepts>

#include "../core/asserts.h"


namespace yama::internal {


    // 'ha' stands for 'header-array'

    // this is a useful little system for creating datastructures where you
    // have a single memory block, the first part of which encapsulates a
    // 'header' struct, and then after it is an array of elements of some
    // 'element' type, w/ this array's length being defined at runtime

    // I like these data structures as they're really useful in helping us
    // avoid unneeded memory indirection


    template<typename T>
    concept ha_struct_header =
        std::copy_constructible<T> &&
        std::move_constructible<T> &&
        std::destructible<T> &&
        std::is_move_assignable_v<T> &&
        requires (T v)
    {
        // length method for querying array length

        { v.length() } noexcept -> std::convertible_to<size_t>;
    };

    template<typename T>
    concept ha_struct_element =
        std::default_initializable<T> &&
        std::copy_constructible<T> &&
        std::destructible<T>;


    // it's really useful to be able to identify a ha_struct by a type-erased
    // pointer-like handle which can later be used to create a ha_struct w/
    // the erased type information

    // ha_struct_anon_ref exists for this purpose, being able to be created
    // from a ha_struct (see anon_ref) and can be used to then create a
    // non-type-erased ha_struct (see from_anon_ref)

    // ha_struct_anon_ref has a nullptr state (ie. it's literally just a void*)

    using ha_struct_anon_ref = void*;


    // ha_struct is a lightweight trivially-copyable struct encapsulating
    // a pointer to the datastructures, providing an API to interact w/ it

    template<ha_struct_header Header, ha_struct_element Elem>
    struct ha_struct final {

        // requiring 'alignof(Header) == alignof(Elem)' makes this impl a lot easier, as
        // we can know for 100% certain that the memory address immediately following
        // the header will be the address of the first element

        static_assert(alignof(Header) == alignof(Elem));


        // the memory block for the ha_struct will be an array of these 'units', which
        // satisfy 'sizeof(unit) == alignof(unit) == alignof(Header)'

        struct alignas(alignof(Header)) unit final {
            uint8_t v[sizeof(alignof(Header))] = { 0 };
        };

        static_assert(sizeof(unit) == alignof(Header));
        static_assert(alignof(unit) == alignof(Header));
        static_assert(std::is_trivially_copyable_v<unit>);

        // the number of units per header/element

        static constexpr size_t units_per_header = sizeof(Header) / sizeof(unit);
        static constexpr size_t units_per_elem = sizeof(Elem) / sizeof(unit);


        unit* _ptr;


        inline ha_struct_anon_ref anon_ref() const noexcept { return (ha_struct_anon_ref)_ptr; }

        // header returns the header of the ha_struct

        inline Header& header() noexcept { return *_header_ptr(); }
        inline const Header& header() const noexcept { return *_header_ptr(); }

        // we'll use '->' for summary access to the header, as that could be useful

        inline Header* operator->() noexcept { return _header_ptr(); }
        inline const Header* operator->() const noexcept { return _header_ptr(); }

        // elems returns a span of the ha_struct's element array

        inline std::span<Elem> elems() noexcept { return std::span<Elem>(_post_header_ptr(), header().length()); }
        inline std::span<const Elem> elems() const noexcept { return std::span<const Elem>(_post_header_ptr(), header().length()); }

        // ha_struct may be compared by reference

        inline bool operator==(const ha_struct<Header, Elem>& other) const noexcept { return _ptr == other._ptr; }


        // from_ptr is used to initialize a ha_struct from a type-erased 
        // ha_struct_anon_ref acquired using anon_ref

        static inline ha_struct<Header, Elem> from_anon_ref(ha_struct_anon_ref x) noexcept;

        // these are used to create/destroy datastructures, w/ manual memory
        // management being required at this low-level

        // create takes a header value instead of 'Args&&... args', as the header
        // needs to be available prior to allocation in order to get the desired
        // element array length

        // the use of an allocator is REQUIRED, and the end-user must provide 
        // the allocator to use for create/destroy of ha_struct

        template<typename Allocator>
        static inline ha_struct<Header, Elem> create(Allocator al, Header&& header);

        template<typename Allocator>
        static inline void destroy(Allocator al, ha_struct<Header, Elem> x) noexcept;

        // clone is a special version of create which creates a new datastructure
        // who's header/elements have been copy-constructed from other

        template<typename Allocator>
        static inline ha_struct<Header, Elem> clone(Allocator al, ha_struct<Header, Elem> other);


    private:

        inline Header* _header_ptr() const noexcept;
        inline Elem* _elem_ptr(size_t index) const noexcept;

        // this returns _elem_ptr(0), but w/out the bounds checking assertion,
        // which isn't desired if elems is called on a zero-length ha_struct

        inline Elem* _post_header_ptr() const noexcept;
    };

    template<ha_struct_header Header, ha_struct_element Elem>
    inline ha_struct<Header, Elem> ha_struct<Header, Elem>::from_anon_ref(ha_struct_anon_ref x) noexcept {
        YAMA_ASSERT(x);
        return ha_struct<Header, Elem>{ (unit*)x };
    }

    template<ha_struct_header Header, ha_struct_element Elem>
    template<typename Allocator>
    inline ha_struct<Header, Elem> ha_struct<Header, Elem>::create(Allocator al, Header&& header) {
        Header _header(std::forward<Header>(header));
        size_t length = _header.length();

        // prepare allocator to use
        using _Traits = std::allocator_traits<Allocator>::template rebind_traits<unit>;
        using _Allocator = typename _Traits::allocator_type;
        // init like this will ensure _al works when al is a *stateful* allocator
        _Allocator _al(al);

        // allocate memory block
        size_t required_units = units_per_header + units_per_elem * length;
        auto block = _Traits::allocate(_al, required_units);
        YAMA_ASSERT(block);

        // prepare result (prior to initialization)
        ha_struct<Header, Elem> result{ block };

        // TODO: can we use if-constexpr here to avoid unneeded work if header/elems are
        //       trivially constructible in some way?

        // initialize header and elems
        _Traits::construct(_al, result._header_ptr(), std::move(_header));
        for (size_t i = 0; i < length; i++) {
            _Traits::construct(_al, result._elem_ptr(i)); // default ctor
        }

        return result;
    }

    template<ha_struct_header Header, ha_struct_element Elem>
    template<typename Allocator>
    inline void ha_struct<Header, Elem>::destroy(Allocator al, ha_struct<Header, Elem> x) noexcept {
        YAMA_ASSERT(x._ptr);
        size_t length = x->length();

        // prepare allocator to use
        using _Traits = std::allocator_traits<Allocator>::template rebind_traits<unit>;
        using _Allocator = typename _Traits::allocator_type;
        // init like this will ensure _al works when al is a *stateful* allocator
        _Allocator _al(al);

        // TODO: can we use if-constexpr here to avoid unneeded work if header/elems are
        //       trivially destructible?

        // deinitialize header and elems
        _Traits::destroy(_al, x._header_ptr());
        for (size_t i = 0; i < length; i++) {
            _Traits::destroy(_al, x._elem_ptr(i));
        }

        // deallocate memory block
        size_t required_units = units_per_header + units_per_elem * length;
        _Traits::deallocate(_al, x._ptr, required_units);
    }

    template<ha_struct_header Header, ha_struct_element Elem>
    template<typename Allocator>
    inline ha_struct<Header, Elem> ha_struct<Header, Elem>::clone(Allocator al, ha_struct<Header, Elem> other) {
        size_t length = other->length();

        // prepare allocator to use
        using _Traits = std::allocator_traits<Allocator>::template rebind_traits<unit>;
        using _Allocator = typename _Traits::allocator_type;
        // init like this will ensure _al works when al is a *stateful* allocator
        _Allocator _al(al);

        // allocate memory block
        size_t required_units = units_per_header + units_per_elem * length;
        auto block = _Traits::allocate(_al, required_units);
        YAMA_ASSERT(block);

        // prepare result (prior to initialization)
        ha_struct<Header, Elem> result{ block };

        // TODO: can we use if-constexpr here to avoid unneeded work if header/elems are
        //       trivially constructible in some way?

        // initialize header and elems (via copy-constructing)
        _Traits::construct(_al, result._header_ptr(), other.header()); // copy ctor
        for (size_t i = 0; i < length; i++) {
            _Traits::construct(_al, result._elem_ptr(i), other.elems()[i]); // copy ctor
        }

        return result;
    }

    template<ha_struct_header Header, ha_struct_element Elem>
    inline Header* ha_struct<Header, Elem>::_header_ptr() const noexcept {
        YAMA_ASSERT(_ptr);
        return (Header*)_ptr;
    }
    
    template<ha_struct_header Header, ha_struct_element Elem>
    inline Elem* ha_struct<Header, Elem>::_elem_ptr(size_t index) const noexcept {
        YAMA_ASSERT(_ptr);
        YAMA_ASSERT(index < header().length());
        return &(_post_header_ptr()[index]);
    }

    template<ha_struct_header Header, ha_struct_element Elem>
    inline Elem* ha_struct<Header, Elem>::_post_header_ptr() const noexcept {
        YAMA_ASSERT(_ptr);
        return (Elem*)&(_ptr[units_per_header]);
    }
}

