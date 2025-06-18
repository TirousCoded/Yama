

#include "expr_analyzer.h"

#include "../core/kind-features.h"

#include "util.h"
#include "compiler.h"


#define _DUMP_LOG 0

#if _DUMP_LOG == 1
#include "domain_data.h"
#endif


std::string yama::internal::expr_analyzer::fmt_category(category x) {
    static_assert(categories == 12);
    switch (x) {
    case category::param_id:                return "param id expr";
    case category::var_id:                  return "local var id expr";
    case category::type_id:                 return "type id expr";
    case category::int_lit:                 return "int literal expr";
    case category::uint_lit:                return "uint literal expr";
    case category::float_lit:               return "float literal expr";
    case category::bool_lit:                return "bool literal expr";
    case category::char_lit:                return "char literal expr";
    case category::call:                    return "call expr";
    case category::unbound_method_access:   return "unbound method access expr";
    case category::default_init:            return "default initialize expr";
    case category::constexpr_guarantee:     return "constexpr guarantee expr";
    default: YAMA_DEADEND; break;
    }
    return std::string();
}

yama::internal::expr_analyzer::expr_analyzer(compiler& cs)
    : cs(cs) {}

void yama::internal::expr_analyzer::acknowledge(translation_unit& tu, const ast_expr& x) {
    _tu_map.insert({ safeptr<const ast_expr>(x), safeptr(tu) });
}

void yama::internal::expr_analyzer::add_root(const ast_Expr& x) {
    YAMA_ASSERT(!_roots.contains(x));
    _roots[x] = _root{
        .must_be_constexpr = x.is_type_spec_crvalue,
    };
}

yama::internal::expr_analyzer::metadata& yama::internal::expr_analyzer::operator[](const ast_expr& x) {
    return _pull(x);
}

yama::internal::expr_analyzer::metadata& yama::internal::expr_analyzer::operator[](const ast_TypeSpec& x) {
    return _pull(x);
}

std::optional<yama::internal::cvalue> yama::internal::expr_analyzer::crvalue(const ast_expr& x) {
    return _pull(x).crvalue;
}

std::optional<yama::internal::cvalue> yama::internal::expr_analyzer::crvalue(const ast_TypeSpec& x) {
    return x.expr ? crvalue(*x.expr) : std::nullopt;
}

std::optional<yama::internal::ctype> yama::internal::expr_analyzer::crvalue_to_type(const ast_expr& x) {
    if (const auto result = crvalue(x)) {
        return result->to_type();
    }
    return std::nullopt;
}

std::optional<yama::internal::ctype> yama::internal::expr_analyzer::crvalue_to_type(const ast_TypeSpec& x) {
    return
        x.expr
        ? crvalue_to_type(*x.expr)
        : std::nullopt;
}

void yama::internal::expr_analyzer::codegen(const ast_Expr& root, size_t ret) {
    _codegen(root, ret);
}

void yama::internal::expr_analyzer::codegen_nr(const ast_Expr& root) {
    _codegen(root, std::nullopt);
}

void yama::internal::expr_analyzer::analyze() {
    for (const auto& [key, value] : _roots) {
        _resolve(*key);
    }
}

void yama::internal::expr_analyzer::cleanup() {
    _roots.clear();
    _tu_map.clear();
    _metadata.clear();
}

bool yama::internal::expr_analyzer::_is_root_and_must_be_constexpr(const ast_Expr& x) const noexcept {
    if (const auto it = _roots.find(safeptr(x)); it != _roots.end()) {
        return it->second.must_be_constexpr;
    }
    else return false;
}

yama::internal::translation_unit& yama::internal::expr_analyzer::_tu(const ast_expr& x) const noexcept {
    YAMA_ASSERT(_tu_map.contains(x));
    return *_tu_map.at(x);
}

yama::internal::expr_analyzer::metadata& yama::internal::expr_analyzer::_pull(const ast_expr& x) {
    _resolve(x);
    return _fetch(x);
}

yama::internal::expr_analyzer::metadata& yama::internal::expr_analyzer::_pull(const ast_TypeSpec& x) {
    return _pull(deref_assert(x.expr));
}

yama::internal::expr_analyzer::metadata& yama::internal::expr_analyzer::_fetch(const ast_expr& x) {
    YAMA_ASSERT(_is_resolved(x));
    return _metadata.at(x);
}

bool yama::internal::expr_analyzer::_is_resolved(const ast_expr& x) const noexcept {
    return _metadata.contains(x);
}

void yama::internal::expr_analyzer::_gen_metadata(const ast_expr& x, taul::source_pos where, category category, mode mode) {
    if (_is_resolved(x)) return;
    metadata new_md{
        .tu = _tu(x),
        .where = where,
        .mode = mode,
        .category = category,
    };
    _metadata.insert({ safeptr(x), std::move(new_md) });
}

