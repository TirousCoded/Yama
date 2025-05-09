

#include "constexpr_solver.h"

#include "compiler.h"


#define _DUMP_LOG 0


yama::internal::constexpr_solver::constexpr_solver(compiler& cs)
    : cs(cs) {}

std::optional<yama::internal::cvalue> yama::internal::constexpr_solver::get(const ast_PrimaryExpr* x) const noexcept {
    return _get(x);
}

std::optional<yama::internal::cvalue> yama::internal::constexpr_solver::get(const ast_Args* x) const noexcept {
    return _get(x);
}

std::optional<yama::internal::cvalue> yama::internal::constexpr_solver::get(const ast_Expr* x) const noexcept {
    return _get(x);
}

void yama::internal::constexpr_solver::add(translation_unit& tu, const ast_PrimaryExpr& x, bool mandatory) {
#if _DUMP_LOG
    println("-- constexpr_solver add ast_PrimaryExpr at {} {}", uintptr_t(&x), tu.src.location_at(x.low_pos()));
#endif
    _mappings.insert({ &x, _entry_t{ safeptr(tu), _mode::primary_expr, std::nullopt, mandatory } });
}

void yama::internal::constexpr_solver::add(translation_unit& tu, const ast_Args& x, bool mandatory) {
#if _DUMP_LOG
    println("-- constexpr_solver add ast_Args at {} {}", uintptr_t(&x), tu.src.location_at(x.low_pos()));
#endif
    _mappings.insert({ &x, _entry_t{ safeptr(tu), _mode::args, std::nullopt, mandatory } });
}

void yama::internal::constexpr_solver::add(translation_unit& tu, const ast_Expr& x, bool mandatory) {
#if _DUMP_LOG
    println("-- constexpr_solver add ast_Expr at {} {}", uintptr_t(&x), tu.src.location_at(x.low_pos()));
#endif
    _mappings.insert({ &x, _entry_t{ safeptr(tu), _mode::expr, std::nullopt, mandatory } });
}

void yama::internal::constexpr_solver::solve() {
#if _DUMP_LOG
    println("-- constexpr_solver solving...");
#endif
    for (auto& [key, value] : _mappings) {
        if (value.v = _solve(*key, value.mode, *value.tu)) continue;
#if _DUMP_LOG
        println("-- constexpr_solver solve failed for node at {}", uintptr_t(key));
#endif
        if (!value.mandatory) continue;
        value.tu->err.error(
            *key,
            dsignal::compile_nonconstexpr_expr,
            "non-constexpr expr!");
    }
}

void yama::internal::constexpr_solver::cleanup() {
    _mappings.clear();
}

std::optional<yama::internal::cvalue> yama::internal::constexpr_solver::_get(const ast_node* x) const noexcept {
    const auto it = _mappings.find(x);
#if _DUMP_LOG
    if (x && it != _mappings.end()) {
        println("-- constexpr_solver get node at {} {}", uintptr_t(x), it->second.tu->src.location_at(x->low_pos()));
    }
    else {
        println("-- constexpr_solver get node at {}", uintptr_t(x));
    }
#endif
    return
        it != _mappings.end()
        ? it->second.v
        : std::nullopt;
}

std::optional<yama::internal::cvalue> yama::internal::constexpr_solver::_solve(const ast_node& x, _mode mode, translation_unit& tu) {
    switch (mode) {
    case _mode::primary_expr:   return _solve(static_cast<const ast_PrimaryExpr&>(x), tu);
    case _mode::args:           return _solve(static_cast<const ast_Args&>(x), tu);
    case _mode::expr:           return _solve(static_cast<const ast_Expr&>(x), tu);
    default:                    { YAMA_DEADEND; return std::nullopt; }
    }
}

