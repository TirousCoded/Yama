

#include "const_table_populator.h"

#include "../core/general.h"

#include "compiler.h"


using namespace yama::string_literals;


yama::internal::const_table_populator::const_table_populator(translation_unit& tu)
    : tu(tu) {}

yama::const_t yama::internal::const_table_populator::pull_int(int_t x) {
    auto& _consts = tu->cgt.target().consts();
    // first try and find existing constant
    if (const auto found = _find_existing_c<int_const>(_consts, x)) {
        return found.value();
    }
    // add new constant and return it
    _consts.add_int(x);
    return _consts.consts.size() - 1;
}

yama::const_t yama::internal::const_table_populator::pull_uint(uint_t x) {
    auto& _consts = tu->cgt.target().consts();
    // first try and find existing constant
    if (const auto found = _find_existing_c<uint_const>(_consts, x)) {
        return found.value();
    }
    // add new constant and return it
    _consts.add_uint(x);
    return _consts.consts.size() - 1;
}

yama::const_t yama::internal::const_table_populator::pull_float(float_t x) {
    auto& _consts = tu->cgt.target().consts();
    // NOTE: as stated, we're not gonna bother trying to compare floats to
    //       avoid duplicates, as comparing floats is never consistent enough
    //       to not potentially cause problems
    _consts.add_float(x);
    return _consts.consts.size() - 1;
}

yama::const_t yama::internal::const_table_populator::pull_bool(bool_t x) {
    auto& _consts = tu->cgt.target().consts();
    // first try and find existing constant
    if (const auto found = _find_existing_c<bool_const>(_consts, x)) {
        return found.value();
    }
    // add new constant and return it
    _consts.add_bool(x);
    return _consts.consts.size() - 1;
}

yama::const_t yama::internal::const_table_populator::pull_char(char_t x) {
    auto& _consts = tu->cgt.target().consts();
    // first try and find existing constant
    if (const auto found = _find_existing_c<char_const>(_consts, x)) {
        return found.value();
    }
    // add new constant and return it
    _consts.add_char(x);
    return _consts.consts.size() - 1;
}

yama::const_t yama::internal::const_table_populator::pull_type(const ctype& t) {
    if (t.kind() == kind::primitive)        return pull_prim_type(t);
    else if (t.kind() == kind::function)    return pull_fn_type(t);
    else if (t.kind() == kind::method)      return pull_method_type(t);
    else if (t.kind() == kind::struct0)     return pull_struct_type(t);
    else                                    YAMA_DEADEND;
    return const_t{};
}

yama::const_t yama::internal::const_table_populator::pull_prim_type(const ctype& t) {
    const auto qn = t.fln().qn().str(tu->e());
    auto& _consts = tu->cgt.target().consts();
    // search for existing constant to use
    if (const auto found = _find_existing_type_c<primitive_type_const>(_consts, qn)) {
        return found.value();
    }
    // add new constant and return it
    _consts.add_primitive_type(qn);
    return _consts.consts.size() - 1;
}

yama::const_t yama::internal::const_table_populator::pull_fn_type(const ctype& t) {
    const auto qn = t.fln().qn().str(tu->e());
    auto& _consts = tu->cgt.target().consts();
    // search for existing constant to use
    if (const auto found = _find_existing_type_c<function_type_const>(_consts, qn)) {
        return found.value();
    }
    // IMPORTANT: in order to allow for fn type constants' callsigs to have cyclical dependence
    //            w/ one another, we gonna first add a fn type constant w/ a *stub* callsig, then
    //            we're gonna build its proper callsig (recursively pulling on other constants),
    //            and then we're gonna *patch* our type constant w/ this new callsig
    // add new constant
    _consts.add_function_type(qn, callsig_info{}); // <- now available for reference
    // get index of our type constant
    const const_t our_index = _consts.consts.size() - 1;
    // build our proper callsig
    callsig_info proper_callsig = build_callsig_for_fn_type(t);
    // patch proper_callsig onto our type constant
    _consts._patch_function_type(our_index, std::move(proper_callsig));
    return our_index; // return index of our type constant
}

yama::const_t yama::internal::const_table_populator::pull_method_type(const ctype& t) {
    const auto qn = t.fln().qn().str(tu->e());
    auto& _consts = tu->cgt.target().consts();
    // search for existing constant to use
    if (const auto found = _find_existing_type_c<method_type_const>(_consts, qn)) {
        return found.value();
    }
    // IMPORTANT: in order to allow for fn type constants' callsigs to have cyclical dependence
    //            w/ one another, we gonna first add a fn type constant w/ a *stub* callsig, then
    //            we're gonna build its proper callsig (recursively pulling on other constants),
    //            and then we're gonna *patch* our type constant w/ this new callsig
    // add new constant
    _consts.add_method_type(qn, callsig_info{}); // <- now available for reference
    // get index of our type constant
    const const_t our_index = _consts.consts.size() - 1;
    // build our proper callsig
    callsig_info proper_callsig = build_callsig_for_fn_type(t);
    // patch proper_callsig onto our type constant
    _consts._patch_method_type(our_index, std::move(proper_callsig));
    return our_index; // return index of our type constant
}

yama::const_t yama::internal::const_table_populator::pull_struct_type(const ctype& t) {
    const auto qn = t.fln().qn().str(tu->e());
    auto& _consts = tu->cgt.target().consts();
    // search for existing constant to use
    if (const auto found = _find_existing_type_c<struct_type_const>(_consts, qn)) {
        return found.value();
    }
    // add new constant and return it
    _consts.add_struct_type(qn);
    return _consts.consts.size() - 1;
}

yama::callsig_info yama::internal::const_table_populator::build_callsig_for_fn_type(const ctype& t) {
    callsig_info result{};
    // resolve parameter types
    for (size_t i = 0; i < t.param_count(); i++) {
        result.params.push_back(pull_type(t.param_type(i, *tu->cs).value()));
    }
    // resolve return type
    result.ret = pull_type(tu->types.default_none(t.return_type(*tu->cs)));
    return result;
}

