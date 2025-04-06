

#include "ctype_resolver.h"

#include "compiler.h"


yama::internal::ctype_resolver::ctype_resolver(compiler& cs)
    : cs(cs) {}

void yama::internal::ctype_resolver::add(translation_unit& tu, const ast_TypeSpec& x) {
    _type_spec_mappings.insert({ &x, _entry_t{ safeptr(tu), std::nullopt } });
}

void yama::internal::ctype_resolver::add(translation_unit& tu, const ast_PrimaryExpr& x) {
    _primary_expr_mappings.insert({ &x, _entry_t{ safeptr(tu), std::nullopt } });
}

void yama::internal::ctype_resolver::resolve() {
    for (auto& [key, value] : _type_spec_mappings)      _resolve(*value.tu, deref_assert(key), value.t);
    for (auto& [key, value] : _primary_expr_mappings)   _resolve(*value.tu, deref_assert(key), value.t);
}

std::optional<yama::internal::ctype> yama::internal::ctype_resolver::get(const ast_TypeSpec* x) const noexcept {
    const auto it = _type_spec_mappings.find(x);
    return
        it != _type_spec_mappings.end()
        ? it->second.t
        : std::nullopt;
}

std::optional<yama::internal::ctype> yama::internal::ctype_resolver::get(const ast_PrimaryExpr* x) const noexcept {
    const auto it = _primary_expr_mappings.find(x);
    return
        it != _primary_expr_mappings.end()
        ? it->second.t
        : std::nullopt;
}

void yama::internal::ctype_resolver::_resolve(translation_unit& tu, const ast_TypeSpec& x, std::optional<ctype>& target) {
    if (const auto sym = tu.syms.lookup(x, x.name.value().str(tu.src), x.low_pos()); sym && !sym->is_type()) {
        tu.er.error(
            x,
            dsignal::compile_not_a_type,
            "not a type!");
        return;
    }
    bool ambiguous{};
    bool bad_qualifier{};
    if (const auto t = tu.types.load(x, ambiguous, bad_qualifier)) {
        target = *t;
    }
    else _report(tu, ambiguous, bad_qualifier, x, x.qualifier ? std::make_optional(x.qualifier->str(tu.src)) : std::nullopt, x.fmt_name(tu.src));
}

void yama::internal::ctype_resolver::_resolve(translation_unit& tu, const ast_PrimaryExpr& x, std::optional<ctype>& target) {
    if (!_is_type_spec_id_expr(tu, x)) return; // don't want to raise error if x isn't *supposed* to even be a type spec
    bool ambiguous{};
    bool bad_qualifier{};
    if (const auto t = tu.types.load(x, ambiguous, bad_qualifier)) {
        target = *t;
    }
    else _report(tu, ambiguous, bad_qualifier, x, x.qualifier ? std::make_optional(x.qualifier->str(tu.src)) : std::nullopt, x.fmt_name(tu.src));
}

bool yama::internal::ctype_resolver::_is_type_spec_id_expr(translation_unit& tu, const ast_PrimaryExpr& x) {
    if (!x.name) return false; // not a type spec if it's not even an id expr
    const auto sym = tu.syms.lookup(x, x.name->str(tu.src), x.low_pos());
    // succeed if nullptr, as that means it MUST ref type not in compiling module
    return !sym || sym->is_type();
}

void yama::internal::ctype_resolver::_report(translation_unit& tu, bool ambiguous, bool bad_qualifier, const ast_node& where, std::optional<std::string_view> qualifier, const std::string& name) {
    if (ambiguous) { // ambiguous
        tu.er.error(
            where,
            dsignal::compile_ambiguous_name,
            "ambiguous name {}!",
            name);
    }
    if (bad_qualifier) { // bad qualifier
        tu.er.error(
            where,
            dsignal::compile_undeclared_qualifier,
            "undeclared qualifier {}!",
            std::string(qualifier.value()));
    }
    else { // nothing found
        tu.er.error(
            where,
            dsignal::compile_undeclared_name,
            "undeclared name {}!",
            name);
    }
}