std::optional<yama::internal::cvalue> yama::internal::constexpr_solver::_solve(const ast_PrimaryExpr& x, translation_unit& tu) {
#if _DUMP_LOG
    println("-- constexpr_solver solve ast_PrimaryExpr at {}", uintptr_t(&x));
#endif
    if (x.name) {
        const auto name = x.name->str(tu.src);
        const auto symbol = tu.syms.lookup(x, name, x.low_pos());
        const bool is_param = symbol && symbol->is<param_csym>();
        const bool is_var = symbol && symbol->is<var_csym>();
        // param, local var, prim or fn
        if (is_param) {
            // TODO: NEVER constexpr
            return std::nullopt;
        }
        else if (is_var) {
            // TODO: NEVER constexpr
            return std::nullopt;
        }
        // below check is used *instead of* checking if symbol is a prim, fn, etc.
        // as it handles both that and cases of another module defining a type
        else if (const auto type = tu.cs->resolver[&x]) {
            if (type->kind() == kind::primitive) {
                return cvalue::type_v(*type, tu.types);
            }
            else if (type->kind() == kind::function) {
                return cvalue::fn_v(*type);
            }
            else YAMA_DEADEND;
        }
        else return std::nullopt; // fail quietly
    }
    else if (x.lit) {
        if (x.lit->is_int()) {
            const auto lit_nd = res(x.lit->as_int());
            const parsed_int v = parse_int(lit_nd->lit.str(tu.src)).value();
            if (v.overflow) { // overflow error
                // TODO: should we raise this?
                //tu.err.error(
                //    x,
                //    dsignal::compile_numeric_overflow,
                //    "numeric overflow ({})!",
                //    lit_nd->lit.str(tu.src));
                return std::nullopt; // fail quietly
            }
            else if (v.underflow) { // underflow error
                // TODO: should we raise this?
                //tu.err.error(
                //    x,
                //    dsignal::compile_numeric_underflow,
                //    "numeric underflow ({})!",
                //    lit_nd->lit.str(tu.src));
                return std::nullopt; // fail quietly
            }
            else { // valid
                return cvalue::int_v(v.v, tu.types);
            }
        }
        else if (x.lit->is_uint()) {
            const auto lit_nd = res(x.lit->as_uint());
            const parsed_uint v = parse_uint(lit_nd->lit.str(tu.src)).value();
            if (v.overflow) { // overflow error
                // TODO: should we raise this?
                //tu.err.error(
                //    x,
                //    dsignal::compile_numeric_overflow,
                //    "numeric overflow ({})!",
                //    lit_nd->lit.str(tu.src));
                return std::nullopt; // fail quietly
            }
            else { // valid
                return cvalue::uint_v(v.v, tu.types);
            }
        }
        else if (x.lit->is_float()) {
            const auto lit_nd = res(x.lit->as_float());
            const parsed_float v = parse_float(lit_nd->lit.str(tu.src)).value();
            // below can handle cases of overflow/underflow
            return cvalue::float_v(v.v, tu.types);
        }
        else if (x.lit->is_bool()) {
            const auto lit_nd = res(x.lit->as_bool());
            const parsed_bool v = parse_bool(lit_nd->lit.str(tu.src)).value();
            return cvalue::bool_v(v.v, tu.types);
        }
        else if (x.lit->is_char()) {
            const auto lit_nd = res(x.lit->as_char());
            const auto txt = lit_nd->lit.str(tu.src); // <- text w/ single-quotes
            const auto txt_corrected = txt.substr(1, txt.length() - 2); // <- text w/out single-quotes
            const parsed_char v = parse_char(txt_corrected).value();
            if (v.bytes < txt_corrected.length()) { // illegal multi-codepoint char literal error
                // TODO: should we raise this?
                //tu.err.error(
                //    x,
                //    dsignal::compile_malformed_literal,
                //    "illegal multi-codepoint char literal ({})!",
                //    fmt_string_literal(txt_corrected));
                return std::nullopt; // fail quietly
            }
            else if (!taul::is_unicode(v.v)) { // illegal Unicode error
                // TODO: should we raise this?
                //tu.err.error(
                //    x,
                //    dsignal::compile_illegal_unicode,
                //    "illegal Unicode ({})!",
                //    fmt_string_literal(txt_corrected));
                return std::nullopt; // fail quietly
            }
            else { // valid
                return cvalue::char_v(v.v, tu.types);
            }
        }
        else YAMA_DEADEND;
    }
    else YAMA_DEADEND;
    return std::nullopt;
}

std::optional<yama::internal::cvalue> yama::internal::constexpr_solver::_solve(const ast_Args& x, translation_unit& tu) {
#if _DUMP_LOG
    println("-- constexpr_solver solve ast_Args at {}", uintptr_t(&x));
#endif
    return
        x.is_constexpr_guarantee_expr_args()
        ? _solve_constexpr_guarantee_expr_args(x, tu)
        : _solve_call_expr_args(x, tu);
}

std::optional<yama::internal::cvalue> yama::internal::constexpr_solver::_solve(const ast_Expr& x, translation_unit& tu) {
#if _DUMP_LOG
    println("-- constexpr_solver solve ast_Expr at {}", uintptr_t(&x));
#endif
    return
        x.args.empty()
        ? _solve(*res(x.primary), tu)
        : _solve(*x.args.back(), tu);
}

std::optional<yama::internal::cvalue> yama::internal::constexpr_solver::_solve_call_expr_args(const ast_Args& x, translation_unit& tu) {
#if _DUMP_LOG
    println("-- constexpr_solver solve ast_Args at {} (call expr)", uintptr_t(&x));
#endif
    // TODO: NEVER constexpr
    return std::nullopt;
}

std::optional<yama::internal::cvalue> yama::internal::constexpr_solver::_solve_constexpr_guarantee_expr_args(const ast_Args& x, translation_unit& tu) {
#if _DUMP_LOG
    println("-- constexpr_solver solve ast_Args at {} (constexpr guarantee expr)", uintptr_t(&x));
#endif
    return
        x.args.size() == 1
        ? _solve(*res(x.args[0]), tu)
        : std::nullopt; // fail quietly
}

