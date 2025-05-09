

#include "ctype_resolver.h"

#include "compiler.h"


yama::internal::ctype_resolver::ctype_resolver(compiler& cs)
    : cs(cs) {}

std::optional<yama::internal::ctype> yama::internal::ctype_resolver::get(const ast_PrimaryExpr* x) const noexcept {
    const auto it = _mappings.find(x);
    return
        it != _mappings.end()
        ? it->second.t
        : std::nullopt;
}

void yama::internal::ctype_resolver::add(translation_unit& tu, const ast_PrimaryExpr& x) {
    _mappings.insert({ &x, _entry_t{ safeptr(tu), std::nullopt } });
}

void yama::internal::ctype_resolver::resolve() {
    for (auto& [key, value] : _mappings) {
        _resolve(*value.tu, deref_assert(key), value.t);
    }
}

void yama::internal::ctype_resolver::cleanup() {
    _mappings.clear();
}

void yama::internal::ctype_resolver::_resolve(translation_unit& tu, const ast_PrimaryExpr& x, std::optional<ctype>& target) {
    if (!_is_type_ref_id_expr(tu, x)) return; // skip if x isn't *supposed* to even be a type ref
    bool ambiguous{};
    bool bad_qualifier{};
    if (const auto t = tu.types.load(x, ambiguous, bad_qualifier)) {
        target = *t;
    }
    else {
        const std::string name = x.fmt_name(tu.src).value();
        if (ambiguous) { // ambiguous
            tu.err.error(
                x,
                dsignal::compile_ambiguous_name,
                "ambiguous name {}!",
                name);
        }
        if (bad_qualifier) { // bad qualifier
            tu.err.error(
                x,
                dsignal::compile_undeclared_qualifier,
                "undeclared qualifier {}!",
                x.qualifier->str(tu.src));
        }
        else { // nothing found
            tu.err.error(
                x,
                dsignal::compile_undeclared_name,
                "undeclared name {}!",
                name);
        }
    }
}

bool yama::internal::ctype_resolver::_is_type_ref_id_expr(translation_unit& tu, const ast_PrimaryExpr& x) {
    if (!x.name) return false; // not a type ref if it's not even an id expr
    const auto sym = tu.syms.lookup(x, x.name->str(tu.src), x.low_pos());
    // succeed if nullptr, as that means it MUST ref type not in compiling module
    return !sym || sym->is_type();
}

