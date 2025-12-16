

#pragma once


#include <concepts>

#include "../yama/yama.h"

#include "hash.h"


namespace ym {


    // TODO: Safe throwing std::invalid_argument in release mode hasn't been unit tested.
    
    // TODO: Safe<void>, Safe<const void>, etc. currently don't compile, and in order to fix issue
    //       I added a quick-n'-dirty cast to void* to compensate, but the real solution would
    //       probably be a template specialization (also, no unit tests for this yet either.)

    // Raw pointer guaranteed to not be nullptr.
    // Provides a std::optional-like interface.
    template<typename T>
    class Safe final {
    public:
        constexpr Safe(T& x) noexcept : _ptr(&x) {}
        // Fail asserts false or throws std::invalid_argument based on if YM_DEBUG is defined.
        inline explicit Safe(T* x) : _ptr(x) { // Explicit
            ymAssert(x != nullptr);
#ifndef YM_DEBUG
            if (!x) throw std::invalid_argument("ym::Safe cannot initialize to nullptr!");
#endif
        }
        // Fail asserts false or throws std::invalid_argument based on if YM_DEBUG is defined.
        template<typename U>
        inline explicit Safe(U* x) : Safe(static_cast<T*>(x)) {} // Explicit
        template<typename U>
        inline explicit Safe(Safe<U> other) noexcept : Safe(static_cast<T&>(other.value())) {} // Explicit
        Safe(std::nullptr_t) = delete;

        Safe() = delete;
        constexpr Safe(const Safe&) = default;
        constexpr Safe(Safe&&) noexcept = default;
        constexpr ~Safe() noexcept = default;
        constexpr Safe& operator=(const Safe&) = default;
        constexpr Safe& operator=(Safe&&) noexcept = default;


        constexpr bool operator==(const Safe&) const noexcept = default;

        constexpr T* get() const noexcept { return _ptr; }
        constexpr T& value() const noexcept { return *_ptr; }

        constexpr T* operator->() const noexcept { return get(); }
        constexpr T& operator*() const noexcept { return value(); }
        constexpr T& operator[](size_t x) const noexcept { return get()[x]; }

        constexpr operator T* () const noexcept { return get(); } // Implicit
        template<typename U>
        constexpr explicit operator U* () const noexcept { return into<U>(); } // Explicit

        // TODO: Temporary until we resolve Safe<void> not compiling.
        constexpr explicit operator void* () const noexcept { return (void*)get(); } // Explicit
        constexpr explicit operator const void* () const noexcept { return (void*)get(); } // Explicit

        template<typename U>
        constexpr Safe<U> into() const noexcept { return Safe<U>(*this); }
        template<typename U>
        inline Safe<U> downcastInto() const noexcept {
            ymAssert(dynamic_cast<U*>(get()) != nullptr);
            return Safe<U>((U*)get()); // Skip safety check in release mode.
        }

        inline size_t hash() const noexcept { return ym::hash(get()); }

        template<std::integral U>
        constexpr Safe operator+(const U& x) const noexcept { return Safe(get() + x); }
        template<std::integral U>
        constexpr Safe operator-(const U& x) const noexcept { return Safe(get() - x); }

        template<std::integral U>
        constexpr Safe& operator+=(const U& x) noexcept { return *this = *this + x; }
        template<std::integral U>
        constexpr Safe& operator-=(const U& x) noexcept { return *this = *this - x; }

        constexpr Safe& operator++() noexcept { return *this += 1; }
        constexpr Safe& operator--() noexcept { return *this -= 1; }
        constexpr Safe operator++(int) noexcept { auto old = *this; ++*this; return old; }
        constexpr Safe operator--(int) noexcept { auto old = *this; --*this; return old; }


    private:
        T* _ptr = nullptr;
    };
}

template<typename T>
struct std::hash<ym::Safe<T>> {
    inline size_t operator()(const ym::Safe<T>& x) const noexcept {
        return x.hash();
    }
};

namespace ym {


    template<typename T>
    inline void assertSafe(T* x) noexcept {
        // Disable if !defined(YM_DEBUG) to avoid throwing.
#ifdef YM_DEBUG
        (void)Safe(x);
#endif
    }
}

