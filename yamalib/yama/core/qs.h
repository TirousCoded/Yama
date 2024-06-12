

#pragma once


#include <cstdint>
#include <string>
#include <format>

#include "../query-systems/system_traits.h"


namespace yama {


    // this code defines the query system usage in Yama


    enum class qtype : uint8_t {
        todo, // TODO: also be sure to update fmt_qtype too
    };


    std::string fmt_qtype(qtype x);
}


template<>
struct yama::qs::system_traits<yama::qtype> final {
    using qtypes = yama::qtype;
};


template<>
struct std::formatter<yama::qtype> final : std::formatter<std::string> {
    auto format(yama::qtype x, format_context& ctx) const {
        return formatter<string>::format(yama::fmt_qtype(x), ctx);
    }
};

namespace std {
    inline std::ostream& operator<<(std::ostream& stream, const yama::qtype& x) {
        return stream << yama::fmt_qtype(x);
    }
}

