

#pragma once


#include <concepts>

#include "../yama/yama.h"
#include "general.h"
#include "resources.h"
#include "Safe.h"


namespace ym {


    // Base class of handles to Yama resources.
    // Destroyable resources have non-copyable RAII handles.
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


    private:
        // _res is raw pointer so _res has a value when moved-from.
        T* _res = nullptr;
    };

    // Base class of handles to Yama resources.
    // Destroyable resources have non-copyable RAII handles.
    template<UserManagedRes T>
    class Handle<T> {
    public:
        inline explicit Handle(Safe<T> resource) noexcept :
            _res(resource) {
        }

        inline virtual ~Handle() noexcept {
            drop(); // RAII
        }

        // NOTE: Resources which can be destroyed won't get copy ctor/assign.

        Handle(const Handle&) = delete;
        Handle(Handle&&) noexcept = default;
        Handle& operator=(const Handle&) = delete;
        Handle& operator=(Handle&&) noexcept = default;


        inline bool operator==(std::convertible_to<Safe<T>> auto const& x) const noexcept { return get() == x; }
        inline bool operator!=(std::convertible_to<Safe<T>> auto const& x) const noexcept { return !(*this == x); }

        inline Safe<T> get() const noexcept { return Safe(_res); }
        inline operator Safe<T>() const noexcept { return get(); } // Implicit
        inline operator T* () const noexcept { return get(); } // Implicit


        // Disowns the ownership this handle's resource, invalidating the handle.
        inline void disown() noexcept {
            _res = nullptr;
        }

        // Destroys the resource owned by this handle, invalidating the handle.
        inline void drop() noexcept {
            destroy(_res);
            // NOTE: This stops dtor from attempting destroy.
            disown();
        }


    private:
        // _res is raw pointer so _res has a value when moved-from.
        T* _res = nullptr;
    };
}

