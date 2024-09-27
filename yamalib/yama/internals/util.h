

#pragma once


#include "../core/asserts.h"


namespace yama::internal {


    // NOTE: copied range_overlap from TAUL

    // below are exclusive ranges [a_first, a_last) and [b_first, b_last)

    // range_overlap returns false if the two ranges are 0 units apart, but not overlapping

    template<typename T>
    inline bool range_overlap(T a_first, T a_last, T b_first, T b_last) noexcept {
        YAMA_ASSERT(a_first <= a_last);
        YAMA_ASSERT(b_first <= b_last);
        //return !(a_first >= b_last || a_last <= b_first); <- helps me understand what below does
        return a_first < b_last && b_first < a_last;
    }

    // range_contains returns if [a_first, a_last) *contains fully* [b_first, b_last)

    template<typename T>
    inline bool range_contains(T a_first, T a_last, T b_first, T b_last) noexcept {
        YAMA_ASSERT(a_first <= a_last);
        YAMA_ASSERT(b_first <= b_last);
        return b_first >= a_first && b_last <= a_last;
    }
}

