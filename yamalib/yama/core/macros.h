

#pragma once


// general-purpose helper macros


// including these so hash/format stuff works *out of the box*

#include <functional>
#include <format>
#include <iostream>


// YAMA_COND performs expr if cond == true, w/ the macro acting
// as a void returning expression

#define YAMA_COND(cond, expr) \
(void)((cond) && ((void)(expr), true))


// YAMA_SETUP_HASH prepares hashing stuff for a given type

// this macro should only be used in the global namespace

// T is the type which the hashing code is for

// e is the expression used to perform the hashing, operating
// w/ a parameter 'x' of type 'const T&'

#define YAMA_SETUP_HASH(T, e) \
template<> \
struct std::hash<T> { \
    size_t operator()(const T& x) const noexcept { \
        return e; \
    } \
} // <- excluded last semicolon, as callsite will have one


// YAMA_SETUP_FORMAT prepares formatter stuff for a given type

// this macro should only be used in the global namespace

// T is the type which the formatter code is for

// e is the expression used to perform the formatting, operating
// w/ a parameter 'x' of type 'const T&'

#define YAMA_SETUP_FORMAT(T, e) \
template<> \
struct std::formatter<T> final : std::formatter<std::string> { \
    auto format(const T& x, format_context& ctx) const { \
        return formatter<string>::format(e, ctx); \
    } \
}; \
namespace std { \
    inline std::ostream& operator<<(std::ostream& stream, const T& x) { \
        return stream << e; \
    } \
}

