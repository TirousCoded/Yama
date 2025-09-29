

#include "expr_analyzer.h"

#include <ranges>

#include "../core/kind-features.h"

#include "util.h"
#include "domain_data.h"
#include "compiler.h"
#include "conv_manager.h"


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

void yama::internal::expr_analyzer::codegen(const ast_Expr& root, size_t ret, std::optional<ctype> convert_to) {
    if (convert_to) {
        YAMA_LOG(cs->dbg(), compile_c, "expr codegen (root {}) (result sent to {}) (implicit conv to {})...",
            _tu(root).src.location_at(root.low_pos()),
            ret == (size_t)newtop ? "*newtop*" : std::format("{}", ret),
            convert_to->fmt(_tu(root).e()));
    }
    else {
        YAMA_LOG(cs->dbg(), compile_c, "expr codegen (root {}) (result sent to {})...",
            _tu(root).src.location_at(root.low_pos()),
            ret == (size_t)newtop ? "*newtop*" : std::format("{}", ret));
    }
    _codegen(root, ret, convert_to);
    YAMA_LOG(cs->dbg(), compile_c, "expr codegen complete!");
}

void yama::internal::expr_analyzer::codegen_nr(const ast_Expr& root, std::optional<ctype> convert_to) {
    if (convert_to) {
        YAMA_LOG(cs->dbg(), compile_c, "expr codegen (root {}) (result discarded) (implicit conv to {})...",
            _tu(root).src.location_at(root.low_pos()),
            convert_to->fmt(_tu(root).e()));
    }
    else {
        YAMA_LOG(cs->dbg(), compile_c, "expr codegen (root {}) (result discarded)...",
            _tu(root).src.location_at(root.low_pos()));
    }
    _codegen(root, std::nullopt, convert_to);
    YAMA_LOG(cs->dbg(), compile_c, "expr codegen complete!");
}

void yama::internal::expr_analyzer::analyze() {
    YAMA_LOG(cs->dbg(), compile_c, "analyzing exprs ({} roots)...", _roots.size());
    for (const auto& [key, value] : _roots) {
        _resolve(*key);
    }
    YAMA_LOG(cs->dbg(), compile_c, "analyzing exprs complete!");
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
    YAMA_LOG(cs->dbg(), compile_c, "generating metadata ({}, {}, {})...",
        _tu(x).src.location_at(where),
        fmt_category(category),
        mode == mode::rvalue ? "rvalue" : "lvalue");
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
    if (const auto r = x.as<ast_PrimaryExpr>())                 return _resolve(*r);
    else if (const auto r = x.as<ast_ParenthesizedExpr>())      return _resolve(*r);
    else if (const auto r = x.as<ast_ConstexprGuaranteeExpr>()) return _resolve(*r);
    else if (const auto r = x.as<ast_Args>())                   return _resolve(*r);
    else if (const auto r = x.as<ast_Conv>())                   return _resolve(*r);
    else if (const auto r = x.as<ast_TypeMember>())             return _resolve(*r);
    else if (const auto r = x.as<ast_ObjectMember>())           return _resolve(*r);
    else if (const auto r = x.as<ast_Expr>())                   return _resolve(*r);
    else                                                        YAMA_DEADEND;
    return bool();
}

bool yama::internal::expr_analyzer::_resolve(const ast_PrimaryExpr& x) {
    YAMA_LOG(cs->dbg(), compile_c, "analyzing PrimaryExpr {}...", _tu(x).src.location_at(x.low_pos()));
    _gen_metadata(x, x.low_pos(), _discern_category(x), _discern_mode(x));
    const bool result = _resolve_expr(x);
    _trace_result(result, x);
    YAMA_LOG(cs->dbg(), compile_c, "analyze PrimaryExpr complete!");
    return result;
}

bool yama::internal::expr_analyzer::_resolve(const ast_ParenthesizedExpr& x) {
    YAMA_LOG(cs->dbg(), compile_c, "analyzing ParenthesizedExpr {}...", _tu(x).src.location_at(x.low_pos()));
    // Skip _resolve_expr unless we can rely 100% on subexprs being valid.
    const bool can_resolve_expr = _resolve_children(x);
    // Call after _resolve_children for _discern_category.
    _gen_metadata(x, x.root_expr()->low_pos(), _discern_category(x), _discern_mode(x));
    const bool result =
        can_resolve_expr
        ? _resolve_expr(x)
        : false;
    _trace_result(result, x);
    YAMA_LOG(cs->dbg(), compile_c, "analyze ParenthesizedExpr complete!");
    return result;
}

bool yama::internal::expr_analyzer::_resolve(const ast_ConstexprGuaranteeExpr& x) {
    YAMA_LOG(cs->dbg(), compile_c, "analyzing ConstexprGuaranteeExpr {}...", _tu(x).src.location_at(x.low_pos()));
    // Skip _resolve_expr unless we can rely 100% on subexprs being valid.
    const bool can_resolve_expr = _resolve_children(x);
    // Call after _resolve_children for _discern_category.
    _gen_metadata(x, x.root_expr()->low_pos(), _discern_category(x), _discern_mode(x));
    const bool result =
        can_resolve_expr
        ? _resolve_expr(x)
        : false;
    _trace_result(result, x);
    YAMA_LOG(cs->dbg(), compile_c, "analyze ConstexprGuaranteeExpr complete!");
    return result;
}

