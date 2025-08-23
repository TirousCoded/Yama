

#pragma once


#include <optional>
#include <variant>

#include "../core/res.h"
#include "../core/ptype.h"
#include "../core/const_type.h"
#include "../core/ids.h"
#include "../core/module.h"

#include "ha_struct.h"


namespace yama::internal {


    struct type_mem_header final {
        str                     fullname;
        size_t                  len;                    // size of constant table
        size_t                  stubs;                  // constant table stubs
        module::item            info;
        mid_t                   mid;

        // storing these for fast RTTI access if we need it

        kind                    kind;                   // the kind of type this is
        std::optional<ptype>    ptype;                  // the ptype, if any, of this type
        yama::call_fn           cf          = nullptr;
        size_t                  max_locals  = 0;


        inline size_t length() const noexcept { return len; }
    };

    // NOTE: ran into NASTY cyclical include problems by trying to use std::variant
    //       for type_mem_elem, so instead we're gonna just use a type-erased thing
    //       to get around the issue

    // type_mem_elem will use 'index' to encode the const_type value, w/ const_types
    // being a special value encapsulating a 'stub' entry

    // type_mem_elem will use 'data' to encode the constant value, which will be 
    // reinterpreted as whatever data is encoded by the constant

    struct type_mem_elem final {
        size_t      index   = stub_index;
        uint64_t    data    = 0;


        inline bool holds_stub() const noexcept {
            return index == stub_index;
        }

        inline bool holds(const_type x) const noexcept {
            return index == size_t(x);
        }

        template<const_type C>
            requires std::is_trivially_copyable_v<const_data_of_t<C>>
        inline const_data_of_t<C>& as() noexcept {
            using T = const_data_of_t<C>;
            static_assert(sizeof(T) <= sizeof(data));
            return *(T*)&data;
        }
        
        template<const_type C>
            requires std::is_trivially_copyable_v<const_data_of_t<C>>
        inline const const_data_of_t<C>& as() const noexcept {
            using T = const_data_of_t<C>;
            static_assert(sizeof(T) <= sizeof(data));
            return *(const T*)&data;
        }


        static constexpr size_t stub_index = const_types;


        template<const_type C>
            requires std::is_trivially_copyable_v<const_data_of_t<C>>
        static inline type_mem_elem init(const const_data_of_t<C>& v) noexcept {
            type_mem_elem result{};
            result.index = size_t(C);
            result.as<C>() = v;
            return result;
        }
    };

    using type_mem = ha_struct<type_mem_header, type_mem_elem>;
}