bool yama::internal::expr_analyzer::_resolve(const ast_expr& x) {
    if (_is_resolved(x)) {
        return _fetch(x).good();
    }
    if (const auto r = x.as<ast_PrimaryExpr>())         return _resolve(*r);
    else if (const auto r = x.as<ast_Args>())           return _resolve(*r);
    else if (const auto r = x.as<ast_MemberAccess>())   return _resolve(*r);
    else if (const auto r = x.as<ast_Expr>())           return _resolve(*r);
    else                                                YAMA_DEADEND;
    return bool();
}

bool yama::internal::expr_analyzer::_resolve(const ast_PrimaryExpr& x) {
    _gen_metadata(x, x.low_pos(), _discern_category(x), _discern_mode(x));
    const bool result = _resolve_expr(x);
    _dump_log_helper(result, x);
    return result;
}

bool yama::internal::expr_analyzer::_resolve(const ast_Args& x) {
    // skip _resolve_expr unless we can rely 100% on subexprs being valid
    const bool can_resolve_expr = _resolve_children(x);
    // call after _resolve_children for _discern_category
    _gen_metadata(x, x.root_expr()->low_pos(), _discern_category(x), _discern_mode(x));
    const bool result =
        can_resolve_expr
        ? _resolve_expr(x)
        : false;
    _dump_log_helper(result, x);
    return result;
}

bool yama::internal::expr_analyzer::_resolve(const ast_MemberAccess& x) {
    // skip _resolve_expr unless we can rely 100% on subexprs being valid
    const bool can_resolve_expr = _resolve_children(x);
    // call after _resolve_children for _discern_category
    _gen_metadata(x, x.root_expr()->low_pos(), _discern_category(x), _discern_mode(x));
    const bool result =
        can_resolve_expr
        ? _resolve_expr(x)
        : false;
    _dump_log_helper(result, x);
    return result;
}

bool yama::internal::expr_analyzer::_resolve(const ast_Expr& x) {
    // skip _resolve_expr unless we can rely 100% on subexprs being valid
    const bool can_resolve_expr = _resolve_children(x);
    // call after _resolve_children for _discern_category
    _gen_metadata(x, x.low_pos(), _discern_category(x), _discern_mode(x));
    const bool result =
        can_resolve_expr
        ? _resolve_expr(x)
        : false;
    _dump_log_helper(result, x);
    return result;
}

bool yama::internal::expr_analyzer::_resolve_children(const ast_Args& x) {
    bool success = true;
    // using '&& success' ensures short-circuiting doesn't skip method call
    if (const auto prior = x.get_primary_subexpr()) { // skip if we're constexpr guarantee expr
        success = _resolve(*prior) && success;
    }
    for (const auto& arg : x.args) { // resolve args, sequentially
        success = _resolve(*arg) && success;
    }
    return success;
}

bool yama::internal::expr_analyzer::_resolve_children(const ast_MemberAccess& x) {
    return _resolve(*res(x.get_primary_subexpr()));
}

bool yama::internal::expr_analyzer::_resolve_children(const ast_Expr& x) {
    // resolve prior base/suffix expr (at least a base expr MUST be present)
    return _resolve(*res(x.get_primary_subexpr()));
}

yama::internal::expr_analyzer::category yama::internal::expr_analyzer::_discern_category(const ast_PrimaryExpr& x) {
    if (x.name) {
        const auto& tu = _tu(x);
        const auto name = x.name->str(tu.src);
        const bool has_qualifier = x.qualifier.has_value();
        const auto symbol = tu.syms.lookup(x, name, x.low_pos());
        const bool is_param_id = !has_qualifier && symbol && symbol->is<param_csym>();
        const bool is_var_id = !has_qualifier && symbol && symbol->is<var_csym>();
        const bool is_type_id = !is_param_id && !is_var_id;
        if (is_param_id)            return category::param_id;
        else if (is_var_id)         return category::var_id;
        else if (is_type_id)        return category::type_id;
    }
    else if (x.lit) {
        if (x.lit->is_int())        return category::int_lit;
        else if (x.lit->is_uint())  return category::uint_lit;
        else if (x.lit->is_float()) return category::float_lit;
        else if (x.lit->is_bool())  return category::bool_lit;
        else if (x.lit->is_char())  return category::char_lit;
    }
    YAMA_DEADEND;
    return category();
}

yama::internal::expr_analyzer::category yama::internal::expr_analyzer::_discern_category(const ast_Args& x) {
    if (x.is_constexpr_guarantee_expr_args()) {
        return category::constexpr_guarantee;
    }
    else if ((*this)[deref_assert(x.get_primary_subexpr())].type == _tu(x).types.type_type()) {
        return category::default_init;
    }
    else {
        return category::call;
    }
}

