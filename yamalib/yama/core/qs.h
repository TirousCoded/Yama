

#pragma once


#include <cstdint>
#include <string>
#include <format>
#include <iostream>

#include "general.h"

#include "../query-systems/system_traits.h"


namespace yama {


    // this code defines the query system usage in Yama


    // IMPORTANT:
    //      for each qtype constant below, we'll have a header file for
    //      defining its result type (of the same name), and in these
    //      header files we'll define the qtype's key type, and provider
    //      and key traits specializations

    enum class qtype : uint8_t {
        type_data,  // pre-instantiation type information
        type,       // post-instantiation type information

        num,        // this is not a valid qtype
    };

    constexpr size_t qtype_count = size_t(qtype::num);


    std::string fmt_qtype(qtype x);


    // quality-of-life helper constants

    static_assert(qtype_count == 2);

    constexpr qtype type_data_qt = qtype::type_data;
    constexpr qtype type_qt = qtype::type;
}


// query system traits

template<>
struct yama::qs::system_traits<yama::qtype> final {
    using qtype_enum = yama::qtype;
};

static_assert(yama::qs::system_traits_type<yama::qs::system_traits<yama::qtype>, yama::qtype>);


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

