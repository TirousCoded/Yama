

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
        static constexpr auto secure = ymDm_Secure;
        static constexpr auto release = ymDm_Release;
        static constexpr auto refCount = ymDm_RefCount;
    };
    template<>
    struct ResTraits<YmCtx> final {
        static constexpr auto create = ymCtx_Create;
        static constexpr auto secure = ymCtx_Secure;
        static constexpr auto release = ymCtx_Release;
        static constexpr auto refCount = ymCtx_RefCount;
    };
    template<>
    struct ResTraits<YmParcelDef> final {
        static constexpr auto create = ymParcelDef_Create;
        static constexpr auto secure = ymParcelDef_Secure;
        static constexpr auto release = ymParcelDef_Release;
        static constexpr auto refCount = ymParcelDef_RefCount;
    };
    template<>
    struct ResTraits<YmParcel> final {
        //
    };
    template<>
    struct ResTraits<YmType> final {
        //
    };


    template<typename T>
    concept CreatableRes =
        requires
    {
        ResTraits<T>::create;
    };
    template<typename T>
    concept RefCountedRes =
        requires (T v)
    {
        { ResTraits<T>::secure(&v) } -> std::convertible_to<YmRefCount>;
        { ResTraits<T>::release(&v) } -> std::convertible_to<YmRefCount>;
        { ResTraits<T>::refCount(&v) } -> std::convertible_to<YmRefCount>;
    };
    template<typename T>
    concept CreatableRefCountedRes =
        CreatableRes<T> &&
        RefCountedRes<T>;


    // TODO: Below have not been unit tested.

    template<CreatableRes T, typename... Args>
    inline Safe<T> create(Args&&... args) {
        return Safe(ResTraits<T>::create(std::forward<Args>(args)...));
    }
    // Fails quietly if x == nullptr, returning 0.
    template<RefCountedRes T>
    inline YmRefCount secure(T* x) noexcept {
        return
            x
            ? ResTraits<T>::secure(Safe(x))
            : 0;
    }
    // Fails quietly if x == nullptr, returning 0.
    template<RefCountedRes T>
    inline YmRefCount release(T* x) noexcept {
        return
            x
            ? ResTraits<T>::release(Safe(x))
            : 0;
    }
    // Fails quietly if x == nullptr, returning 0.
    template<RefCountedRes T>
    inline YmRefCount refCount(T* x) noexcept {
        return
            x
            ? ResTraits<T>::refCount(Safe(x))
            : 0;
    }


    template<RefCountedRes T>
    using Scoped = std::unique_ptr<T, decltype(&release<T>)>;

    // TODO: Below have not been unit tested.

    // Binds existing resource x to a Scoped.
    // This does not increment x's ref count.
    template<RefCountedRes T>
    inline Scoped<T> bindScoped(Safe<T> x) noexcept {
        return Scoped<T>(x, release<T>);
    }
    template<CreatableRefCountedRes T, typename... Args>
    inline Scoped<T> makeScoped(Args&&... args) {
        return bindScoped<T>(create<T>(std::forward<Args>(args)...));
    }
}

