

#include "compiler.h"

#include "domain.h"

#include "../internals/ast.h"
#include "../internals/first_pass.h"
#include "../internals/second_pass.h"


yama::compiler::compiler(std::shared_ptr<debug> dbg)
    : api_component(dbg) {}

yama::default_compiler::default_compiler(std::shared_ptr<debug> dbg)
    : compiler(dbg),
    _services_ptr(nullptr) {}

std::shared_ptr<const yama::module_info> yama::default_compiler::compile(
    res<domain> dm,
    const taul::source_code& src) {
    _services_ptr = dm.get();
    const auto ast = yama::internal::ast_parser().parse(src);
    if (const auto syntax_error = ast.syntax_error) {
        YAMA_RAISE(dbg(), dsignal::compile_syntax_error);
        YAMA_LOG(
            dbg(), compile_error_c,
            "error: {0} syntax error!",
            src.location_at(*syntax_error));
        return nullptr;
    }
    internal::csymtab_group csymtabs{};
    internal::csymtab_group_ctti csymtabs_ctti(_services(), *ast.root, csymtabs);
    internal::first_pass fp(dbg(), _services(), *ast.root, src, csymtabs_ctti);
    ast.root->accept(fp);
    if (!fp.good()) {
        return nullptr;
    }
    internal::second_pass sp(dbg(), *ast.root, src, csymtabs_ctti);
    ast.root->accept(sp);
    if (!sp.good()) {
        return nullptr;
    }
    return std::make_shared<module_info>(std::move(sp.results.done()));
}

yama::domain& yama::default_compiler::_services() const noexcept {
    return deref_assert(_services_ptr);
}

