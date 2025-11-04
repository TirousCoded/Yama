

#pragma once


#include "../yama++/Safe.h"
#include "../yama++/ScopedDrop.h"
#include "Resource.h"


namespace _ym {


    // NOTE: I changed some of the std::derived_from<Resource> below to typename as I was
    //       getting some annoying issues where classes that were only forward declared
    //       won't work w/ it when I wanted them to.


    // A RAII automatic RC/ARC wrapper around resources.
    // Use ym::Safe for borrowed references.
    template<typename T>
    class Res final {
    public:
        inline explicit Res(ym::Safe<T> x) noexcept : // Explicit
            _raii(x) {
            _value().addRef();
        }
        template<std::derived_from<Resource> U>
        inline explicit Res(ym::Safe<U> x) noexcept : Res(ym::Safe<T>(x)) {} // Explicit

        inline explicit Res(T& x) noexcept : Res(ym::Safe<T>(x)) {} // Explicit
        template<std::derived_from<Resource> U>
        inline explicit Res(U& x) noexcept : Res(ym::Safe<U>(x)) {} // Explicit

        inline explicit Res(T* x) : Res(ym::Safe<T>(x)) {} // Explicit
        template<std::derived_from<Resource> U>
        inline explicit Res(U* x) : Res(ym::Safe<U>(x)) {} // Explicit

        Res() = delete;
        inline Res(const Res& other) :
            _raii(other._get()) {
            _value().addRef();
        }
        Res(Res&&) noexcept = default;
        inline Res& operator=(const Res& other) {
            _raii = ym::ScopedDrop(other._get());
            _value().addRef();
            return *this;
        }
        Res& operator=(Res&&) noexcept = default;


        inline bool operator==(const Res& other) const noexcept { return _get() == other._get(); }
        inline bool operator!=(const Res& other) const noexcept { return !(*this == other); }

        inline T* get() const noexcept { return _get(); }
        inline T& value() const noexcept { return _value(); }

        inline T* operator->() const noexcept { return get(); }
        inline T& operator*() const noexcept { return value(); }

        inline ym::Safe<T> borrow() const noexcept { return value(); }

        // NOTE: Unlike Safe, Res isn't safe to have implicit conversion to things like raw pointers,
        //       as doing so could result in scenarios like returning a Res resulting in a raw pointer
        //       implicit convert, only for the Res drop resulting in the pointer quietly becoming
        //       dangling.

        inline explicit operator T* () const noexcept { return get(); } // Explicit
        template<std::derived_from<Resource> U>
        inline explicit operator U* () const noexcept { return borrow().into<U>(); } // Explicit

        inline explicit operator ym::Safe<T> () const noexcept { return borrow(); } // Explicit
        template<std::derived_from<Resource> U>
        inline explicit operator ym::Safe<U> () const noexcept { return borrow().into<U>(); } // Explicit

        template<std::derived_from<Resource> U>
        inline Res<U> into() const noexcept { return Res<U>(*this); }
        template<std::derived_from<Resource> U>
        inline Res<U> downcastInto() const noexcept { return Res<U>(borrow().downcastInto<U>()); }

        inline size_t hash() const noexcept { return borrow().hash(); }


        // Disarms the RAII mechanism of the Res, invalidating the Res object, and returns the resource pointer it held.
        // This is used when we need to extract a reference w/out ymDrop-ing it (ie. for reference stealing.)
        inline ym::Safe<T> disarm() noexcept { return ym::Safe<T>(_raii.disarm()); }


        // Steal ownership of existing reference pointer (ie. don't ymAddRef it.)
        template<std::derived_from<Resource> U>
        static inline Res steal(ym::Safe<U> x) noexcept { return Res(x, _StealTag{}); }


    private:
        // We wrap our resource in this for RAII release.
        ym::ScopedDrop _raii;


        struct _StealTag {};
        template<std::derived_from<Resource> U>
        inline Res(ym::Safe<U> x, _StealTag) noexcept : _raii(x) {}


        inline T* _get() const noexcept {
            return static_cast<T*>(_raii.get());
        }
        inline T& _value() const noexcept {
            ym::assertSafe(_get());
            return *_get();
        }
    };
}

template<std::derived_from<_ym::Resource> T>
struct std::hash<_ym::Res<T>> {
    inline size_t operator()(const _ym::Res<T>& x) const noexcept {
        return x.hash();
    }
};

namespace _ym {


    template<std::derived_from<Resource> T>
    static inline T* disarmOrNull(std::optional<Res<T>>&& x) noexcept {
        std::optional temp(std::forward<decltype(x)>(x));
        return
            temp
            ? temp->disarm().get()
            : nullptr;
    }
}

