

#pragma once


#include "asserts.h"


namespace yama {


    // yama::opaque is a quality-of-life wrapper over a void* in order to 
    // provide an object which encapsulates a non-owning reference to 
    // something elsewhere in memory, w/out encapsulating information about
    // what its type is

    // yama::opaque semantically guarantees that it always encapsulates a
    // reference to something, never nothing

    // yama::opaque has a _ptr field which is kept public to keep the struct
    // itself trivially copyable, but which is still an implementation detail,
    // meaning aggregate initializing such that _ptr == nullptr results in
    // a yama::opaque in an undefined/invalid state

    // yama::opaque has not been unit tested

    struct opaque final {
        void* _ptr = nullptr; // internal, do not use


        // as returns a lvalue-reference to the object referenced
        // by the yama::opaque, interpreted as type T&

        template<typename T>
        inline T& as() noexcept {
            YAMA_ASSERT(_ptr);
            return *(T*)_ptr;
        }


        inline bool equal(const opaque& other) const noexcept {
            return _ptr == other._ptr;
        }

        inline bool operator==(const opaque& rhs) const noexcept { return equal(rhs); }
        inline bool operator!=(const opaque& rhs) const noexcept { return !equal(rhs); }


        // of is used to initialize yama::opaque objects

        template<typename T>
        static inline opaque of(const T& x) noexcept {
            return opaque{ (void*)&x };
        }
    };
}

