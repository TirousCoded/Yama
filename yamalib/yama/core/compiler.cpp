

#include "compiler.h"

#include "domain.h"

#include "../internals/ast.h"
#include "../internals/first_pass.h"
#include "../internals/second_pass.h"


yama::compiler::compiler(std::shared_ptr<debug> dbg)
    : api_component(dbg) {}

yama::default_compiler::default_compiler(std::shared_ptr<debug> dbg)
    : compiler(dbg),
    _dm_ptr(nullptr) {}

std::optional<std::vector<yama::type_info>> yama::default_compiler::compile(
    res<domain> dm,
    const taul::source_code& src) {
    _dm_ptr = dm.get();
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

yama::domain& yama::default_compiler::_dm() const noexcept {
    return deref_assert(_dm_ptr);
}

