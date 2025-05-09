

#pragma once


#include <variant>

#include "ctypesys.h"


namespace yama::internal {


    struct cvalue final {
        struct stateless_t final {
            constexpr bool operator==(const stateless_t&) const noexcept = default;
        };
        using data_t = std::variant<
            int_t,
            uint_t,
            float_t,
            bool_t,
            char_t,
            ctype,
            stateless_t>;


        ctype t;
        data_t v;


        bool is(const ctype& x) const noexcept;
        bool is(kind x) const noexcept;

        template<typename T>
        inline std::optional<T> as() const noexcept {
            return
                std::holds_alternative<T>(v)
                ? std::make_optional(std::get<T>(v))
                : std::nullopt;
        }

        // to_type is used to extract type value from yama:Type and fn typed cvalues

        // we can't use as<ctype> for this as fn typed cvalues DO NOT store yama:Type
        // value, so we have to use this instead

        std::optional<ctype> to_type() const noexcept;


        bool operator==(const cvalue&) const noexcept = default;

        std::string fmt() const;


        static cvalue none_v(ctypesys_local& types);
        static cvalue int_v(int_t x, ctypesys_local& types);
        static cvalue uint_v(uint_t x, ctypesys_local& types);
        static cvalue float_v(float_t x, ctypesys_local& types);
        static cvalue bool_v(bool_t x, ctypesys_local& types);
        static cvalue char_v(char_t x, ctypesys_local& types);
        static cvalue type_v(ctype x, ctypesys_local& types);
        static cvalue fn_v(ctype x);
    };
}