bool yama::internal::expr_analyzer::_resolve(const ast_Args& x) {
    YAMA_LOG(cs->dbg(), compile_c, "analyzing Args {}...", _tu(x).src.location_at(x.low_pos()));
    // skip _resolve_expr unless we can rely 100% on subexprs being valid
    const bool can_resolve_expr = _resolve_children(x);
    // call after _resolve_children for _discern_category
    _gen_metadata(x, x.root_expr()->low_pos(), _discern_category(x), _discern_mode(x));
    const bool result =
        can_resolve_expr
        ? _resolve_expr(x)
        : false;
    _trace_result(result, x);
    YAMA_LOG(cs->dbg(), compile_c, "analyze Args complete!");
    return result;
}

bool yama::internal::expr_analyzer::_resolve(const ast_Conv& x) {
    YAMA_LOG(cs->dbg(), compile_c, "analyzing Conv {}...", _tu(x).src.location_at(x.low_pos()));
    // Skip _resolve_expr unless we can rely 100% on subexprs being valid.
    const bool can_resolve_expr = _resolve_children(x);
    // Call after _resolve_children for _discern_category.
    _gen_metadata(x, x.root_expr()->low_pos(), _discern_category(x), _discern_mode(x));
    const bool result =
        can_resolve_expr
        ? _resolve_expr(x)
        : false;
    _trace_result(result, x);
    YAMA_LOG(cs->dbg(), compile_c, "analyze Conv complete!");
    return result;
}

bool yama::internal::expr_analyzer::_resolve(const ast_TypeMember& x) {
    YAMA_LOG(cs->dbg(), compile_c, "analyzing TypeMember {}...", _tu(x).src.location_at(x.low_pos()));
    // skip _resolve_expr unless we can rely 100% on subexprs being valid
    const bool can_resolve_expr = _resolve_children(x);
    // call after _resolve_children for _discern_category
    _gen_metadata(x, x.root_expr()->low_pos(), _discern_category(x), _discern_mode(x));
    const bool result =
        can_resolve_expr
        ? _resolve_expr(x)
        : false;
    _trace_result(result, x);
    YAMA_LOG(cs->dbg(), compile_c, "analyze TypeMember complete!");
    return result;
}

bool yama::internal::expr_analyzer::_resolve(const ast_ObjectMember& x) {
    YAMA_LOG(cs->dbg(), compile_c, "analyzing ObjectMember {}...", _tu(x).src.location_at(x.low_pos()));
    // skip _resolve_expr unless we can rely 100% on subexprs being valid
    const bool can_resolve_expr = _resolve_children(x);
    // call after _resolve_children for _discern_category
    _gen_metadata(x, x.root_expr()->low_pos(), _discern_category(x), _discern_mode(x));
    const bool result =
        can_resolve_expr
        ? _resolve_expr(x)
        : false;
    _trace_result(result, x);
    YAMA_LOG(cs->dbg(), compile_c, "analyze ObjectMember complete!");
    return result;
}

bool yama::internal::expr_analyzer::_resolve(const ast_Expr& x) {
    YAMA_LOG(cs->dbg(), compile_c, "analyzing Expr {}...", _tu(x).src.location_at(x.low_pos()));
    // skip _resolve_expr unless we can rely 100% on subexprs being valid
    const bool can_resolve_expr = _resolve_children(x);
    // call after _resolve_children for _discern_category
    _gen_metadata(x, x.low_pos(), _discern_category(x), _discern_mode(x));
    const bool result =
        can_resolve_expr
        ? _resolve_expr(x)
        : false;
    _trace_result(result, x);
    YAMA_LOG(cs->dbg(), compile_c, "analyze Expr complete!");
    return result;
}

bool yama::internal::expr_analyzer::_resolve_children(const ast_ParenthesizedExpr& x) {
    return _resolve(*res(x.expr));
}

bool yama::internal::expr_analyzer::_resolve_children(const ast_ConstexprGuaranteeExpr& x) {
    return _resolve(*res(x.expr));
}

bool yama::internal::expr_analyzer::_resolve_children(const ast_Args& x) {
    bool success = true;
    // Using '&& success' ensures short-circuiting doesn't skip method call.
    if (const auto prior = x.get_primary_subexpr()) { // Skip if we're constexpr guarantee expr.
        success = _resolve(*prior) && success;
    }
    for (const auto& arg : x.args) { // Resolve args, sequentially.
        success = _resolve(*arg) && success;
    }
    return success;
}

bool yama::internal::expr_analyzer::_resolve_children(const ast_Conv& x) {
    bool success = true;
    // Using '&& success' ensures short-circuiting doesn't skip method call.
    success = _resolve(*res(x.get_primary_subexpr())) && success;
    if (x.target) { // Skip if we (for some reason) don't have one.
        success = _resolve(*x.target) && success;
    }
    return success;
}

bool yama::internal::expr_analyzer::_resolve_children(const ast_TypeMember& x) {
    return _resolve(*res(x.get_primary_subexpr()));
}

