

#pragma once


#include "../core/asserts.h"


namespace yama::internal {


    class scope_counter final {
    public:
        scope_counter() = default;


        inline bool in_a_scope() const noexcept { return _count >= 1; }
        inline operator bool() const noexcept { return in_a_scope(); }

        inline void push() noexcept { YAMA_ASSERT(_count < size_t(-1)); _count++; }
        inline void push_if(bool x) noexcept { if (x) push(); }

        inline void pop() noexcept { YAMA_ASSERT(in_a_scope()); _count--; }
        inline void pop_if(bool x) noexcept { if (x) pop(); }

        inline void reset() noexcept { _count = 0; }


    private:
        size_t _count = 0;
    };
}

