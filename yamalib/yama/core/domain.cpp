

#include "domain.h"

#include "../internals/util.h"
#include "../internals/yama_parcel.h"


using namespace yama::string_literals;


yama::domain::domain(std::shared_ptr<debug> dbg)
    : api_component(dbg) {}

yama::type yama::domain::load_none() {
    return deref_assert(_quick_access).none;
}

yama::type yama::domain::load_int() {
    return deref_assert(_quick_access).int0;
}

yama::type yama::domain::load_uint() {
    return deref_assert(_quick_access).uint;
}

yama::type yama::domain::load_float() {
    return deref_assert(_quick_access).float0;
}

yama::type yama::domain::load_bool() {
    return deref_assert(_quick_access).bool0;
}

yama::type yama::domain::load_char() {
    return deref_assert(_quick_access).char0;
}

void yama::domain::finish_setup() {
    _quick_access = do_preload_builtins();
}

yama::default_domain::default_domain(domain_config config, std::shared_ptr<debug> dbg)
    : domain(dbg),
    _verif(dbg),
    _compiler(config.compiler ? res(config.compiler) : (res<compiler>)make_res<default_compiler>(dbg)),
    _state(),
    _install_manager(dbg),
    _importer(dbg, _install_manager, _verif, *_compiler),
    _loader(dbg, _state, _install_manager, _importer) {
    finish_setup();
}

yama::default_domain::default_domain(std::shared_ptr<debug> dbg)
    : default_domain(domain_config{}, dbg) {}

bool yama::default_domain::install(install_batch&& x) {
    return _install_manager.install(std::forward<install_batch>(x));
}

size_t yama::default_domain::install_count() const noexcept {
    return _install_manager.install_count();
}

bool yama::default_domain::is_installed(const str& install_name) const noexcept {
    return _install_manager.is_installed(install_name);
}

bool yama::default_domain::is_installed(parcel_id id) const noexcept {
    return _install_manager.is_installed(id);
}

std::shared_ptr<yama::parcel> yama::default_domain::get_installed(const str& install_name) const noexcept {
    return _install_manager.get_installed(install_name);
}

std::shared_ptr<yama::parcel> yama::default_domain::get_installed(parcel_id id) const noexcept {
    return _install_manager.get_installed(id);
}

std::optional<yama::module> yama::default_domain::import(const str& import_path) {
    return _importer.import(domain_env(), import_path, _state);
}

std::optional<yama::type> yama::default_domain::load(const str& fullname) {
    return _loader.load(fullname);
}

yama::env yama::default_domain::domain_env() {
    return _install_manager.domain_env();
}

std::optional<yama::env> yama::default_domain::parcel_env(const str& install_name) {
    return _install_manager.parcel_env(install_name);
}

yama::domain::quick_access yama::default_domain::do_preload_builtins() {
    _install_builtin_yama_parcel();
    return _prepare_quick_access_data();
}

void yama::default_domain::_install_builtin_yama_parcel() {
    YAMA_ASSERT(!is_installed("yama"_str));
    install_batch ib{};
    ib.install("yama"_str, make_res<internal::yama_parcel>());
    const bool success = install(std::move(ib));
    YAMA_ASSERT(success);
}

yama::domain::quick_access yama::default_domain::_prepare_quick_access_data() {
    return quick_access{
        .none = load("yama:None"_str).value(),
        .int0 = load("yama:Int"_str).value(),
        .uint = load("yama:UInt"_str).value(),
        .float0 = load("yama:Float"_str).value(),
        .bool0 = load("yama:Bool"_str).value(),
        .char0 = load("yama:Char"_str).value(),
    };
}

