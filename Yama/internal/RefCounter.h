

#pragma once


#include <atomic>
#include <limits>

#include "../yama/asserts.h"


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


    template<typename UInt>
    class RefCounter final {
    public:
        RefCounter() = default;


        constexpr const UInt& count() const noexcept { return _refs; }

        inline void addRef() noexcept {
            ymAssert(_refs < std::numeric_limits<UInt>::max());
            _refs++;
        }

        // Returns if the reference count reached 0.
        inline bool drop() noexcept {
            ymAssert(_refs >= 1);
            _refs--;
            return _refs == 0;
        }


    private:
        UInt _refs = 0;
    };


    template<typename UInt>
    class AtomicRefCounter final {
    public:
        AtomicRefCounter() = default;


        inline UInt count() const noexcept {
            // TODO: I'm not 100% sure if memory_order_relaxed is totally the correct
            //		 call here, but given that this isn't trying to incr/decr _refs,
            //		 I think a looser memory order should be okay (and a bit faster!)
            return _refs.load(std::memory_order_relaxed);
        }

        inline void addRef() noexcept {
            const auto old = _refs.fetch_add(1, std::memory_order_relaxed);
            ymAssert(old < std::numeric_limits<UInt>::max());
        }

        // Returns if the reference count reached 0.
        inline bool drop() noexcept {
            const auto old = _refs.fetch_sub(1, std::memory_order_acq_rel);
            ymAssert(old >= 1);
            return old == 1;
        }


    private:
        std::atomic<UInt> _refs = 0;
    };
}