yama::internal::expr_analyzer::category yama::internal::expr_analyzer::_discern_category(const ast_MemberAccess& x) {
    return category::unbound_method_access;
}

yama::internal::expr_analyzer::category yama::internal::expr_analyzer::_discern_category(const ast_Expr& x) {
    return _pull(*res(x.get_primary_subexpr())).category;
}

bool yama::internal::expr_analyzer::_resolve_expr(const ast_PrimaryExpr& x) {
    metadata& md = _fetch(x);
    switch (md.category) {
    case category::param_id:
    {
        if (_raise_nonassignable_expr_if(md.lvalue(), md)) {
            return false;
        }
        const auto name = x.name->str(md.tu->src);
        const auto symbol = md.tu->syms.lookup(x, name, x.low_pos());
        const auto t = symbol->as<param_csym>().get_type(*cs);
        // TODO: we don't really have unit tests covering the below case of
        //       undeclared name error arising here specifically
        //       (ie. removing below error check doesn't result in unit tests
        //       failing, but has resulted in program crashing)
        if (_raise_undeclared_name_if(!t, md, name)) {
            return false;
        }
        md.type = t.value();
        md.crvalue = _runtime_only;
    }
    break;
    case category::var_id:
    {
        const auto name = x.name->str(md.tu->src);
        const auto symbol = md.tu->syms.lookup(x, name, x.low_pos());
        const auto t = symbol->as<var_csym>().get_type(*cs);
        // TODO: we don't really have unit tests covering the below case of
        //       undeclared name error arising here specifically
        //       (ie. removing below error check doesn't result in unit tests
        //       failing, but has resulted in program crashing)
        if (_raise_undeclared_name_if(!t, md, name)) {
            return false;
        }
        md.type = t.value();
        md.crvalue = _runtime_only;
    }
    break;
    case category::type_id:
    {
        YAMA_ASSERT(_is_type_ref_id_expr(x));
        if (_raise_nonassignable_expr_if(md.lvalue(), md)) {
            return false;
        }
        bool ambiguous{};
        bool bad_qualifier{};
        const auto t = md.tu->types.load(x, ambiguous, bad_qualifier);
        const std::string name = x.fmt_name(md.tu->src).value();
        if (_raise_ambiguous_name_if(!t && ambiguous, md, name)) {
            return false;
        }
        if (_raise_undeclared_qualifier_if(!t && bad_qualifier, md, x.qualifier)) {
            return false;
        }
        if (_raise_undeclared_name_if(!t, md, name)) {
            return false;
        }
        const ctype& our_t = deref_assert(t);
        static_assert(kinds == 4); // reminder
        if (our_t.kind() == kind::function) { // output if fn typed
            md.type = our_t;
            md.crvalue = cvalue::fn_v(our_t);
        }
        else { // output is not fn typed
            md.type = md.tu->types.type_type();
            md.crvalue = cvalue::type_v(our_t, md.tu->types);
        }
    }
    break;
    case category::int_lit:
    {
        if (_raise_nonassignable_expr_if(md.lvalue(), md)) {
            return false;
        }
        const auto lit_nd = res(x.lit->as_int());
        const parsed_int v = parse_int(lit_nd->lit.str(md.tu->src)).value();
        if (_raise_numeric_overflow_if(v.overflow, md, lit_nd->lit)) {
            return false;
        }
        if (_raise_numeric_underflow_if(v.underflow, md, lit_nd->lit)) {
            return false;
        }
        md.type = md.tu->types.int_type();
        md.crvalue = cvalue::int_v(v.v, md.tu->types);
    }
    break;
    case category::uint_lit:
    {
        if (_raise_nonassignable_expr_if(md.lvalue(), md)) {
            return false;
        }
        const auto lit_nd = res(x.lit->as_uint());
        const parsed_uint v = parse_uint(lit_nd->lit.str(md.tu->src)).value();
        if (_raise_numeric_overflow_if(v.overflow, md, lit_nd->lit)) {
            return false;
        }
        md.type = md.tu->types.uint_type();
        md.crvalue = cvalue::uint_v(v.v, md.tu->types);
    }
    break;
    case category::float_lit:
    {
        if (_raise_nonassignable_expr_if(md.lvalue(), md)) {
            return false;
        }
        const auto lit_nd = res(x.lit->as_float());
        const parsed_float v = parse_float(lit_nd->lit.str(md.tu->src)).value();
        // below can handle cases of overflow/underflow
        md.type = md.tu->types.float_type();
        md.crvalue = cvalue::float_v(v.v, md.tu->types);
    }
    break;
    case category::bool_lit:
    {
        if (_raise_nonassignable_expr_if(md.lvalue(), md)) {
            return false;
        }
        const auto lit_nd = res(x.lit->as_bool());
        const parsed_bool v = parse_bool(lit_nd->lit.str(md.tu->src)).value();
        md.type = md.tu->types.bool_type();
        md.crvalue = cvalue::bool_v(v.v, md.tu->types);
    }
    break;
    case category::char_lit:
    {
        if (_raise_nonassignable_expr_if(md.lvalue(), md)) {
            return false;
        }
        const auto lit_nd = res(x.lit->as_char());
        const str unquoted = _remove_quotes(lit_nd->lit.str(md.tu->src));
        const parsed_char v = parse_char(unquoted).value();
        const bool parsed_bytes_eqs_unquoted = v.bytes == unquoted.length();
        if (_raise_malformed_literal_for_char_lit_if(!parsed_bytes_eqs_unquoted, md, unquoted)) {
            return false;
        }
        if (_raise_illegal_unicode_if(!taul::is_unicode(v.v), md, unquoted)) {
            return false;
        }
        md.type = md.tu->types.char_type();
        md.crvalue = cvalue::char_v(v.v, md.tu->types);
    }
    break;
    default: YAMA_DEADEND; break;
    }
    return md.good();
}

