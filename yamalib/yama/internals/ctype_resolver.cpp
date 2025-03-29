

#include "ctype_resolver.h"


yama::internal::ctype_resolver::ctype_resolver(
    csymtab_group& csymtabs,
    error_reporter& er,
    const taul::source_code& src)
    : _csymtabs(&csymtabs),
    _er(&er),
    _src(&src) {}

void yama::internal::ctype_resolver::add(const ast_TypeSpec& x) {
    _type_spec_mappings.insert({ &x, std::nullopt });
}

void yama::internal::ctype_resolver::add(const ast_PrimaryExpr& x) {
    _primary_expr_mappings.insert({ &x, std::nullopt });
}

void yama::internal::ctype_resolver::resolve(ctypesys_local& ctypesys) {
    for (auto& [key, value] : _type_spec_mappings) {
        _resolve(ctypesys, deref_assert(key), value);
    }
    for (auto& [key, value] : _primary_expr_mappings) {
        _resolve(ctypesys, deref_assert(key), value);
    }
}

std::optional<yama::internal::ctype> yama::internal::ctype_resolver::get(const ast_TypeSpec* x) const noexcept {
    const auto it = _type_spec_mappings.find(x);
    return
        it != _type_spec_mappings.end()
        ? it->second
        : std::nullopt;
}

std::optional<yama::internal::ctype> yama::internal::ctype_resolver::get(const ast_PrimaryExpr* x) const noexcept {
    const auto it = _primary_expr_mappings.find(x);
    return
        it != _primary_expr_mappings.end()
        ? it->second
        : std::nullopt;
}

void yama::internal::ctype_resolver::_resolve(ctypesys_local& ctypesys, const ast_TypeSpec& x, std::optional<ctype>& target) {
    const auto& our_src = yama::deref_assert(_src);
    if (const auto sym = deref_assert(_csymtabs).lookup(x, x.type.str(our_src), x.low_pos()); sym && !sym->is_type()) {
        deref_assert(_er).error(
            x,
            dsignal::compile_not_a_type,
            "not a type!");
        return;
    }
    bool ambiguous{};
    if (const auto t = ctypesys.load(x, ambiguous)) {
        target = *t;
    }
    else _report(ambiguous, x, x.fmt_type(our_src));
}

void yama::internal::ctype_resolver::_resolve(ctypesys_local& ctypesys, const ast_PrimaryExpr& x, std::optional<ctype>& target) {
    if (!_is_type_spec_id_expr(x)) return; // don't want to raise error if x isn't *supposed* to even be a type spec
    const auto name = x.name.value().str(yama::deref_assert(_src));
    bool ambiguous{};
    if (const auto t = ctypesys.load(name, ambiguous)) {
        target = *t;
    }
    else _report(ambiguous, x, name.fmt());
}

bool yama::internal::ctype_resolver::_is_type_spec_id_expr(const ast_PrimaryExpr& x) {
    if (!x.name) return false; // not a type spec if it's not even an id expr
    const auto sym = deref_assert(_csymtabs).lookup(x, x.name->str(yama::deref_assert(_src)), x.low_pos());
    // succeed if nullptr, as that means it MUST ref type not in compiling module
    return !sym || sym->is_type();
}

void yama::internal::ctype_resolver::_report(bool ambiguous, const ast_node& where, const std::string& name) {
    if (ambiguous) { // ambiguous
        deref_assert(_er).error(
            where,
            dsignal::compile_ambiguous_name,
            "ambiguous name {}!",
            name);
    }
    else { // nothing found
        deref_assert(_er).error(
            where,
            dsignal::compile_undeclared_name,
            "undeclared name {}!",
            name);
    }
}

