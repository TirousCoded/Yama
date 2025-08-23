

#include "compiler.h"

#include "domain_data.h"


yama::internal::compiler::compiler(const std::shared_ptr<debug>& dbg, domain_data& dd)
    : api_component(dbg),
    dd(dd),
    sp(),
    types(*this),
    ea(*this) {}

bool yama::internal::compiler::compile(const taul::source_code& src, const import_path& src_path) {
    YAMA_LOG(dbg(), compile_c, "compiling {}...", src_path.fmt(dd->installs.domain_env()));
    push_new_unit(make_res<translation_unit>(*this, taul::source_code(src), src_path));
    // No second passes if first passes fail.
    const bool success =
        first_passes() &&
        second_passes();
    cleanup(); // <- Can't forget.
    YAMA_LOG(dbg(), compile_c, "compilation complete!");
    return success;
}

bool yama::internal::compiler::first_passes() {
    YAMA_LOG(dbg(), compile_c, "first passes...");
    bool success = true;
    // Below iterates by index so looping works even as new translation
    // units get added.
    for (size_t i = 0; i < units.size(); i++) {
        YAMA_LOG(dbg(), compile_c, "first pass for {}", units[i]->src_path.fmt(units[i]->e()));
        if (first_pass(*units[i])) continue;
        success = false;
        YAMA_LOG(dbg(), compile_c, "first pass failed!");
    }
    YAMA_LOG(dbg(), compile_c, "first passes complete!");
    return success;
}

bool yama::internal::compiler::second_passes() {
    ea.analyze();
    YAMA_LOG(dbg(), compile_c, "second passes...");
    bool success = true;
    for (const auto& unit : units) {
        YAMA_LOG(dbg(), compile_c, "second pass for {}", unit->src_path.fmt(unit->e()));
        if (second_pass(*unit)) continue;
        success = false;
        YAMA_LOG(dbg(), compile_c, "second pass failed!");
    }
    YAMA_LOG(dbg(), compile_c, "second passes complete!");
    return success;
}

void yama::internal::compiler::cleanup() {
    types.cleanup();
    ea.cleanup();
    units.clear();
}

bool yama::internal::compiler::first_pass(translation_unit& unit) {
    return unit.first_pass();
}

bool yama::internal::compiler::second_pass(translation_unit& unit) {
    return unit.second_pass();
}

void yama::internal::compiler::push_new_unit(const res<translation_unit>& unit) {
    YAMA_LOG(dbg(), compile_c, "adding translation unit {}", unit->src_path.fmt(unit->e()));
    units.push_back(unit);
}

yama::internal::translation_unit::translation_unit(compiler& cs, taul::source_code&& src, const import_path& src_path)
    : cs(cs),
    src(std::forward<taul::source_code>(src)),
    src_path(src_path),
    err(*this),
    cgt(*this),
    syms(*this),
    types(*this),
    ctp(*this),
    rs(*this),
    ast(),
    fp(*this),
    sp(*this) {}

bool yama::internal::translation_unit::first_pass() {
    if (!parse_ast()) return false;
    root().accept(fp);
    return err.good();
}

bool yama::internal::translation_unit::second_pass() {
#if 0
    println("{}", ast->fmt_tree(src));
    println("{}", syms.fmt(*cs));
#endif
    root().accept(sp);
    return upload();
}

bool yama::internal::translation_unit::parse_ast() {
    const auto result = ast_parser().parse(src);
    if (const auto syntax_error = result.syntax_error) {
        err.error(
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
    if (err.is_fatal()) return false;
    auto m = make_res<module>(std::move(output));
#if 0
    println("{}", m->fmt());
#endif
    return cs->dd->importer.upload_compiled_module(src_path, std::move(m));
}

yama::internal::env yama::internal::translation_unit::e() const {
    return cs->dd->installs.parcel_env(parcel()).value();
}

yama::internal::ast_Chunk& yama::internal::translation_unit::root() const {
    return deref_assert(ast);
}