bool yama::internal::expr_analyzer::_resolve_expr(const ast_Args& x) {
    metadata& md = _fetch(x);
    switch (md.category) {
    case category::call:
    {
        if (_raise_nonassignable_expr_if(md.lvalue(), md)) {
            return false;
        }
        const ctype callobj_type = _pull(*res(x.get_primary_subexpr())).type.value();
        const bool callobj_is_callable_type = is_callable(callobj_type.kind());
        if (_raise_invalid_operation_due_to_noncallable_type_if(!callobj_is_callable_type, md, callobj_type)) {
            return false;
        }
        const size_t expected_args = callobj_type.param_count();
        const size_t actual_args = x.args.size();
        const bool correct_arg_count = actual_args == expected_args;
        if (_raise_wrong_arg_count_if(!correct_arg_count, md, callobj_type, expected_args)) {
            return false;
        }
        for (size_t i = 0; i < actual_args; i++) { // type check the args
            const ctype expected_type = callobj_type.param_type(i, *cs).value();
            const ctype actual_type = _pull(*x.args[i]).type.value();
            (void)_raise_type_mismatch_for_arg_if(actual_type, expected_type, md, i + 1);
        }
        if (md.tu->err.is_fatal()) { // if arg type check failed
            return false;
        }
        const ctype return_type = md.tu->types.default_none(callobj_type.return_type(*cs));
        md.type = return_type;
        md.crvalue = _runtime_only;
    }
    break;
    case category::default_init:
    {
        if (_raise_nonassignable_expr_if(md.lvalue(), md)) {
            return false;
        }
        const metadata& initialized_type_md = _pull(deref_assert(x.get_primary_subexpr()));
        YAMA_ASSERT(initialized_type_md.type == _tu(x).types.type_type());
        if (_raise_nonconstexpr_expr_if(!initialized_type_md.is_constexpr(), md)) {
            return false;
        }
        const ctype initialized_type = initialized_type_md.crvalue.value().to_type().value();
        if (_raise_wrong_arg_count_if(x.args.size() >= 1, md, 0)) {
            return false;
        }
        md.type = initialized_type;
        md.crvalue = _default_init_crvalue(initialized_type, md);
    }
    break;
    case category::constexpr_guarantee:
    {
        if (_raise_nonassignable_expr_if(md.lvalue(), md)) {
            return false;
        }
        if (_raise_wrong_arg_count_if(x.args.size() != 1, md, 1)) {
            return false;
        }
        const auto& first_arg = *x.args.front();
        const auto& first_arg_md = _pull(first_arg);
        if (_raise_nonconstexpr_expr_if(!first_arg_md.crvalue, md)) {
            return false;
        }
        md.type = first_arg_md.type;
        md.crvalue = first_arg_md.crvalue;
    }
    break;
    default: YAMA_DEADEND; break;
    }
    return md.good();
}

bool yama::internal::expr_analyzer::_resolve_expr(const ast_MemberAccess& x) {
    metadata& md = _fetch(x);
    switch (md.category) {
    case category::unbound_method_access:
    {
        if (_raise_nonassignable_expr_if(md.lvalue(), md)) {
            return false;
        }
        const metadata& owner_md = _pull(*res(x.get_primary_subexpr()));
        const bool owner_type_is_type_type = owner_md.type.value() == _tu(x).types.type_type();
        if (!owner_type_is_type_type) {
            md.tu->err.error(
                md.where,
                dsignal::compile_invalid_operation,
                "cannot member access of {} type expr, type must be {}!",
                owner_md.type.value().fmt(md.tu->e()),
                _tu(x).types.type_type().fmt(md.tu->e()));
            return false;
        }
        if (_raise_nonconstexpr_expr_if(!owner_md.crvalue, md)) {
            return false;
        }
        const ctype accessed_type = owner_md.crvalue.value().as<ctype>().value();
        const str owner_name_part = accessed_type.fullname().unqualified_name();
        const str member_name_part = x.member_name.str(md.tu->src);
        const str method_uqn = str(std::format("{}.{}", owner_name_part, member_name_part));
        const import_path method_ip = accessed_type.fullname().import_path();
        const fullname method_fln = qualified_name(method_ip, method_uqn);
        const auto method_type = md.tu->types.load(method_fln);
        if (!method_type) {
            md.tu->err.error(
                md.where,
                dsignal::compile_invalid_operation,
                "cannot member access non-existent member type {}!",
                method_fln.fmt(md.tu->e()));
            return false;
        }
        md.type = method_type.value();
        md.crvalue = cvalue::method_v(method_type.value());
    }
    break;
    default: YAMA_DEADEND; break;
    }
    return md.good();
}

