

#include "compiler.h"

#include "ast.h"
#include "first_pass.h"
#include "second_pass.h"


#define _DUMP_LOG 0


yama::internal::compiler_services::compiler_services(std::shared_ptr<debug> dbg)
    : api_component(dbg) {}

void yama::internal::compile_result::add(const import_path& path, module_info&& x) {
    results.insert({ path, std::forward<module_info>(x) });
}

yama::internal::compiler::compiler(std::shared_ptr<debug> dbg)
    : api_component(dbg) {}

std::optional<yama::internal::compile_result> yama::internal::compiler::compile(
    res<compiler_services> services,
    const taul::source_code& src,
    const import_path& src_import_path) {
    const auto ast = ast_parser().parse(src);
    if (const auto syntax_error = ast.syntax_error) {
        YAMA_RAISE(dbg(), dsignal::compile_syntax_error);
        YAMA_LOG(
            dbg(), compile_error_c,
            "error: {0} syntax error!",
            src.location_at(*syntax_error));
        return std::nullopt;
    }
    specifier_provider our_sp{};
    error_reporter our_er(dbg(), src);
    csymtab_group our_csymtabs{};
    ctype_resolver our_ctype_resolver(our_csymtabs, our_er, src);
    ctypesys our_ctypesys(our_sp, services, our_ctype_resolver);
    ctypesys_local our_ctypesys_local(our_ctypesys, src, src_import_path);
#if _DUMP_LOG
    std::cerr << "-- first pass!\n";
#endif
    first_pass fp(dbg(), services, src_import_path, *ast.root, src, our_sp, our_er, our_csymtabs, our_ctypesys_local, our_ctype_resolver);
    ast.root->accept(fp);
#if 0
    std::cerr << ast.root->fmt_tree(src) << "\n";
    std::cerr << our_csymtabs.fmt(src) << "\n";
#endif
    if (!fp.good()) {
        return std::nullopt;
    }
#if _DUMP_LOG
    std::cerr << "-- second pass!\n";
#endif
    second_pass sp(dbg(), services, src_import_path, *ast.root, src, our_sp, our_er, our_csymtabs, our_ctypesys_local, our_ctype_resolver);
    ast.root->accept(sp);
    if (!sp.good()) {
        return std::nullopt;
    }
    // TODO: replace w/ proper compile_result gen later
    compile_result results{};
    results.add(src_import_path, std::move(sp.results.done()));
    return results;
}

