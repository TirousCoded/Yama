

#pragma once


#include <memory>

#include "../yama/yama.h"
#include "Safe.h"


namespace ym {


    template<typename T>
    struct ResTraits final {};

    template<>
    struct ResTraits<YmDm> final {
        static constexpr auto create = ymDm_Create;
        static constexpr auto destroy = ymDm_Destroy;
    };
    template<>
    struct ResTraits<YmCtx> final {
        static constexpr auto create = ymCtx_Create;
        static constexpr auto destroy = ymCtx_Destroy;
    };
    template<>
    struct ResTraits<YmParcelDef> final {
        static constexpr auto create = ymParcelDef_Create;
        static constexpr auto destroy = ymParcelDef_Destroy;
    };
    template<>
    struct ResTraits<YmParcel> final {
        //
    };
    template<>
    struct ResTraits<YmItem> final {
        //
    };


    template<typename T>
    concept UserManagedRes =
        requires
    {
        ResTraits<T>::create;
        ResTraits<T>::destroy;
    };

    // TODO: Below have not been unit tested.

    template<UserManagedRes T, typename... Args>
    inline Safe<T> create(Args&&... args) {
        return Safe(ResTraits<T>::create(std::forward<Args>(args)));
    }
    // Fails quietly if x == nullptr.
    template<UserManagedRes T>
    inline void destroy(T* x) noexcept {
        if (x) {
            ResTraits<T>::destroy(Safe(x));
        }
    }


    template<UserManagedRes T>
    using Scoped = std::unique_ptr<T, decltype(&destroy<T>)>;

    // TODO: Below have not been unit tested.

    // Binds existing resource x to the lifetime of an RAII object.
    template<UserManagedRes T>
    inline Scoped<T> bindScoped(Safe<T> x) noexcept {
        return Scoped<T>(x, destroy<T>);
    }
    template<UserManagedRes T, typename... Args>
    inline Scoped<T> makeScoped(Args&&... args) {
        return bindScoped<T>(create<T>(std::forward<Args>(args)));
    }
}

