

#pragma once


#include "../core/type_info.h"
#include "../core/const_table_info.h"

#include "ast.h"
#include "ctypesys.h"


namespace yama::internal {


    class translation_unit;


    class const_table_populator final {
    public:
        safeptr<translation_unit> tu;


        const_table_populator(translation_unit& tu);


        // TODO: I'm not 100% on our whole thing of not avoiding duplicates for floats, so maybe
        //       scrap that in the future, or make it configurable or something

        // IMPORTANT: when refactoring/revising, remember that this constant table exists in
        //            the same parcel env as the compilation, so it's super straightforward
        //            to keep names correct

        // these methods populate the current codegen target w/ constants, populating it in a
        // pull-based manner, w/ these methods also doing things like trying to avoid having
        // duplicates

        // the method for type constants DO NOT check if said types actually exist (except for
        // maybe asserts)

        const_t pull_int(int_t x);
        const_t pull_uint(uint_t x);
        const_t pull_float(float_t x); // not gonna try to avoid duplicates for floats, to avoid potential issues
        const_t pull_bool(bool_t x);
        const_t pull_char(char_t x);
        const_t pull_type(const ctype& t);

        const_t pull_prim_type(const ctype& t);
        const_t pull_fn_type(const ctype& t);
        const_t pull_method_type(const ctype& t);
        const_t pull_struct_type(const ctype& t);


        callsig_info build_callsig_for_fn_type(const ctype& t);


    private:
        template<const_type C>
        inline std::optional<const_t> _find_existing_c(const const_table_info& consts, const const_data_of_t<C>& x) const noexcept;
        template<const_type C>
        inline std::optional<const_t> _find_existing_type_c(const const_table_info& consts, const str& qualified_name) const noexcept;
    };

    template<const_type C>
    inline std::optional<const_t> const_table_populator::_find_existing_c(const const_table_info& consts, const const_data_of_t<C>& x) const noexcept {
        for (const_t i = 0; i < consts.consts.size(); i++) {
            if (const auto c = consts.get<C>(i)) {
                if (c->v != x) continue;
                return i;
            }
        }
        return std::nullopt;
    }
    template<const_type C>
    inline std::optional<const_t> const_table_populator::_find_existing_type_c(const const_table_info& consts, const str& qualified_name) const noexcept {
        for (const_t i = 0; i < consts.consts.size(); i++) {
            if (const auto c = consts.get<C>(i)) {
                if (c->qualified_name != qualified_name) continue;
                return i;
            }
        }
        return std::nullopt;
    }
}