bool yama::internal::expr_analyzer::_resolve_expr(const ast_Expr& x) {
    metadata& md = _fetch(x);
    const auto& child_md = _pull(*res(x.get_primary_subexpr()));
    const bool must_be_constexpr_but_isnt =
        _is_root_and_must_be_constexpr(x) &&
        !child_md.is_constexpr();
    if (_raise_nonconstexpr_expr_if(must_be_constexpr_but_isnt, md)) {
        return false;
    }
    md.type = child_md.type;
    md.crvalue = child_md.crvalue;
    return md.good();
}

void yama::internal::expr_analyzer::_codegen(const ast_Expr& root, std::optional<size_t> ret) {
    const auto& md = _pull(root); // lazy load expr tree if needed
    if (md.tu->err.is_fatal()) return;
    YAMA_ASSERT(md.rvalue()); // can't be lvalue
    _codegen_step(root, ret);
}

void yama::internal::expr_analyzer::_codegen_step(const ast_expr& x, std::optional<size_t> ret) {
    if (const auto r = x.as<ast_PrimaryExpr>())         _codegen_step(*r, ret);
    else if (const auto r = x.as<ast_Args>())           _codegen_step(*r, ret);
    else if (const auto r = x.as<ast_MemberAccess>())   _codegen_step(*r, ret);
    else                                                YAMA_DEADEND;
}

void yama::internal::expr_analyzer::_codegen_step(const ast_PrimaryExpr& x, std::optional<size_t> ret) {
    const auto& md = _fetch(x);
    YAMA_ASSERT(!md.tu->err.is_fatal());
    auto& tu = _tu(x);
    // TODO: below about outputting nothing may need to change if primary exprs
    //       are added which can have side effects (ie. getters)
    // if no result, then just output nothing
    if (!ret) {
        return;
    }
    // expr type and crvalue (if any)
    const ctype& type = md.type.value();
    const auto& crvalue = md.crvalue;
    // if newtop, push a temporary
    if (ret == size_t(newtop)) {
        tu.rs.push_temp(x, type);
    }
    const uint8_t output_reg = uint8_t(deref_assert(ret));
    tu.cgt.autosym(x.low_pos());
    switch (md.category) {
    case category::param_id:
    {
        const auto name = x.name->str(md.tu->src);
        // IMPORTANT: remember that the indices of the args of a call include the callobj,
        //            so we gotta incr the fn param index to account for this
        const uint8_t arg_index = uint8_t(tu.cgt.target_param_index(name).value() + 1);
        // write put_arg loading param into output
        tu.cgt.cw.add_put_arg(output_reg, arg_index);
    }
    break;
    case category::var_id:
    {
        const auto name = x.name->str(md.tu->src);
        const auto symbol = md.tu->syms.lookup(x, name, x.low_pos());
        const uint8_t local_var_reg = uint8_t(symbol->as<var_csym>().reg.value());
        // write copy from local var into output
        tu.cgt.cw.add_copy(local_var_reg, output_reg);
    }
    break;
    case category::type_id:
    case category::int_lit:
    case category::uint_lit:
    case category::float_lit:
    case category::bool_lit:
    case category::char_lit:
    {
        tu.cgt.add_cvalue_put_instr(output_reg, crvalue.value());
    }
    break;
    default: YAMA_DEADEND; break;
    }
}

