

#pragma once


#include <atomic>

#include "meta.h"


namespace ym {


    // TODO: Our locks haven't been unit tested yet (not sure how.)


    // An inert lock which does nothing.
    // For generic systems parameterized with regards to the lock types used, DummyLock can be used to make the system unsynchronized.
    class DummyLock final {
    public:
        constexpr DummyLock() = default;


        constexpr void lock() noexcept {}
        constexpr bool try_lock() noexcept { return true; }
        constexpr void unlock() noexcept {}
    };

    static_assert(Lockable<DummyLock>);
    
    
    // A fine-grain spin-lock class.
    class SpinLock final {
    public:
        SpinLock() = default;


        void lock();
        bool try_lock();
        void unlock() noexcept;


    private:
        std::atomic_flag _locked;
    };

    static_assert(Lockable<SpinLock>);
}

