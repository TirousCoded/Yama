

#pragma once


#include <taul/asserts.h>


// YAMA_ASSERT is here so later we can make assert behaviour configurable

#define YAMA_ASSERT(cond) TAUL_ASSERT(cond)

// YAMA_DEADEND is for asserting that a region of code should not be reached at runtime

#define YAMA_DEADEND YAMA_ASSERT(false)

// YAMA_DEREF_SAFE is a if-statement-like macro used to summarize the checking of pointer 
// safety, calling YAMA_DEADEND if the safety check fails

#define YAMA_DEREF_SAFE(cond) if (!(cond)) YAMA_DEADEND; else

// yama::deref_assert wraps a pointer(-like object) deref in a function call which asserts 
// that bool(x) == true

// yama::deref_assert also helps prevent MSVC from complaining about deref of *potential
// nullptr* in scenarios where we otherwise 100% know the pointer is safe

namespace yama {
    template<typename T>
    concept deref_assert_input_type =
        requires (T v)
    {
        // TODO: how can be require *v to not return void?

        // we don't care what T derefs to
        *v;

        { (bool)v } -> std::convertible_to<bool>;
    };

    template<deref_assert_input_type T>
    inline auto&& deref_assert(T x) {
        YAMA_ASSERT(bool(x));
        return *x;
    }
}

