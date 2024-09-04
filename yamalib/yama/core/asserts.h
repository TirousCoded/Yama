

#pragma once


#include <taul/asserts.h>


// YAMA_ASSERT is here so later we can make assert behaviour configurable

#define YAMA_ASSERT(cond) TAUL_ASSERT(cond)

// YAMA_DEADEND is for asserting that a region of code should not be reached at runtime

#define YAMA_DEADEND YAMA_ASSERT(false)

// YAMA_DEREF_SAFE is a if-statement-like macro used to summarize the checking of pointer 
// safety, calling YAMA_DEADEND if the safety check fails

#define YAMA_DEREF_SAFE(cond) if (!(cond)) YAMA_DEADEND; else

// yama::deref_assert wraps a pointer deref in a function call which asserts that x != nullptr

// yama::deref_assert also helps prevent MSVC from complaining about deref of *potential
// nullptr* in scenarios where we otherwise 100% know the pointer is safe

namespace yama {
    template<typename T>
    inline T& deref_assert(T* x) noexcept {
        YAMA_ASSERT(x);
        return *x;
    }
}

