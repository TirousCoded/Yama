

#pragma once

#include <taul/hashing.h>

#include "../yama/yama.h"


namespace _ym {


    // Raw pointer guaranteed to not be nullptr.
    // Provides a std::optional-like interface.
    template<typename T>
    class Safe final {
    public:
        constexpr Safe(T& x) noexcept : _ptr(&x) {}
        // Fail asserts false or throws std::invalid_argument based on if YM_DEBUG is defined.
        inline explicit Safe(T* x) : _ptr(x) { ymAssert(x != nullptr); } // Explicit
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

        constexpr operator T* () const noexcept { return get(); } // Implicit
        template<typename U>
        constexpr operator U* () const noexcept { return into<U>(); } // Explicit

        template<typename U>
        constexpr Safe<U> into() const noexcept { return Safe<U>(*this); }

        inline size_t hash() const noexcept { return taul::hash(get()); }


    private:
        T* _ptr = nullptr;
    };
}

template<typename T>
struct std::hash<_ym::Safe<T>> {
    inline size_t operator()(const _ym::Safe<T>& x) const noexcept {
        return x.hash();
    }
};