bool yama::internal::expr_analyzer::_resolve_children(const ast_ObjectMember& x) {
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

yama::internal::expr_analyzer::category yama::internal::expr_analyzer::_discern_category(const ast_ParenthesizedExpr& x) {
    return category::parenthesized;
}

yama::internal::expr_analyzer::category yama::internal::expr_analyzer::_discern_category(const ast_ConstexprGuaranteeExpr& x) {
    return category::constexpr_guarantee;
}

yama::internal::expr_analyzer::category yama::internal::expr_analyzer::_discern_category(const ast_Args& x) {
    // TODO: the below are not checked in the order specified in compilation-tests.cpp, which
    //       shouldn't cause an issue, but a future refactor maybe should still correct this
    if ((*this)[deref_assert(x.get_primary_subexpr())].type == _tu(x).types.type_type()) {
        return category::default_init;
    }
    else if ((*this)[deref_assert(x.get_primary_subexpr())].category == category::bound_method) {
        return category::bound_method_call;
    }
    else {
        return category::call;
    }
}

yama::internal::expr_analyzer::category yama::internal::expr_analyzer::_discern_category(const ast_Conv& x) {
    return category::conv;
}

yama::internal::expr_analyzer::category yama::internal::expr_analyzer::_discern_category(const ast_TypeMember& x) {
    return category::unbound_method;
}

yama::internal::expr_analyzer::category yama::internal::expr_analyzer::_discern_category(const ast_ObjectMember& x) {
    return category::bound_method;
}

yama::internal::expr_analyzer::category yama::internal::expr_analyzer::_discern_category(const ast_Expr& x) {
    return _pull(*res(x.get_primary_subexpr())).category;
}

bool yama::internal::expr_analyzer::_resolve_expr(const ast_PrimaryExpr& x) {
    metadata& md = _fetch(x);
    YAMA_LOG(cs->dbg(), compile_c, "resolving PrimaryExpr ({})...", fmt_category(md.category));
    switch (md.category) {
    case category::param_id:
    {
        if (_raise_nonassignable_expr_if(md.lvalue(), md)) {
            return false;
        }
        const auto name = x.name->str(md.tu->src);
        const auto symbol = md.tu->syms.lookup_expect<param_csym>(x, name, x.low_pos());
        // by this point, first pass checks should guarantee that below
        // symbol->get_type() cannot be empty
        md.type = symbol->get_type().value();
        md.crvalue = _runtime_only;
    }
    break;
    case category::var_id:
    {
        const auto name = x.name->str(md.tu->src);
        const auto symbol = md.tu->syms.lookup_expect<var_csym>(x, name, x.low_pos());
        const auto t = symbol->get_type();
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

bool yama::internal::expr_analyzer::_resolve_expr(const ast_ParenthesizedExpr& x) {
    metadata& md = _fetch(x);
    YAMA_LOG(cs->dbg(), compile_c, "resolving ParenthesizedExpr ({})...", fmt_category(md.category));
    switch (md.category) {
    case category::parenthesized:
    {
        if (_raise_nonassignable_expr_if(md.lvalue(), md)) {
            return false;
        }
        metadata& nested_md = _pull(*res(x.expr));
        md.type = nested_md.type;
        md.crvalue = nested_md.crvalue;
    }
    break;
    default: YAMA_DEADEND; break;
    }
    return md.good();
}

bool yama::internal::expr_analyzer::_resolve_expr(const ast_ConstexprGuaranteeExpr& x) {
    metadata& md = _fetch(x);
    YAMA_LOG(cs->dbg(), compile_c, "resolving ConstexprGuaranteeExpr ({})...", fmt_category(md.category));
    switch (md.category) {
    case category::constexpr_guarantee:
    {
        if (_raise_nonassignable_expr_if(md.lvalue(), md)) {
            return false;
        }
        metadata& nested_md = _pull(*res(x.expr));
        if (_raise_nonconstexpr_expr_if(!nested_md.crvalue, md)) {
            return false;
        }
        md.type = nested_md.type;
        md.crvalue = nested_md.crvalue;
    }
    break;
    default: YAMA_DEADEND; break;
    }
    return md.good();
}

bool yama::internal::expr_analyzer::_resolve_expr(const ast_Args& x) {
    metadata& md = _fetch(x);
    YAMA_LOG(cs->dbg(), compile_c, "resolving Args ({})...", fmt_category(md.category));
    switch (md.category) {
    case category::bound_method_call:
    {
        if (_raise_nonassignable_expr_if(md.lvalue(), md)) {
            return false;
        }
        const res bound_method_nd(x.get_primary_subexpr());
        const res owner_nd(bound_method_nd->get_primary_subexpr());
        const ctype method_type = _pull(*bound_method_nd).type.value();
        YAMA_ASSERT(is_callable(method_type.kind()));
        YAMA_ASSERT(method_type.param_count() >= 1);
        const ctype owner_type = method_type.owner_type().value();
        const size_t expected_args = method_type.param_count() - 1; // '- 1' as owner will be used for first arg.
        const size_t actual_args = x.args.size();
        const bool correct_arg_count = actual_args == expected_args;
        if (_raise_wrong_arg_count_if(!correct_arg_count, md, method_type, actual_args, expected_args)) {
            return false;
        }
        for (size_t i = 0; i < actual_args; i++) { // Type check the args.
            const ctype expected_type = method_type.param_type(i + 1, *cs).value(); // 'i + 1' as owner will be used for first arg.
            const ctype actual_type = _pull(*x.args[i]).type.value();
            (void)_raise_coerced_type_mismatch_for_arg_if(actual_type, expected_type, md, i + 1);
        }
        if (md.tu->err.is_fatal()) { // If arg type check failed.
            return false;
        }
        const ctype return_type = md.tu->types.default_none(method_type.return_type(*cs));
        md.type = return_type;
        md.crvalue = _runtime_only;
    }
    break;
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
        if (_raise_wrong_arg_count_if(!correct_arg_count, md, callobj_type, actual_args, expected_args)) {
            return false;
        }
        for (size_t i = 0; i < actual_args; i++) { // Type check the args.
            const ctype expected_type = callobj_type.param_type(i, *cs).value();
            const ctype actual_type = _pull(*x.args[i]).type.value();
            (void)_raise_coerced_type_mismatch_for_arg_if(actual_type, expected_type, md, i + 1);
        }
        if (md.tu->err.is_fatal()) { // If arg type check failed.
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
        if (_raise_wrong_arg_count_if(x.args.size() >= 1, md, x.args.size(), 0)) {
            return false;
        }
        md.type = initialized_type;
        md.crvalue = _default_init_crvalue(initialized_type, md);
    }
    break;
    default: YAMA_DEADEND; break;
    }
    return md.good();
}

bool yama::internal::expr_analyzer::_resolve_expr(const ast_Conv& x) {
    metadata& md = _fetch(x);
    YAMA_LOG(cs->dbg(), compile_c, "resolving Conv ({})...", fmt_category(md.category));
    switch (md.category) {
    case category::conv:
    {
        if (_raise_nonassignable_expr_if(md.lvalue(), md)) {
            return false;
        }
        metadata& target_md = _pull(deref_assert(x.target));
        if (_raise_nonconstexpr_expr_if(!target_md.crvalue, md)) {
            return false;
        }
        if (_raise_type_mismatch_for_conv_target_if(target_md.type.value(), md)) {
            return false;
        }
        metadata& converted_expr_md = _pull(deref_assert(x.get_primary_subexpr()));
        const ctype converted_from = converted_expr_md.type.value();
        const ctype converted_to = target_md.crvalue.value().to_type().value();
        if (_raise_invalid_operation_due_to_illegal_conv_if(converted_from, converted_to, md)) {
            return false;
        }
        md.type = converted_to;
        md.crvalue = _conv_crvalue(converted_expr_md.crvalue, converted_to, md);
    }
    break;
    default: YAMA_DEADEND; break;
    }
    return md.good();
}

bool yama::internal::expr_analyzer::_resolve_expr(const ast_TypeMember& x) {
    metadata& md = _fetch(x);
    YAMA_LOG(cs->dbg(), compile_c, "resolving TypeMember ({})...", fmt_category(md.category));
    switch (md.category) {
    case category::unbound_method:
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
                "cannot (type) member access for {} expr, expr type must be {}!",
                owner_md.type.value().fmt(md.tu->e()),
                _tu(x).types.type_type().fmt(md.tu->e()));
            return false;
        }
        if (_raise_nonconstexpr_expr_if(!owner_md.crvalue, md)) {
            return false;
        }
        const ctype accessed_type = owner_md.crvalue.value().as<ctype>().value();
        const fullname method_fln = qualified_name(
            accessed_type.fln().ip(),
            unqualified_name(
                accessed_type.fln().uqn().owner_name(),
                x.member_name.str(md.tu->src)));
        const auto method_type = md.tu->types.load(method_fln);
        if (!method_type) {
            md.tu->err.error(
                md.where,
                dsignal::compile_invalid_operation,
                "cannot (type) member access non-existent member {}!",
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

bool yama::internal::expr_analyzer::_resolve_expr(const ast_ObjectMember& x) {
    metadata& md = _fetch(x);
    YAMA_LOG(cs->dbg(), compile_c, "resolving ObjectMember ({})...", fmt_category(md.category));
    switch (md.category) {
    case category::bound_method:
    {
        if (_raise_nonassignable_expr_if(md.lvalue(), md)) {
            return false;
        }
        const metadata& owner_md = _pull(*res(x.get_primary_subexpr()));
        const ctype& owner_type = owner_md.type.value();
        const fullname method_fln = qualified_name(
            owner_type.fln().ip(),
            unqualified_name(
                owner_type.fln().uqn().owner_name(),
                x.member_name.str(md.tu->src)));
        const auto method_type_opt = md.tu->types.load(method_fln);
        if (!method_type_opt) {
            md.tu->err.error(
                md.where,
                dsignal::compile_invalid_operation,
                "cannot (object) member access non-existent member {}!",
                method_fln.fmt(md.tu->e()));
            return false;
        }
        const ctype& method_type = method_type_opt.value();
        const bool method_has_params = method_type.param_count() >= 1;
        if (!method_has_params) {
            md.tu->err.error(
                md.where,
                dsignal::compile_invalid_operation,
                "illegal bound method for {}, zero parameters!",
                method_fln.fmt(md.tu->e()));
            return false;
        }
        const ctype first_param_type = method_type.param_type(0, *cs).value();
        const bool first_param_has_owner_type = first_param_type == owner_type;
        if (!first_param_has_owner_type) {
            md.tu->err.error(
                md.where,
                dsignal::compile_invalid_operation,
                "illegal bound method for {}, first parameter type is {}, but expected owner type {}!",
                method_fln.fmt(md.tu->e()),
                first_param_type.fmt(md.tu->e()),
                owner_type.fmt(md.tu->e()));
            return false;
        }
        md.type = method_type;
        md.crvalue = _runtime_only;
    }
    break;
    default: YAMA_DEADEND; break;
    }
    return md.good();
}

bool yama::internal::expr_analyzer::_resolve_expr(const ast_Expr& x) {
    metadata& md = _fetch(x);
    YAMA_LOG(cs->dbg(), compile_c, "resolving Expr ({})...", fmt_category(md.category));
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

void yama::internal::expr_analyzer::_codegen(const ast_Expr& root, std::optional<size_t> ret, std::optional<ctype> convert_to) {
    const auto& md = _pull(root); // lazy load expr tree if needed
    if (md.tu->err.is_fatal()) return;
    YAMA_ASSERT(md.rvalue()); // can't be lvalue
    _codegen_step(root, ret, false, convert_to);
}

void yama::internal::expr_analyzer::_codegen_step(const ast_expr& x, std::optional<size_t> ret, bool ret_puts_must_reinit, std::optional<ctype> convert_to) {
    auto& md = _fetch(x);
    YAMA_ASSERT(!md.tu->err.is_fatal());
    if (convert_to && md.tu->convs.has_side_effects(md.type.value(), *convert_to)) {
        // TODO: At present, we have no convs w/ side-effects, and when we add those, then the below code will
        //       NOT BE CORRECT!!!! as when base expr can be cut entirely, it will also *incorrectly* cut out
        //       the observable conv too!
        //
        //       So when we add convs w/ side-effects, be sure to change ret to, when !ret.has_value(), instead
        //       refer to a temporary on the top of the stack which stores value for conv to use, then popping
        //       this temporary afterwards.
        YAMA_DEADEND;
    }
    if (md.crvalue) { // Constexpr base expr.
        if (convert_to) { // Has implicit conv.
            if (const auto crvalue = _conv_crvalue(md.crvalue, *convert_to, md)) { // Constexpr implicit conv.
                _emit_put_crvalue(x, *crvalue, ret, ret_puts_must_reinit);
            }
            else { // Non-constexpr implicit conv.
                _emit_put_crvalue(x, *md.crvalue, ret, ret_puts_must_reinit);
                // _emit_put_crvalue does nothing if ret is empty, so conv instr can be forwent too.
                // Also, forgo codegen for conv if identity conv.
                if (ret && !md.tu->convs.is_identity_conv(md.crvalue->t, *convert_to)) {
                    // Account for if ret is newtop.
                    const auto dest = uint8_t(ret == size_t(newtop) ? md.tu->rs.top_reg().index : *ret);
                    md.tu->rs.reinit_temp(dest, *convert_to);
                    md.tu->cgt.cw.add_conv(dest, dest, uint8_t(md.tu->ctp.pull_type(*convert_to)), true);
                }
            }
        }
        else { // No implicit conv.
            _emit_put_crvalue(x, *md.crvalue, ret, ret_puts_must_reinit);
        }
    }
    else { // Non-constexpr base expr.
        // Perform codegen for the rest of the expr tree.
        if (const auto r = x.as<ast_PrimaryExpr>())                 _codegen_step(*r, ret, ret_puts_must_reinit);
        else if (const auto r = x.as<ast_ParenthesizedExpr>())      _codegen_step(*r, ret, ret_puts_must_reinit);
        else if (const auto r = x.as<ast_ConstexprGuaranteeExpr>()) _codegen_step(*r, ret, ret_puts_must_reinit);
        else if (const auto r = x.as<ast_Args>())                   _codegen_step(*r, ret, ret_puts_must_reinit);
        else if (const auto r = x.as<ast_Conv>())                   _codegen_step(*r, ret, ret_puts_must_reinit);
        else if (const auto r = x.as<ast_TypeMember>())             _codegen_step(*r, ret, ret_puts_must_reinit);
        else if (const auto r = x.as<ast_ObjectMember>())           _codegen_step(*r, ret, ret_puts_must_reinit);
        else                                                        YAMA_DEADEND;
        // Perform codegen for non-identity conv, if any.
        if (ret && convert_to && !md.tu->convs.is_identity_conv(md.type.value(), *convert_to)) {
            // Account for if ret is newtop.
            const auto dest = uint8_t(ret == size_t(newtop) ? md.tu->rs.top_reg().index : *ret);
            md.tu->rs.reinit_temp(dest, *convert_to);
            md.tu->cgt.cw.add_conv(dest, dest, uint8_t(md.tu->ctp.pull_type(*convert_to)), true);
        }
    }
    // TODO: This error check is SUPER in need of a refactor.
    // TODO: The whole notion of putting an error check here is *iffy*, w/ me in fact needing to
    //       put this at the end here to avoid the 'YAMA_ASSERT(!md.tu->err.is_fatal());' below.
    if (convert_to && !md.tu->convs.is_legal(md.type.value(), *convert_to, true)) {
        md.tu->err.error(
            md.where,
            dsignal::compile_type_mismatch,
            "expr is type {0}, but expected {1}!",
            md.type.value().fmt(md.tu->e()),
            convert_to->fmt(md.tu->e()));
    }
}

void yama::internal::expr_analyzer::_codegen_step(const ast_PrimaryExpr& x, std::optional<size_t> ret, bool ret_puts_must_reinit) {
    const auto& md = _fetch(x);
    YAMA_ASSERT(!md.tu->err.is_fatal());
    auto& tu = _tu(x);
    const ctype& type = md.type.value();
    switch (md.category) {
    case category::param_id:
    {
        // if no result, then just output nothing
        if (!ret) {
            return;
        }
        // if newtop, push a temporary
        if (ret == size_t(newtop)) {
            tu.rs.push_temp(x, type);
        }
        const uint8_t output_reg = uint8_t(deref_assert(ret));
        const auto name = x.name->str(md.tu->src);
        // IMPORTANT: remember that the indices of the args of a call include the callobj,
        //            so we gotta incr the fn param index to account for this
        const uint8_t arg_index = uint8_t(tu.cgt.target_param_index(name).value() + 1);
        tu.cgt.autosym(x.low_pos());
        // write put_arg loading param into output
        tu.cgt.cw.add_put_arg(output_reg, arg_index, ret_puts_must_reinit);
    }
    break;
    case category::var_id:
    {
        // if no result, then just output nothing
        if (!ret) {
            return;
        }
        // if newtop, push a temporary
        if (ret == size_t(newtop)) {
            tu.rs.push_temp(x, type);
        }
        const uint8_t output_reg = uint8_t(deref_assert(ret));
        const auto name = x.name->str(md.tu->src);
        const auto symbol = md.tu->syms.lookup_expect<var_csym>(x, name, x.low_pos());
        const uint8_t local_var_reg = uint8_t(symbol->reg.value());
        tu.cgt.autosym(x.low_pos());
        // write copy from local var into output
        tu.cgt.cw.add_copy(local_var_reg, output_reg, ret_puts_must_reinit);
    }
    break;
    case category::type_id:
    case category::int_lit:
    case category::uint_lit:
    case category::float_lit:
    case category::bool_lit:
    case category::char_lit:
    {
        YAMA_DEADEND;
    }
    break;
    default: YAMA_DEADEND; break;
    }
}

void yama::internal::expr_analyzer::_codegen_step(const ast_ParenthesizedExpr& x, std::optional<size_t> ret, bool ret_puts_must_reinit) {
    auto& md = _fetch(x);
    YAMA_ASSERT(!md.tu->err.is_fatal());
    auto& tu = _tu(x);
    const ctype& type = md.type.value();
    switch (md.category) {
    case category::parenthesized:
    {
        _codegen_step(*res(x.expr), ret, ret_puts_must_reinit);
    }
    break;
    default: YAMA_DEADEND; break;
    }
}

void yama::internal::expr_analyzer::_codegen_step(const ast_ConstexprGuaranteeExpr& x, std::optional<size_t> ret, bool ret_puts_must_reinit) {
    auto& md = _fetch(x);
    YAMA_ASSERT(!md.tu->err.is_fatal());
    auto& tu = _tu(x);
    const ctype& type = md.type.value();
    switch (md.category) {
    case category::constexpr_guarantee:
    {
        YAMA_DEADEND;
    }
    break;
    default: YAMA_DEADEND; break;
    }
}

void yama::internal::expr_analyzer::_codegen_step(const ast_Args& x, std::optional<size_t> ret, bool ret_puts_must_reinit) {
    const auto& md = _fetch(x);
    YAMA_ASSERT(!md.tu->err.is_fatal());
    auto& tu = _tu(x);
    const ctype& type = md.type.value();
    switch (md.category) {
    case category::bound_method_call:
    {
        // IMPORTANT: below code is quite similar to regular call, except that for bound method
        //            call we have to:
        //                  1) codegen method crvalue first (ie. callobj)
        //                  2) codegen owner rvalue (ie. first arg)
        //                  3) codegen args (ie. remaining args)
        //                  4) *skip* codegen of bound method itself
        const res bound_method_nd(x.get_primary_subexpr());
        const res owner_nd(bound_method_nd->get_primary_subexpr());
        const ctype& method_type = _pull(*bound_method_nd).type.value(); // <- use this to get method type
        // autosym w/ expr this is a suffix of
        tu.cgt.autosym(x.root_expr()->low_pos());
        // codegen method crvalue + push its reg
        tu.cgt.add_cvalue_put_instr(newtop, cvalue::method_v(method_type));
        tu.rs.push_temp(x, method_type);
        // codegen owner expr
        _codegen_step(*owner_nd, size_t(newtop));
        // codegen arg exprs
        size_t param_ind = 1; // Start counting from 1 as 0 is the owner param.
        for (const auto& I : x.args) {
            const ctype expected_param_type = method_type.param_type(param_ind, *cs).value();
            param_ind++;
            _codegen_step(*I, size_t(newtop), false, expected_param_type);
        }
        // autosym w/ expr this is a suffix of
        tu.cgt.autosym(x.root_expr()->low_pos());
        // on the register stack will be the temporaries corresponding to the callobj
        // and args of the call this code will resolve
        const size_t param_args = x.args.size() + 1; // param-args == owner (first arg) + other args
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
            tu.cgt.cw.add_call(uint8_t(args), uint8_t(return_value_reg), ret_puts_must_reinit);
            // pop args, ie. including callobj, as our result register won't
            // be a new register pushed, but instead some other one
            tu.rs.pop_temp(args, false);
        }
    }
    break;
    case category::call:
    {
        const auto callobj_nd = res(x.get_primary_subexpr());
        const ctype& callobj_type = _pull(*callobj_nd).type.value();
        // perform prior steps
        // (gotta do these BEFORE our autosym, as _codegen_step will overwrite it)
        _codegen_step(*callobj_nd, size_t(newtop));
        size_t param_ind = 0;
        for (const auto& I : x.args) { // codegen arg exprs
            const ctype expected_param_type = callobj_type.param_type(param_ind, *cs).value();
            param_ind++;
            _codegen_step(*I, size_t(newtop), false, expected_param_type);
        }
        // autosym w/ expr this is a suffix of
        tu.cgt.autosym(x.root_expr()->low_pos());
        // on the register stack will be the temporaries corresponding to the callobj
        // and args of the call this code will resolve
        const size_t param_args = x.args.size();
        const size_t args = param_args + 1; // args == param-args + callobj
        const size_t callobj_reg = tu.rs.regs() - args;
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
            tu.cgt.cw.add_call(uint8_t(args), uint8_t(return_value_reg), ret_puts_must_reinit);
            // pop args, ie. including callobj, as our result register won't
            // be a new register pushed, but instead some other one
            tu.rs.pop_temp(args, false);
        }
    }
    break;
    case category::default_init:
    {
        // TODO: later on, when we add default initializers which actually involve
        //       a ctor call, we'll in that scenario STILL be able to forgo codegen in
        //       situations where (1) this ctor call is transparent, and (2) the result
        //       of this default init expr isn't needed
        //
        //       not gonna add this now, as I don't have a way to distinguish between
        //       these different types of initializers, and I don't wanna risk creating
        //       bugs if later, when we add these initializers, I forget about this
        //       part of our code
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
        tu.cgt.autosym(x.root_expr()->low_pos());
        // write default_init instr to initialize the object
        tu.cgt.cw.add_default_init(output_reg, uint8_t(tu.ctp.pull_type(type)), ret_puts_must_reinit);
    }
    break;
    default: YAMA_DEADEND; break;
    }
}

void yama::internal::expr_analyzer::_codegen_step(const ast_Conv& x, std::optional<size_t> ret, bool ret_puts_must_reinit) {
    auto& md = _fetch(x);
    YAMA_ASSERT(!md.tu->err.is_fatal());
    auto& tu = _tu(x);
    const ctype& type = md.type.value();
    switch (md.category) {
    case category::conv:
    {
        const ctype converted_from = deref_assert(x.get_primary_subexpr()).get_type(*cs).value();
        const ctype converted_to = deref_assert(x.target).get_type(*cs).value();
        // If the return value is just gonna be discarded BUT the conversion operation itself
        // has observable side-effects, then in that case we emit the conversion anyway.
        const bool should_emit_conv =
            ret ||
            tu.convs.has_side_effects(converted_from, converted_to);
        if (should_emit_conv) {
            const bool should_force_reinit =
                ret && // Need to confirm it's safe to *ret.
                ret != size_t(newtop) &&
                converted_to != tu.rs.reg_abs(*ret).type;
            // It's our responsibility to do this.
            if (should_force_reinit) {
                tu.rs.reinit_temp(ssize_t(ret.value()), converted_to);
            }
            // Codegen converted expr, putting it into return value register, which we'll then overwrite
            // w/ conversion result.
            _codegen_step(
                *res(x.get_primary_subexpr()),
                ret,
                should_force_reinit || ret_puts_must_reinit);
            // Need to handle if ret is newtop, as if ret is newtop, then instr gen will be incorrect,
            // as newtop stuff will have already been handled by above _codegen_step for child.
            const size_t return_value_reg =
                (!ret || ret == size_t(newtop))
                ? tu.rs.top_reg().index
                : ret.value();
            tu.cgt.autosym(x.low_pos());
            tu.rs.reinit_temp(ssize_t(return_value_reg), type);
            tu.cgt.cw.add_conv(
                uint8_t(return_value_reg),
                uint8_t(return_value_reg),
                uint8_t(tu.ctp.pull_type(type)),
                true);
        }
        else { // This expr is 100% transparent EXCEPT for any side-effects arising from converted expr.
            // Codegen converted expr (we don't care about its return value, only its side-effects.)
            _codegen_step(*res(x.get_primary_subexpr()), std::nullopt);
        }
    }
    break;
    default: YAMA_DEADEND; break;
    }
}

void yama::internal::expr_analyzer::_codegen_step(const ast_TypeMember& x, std::optional<size_t> ret, bool ret_puts_must_reinit) {
    const auto& md = _fetch(x);
    YAMA_ASSERT(!md.tu->err.is_fatal());
    auto& tu = _tu(x);
    const ctype& type = md.type.value();
    switch (md.category) {
    case category::unbound_method:
    {
        YAMA_DEADEND;
    }
    break;
    default: YAMA_DEADEND; break;
    }
}

void yama::internal::expr_analyzer::_codegen_step(const ast_ObjectMember& x, std::optional<size_t> ret, bool ret_puts_must_reinit) {
    const auto& md = _fetch(x);
    YAMA_ASSERT(!md.tu->err.is_fatal());
    auto& tu = _tu(x);
    const ctype& type = md.type.value();
    switch (md.category) {
    case category::bound_method:
    {
        // Codegen owner (we don't care about its return value, only its side-effects.)
        _codegen_step(*res(x.get_primary_subexpr()), std::nullopt);
        _emit_put_crvalue(x, cvalue::method_v(type), ret);
    }
    break;
    default: YAMA_DEADEND; break;
    }
}

void yama::internal::expr_analyzer::_codegen_step(const ast_Expr& x, std::optional<size_t> ret, bool ret_puts_must_reinit, std::optional<ctype> convert_to) {
    _codegen_step(*res(x.get_primary_subexpr()), ret, ret_puts_must_reinit, convert_to);
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

void yama::internal::expr_analyzer::_emit_put_crvalue(const ast_expr& x, cvalue crvalue, std::optional<size_t> ret, bool ret_puts_must_reinit) {
    auto& tu = _tu(x);
    // If expr is not even gonna be used for its return value, then this expr is
    // 100% transparent, and codegen can just be forwent altogether.
    if (!ret) {
        return;
    }
    const uint8_t output_reg = uint8_t(deref_assert(ret));
    // If newtop, push a temporary.
    if (ret == size_t(newtop)) {
        tu.rs.push_temp(x, crvalue.t);
    }
    tu.cgt.autosym(x.low_pos());
    tu.cgt.add_cvalue_put_instr(output_reg, crvalue, ret_puts_must_reinit);
}

std::optional<yama::internal::cvalue> yama::internal::expr_analyzer::_default_init_crvalue(ctype type, metadata& md) {
    auto& tu = *md.tu;
    static_assert(kinds == 4); // reminder
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
    else if (is_method(type.kind()))            return cvalue::method_v(type);
    else                                        return _runtime_only;
    return std::nullopt; // dummy
}

std::optional<yama::internal::cvalue> yama::internal::expr_analyzer::_conv_crvalue(std::optional<cvalue> x, ctype target, metadata& md) {
    auto& tu = *md.tu;
    if (!x) {
        return std::nullopt;
    }
    const cvalue& value = *x;
    const auto conv_type = tu.convs.discern_type(value.t, target);
#if 0
    println("-- {} -> {} ({})", value.t.fmt(md.tu->e()), target.fmt(md.tu->e()), fmt_conv_type(conv_type));
#endif
    if (conv_type == conv_type::identity) {
        return x;
    }
    else if (conv_type == conv_type::primitive_type) {
        const ctype& in = value.t;
        const ctype& out = target;
        auto _output = [&](auto v) -> std::optional<cvalue> {
            if (out == tu.types.int_type())         return cvalue::int_v(int_t(v), tu.types);
            else if (out == tu.types.uint_type())   return cvalue::uint_v(uint_t(v), tu.types);
            else if (out == tu.types.float_type())  return cvalue::float_v(float_t(v), tu.types);
            else if (out == tu.types.bool_type())   return cvalue::bool_v(bool_t(v), tu.types);
            else if (out == tu.types.char_type())   return cvalue::char_v(char_t(v), tu.types);
            else                                    return _runtime_only;
        };
        if (in == tu.types.int_type())          return _output(value.as<int_t>().value());
        else if (in == tu.types.uint_type())    return _output(value.as<uint_t>().value());
        else if (in == tu.types.float_type())   return _output(value.as<float_t>().value());
        else if (in == tu.types.bool_type())    return _output(value.as<bool_t>().value());
        else if (in == tu.types.char_type())    return _output(value.as<char_t>().value());
        else                                    return _runtime_only;
    }
    else if (conv_type == conv_type::fn_type_narrow_to_type_type) {
        return cvalue::type_v(value.t, tu.types);
    }
    else if (conv_type == conv_type::method_type_narrow_to_type_type) {
        return cvalue::type_v(value.t, tu.types);
    }
    else return _runtime_only;
}

bool yama::internal::expr_analyzer::_is_type_ref_id_expr(const ast_PrimaryExpr& x) {
    if (!x.name) return false; // not a type ref if it's not even an id expr
    if (x.qualifier) return true; // definitely a type ref if has qualifier
    const auto& tu = _tu(x);
    const auto sym = tu.syms.lookup(x, x.name->str(tu.src), x.low_pos());
    // succeed if nullptr, as that means it MUST ref type not in compiling module
    return !sym || sym->is_type;
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

bool yama::internal::expr_analyzer::_raise_wrong_arg_count_if(bool x, metadata& md, size_t actual_args, size_t expected_args) {
    if (x) {
        md.tu->err.error(
            md.where,
            dsignal::compile_wrong_arg_count,
            "{} given {} args, but expected {}!",
            fmt_category(md.category),
            actual_args,
            expected_args);
    }
    return x;
}

bool yama::internal::expr_analyzer::_raise_wrong_arg_count_if(bool x, metadata& md, ctype call_to, size_t actual_args, size_t expected_args) {
    if (x) {
        md.tu->err.error(
            md.where,
            dsignal::compile_wrong_arg_count,
            "call to {} given {} args, but expected {}!",
            call_to.fmt(md.tu->e()),
            actual_args,
            expected_args);
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

bool yama::internal::expr_analyzer::_raise_invalid_operation_due_to_illegal_conv_if(ctype from, ctype to, metadata& md) {
    const bool result = !md.tu->convs.is_legal(from, to);
    if (result) {
        md.tu->err.error(
            md.where,
            dsignal::compile_invalid_operation,
            "illegal conversion {} -> {}!",
            from.fmt(md.tu->e()),
            to.fmt(md.tu->e()));
    }
    return result;
}

bool yama::internal::expr_analyzer::_raise_coerced_type_mismatch_for_arg_if(ctype actual, ctype expected, metadata& md, size_t arg_display_number) {
    const bool result = !md.tu->convs.is_legal(actual, expected, true);
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

bool yama::internal::expr_analyzer::_raise_type_mismatch_for_conv_target_if(ctype actual, metadata& md) {
    const ctype expected = md.tu->types.type_type();
    const bool result = actual != expected;
    if (result) {
        md.tu->err.error(
            md.where,
            dsignal::compile_type_mismatch,
            "conversion target expr is type {0}, but expected {1}!",
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

void yama::internal::expr_analyzer::_trace_result(bool success, const ast_expr& x) {
    auto msg = [&]() -> std::string {
        const auto& e = cs->dd->installs.domain_env();
        const auto& fetched = _fetch(x);
        if (!success)                           return "fail";
        else if (auto crv = fetched.crvalue)    return crv.value().fmt(e);
        else                                    return std::format("{} (*runtime-only*)", fetched.type.value().fmt(e));
        };
    YAMA_LOG(cs->dbg(), compile_c, "result: {}", msg());
}

