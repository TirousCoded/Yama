

#pragma once


#include <concepts>

#include "../yama/yama.h"
#include "general.h"
#include "resources.h"
#include "Safe.h"


namespace ym {


    // Base class of handles to Yama resources.
    // RAII manages RC/ARC resources.
    template<typename T>
    class Handle {
    public:
        inline explicit Handle(Safe<T> resource) :
            _res(resource) {
        }

        // NOTE: Non-RAII Handle derivatives don't need dtor behaviour, and so we'll
        //       just not bother making the dtor virtual in this case.

        ~Handle() noexcept = default;

        Handle(const Handle&) = default;
        Handle(Handle&&) noexcept = default;
        Handle& operator=(const Handle&) = default;
        Handle& operator=(Handle&&) noexcept = default;


        inline bool operator==(std::convertible_to<Safe<T>> auto const& x) const noexcept { return get() == x; }
        inline bool operator!=(std::convertible_to<Safe<T>> auto const& x) const noexcept { return !(*this == x); }

        inline Safe<T> get() const noexcept { return Safe(_res); }
        inline operator Safe<T>() const noexcept { return get(); } // Implicit
        inline operator T* () const noexcept { return get(); } // Implicit


        // Dissociates the handle from its resource, invalidating the handle.
        inline void disown() noexcept {
            _res = nullptr;
        }


    private:
        // _res is raw pointer so _res has a value when moved-from.
        T* _res = nullptr;
    };

    // Base class of handles to Yama resources.
    // RAII manages RC/ARC resources.
    template<RefCountedRes T>
    class Handle<T> {
    public:
        // Does not increment the ref count of resource.
        inline explicit Handle(Safe<T> resource) noexcept :
            _res(resource) {
        }

        inline virtual ~Handle() noexcept {
            release(); // RAII
        }

        inline Handle(const Handle& other) :
            Handle(other.get()) {
            ym::secure(other);
        }
        inline Handle(Handle&& other) noexcept :
            Handle(other.get()) {
            other.disown();
        }
        inline Handle& operator=(const Handle& other) {
            if (&other != this) {
                release();
                _res = other._res;
            }
            return *this;
        }
        inline Handle& operator=(Handle&& other) noexcept {
            if (&other != this) {
                release();
                std::swap(_res, other._res); // Invalidate other.
            }
            return *this;
        }


        inline YmRefCount refCount() const noexcept {
            return ym::refCount(get());
        }


        inline bool operator==(std::convertible_to<Safe<T>> auto const& x) const noexcept { return get() == x; }
        inline bool operator!=(std::convertible_to<Safe<T>> auto const& x) const noexcept { return !(*this == x); }

        inline Safe<T> get() const noexcept { return Safe(_res); }
        inline operator Safe<T>() const noexcept { return get(); } // Implicit
        inline operator T* () const noexcept { return get(); } // Implicit


        // Dissociates the handle from its resource, invalidating the handle.
        // Does not release the resource of the handle.
        inline void disown() noexcept {
            _res = nullptr;
        }
        // Dissociates the handle from its resource, invalidating the handle.
        // Releases the resource of the handle.
        inline void release() noexcept {
            ym::release(_res);
            // NOTE: dtor will double-free if disown isn't called here.
            disown();
        }


    private:
        // _res is raw pointer so _res has a value when moved-from.
        T* _res = nullptr;
    };
}

