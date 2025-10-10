

#pragma once


#include <string>
#include <vector>
#include <variant>

#include "scalars.h"
#include "kind.h"
#include "callsig.h"
#include "const_type.h"


namespace yama {


    namespace internal {
        class const_table_populator;
    }


    // These define the 'symbol' structs used to populate a const_table w/
    // data about the consts described by the table.

    static_assert(const_types == 9);

    struct int_const_info final {
        int_t           v;


        bool operator==(const int_const_info&) const noexcept = default;
        std::string fmt(const const_table&) const;
    };
    struct uint_const_info final {
        uint_t          v;


        bool operator==(const uint_const_info&) const noexcept = default;
        std::string fmt(const const_table&) const;
    };
    struct float_const_info final {
        float_t         v;


        bool operator==(const float_const_info&) const noexcept = default;
        std::string fmt(const const_table&) const;
    };
    struct bool_const_info final {
        bool_t          v;


        bool operator==(const bool_const_info&) const noexcept = default;
        std::string fmt(const const_table&) const;
    };
    struct char_const_info final {
        char_t          v;


        bool operator==(const char_const_info&) const noexcept = default;
        std::string fmt(const const_table&) const;
    };
    struct primitive_type_const_info final {
        str             qualified_name;


        kind kind() const noexcept;

        bool operator==(const primitive_type_const_info&) const noexcept = default;
        std::string fmt(const const_table&) const;
    };
    struct function_type_const_info final {
        str             qualified_name;
        callsig    callsig;


        kind kind() const noexcept;

        bool operator==(const function_type_const_info&) const noexcept = default;
        std::string fmt(const const_table& consts) const;
    };
    struct method_type_const_info final {
        str             qualified_name;
        callsig    callsig;


        kind kind() const noexcept;

        bool operator==(const method_type_const_info&) const noexcept = default;
        std::string fmt(const const_table& consts) const;
    };
    struct struct_type_const_info final {
        str             qualified_name;


        kind kind() const noexcept;

        bool operator==(const struct_type_const_info&) const noexcept = default;
        std::string fmt(const const_table&) const;
    };


    static_assert(const_types == 9);

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
    template<>
    struct const_info_of<method_type_const> final {
        using type = method_type_const_info;
    };
    template<>
    struct const_info_of<struct_type_const> final {
        using type = struct_type_const_info;
    };


    struct const_table final {
        using info = std::variant<
            int_const_info,
            uint_const_info,
            float_const_info,
            bool_const_info,
            char_const_info,
            primitive_type_const_info,
            function_type_const_info,
            method_type_const_info,
            struct_type_const_info>;

        static std::string fmt_info(const info& x, const const_table& consts);

        template<const_type C>
        using info_t = std::variant_alternative_t<size_t(C), info>;

        static_assert(const_types == std::variant_size_v<info>);

        static_assert(const_types == 9);
        static_assert(std::is_same_v<info_t<int_const>, int_const_info>);
        static_assert(std::is_same_v<info_t<uint_const>, uint_const_info>);
        static_assert(std::is_same_v<info_t<float_const>, float_const_info>);
        static_assert(std::is_same_v<info_t<bool_const>, bool_const_info>);
        static_assert(std::is_same_v<info_t<char_const>, char_const_info>);
        static_assert(std::is_same_v<info_t<primitive_type_const>, primitive_type_const_info>);
        static_assert(std::is_same_v<info_t<function_type_const>, function_type_const_info>);
        static_assert(std::is_same_v<info_t<method_type_const>, method_type_const_info>);
        static_assert(std::is_same_v<info_t<struct_type_const>, struct_type_const_info>);


        std::vector<info> consts;


        // Returns the size of the constant table.
        inline size_t size() const noexcept { return consts.size(); }


        // Returns constant at x, if any, as const_info_of_t<C>.
        // Returns nullptr if C is not the constant type of the constant at x.
        template<const_type C>
        inline const const_info_of_t<C>* get(const_t x) const noexcept;


        // Returns the const_type of the constant at x, if any.
        std::optional<const_type> const_type(const_t x) const noexcept;


        // NOTE: The purpose of the below methods is to query shared characteristics,
        //       in a way that *abstracts away* the actual constant types.

        // Returns the kind of object type encapsulated by the constant at x, if any.
        std::optional<kind> kind(const_t x) const noexcept;

        // Returns the qualified name of the constant at x, if any.
        std::optional<str> qualified_name(const_t x) const noexcept;

        // Returns the call signature of the constant at x, if any.
        const callsig* callsig(const_t x) const noexcept;


        bool operator==(const const_table&) const noexcept = default;


        std::string fmt_const(const_t x) const;
        std::string fmt_item_const(const_t x) const;
        std::string fmt(const char* tab = default_tab) const;


        static_assert(const_types == 9);

        const_table& add_int(int_t v);
        const_table& add_uint(uint_t v);
        const_table& add_float(float_t v);
        const_table& add_bool(bool_t v);
        const_table& add_char(char_t v);
        const_table& add_primitive_type(const str& qualified_name);
        const_table& add_function_type(const str& qualified_name, yama::callsig callsig);
        const_table& add_method_type(const str& qualified_name, yama::callsig callsig);
        const_table& add_struct_type(const str& qualified_name);


    private:
        friend class yama::internal::const_table_populator;


        // Hacky little thing needed for const_table_populator.

        const_table& _patch_function_type(const_t x, yama::callsig new_callsig);
        const_table& _patch_method_type(const_t x, yama::callsig new_callsig);
    };

    template<const_type C>
    inline const const_info_of_t<C>* const_table::get(const_t x) const noexcept {
        return
            x < consts.size() && std::holds_alternative<const_info_of_t<C>>(consts[x])
            ? &std::get<const_info_of_t<C>>(consts[x])
            : nullptr;
    }
}

YAMA_SETUP_FORMAT(yama::const_table, x.fmt());

