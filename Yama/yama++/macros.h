

#pragma once


#include <format>
#include <iostream>


// YM_HASHER prepares std::hash specialization for a given type.
// This macro should only be used in the global namespace.
// T is the type which the hashing code is for.
// e is the expression used to perform the hashing, operating
// w/ a parameter 'x' of type 'const T&'.
#define YM_HASHER(T, e) \
template<> \
struct std::hash<T> { \
    size_t operator()(const T& x) const noexcept { \
        return e; \
    } \
} // <- Excluded last semicolon, as callsite will have one.

// YM_FORMATTER prepares std::formatter specialization for a given type.
// This macro should only be used in the global namespace.
// T is the type which the formatter code is for.
// e is the expression used to perform the formatting, operating
// w/ a parameter 'x' of type 'const T&'.
#define YM_FORMATTER(T, e) \
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

