

#pragma once


#include "macros.h"
#include "asserts.h"
#include "general.h"
#include "scalars.h"
#include "kind.h"


namespace yama {


    class item_ref;
    struct const_table;


    // const_t specifies an index into a const_table_ref or const_table

    using const_t = size_t;


    static_assert(kinds == 4); // reminder

    enum class const_type : uint8_t {
        int0,               // Int value constant
        uint,               // UInt value constant
        float0,             // Float value constant
        bool0,              // Bool value constant
        char0,              // Char value constant

        primitive_type,     // primitive type ref constant
        function_type,      // function type ref constant
        method_type,        // method type ref constant
        struct_type,        // struct type ref constant
        
        num,                // this is not a valid constant type
    };

    constexpr size_t const_types = size_t(const_type::num);


    // these special constants are used to make usage of const_type more *aesthetic*

    static_assert(const_types == 9);

    constexpr auto int_const                = const_type::int0;
    constexpr auto uint_const               = const_type::uint;
    constexpr auto float_const              = const_type::float0;
    constexpr auto bool_const               = const_type::bool0;
    constexpr auto char_const               = const_type::char0;
    constexpr auto primitive_type_const     = const_type::primitive_type;
    constexpr auto function_type_const      = const_type::function_type;
    constexpr auto method_type_const        = const_type::method_type;
    constexpr auto struct_type_const        = const_type::struct_type;


    inline std::string fmt_const_type(const_type x) {
        static_assert(const_types == 9);
        std::string result{};
        switch (x) {
        case int_const:             result = "int";             break;
        case uint_const:            result = "uint";            break;
        case float_const:           result = "float";           break;
        case bool_const:            result = "bool";            break;
        case char_const:            result = "char";            break;
        case primitive_type_const:  result = "primitive-type";  break;
        case function_type_const:   result = "function-type";   break;
        case method_type_const:     result = "method-type";     break;
        case struct_type_const:     result = "struct-type";     break;
        default:                    YAMA_DEADEND;               break;
        }
        return result;
    }
}


YAMA_SETUP_HASH(yama::const_type, std::hash<size_t>{}(size_t(x)));

YAMA_SETUP_FORMAT(yama::const_type, yama::fmt_const_type(x));


namespace yama {


    // is_type_const returns if constants of type x are able to be
    // used as references to types (loaded or otherwise)

    constexpr bool is_type_const(const_type x) noexcept {
        static_assert(const_types == 9);
        switch (x) {
        case int_const:             return false;   break;
        case uint_const:            return false;   break;
        case float_const:           return false;   break;
        case bool_const:            return false;   break;
        case char_const:            return false;   break;
        case primitive_type_const:  return true;    break;
        case function_type_const:   return true;    break;
        case method_type_const:     return true;    break;
        case struct_type_const:     return true;    break;
        default:                    return bool{};  break;
        }
    }

    // is_object_const returns if constants of type x are able to be loaded
    // as objects into object registers

    // TODO: maybe in the future generalize the below to other type consts

    // NOTE: we're gonna have it be that function/method type consts can be
    //       used as object consts in order to load stateless objects of said
    //       types

    constexpr bool is_object_const(const_type x) noexcept {
        static_assert(const_types == 9);
        switch (x) {
        case int_const:             return true;    break;
        case uint_const:            return true;    break;
        case float_const:           return true;    break;
        case bool_const:            return true;    break;
        case char_const:            return true;    break;
        case primitive_type_const:  return false;   break;
        case function_type_const:   return true;    break;
        case method_type_const:     return true;    break;
        case struct_type_const:     return false;   break;
        default:                    return bool{};  break;
        }
    }


    // type traits

    // const_info_of queries the #_const_info struct encapsulating
    // a symbol for a constant of type C in a const_table

    template<const_type C>
    struct const_info_of final {};

    template<const_type C>
    using const_info_of_t = typename const_info_of<C>::type;

    // const_data_of queries the const_table_ref entry value which
    // constants of type C ultimately map to

    template<const_type C>
    struct const_data_of final {};

    template<const_type C>
    using const_data_of_t = typename const_data_of<C>::type;


    static_assert(const_types == 9);

    template<>
    struct const_data_of<int_const> final {
        using type = int_t;
    };
    template<>
    struct const_data_of<uint_const> final {
        using type = uint_t;
    };
    template<>
    struct const_data_of<float_const> final {
        using type = float_t;
    };
    template<>
    struct const_data_of<bool_const> final {
        using type = bool_t;
    };
    template<>
    struct const_data_of<char_const> final {
        using type = char_t;
    };
    template<>
    struct const_data_of<primitive_type_const> final {
        using type = yama::item_ref;
    };
    template<>
    struct const_data_of<function_type_const> final {
        using type = yama::item_ref;
    };
    template<>
    struct const_data_of<method_type_const> final {
        using type = yama::item_ref;
    };
    template<>
    struct const_data_of<struct_type_const> final {
        using type = yama::item_ref;
    };
}

