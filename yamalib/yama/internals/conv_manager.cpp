

#include "conv_manager.h"

#include "../core/kind-features.h"
#include "compiler.h"


std::string yama::internal::fmt_conv_type(conv_type x) {
    switch (x) {
    case conv_type::identity:                           return "identity";
    case conv_type::primitive_type:                     return "primitive_type";
    case conv_type::fn_type_narrow_to_type_type:        return "fn_type_narrow_to_type_type";
    case conv_type::method_type_narrow_to_type_type:    return "method_type_narrow_to_type_type";
    case conv_type::illegal:                            return "illegal";
    default: YAMA_DEADEND; break;
    }
    return {};
}

yama::internal::conv_manager_local::conv_manager_local(translation_unit& tu)
    : tu(tu) {}

yama::internal::conv_type yama::internal::conv_manager_local::discern_type(const ctype& from, const ctype& to, bool implicit) {
    if (is_identity_conv(from, to, implicit))                               return conv_type::identity;
    else if (is_primitive_type_conv(from, to, implicit))                    return conv_type::primitive_type;
    else if (is_fn_type_narrow_to_type_type_conv(from, to, implicit))       return conv_type::fn_type_narrow_to_type_type;
    else if (is_method_type_narrow_to_type_type_conv(from, to, implicit))   return conv_type::method_type_narrow_to_type_type;
    else                                                                    return conv_type::illegal;
}

bool yama::internal::conv_manager_local::is_legal(const ctype& from, const ctype& to, bool implicit) {
    return discern_type(from, to, implicit) != conv_type::illegal;
}

bool yama::internal::conv_manager_local::has_side_effects(const ctype& from, const ctype& to) {
    // TODO: This is for later when we add non-constexpr conversions which may have runtime
    //       side-effects (ie. they can observably fail at runtime.)
    return false;
}

bool yama::internal::conv_manager_local::is_constexpr_conv(const ctype& from, const ctype& to) {
    return !has_side_effects(from, to);
}

bool yama::internal::conv_manager_local::is_identity_conv(const ctype& from, const ctype& to, bool implicit) {
    return from == to;
}

bool yama::internal::conv_manager_local::is_primitive_type_conv(const ctype& from, const ctype& to, bool implicit) {
    auto is_correct_type = [&](const ctype& x) -> bool {
        return
            x == tu->types.int_type() ||
            x == tu->types.uint_type() ||
            x == tu->types.float_type() ||
            x == tu->types.bool_type() ||
            x == tu->types.char_type();
        };
    return !implicit && is_correct_type(from) && is_correct_type(to);
}

bool yama::internal::conv_manager_local::is_fn_type_narrow_to_type_type_conv(const ctype& from, const ctype& to, bool implicit) {
    return is_function(from.kind()) && to == tu->types.type_type();
}


bool yama::internal::conv_manager_local::is_method_type_narrow_to_type_type_conv(const ctype& from, const ctype& to, bool implicit) {
    return is_method(from.kind()) && to == tu->types.type_type();
}

