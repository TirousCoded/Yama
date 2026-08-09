

#pragma once


#include <atomic>
#include <limits>
#include <concepts>
#include <mutex>

#include "../yama/asserts.h"
#include "../yama/yama.h"
#include "../yama++/locks.h"


namespace _ym {


    // NOTE: These exist mainly as I wanted a way to nuke our current ARC code as part of
    //		 a frontend redesign while still being able to keep it in case I ever need it
    //		 again, lol.
    
    // NOTE: See https://stackoverflow.com/questions/41424539/release-consume-ordering-for-reference-counting.
    // NOTE: Also https://github.com/gershnik/intrusive_shared_ptr/blob/master/doc/reference_counting.md.
    //          * This second one is REALLY useful!
    
    // NOTE: Some more reading material from old code comments:
    //          * See https://stackoverflow.com/questions/12092933/calling-virtual-function-from-destructor.
    //          * See https://www.artima.com/articles/never-call-virtual-functions-during-construction-or-destruction.


    template<typename T>
    concept RefCounterType =
        std::default_initializable<T> &&
        std::destructible<T> &&
        requires (T v, const T cv)
    {
        { cv.count() } noexcept -> std::convertible_to<YmRefCount>;
        { v.secure() } noexcept -> std::convertible_to<YmRefCount>;
        { v.release() } noexcept -> std::convertible_to<YmRefCount>;
    };

    template<typename T>
    concept DualRefCounterType =
        std::default_initializable<T> &&
        std::destructible<T> &&
        requires (T v, const T cv, bool frontendRef)
    {
        { cv.total() } noexcept -> std::convertible_to<YmRefCount>;
        { cv.count(frontendRef) } noexcept -> std::convertible_to<YmRefCount>;
        { v.secure(frontendRef) } noexcept -> std::convertible_to<YmRefCount>;
        { v.release(frontendRef) } noexcept -> std::convertible_to<YmRefCount>;
    };


    class RefCounter final {
    public:
        RefCounter() = default;


        constexpr const YmRefCount& count() const noexcept { return _refs; }

        // Returns the old ref count value.
        inline YmRefCount secure() noexcept {
            ymAssert(_refs < std::numeric_limits<YmRefCount>::max());
            _refs++;
            return _refs - 1;
        }

        // Returns the old ref count value.
        inline YmRefCount release() noexcept {
            ymAssert(_refs >= 1);
            _refs--;
            return _refs + 1;
        }


    private:
        YmRefCount _refs = 0;
    };

    static_assert(RefCounterType<RefCounter>);


    class AtomicRefCounter final {
    public:
        AtomicRefCounter() = default;


        inline YmRefCount count() const noexcept {
            // TODO: I'm not 100% sure if memory_order_relaxed is totally the correct
            //		 call here, but given that this isn't trying to incr/decr _refs,
            //		 I think a looser memory order should be okay (and a bit faster!)
            return _refs.load(std::memory_order_relaxed);
        }

        // Returns the old ref count value.
        inline YmRefCount secure() noexcept {
            const auto old = _refs.fetch_add(1, std::memory_order_relaxed);
            ymAssert(old < std::numeric_limits<YmRefCount>::max());
            return old;
        }

        // Returns the old ref count value.
        inline YmRefCount release() noexcept {
            const auto old = _refs.fetch_sub(1, std::memory_order_acq_rel);
            ymAssert(old >= 1);
            return old;
        }


    private:
        std::atomic<YmRefCount> _refs = 0;
    };

    static_assert(RefCounterType<AtomicRefCounter>);


    class DualRefCounter final {
    public:
        DualRefCounter() = default;


        inline YmRefCount total() const noexcept {
            return _frontend.count() + _backend.count();
        }
        inline YmRefCount count(bool frontendRef) const noexcept {
            return _select(frontendRef).count();
        }
        inline YmRefCount secure(bool frontendRef) noexcept {
            // Putting this limit here as it shouldn't ever be reached, and it's needed
            // so that total() return value can't overflow.
            ymAssert(count(frontendRef) < std::numeric_limits<YmRefCount>::max() / 2);
            (void)_select(frontendRef).secure();
            return total() - 1;
        }
        inline YmRefCount release(bool frontendRef) noexcept {
            (void)_select(frontendRef).release();
            return total() + 1;
        }


    private:
        RefCounter _frontend, _backend;


        constexpr RefCounter& _select(bool frontendRef) noexcept {
            return frontendRef ? _frontend : _backend;
        }
        constexpr const RefCounter& _select(bool frontendRef) const noexcept {
            return frontendRef ? _frontend : _backend;
        }
    };

    static_assert(DualRefCounterType<DualRefCounter>);


    // NOTE: Still not 100% sure about below being blocking, but w/ a spin-lock, I think it
    //       should be fine in terms of performance.

    class AtomicDualRefCounter final {
    public:
        AtomicDualRefCounter() = default;


        inline YmRefCount total() const noexcept {
            std::scoped_lock lk(_lk);
            return _drc.total();
        }
        inline YmRefCount count(bool frontendRef) const noexcept {
            std::scoped_lock lk(_lk);
            return _drc.count(frontendRef);
        }
        inline YmRefCount secure(bool frontendRef) noexcept {
            std::scoped_lock lk(_lk);
            return _drc.secure(frontendRef);
        }
        inline YmRefCount release(bool frontendRef) noexcept {
            std::scoped_lock lk(_lk);
            return _drc.release(frontendRef);
        }


    private:
        DualRefCounter _drc;
        mutable ym::SpinLock _lk;
    };

    static_assert(DualRefCounterType<AtomicDualRefCounter>);
}

