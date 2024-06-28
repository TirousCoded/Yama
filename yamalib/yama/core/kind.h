

#pragma once


#include <cstdint>
#include <string>
#include <format>
#include <iostream>


namespace yama {


    // yama::kind defines the different *kinds of* types in Yama

    enum class kind : uint8_t {
        primitive,
        function,

        num,        // this is not a valid kind of type
    };

    constexpr size_t kind_count = size_t(kind::num);


    std::string fmt_kind(kind x);
}


template<>
struct std::formatter<yama::kind> final : std::formatter<std::string> {
    auto format(yama::kind x, format_context& ctx) const {
        return formatter<string>::format(yama::fmt_kind(x), ctx);
    }
};

namespace std {
    inline std::ostream& operator<<(std::ostream& stream, const yama::kind& x) {
        return stream << yama::fmt_kind(x);
    }
}

