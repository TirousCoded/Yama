

#pragma once


#include "safeptr.h"
#include "ast.h"
#include "ctypesys.h"


namespace yama::internal {


	class translation_unit;


    enum class conv_type : uint8_t {
        identity,
        primitive_type,
        fn_type_narrow_to_type_type,
        method_type_narrow_to_type_type,
        illegal,
    };

    std::string fmt_conv_type(conv_type x);


    // TODO: ***_local is used akin to ctypesis_local, w/ me intending to later add
    //       a non-local version upstream of the below when it's needed.

	class conv_manager_local final {
	public:
		safeptr<translation_unit> tu;


		conv_manager_local(translation_unit& tu);


        conv_type discern_type(const ctype& from, const ctype& to, bool implicit = false);
		bool is_legal(const ctype& from, const ctype& to, bool implicit = false);
		bool has_side_effects(const ctype& from, const ctype& to);
		bool is_constexpr_conv(const ctype& from, const ctype& to);

        bool is_identity_conv(const ctype& from, const ctype& to, bool implicit = false);
        bool is_primitive_type_conv(const ctype& from, const ctype& to, bool implicit = false);
        bool is_fn_type_narrow_to_type_type_conv(const ctype& from, const ctype& to, bool implicit = false);
        bool is_method_type_narrow_to_type_type_conv(const ctype& from, const ctype& to, bool implicit = false);
	};
}

