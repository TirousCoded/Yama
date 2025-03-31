

#pragma once


#include <taul/hashing.h>

#include "../core/asserts.h"


namespace yama::internal {
    

    // TODO: maybe bring this to the frontend

    // safeptr is a raw pointer wrapper which guarantees the pointer is non-null (ie. it's safe)

    // safeptr also provides a std::optional-like interface

    template<typename T>
    class safeptr final {
    public:
        constexpr safeptr(T& x) noexcept : _ptr(&x) {}
        inline explicit safeptr(T* x) noexcept : safeptr(deref_assert(x)) {} // explicit convert

        safeptr() = delete;
        constexpr safeptr(const safeptr&) = default;
        constexpr safeptr(safeptr&&) noexcept = default;

        constexpr ~safeptr() noexcept = default;

        constexpr safeptr& operator=(const safeptr&) = default;
        constexpr safeptr& operator=(safeptr&&) noexcept = default;


        constexpr T* get() const noexcept { return _ptr; }
        constexpr T* operator->() const noexcept { return get(); }

        constexpr operator T* () const noexcept { return get(); } // implicit convert

        constexpr T& value() const noexcept { return *_ptr; }
        constexpr T& operator*() const noexcept { return value(); }

        constexpr bool operator==(const safeptr&) const noexcept = default;

        inline size_t hash() const noexcept { return taul::hash(_ptr); }


    private:
        T* _ptr;
    };
}

template<typename T>
struct std::hash<yama::internal::safeptr<T>> {
    size_t operator()(const yama::internal::safeptr<T>& x) const noexcept {
        return x.hash();
    }
};

