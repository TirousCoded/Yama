

#include "compiler.h"

#include "../internals/ast.h"
#include "../internals/first_pass.h"
#include "../internals/second_pass.h"


yama::dm::compiler::compiler(domain& dm, std::shared_ptr<debug> dbg)
    : api_component(dbg),
    _dm_ptr(&dm) {}

std::optional<std::vector<yama::type_info>> yama::dm::compiler::compile(const taul::source_code& src) {
    const auto ast = yama::internal::ast_parser().parse(src);
    if (const auto syntax_error = ast.syntax_error) {
        YAMA_RAISE(dbg(), dsignal::compile_syntax_error);
        YAMA_LOG(
            dbg(), compile_c,
            "error: {0} syntax error!",
            src.location_at(*syntax_error));
        return std::nullopt;
    }
    internal::csymtab_group csymtabs{};
    internal::csymtab_group_ctti csymtabs_ctti(_dm(), *ast.root, csymtabs);
    internal::first_pass fp(dbg(), _dm(), *ast.root, src, csymtabs_ctti);
    ast.root->accept(fp);
    if (!fp.good()) {
        return std::nullopt;
    }
    internal::second_pass sp(dbg(), *ast.root, src, csymtabs_ctti);
    ast.root->accept(sp);
    if (!sp.good()) {
        return std::nullopt;
    }
    return sp.results;
}