void yama::internal::expr_analyzer::_codegen_step(const ast_Args& x, std::optional<size_t> ret) {
    const auto& md = _fetch(x);
    YAMA_ASSERT(!md.tu->err.is_fatal());
    auto& tu = _tu(x);
    // expr type and crvalue (if any)
    const ctype& type = md.type.value();
    const auto& crvalue = md.crvalue;
    switch (md.category) {
    case category::call:
    {
        // perform prior steps
        // (gotta do these BEFORE our autosym, as _codegen_step will overwrite it)
        _codegen_step(*res(x.get_primary_subexpr()), size_t(newtop));
        for (const auto& I : x.args) { // codegen arg exprs
            _codegen_step(*I, size_t(newtop));
        }
        // autosym w/ expr this is a suffix of
        tu.cgt.autosym(x.root_expr()->low_pos());
        // on the register stack will be the temporaries corresponding to the callobj
        // and args of the call this code will resolve
        const size_t param_args = x.args.size();
        const size_t args = param_args + 1; // args == param-args + callobj
        const size_t callobj_reg = tu.rs.regs() - args;
        const ctype callobj_type = tu.rs.reg_abs(callobj_reg).type;
        if (!ret) { // no result
            // write the call_nr instr
            tu.cgt.cw.add_call_nr(uint8_t(args));
            // pop args, including callobj
            tu.rs.pop_temp(args, false);
        }
        else if (ret == size_t(newtop)) { // result reg is newtop
            const auto return_type = tu.types.default_none(callobj_type.return_type(*tu.cs));
            const size_t return_value_reg = callobj_reg;
            // write the call instr
            tu.cgt.cw.add_call(uint8_t(args), newtop);
            // pop param-args, ie. not the callobj, as that register will be
            // reused for our return value
            tu.rs.pop_temp(param_args, false);
            // reinit callobj register as return value register
            tu.rs.reinit_temp(ssize_t(return_value_reg), return_type);
        }
        else { // result reg is not newtop
            const auto return_type = tu.types.default_none(callobj_type.return_type(*tu.cs));
            const size_t return_value_reg = ret.value();
            // write the call instr
            tu.cgt.cw.add_call(uint8_t(args), uint8_t(return_value_reg));
            // pop args, ie. including callobj, as our result register won't
            // be a new register pushed, but instead some other one
            tu.rs.pop_temp(args, false);
        }
    }
    break;
    case category::default_init:
    {
        // autosym w/ expr this is a suffix of
        tu.cgt.autosym(x.root_expr()->low_pos());
        if (crvalue) { // constexpr
            // if expr is not even gonna be used for its return value, then this expr is
            // 100% transparent, and codegen can just be forwent altogether
            if (!ret) {
                return;
            }
            const uint8_t output_reg = uint8_t(deref_assert(ret));
            // if newtop, push a temporary
            if (ret == size_t(newtop)) {
                tu.rs.push_temp(x, type);
            }
            // just write an instr outputting its value
            tu.cgt.add_cvalue_put_instr(output_reg, crvalue.value());
        }
        else { // non-constexpr
            // TODO: later on, when we add default initializers which actually involve
            //       a ctor call, we'll in that scenario STILL be able to forgo codegen in
            //       situations where (1) this ctor call is transparent, and (2) the result
            //       of this default init expr isn't needed
            //
            //       not gonna add this now, as I don't have a way to distinguish between
            //       these different types of initializers, and I don't wanna risk creating
            //       bugs if later, when we add these initializers, I forget about this
            //       part of our code
            const uint8_t output_reg = uint8_t(deref_assert(ret));
            // if newtop, push a temporary
            if (ret == size_t(newtop)) {
                tu.rs.push_temp(x, type);
            }
            // write default_init instr to initialize the object
            tu.cgt.cw.add_default_init(output_reg, uint8_t(tu.ctp.pull_type(type)));
        }
    }
    break;
    case category::constexpr_guarantee:
    {
        // if expr is not even gonna be used for its return value, then this expr is
        // 100% transparent, and codegen can just be forwent altogether
        if (!ret) {
            return;
        }
        const uint8_t output_reg = uint8_t(deref_assert(ret));
        // if newtop, push a temporary
        if (ret == size_t(newtop)) {
            tu.rs.push_temp(x, type);
        }
        tu.cgt.autosym(x.low_pos());
        YAMA_ASSERT(x.args.size() == 1);
        // arg of constexpr-guarantee is *guaranteed* to be constexpr, so we can
        // just write an instr outputting its value
        tu.cgt.add_cvalue_put_instr(output_reg, crvalue.value());
    }
    break;
    default: YAMA_DEADEND; break;
    }
}

void yama::internal::expr_analyzer::_codegen_step(const ast_MemberAccess& x, std::optional<size_t> ret) {
    const auto& md = _fetch(x);
    YAMA_ASSERT(!md.tu->err.is_fatal());
    auto& tu = _tu(x);
    // expr type and crvalue (if any)
    const ctype& type = md.type.value();
    const auto& crvalue = md.crvalue;
    switch (md.category) {
    case category::unbound_method_access:
    {
        // if expr is not even gonna be used for its return value, then this expr is
        // 100% transparent, and codegen can just be forwent altogether
        if (!ret) {
            return;
        }
        const uint8_t output_reg = uint8_t(deref_assert(ret));
        // if newtop, push a temporary
        if (ret == size_t(newtop)) {
            tu.rs.push_temp(x, type);
        }
        tu.cgt.autosym(x.low_pos());
        tu.cgt.add_cvalue_put_instr(output_reg, crvalue.value());
    }
    break;
    default: YAMA_DEADEND; break;
    }
}

