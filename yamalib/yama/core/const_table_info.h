

#pragma once


#include <string>
#include <vector>
#include <variant>

#include "scalars.h"
#include "kind.h"
#include "callsig_info.h"
#include "const_type.h"


namespace yama {


    namespace internal {
        class const_table_populator;
    }


    // IMPORTANT:
    //      this code is provided to allow for the end-user to define their own
    //      types, which they can then push to a yama::domain
    //
    //      this is part of the frontend, but being technical and niche, should
    //      avoid being liberally exposed on the interfaces of things like 
    //      yama::type, yama::callsig, etc.


    // these define the 'symbol' structs used to populate a const_table_info
    // w/ data about the consts described by the table

    static_assert(const_types == 7);

    struct int_const_info final {
        int_t           v;


        bool operator==(const int_const_info&) const noexcept = default;
        std::string fmt(const const_table_info&) const;
    };

    struct uint_const_info final {
        uint_t          v;


        bool operator==(const uint_const_info&) const noexcept = default;
        std::string fmt(const const_table_info&) const;
    };

    struct float_const_info final {
        float_t         v;


        bool operator==(const float_const_info&) const noexcept = default;
        std::string fmt(const const_table_info&) const;
    };

    struct bool_const_info final {
        bool_t          v;


        bool operator==(const bool_const_info&) const noexcept = default;
        std::string fmt(const const_table_info&) const;
    };

    struct char_const_info final {
        char_t          v;


        bool operator==(const char_const_info&) const noexcept = default;
        std::string fmt(const const_table_info&) const;
    };

    struct primitive_type_const_info final {
        str             qualified_name;


        kind kind() const noexcept;

        bool operator==(const primitive_type_const_info&) const noexcept = default;
        std::string fmt(const const_table_info&) const;
    };

    struct function_type_const_info final {
        str             qualified_name;
        callsig_info    callsig;


        kind kind() const noexcept;

        bool operator==(const function_type_const_info&) const noexcept = default;
        std::string fmt(const const_table_info& consts) const;
    };


    static_assert(const_types == 7);

    template<>
    struct const_info_of<int_const> final {
        using type = int_const_info;
    };
    template<>
    struct const_info_of<uint_const> final {
        using type = uint_const_info;
    };
    template<>
    struct const_info_of<float_const> final {
        using type = float_const_info;
    };
    template<>
    struct const_info_of<bool_const> final {
        using type = bool_const_info;
    };
    template<>
    struct const_info_of<char_const> final {
        using type = char_const_info;
    };
    template<>
    struct const_info_of<primitive_type_const> final {
        using type = primitive_type_const_info;
    };
    template<>
    struct const_info_of<function_type_const> final {
        using type = function_type_const_info;
    };


    struct const_table_info final {
        using info = std::variant<
            int_const_info,
            uint_const_info,
            float_const_info,
            bool_const_info,
            char_const_info,
            primitive_type_const_info,
            function_type_const_info>;

        static std::string fmt_info(const info& x, const const_table_info& consts);

        template<const_type C>
        using info_t = std::variant_alternative_t<size_t(C), info>;

        static_assert(const_types == std::variant_size_v<info>);

        static_assert(const_types == 7);
        static_assert(std::is_same_v<info_t<int_const>, int_const_info>);
        static_assert(std::is_same_v<info_t<uint_const>, uint_const_info>);
        static_assert(std::is_same_v<info_t<float_const>, float_const_info>);
        static_assert(std::is_same_v<info_t<bool_const>, bool_const_info>);
        static_assert(std::is_same_v<info_t<char_const>, char_const_info>);
        static_assert(std::is_same_v<info_t<primitive_type_const>, primitive_type_const_info>);
        static_assert(std::is_same_v<info_t<function_type_const>, function_type_const_info>);


        std::vector<info> consts;


        // size returns the size of the constant table

        inline size_t size() const noexcept { return consts.size(); }


        // get returns constant at x, if any, as const_info_of_t<C>

        // get returns nullptr if C is not the constant type of the constant at x

        template<const_type C>
        inline const const_info_of_t<C>* get(const_t x) const noexcept;


        // const_type returns the const_type of the constant at x, if any

        std::optional<const_type> const_type(const_t x) const noexcept;


        // NOTE: the purpose of the below methods is to query shared characteristics,
        //       in a way that *abstracts away* the actual constant types

        // kind returns the kind of object type encapsulated by the constant at x, if any

        std::optional<kind> kind(const_t x) const noexcept;

        // qualified_name returns the qualified name of the constant at x, if any

        std::optional<str> qualified_name(const_t x) const noexcept;

        // callsig returns the call signature of the constant at x, if any

        const callsig_info* callsig(const_t x) const noexcept;


        bool operator==(const const_table_info&) const noexcept = default;


        std::string fmt_const(const_t x) const;
        std::string fmt_type_const(const_t x) const;
        std::string fmt(const char* tab = "    ") const;


        // these methods add new entries into consts

        static_assert(const_types == 7);

        const_table_info& add_int(int_t v);
        const_table_info& add_uint(uint_t v);
        const_table_info& add_float(float_t v);
        const_table_info& add_bool(bool_t v);
        const_table_info& add_char(char_t v);
        const_table_info& add_primitive_type(const str& qualified_name);
        const_table_info& add_function_type(const str& qualified_name, callsig_info callsig);


    private:
        friend class yama::internal::const_table_populator;


        // hacky little thing needed for const_table_populator

        const_table_info& _patch_function_type(const_t x, callsig_info new_callsig);
    };


    template<const_type C>
    inline const const_info_of_t<C>* const_table_info::get(const_t x) const noexcept {
        return
            x < consts.size() && std::holds_alternative<const_info_of_t<C>>(consts[x])
            ? &std::get<const_info_of_t<C>>(consts[x])
            : nullptr;
    }
}


YAMA_SETUP_FORMAT(yama::const_table_info, x.fmt());

