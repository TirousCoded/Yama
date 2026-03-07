

#pragma once


#include <concepts>
#include <memory>

#include "../yama/asserts.h"
#include "../yama/scalars.h"
#include "../yama++/meta.h"


namespace _ym {


    template<typename T>
    concept HALHeader =
        std::move_constructible<T> &&
        std::destructible<T> &&
        requires (const T v)
    {
        // Queries array size.
        { v.size() } noexcept -> std::convertible_to<size_t>;
    };

    template<typename T>
    concept HALElement =
        std::default_initializable<T> &&
        std::destructible<T>;


    // Header/Array Layouts (or HALs) is used to define data structures composed of a
    // header struct, followed by an array of element structs, all stored contiguously
    // within the same memory block.
    //
    // The header type is required to provide a method 'size' which specifies the length
    // of the array at runtime.
    // 
    // The HAL class itself is a static class exposing an interface by which to use HAL
    // data structures. HAL data structures are identified using pointers/references to
    // the header.
    template<HALHeader Header, HALElement Element>
    class HAL final {
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


        HAL() = delete;
        

        inline static size_t size(const Header& object) noexcept {
            return object.size();
        }

        inline static Element* elementPtr(Header& object, size_t index) noexcept {
            return (Element*)(((Unit*)&object) + unitsPerHeader + unitsPerElement * index);
        }
        inline static const Element* elementPtr(const Header& object, size_t index) noexcept {
            return (const Element*)(((Unit*)&object) + unitsPerHeader + unitsPerElement * index);
        }
        inline static Element& element(Header& object, size_t index) noexcept {
            ymAssert(index < size(object));
            return *elementPtr(object, index);
        }
        inline static const Element& element(const Header& object, size_t index) noexcept {
            ymAssert(index < size(object));
            return *elementPtr(object, index);
        }

        template<ym::Allocator Alloc, typename AllocTraits = std::allocator_traits<Alloc>>
        inline static Header* create(Header&& header, Alloc& al) {
            auto s = size(header);
            Header* result = (Header*)AllocTraits::template rebind_alloc<Unit>(al).allocate(unitsPerHeader + unitsPerElement * s);
            ymAssert(result != nullptr);
            std::construct_at(result, std::forward<Header>(header));
            for (size_t i = 0; i < s; i++) {
                std::construct_at(elementPtr(*result, i));
            }
            return result;
        }
        inline static Header* create(Header&& header) {
            std::allocator<Unit> al{};
            return create(std::forward<Header>(header), al);
        }

        template<ym::Allocator Alloc, typename AllocTraits = std::allocator_traits<Alloc>>
        inline static void destroy(Header& object, Alloc& al) noexcept {
            auto s = size(object); // Query before destroying &object.
            std::destroy_at(&object);
            for (size_t i = 0; i < s; i++) {
                std::destroy_at(elementPtr(object, i));
            }
            AllocTraits::template rebind_alloc<Unit>(al).deallocate((Unit*)&object, unitsPerHeader + unitsPerElement * s);
        }
        inline static void destroy(Header& object) noexcept {
            std::allocator<Unit> al{};
            destroy(object, al);
        }

        template<ym::Allocator Alloc, typename AllocTraits = std::allocator_traits<Alloc>>
        inline static Header* clone(const Header& object, Alloc& al)
            requires std::copy_constructible<Header> && std::copy_constructible<Element> {
            Header* result = (Header*)AllocTraits::template rebind_alloc<Unit>(al).allocate(unitsPerHeader + unitsPerElement * size(object));
            ymAssert(result != nullptr);
            std::construct_at(result, object);
            for (size_t i = 0; i < size(object); i++) {
                std::construct_at(elementPtr(*result, i), element(object, i));
            }
            return result;
        }
        inline static Header* clone(const Header& object)
            requires std::copy_constructible<Header> && std::copy_constructible<Element> {
            std::allocator<Unit> al{};
            return clone(object, al);
        }
    };
}

