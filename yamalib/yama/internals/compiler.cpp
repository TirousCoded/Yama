

#include "compiler.h"

#include "domain_data.h"


yama::internal::compiler::compiler(const std::shared_ptr<debug>& dbg, domain_data& dd)
    : api_component(dbg),
    dd(dd),
    sp(),
    types(*this),
    resolver(*this) {}

bool yama::internal::compiler::compile(const taul::source_code& src, const import_path& src_path) {
    push_new_unit(make_res<translation_unit>(*this, taul::source_code(src), src_path));
    // no second passes if first passes fail
    const bool success =
        first_passes() &&
        second_passes();
    cleanup(); // <- can't forget
    return success;
}

bool yama::internal::compiler::first_passes() {
    bool success = true;
    // below iterates by index so looping works even as new translation
    // units get added
    for (size_t i = 0; i < units.size(); i++) {
        if (first_pass(*units[i])) continue;
        success = false;
    }
    return success;
}

bool yama::internal::compiler::second_passes() {
    resolver.resolve(); // <- before doing second passes
    bool success = true;
    for (const auto& unit : units) {
        if (second_pass(*unit)) continue;
        success = false;
    }
    return success;
}

void yama::internal::compiler::cleanup() {
    types.cleanup();
    resolver.cleanup();
    units.clear();
}

bool yama::internal::compiler::first_pass(translation_unit& unit) {
    return unit.first_pass();
}

bool yama::internal::compiler::second_pass(translation_unit& unit) {
    return unit.second_pass();
}

void yama::internal::compiler::push_new_unit(const res<translation_unit>& unit) {
    units.push_back(unit);
}

yama::internal::translation_unit::translation_unit(compiler& cs, taul::source_code&& src, const import_path& src_path)
    : cs(cs),
    src(std::forward<taul::source_code>(src)),
    src_path(src_path),
    er(*this),
    syms(),
    types(*this),
    ctp(*this),
    ast(),
    fp(*this),
    sp(*this) {}

bool yama::internal::translation_unit::first_pass() {
    if (!parse_ast()) return false;
    root().accept(fp);
    return er.good();
}

bool yama::internal::translation_unit::second_pass() {
#if 0
    println("{}", ast->fmt_tree(src));
    println("{}", syms.fmt(src));
#endif
    root().accept(sp);
    return upload();
}

bool yama::internal::translation_unit::parse_ast() {
    const auto result = ast_parser().parse(src);
    if (const auto syntax_error = result.syntax_error) {
        er.error(
            *syntax_error,
            dsignal::compile_syntax_error,
            "syntax error!");
        return false;
    }
    else {
        ast = result.root;
        return true;
    }
}

bool yama::internal::translation_unit::upload() {
    if (er.is_fatal()) return false;
    return cs->dd->importer.upload_compiled_module(src_path, make_res<module_info>(output.done()));
}

yama::internal::env yama::internal::translation_unit::e() const {
    return cs->dd->installs.parcel_env(parcel()).value();
}

yama::internal::ast_Chunk& yama::internal::translation_unit::root() const {
    return deref_assert(ast);
}

