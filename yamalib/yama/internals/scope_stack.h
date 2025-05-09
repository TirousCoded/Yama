

#pragma once


#include <vector>

#include "../core/asserts.h"
#include "../core/general.h"


namespace yama::internal {


    template<typename T>
    class scope_stack final {
    public:
        scope_stack() = default;


        inline size_t size() const noexcept { return _stk.size(); }
        constexpr size_t max_size() const noexcept { return _stk.max_size(); }
        inline bool full() const noexcept { return size() == max_size(); }

        inline bool in_a_scope() const noexcept { return !_stk.empty(); }
        inline operator bool() const noexcept { return in_a_scope(); }

        // negative indices index from top downwards (in Lua style)
        inline T& at(ssize_t index) { YAMA_ASSERT(_in_bounds(index)); return _stk[_abs_index(index)]; }
        inline const T& at(ssize_t index) const { YAMA_ASSERT(_in_bounds(index)); return _stk[_abs_index(index)]; }
        inline T& operator[](ssize_t index) { return at(index); }
        inline const T& operator[](ssize_t index) const { return at(index); }

        inline T& top() { return at(-1); }
        inline const T& top() const { return at(-1); }

        inline void push(T x) { YAMA_ASSERT(!full()); _stk.push_back(std::move(x)); }
        inline void pop() noexcept { YAMA_ASSERT(in_a_scope()); _stk.pop_back(); }

        inline void reset() noexcept { _stk.clear(); }


    private:
        std::vector<T> _stk;


        inline size_t _abs_index(ssize_t index) const noexcept {
            return index >= 0 ? size_t(index) : size() + index;
        }
        inline bool _in_bounds(ssize_t index) const noexcept {
            return _abs_index(index) < size();
        }
    };
}

