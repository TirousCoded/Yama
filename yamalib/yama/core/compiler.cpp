

#include "compiler.h"

#include "domain.h"

#include "../internals/ast.h"
#include "../internals/first_pass.h"
#include "../internals/second_pass.h"


#define _DUMP_LOG 0


yama::compiler_services::compiler_services(std::shared_ptr<debug> dbg)
    : api_component(dbg) {}

void yama::compile_result::add(const import_path& path, module_info&& x) {
    results.insert({ path, std::forward<module_info>(x) });
}

yama::compiler::compiler(std::shared_ptr<debug> dbg)
    : api_component(dbg) {}

yama::default_compiler::default_compiler(std::shared_ptr<debug> dbg)
    : compiler(dbg) {}

std::optional<yama::compile_result> yama::default_compiler::compile(
    res<compiler_services> services,
    const taul::source_code& src,
    const import_path& src_import_path) {
    const auto ast = yama::internal::ast_parser().parse(src);
    if (const auto syntax_error = ast.syntax_error) {
        YAMA_RAISE(dbg(), dsignal::compile_syntax_error);
        YAMA_LOG(
            dbg(), compile_error_c,
            "error: {0} syntax error!",
            src.location_at(*syntax_error));
        return std::nullopt;
    }
    internal::csymtab_group csymtabs{};
    internal::csymtab_group_ctti csymtabs_ctti(services, src_import_path, *ast.root, csymtabs);
#if _DUMP_LOG
    std::cerr << "-- first pass!\n";
#endif
    internal::first_pass fp(dbg(), services, *ast.root, src, csymtabs_ctti);
    ast.root->accept(fp);
    if (!fp.good()) {
        return std::nullopt;
    }
#if _DUMP_LOG
    std::cerr << "-- second pass!\n";
#endif
    internal::second_pass sp(dbg(), services, src_import_path, *ast.root, src, csymtabs_ctti);
    ast.root->accept(sp);
    if (!sp.good()) {
        return std::nullopt;
    }
    // TODO: replace w/ proper compile_result gen later
    compile_result results{};
    results.add(src_import_path, std::move(sp.results.done()));
    return results;
}

