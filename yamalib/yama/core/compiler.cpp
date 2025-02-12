

#include "compiler.h"

#include "domain.h"

#include "../internals/ast.h"
#include "../internals/first_pass.h"
#include "../internals/second_pass.h"


std::optional<yama::module> yama::compiler_services::import(const str & import_path) {
    return deref_assert(_client).do_cs_import(import_path, _parcel_env);
}

std::optional<yama::type> yama::compiler_services::load(const str& fullname) {
    return deref_assert(_client).do_cs_load(fullname, _parcel_env);
}

yama::type yama::compiler_services::load_none() {
    return deref_assert(_client).load_none();
}

yama::type yama::compiler_services::load_int() {
    return deref_assert(_client).load_int();
}

yama::type yama::compiler_services::load_uint() {
    return deref_assert(_client).load_uint();
}

yama::type yama::compiler_services::load_float() {
    return deref_assert(_client).load_float();
}

yama::type yama::compiler_services::load_bool() {
    return deref_assert(_client).load_bool();
}

yama::type yama::compiler_services::load_char() {
    return deref_assert(_client).load_char();
}

yama::compiler_services::compiler_services(domain& client, const str& parcel_env)
    : _client(&client),
    _parcel_env(parcel_env) {}

yama::compiler::compiler(std::shared_ptr<debug> dbg)
    : api_component(dbg) {}

yama::default_compiler::default_compiler(std::shared_ptr<debug> dbg)
    : compiler(dbg) {}

std::shared_ptr<const yama::module_info> yama::default_compiler::compile(
    compiler_services services,
    const taul::source_code& src) {
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
    internal::csymtab_group_ctti csymtabs_ctti(services, *ast.root, csymtabs);
    internal::first_pass fp(dbg(), services, *ast.root, src, csymtabs_ctti);
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