void yama::internal::expr_analyzer::_codegen_step(const ast_Expr& x, std::optional<size_t> ret) {
    const auto& md = _fetch(x);
    YAMA_ASSERT(!md.tu->err.is_fatal());
    auto& tu = _tu(x);
    // expr type and crvalue (if any)
    const ctype& type = md.type.value();
    const auto& crvalue = md.crvalue;
    if (crvalue) { // constexpr
        // if expr is not even gonna be used for its return value, then this expr is
        // 100% transparent, and codegen can just be forwent altogether
        if (!ret) {
            return;
        }
        const uint8_t output_reg = uint8_t(deref_assert(ret));
        // if newtop, push a temporary
        if (ret == size_t(newtop)) {
            tu.rs.push_temp(x, type);
        }
        tu.cgt.autosym(x.low_pos());
        tu.cgt.add_cvalue_put_instr(output_reg, crvalue.value());
    }
    else { // non-constexpr
        // forward to nested, including forwarding ret
        _codegen_step(*res(x.get_primary_subexpr()), ret);
    }
}

yama::internal::expr_analyzer::mode yama::internal::expr_analyzer::_discern_mode(const ast_base_expr& x) const noexcept {
    return _discern_mode(*x.root_expr());
}

yama::internal::expr_analyzer::mode yama::internal::expr_analyzer::_discern_mode(const ast_suffix_expr& x) const noexcept {
    return _discern_mode(*x.root_expr());
}

yama::internal::expr_analyzer::mode yama::internal::expr_analyzer::_discern_mode(const ast_Expr& x) const noexcept {
    return
        x.is_assign_stmt_lvalue
        ? mode::lvalue
        : mode::rvalue;
}

std::optional<yama::internal::cvalue> yama::internal::expr_analyzer::_default_init_crvalue(const ctype& type, metadata& md) {
    auto& tu = *md.tu;
    if (is_primitive(type.kind())) {
        if (type == tu.types.none_type())       return cvalue::none_v(tu.types);
        else if (type == tu.types.int_type())   return cvalue::int_v(0, tu.types);
        else if (type == tu.types.uint_type())  return cvalue::uint_v(0u, tu.types);
        else if (type == tu.types.float_type()) return cvalue::float_v(0.0, tu.types);
        else if (type == tu.types.bool_type())  return cvalue::bool_v(false, tu.types);
        else if (type == tu.types.char_type())  return cvalue::char_v(U'\0', tu.types);
        else if (type == tu.types.type_type())  return cvalue::type_v(tu.types.none_type(), tu.types);
        else                                    YAMA_DEADEND;
    }
    else if (is_function(type.kind()))          return cvalue::fn_v(type);
    else                                        return _runtime_only;
    return std::nullopt; // dummy
}

bool yama::internal::expr_analyzer::_is_type_ref_id_expr(const ast_PrimaryExpr& x) {
    if (!x.name) return false; // not a type ref if it's not even an id expr
    if (x.qualifier) return true; // definitely a type ref if has qualifier
    const auto& tu = _tu(x);
    const auto sym = tu.syms.lookup(x, x.name->str(tu.src), x.low_pos());
    // succeed if nullptr, as that means it MUST ref type not in compiling module
    return !sym || sym->is_type();
}

bool yama::internal::expr_analyzer::_raise_numeric_overflow_if(bool x, metadata& md, const taul::token& lit) {
    if (x) {
        md.tu->err.error(
            md.where,
            dsignal::compile_numeric_overflow,
            "numeric overflow ({})!",
            lit.str(md.tu->src));
    }
    return x;
}

bool yama::internal::expr_analyzer::_raise_numeric_underflow_if(bool x, metadata& md, const taul::token& lit) {
    if (x) {
        md.tu->err.error(
            md.where,
            dsignal::compile_numeric_underflow,
            "numeric underflow ({})!",
            lit.str(md.tu->src));
    }
    return x;
}

bool yama::internal::expr_analyzer::_raise_malformed_literal_for_char_lit_if(bool x, metadata& md, const str& unquoted) {
    if (x) {
        md.tu->err.error(
            md.where,
            dsignal::compile_malformed_literal,
            "illegal multi-codepoint char literal ({})!",
            fmt_string_literal(unquoted));
    }
    return x;
}

bool yama::internal::expr_analyzer::_raise_illegal_unicode_if(bool x, metadata& md, const str& unquoted) {
    if (x) {
        md.tu->err.error(
            md.where,
            dsignal::compile_illegal_unicode,
            "illegal Unicode ({})!",
            fmt_string_literal(unquoted));
    }
    return x;
}

bool yama::internal::expr_analyzer::_raise_undeclared_name_if(bool x, metadata& md, std::string_view name) {
    if (x) {
        md.tu->err.error(
            md.where,
            dsignal::compile_undeclared_name,
            "undeclared name {}!",
            std::string(name));
    }
    return x;
}

bool yama::internal::expr_analyzer::_raise_ambiguous_name_if(bool x, metadata& md, std::string_view name) {
    if (x) {
        md.tu->err.error(
            md.where,
            dsignal::compile_ambiguous_name,
            "ambiguous name {}!",
            std::string(name));
    }
    return x;
}

bool yama::internal::expr_analyzer::_raise_undeclared_qualifier_if(bool x, metadata& md, std::optional<taul::token> qualifier) {
    if (!qualifier) return false; // skip if qualifier doesn't even exist
    if (x) {
        md.tu->err.error(
            md.where,
            dsignal::compile_undeclared_qualifier,
            "undeclared qualifier {}!",
            qualifier.value().str(md.tu->src).fmt());
    }
    return x;
}

bool yama::internal::expr_analyzer::_raise_nonassignable_expr_if(bool x, metadata& md) {
    if (x) {
        md.tu->err.error(
            md.where,
            dsignal::compile_nonassignable_expr,
            "non-assignable expr!");
    }
    return x;
}

bool yama::internal::expr_analyzer::_raise_nonconstexpr_expr_if(bool x, metadata& md) {
    if (x) {
        md.tu->err.error(
            md.where,
            dsignal::compile_nonconstexpr_expr,
            "non-constexpr expr!");
    }
    return x;
}

bool yama::internal::expr_analyzer::_raise_wrong_arg_count_if(bool x, metadata& md, size_t expected_args) {
    if (x) {
        md.tu->err.error(
            md.where,
            dsignal::compile_wrong_arg_count,
            "{} given {} args, but expected {}!",
            fmt_category(md.category),
            expected_args,
            1);
    }
    return x;
}

bool yama::internal::expr_analyzer::_raise_wrong_arg_count_if(bool x, metadata& md, ctype call_to, size_t expected_args) {
    if (x) {
        md.tu->err.error(
            md.where,
            dsignal::compile_wrong_arg_count,
            "call to {} given {} args, but expected {}!",
            call_to.fmt(md.tu->e()),
            expected_args,
            1);
    }
    return x;
}

bool yama::internal::expr_analyzer::_raise_invalid_operation_due_to_noncallable_type_if(bool x, metadata& md, ctype t) {
    if (x) {
        md.tu->err.error(
            md.where,
            dsignal::compile_invalid_operation,
            "cannot call non-callable type {}!",
            t.fmt(md.tu->e()));
    }
    return x;
}

bool yama::internal::expr_analyzer::_raise_type_mismatch_for_arg_if(ctype actual, ctype expected, metadata& md, size_t arg_display_number) {
    const bool result = actual != expected;
    if (result) {
        md.tu->err.error(
            md.where,
            dsignal::compile_type_mismatch,
            "arg {0} expr is type {1}, but expected {2}!",
            arg_display_number,
            actual.fmt(md.tu->e()),
            expected.fmt(md.tu->e()));
    }
    return result;
}

bool yama::internal::expr_analyzer::_raise_type_mismatch_for_initialized_type_if(ctype actual, ctype expected, metadata& md) {
    const bool result = actual != expected;
    if (result) {
        md.tu->err.error(
            md.where,
            dsignal::compile_type_mismatch,
            "initialized type expr is type {0}, but expected {1}!",
            actual.fmt(md.tu->e()),
            expected.fmt(md.tu->e()));
    }
    return result;
}

yama::str yama::internal::expr_analyzer::_remove_quotes(const str& x) noexcept {
    YAMA_ASSERT(
        x.size() >= 2 &&
        // macro expansion stupidly rewrote our char literals to be illegal multi-char ones
        (x.back() == 39 || x.back() == 34) && // 39 == '\'', 34 == '\"'
        x.back() == x.front());
    return x.substr(1, x.length() - 2);
}

void yama::internal::expr_analyzer::_dump_log_helper(bool success, const ast_expr& x) {
#if _DUMP_LOG == 1
    if (success) {
        const auto& e = cs->dd->installs.domain_env();
        const auto& fetched = _fetch(x);
        std::string txt{};
        if (!success)                           txt = "fail";
        else if (auto crv = fetched.crvalue)    txt = crv.value().fmt(e);
        else                                    txt = std::format("{} (*runtime-only*)", fetched.type.value().fmt(e));
        println(
            "-- _resolve {} {} (ID={}) => {}",
            _tu(x).src.location_at(x.low_pos()), x.node_type, x.id,
            txt);
    }
#endif
}

