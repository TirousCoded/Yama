

#include "domain.h"

#include "../internals/util.h"
#include "../internals/yama_parcel.h"


using namespace yama::string_literals;


yama::domain::domain(std::shared_ptr<debug> dbg)
    : api_component(dbg),
    _dd(dbg) {
    _finish_setup();
}

bool yama::domain::install(install_batch&& x) {
    std::scoped_lock lk(_dd.new_data_mtx, _dd.state_mtx);
    // TODO: maybe change frontend install method to take lvalue?
    return _dd.installer.install(install_batch(std::forward<install_batch>(x)));
}

size_t yama::domain::install_count() const noexcept {
    std::shared_lock lk(_dd.state_mtx);
    return _dd.installs.install_count();
}

bool yama::domain::is_installed(const str& install_name) const noexcept {
    std::shared_lock lk(_dd.state_mtx);
    return _dd.installs.is_installed(install_name);
}

bool yama::domain::is_installed(parcel_id id) const noexcept {
    std::shared_lock lk(_dd.state_mtx);
    return _dd.installs.is_installed(id);
}

std::shared_ptr<yama::parcel> yama::domain::get_installed(const str& install_name) const noexcept {
    std::shared_lock lk(_dd.state_mtx);
    return _dd.installs.get_installed(install_name);
}

std::shared_ptr<yama::parcel> yama::domain::get_installed(parcel_id id) const noexcept {
    std::shared_lock lk(_dd.state_mtx);
    return _dd.installs.get_installed(id);
}

std::optional<yama::module> yama::domain::import(const str& import_path) {
    std::optional<module> result{};
    {
        std::shared_lock lk(_dd.state_mtx); // <- releases before commit_or_discard
        result = _dd.importer.import(_dd.installs.domain_env(), import_path, _dd.state);
    }
    _dd.importer.commit_or_discard(_dd.state_mtx); // can't forget
    return result;
}

std::optional<yama::type> yama::domain::load(const str& fullname) {
    std::optional<type> result{};
    {
        std::shared_lock lk(_dd.state_mtx); // <- releases before commit_or_discard
        result = _dd.loader.load(fullname);
    }
    _dd.loader.commit_or_discard(_dd.state_mtx); // can't forget
    return result;
}

yama::type yama::domain::load_none() {
    return deref_assert(_dd.quick_access).none;
}

yama::type yama::domain::load_int() {
    return deref_assert(_dd.quick_access).int0;
}

yama::type yama::domain::load_uint() {
    return deref_assert(_dd.quick_access).uint;
}

yama::type yama::domain::load_float() {
    return deref_assert(_dd.quick_access).float0;
}

yama::type yama::domain::load_bool() {
    return deref_assert(_dd.quick_access).bool0;
}

yama::type yama::domain::load_char() {
    return deref_assert(_dd.quick_access).char0;
}

void yama::domain::_finish_setup() {
    _install_builtin_yama_parcel();
    _preload_builtin_types();
}

void yama::domain::_install_builtin_yama_parcel() {
    YAMA_ASSERT(!is_installed("yama"_str));
    install_batch ib{};
    ib.install("yama"_str, make_res<internal::yama_parcel>());
    const bool success = install(std::move(ib));
    YAMA_ASSERT(success);
}

void yama::domain::_preload_builtin_types() {
    _dd.quick_access = internal::quick_access{
        .none = load("yama:None"_str).value(),
        .int0 = load("yama:Int"_str).value(),
        .uint = load("yama:UInt"_str).value(),
        .float0 = load("yama:Float"_str).value(),
        .bool0 = load("yama:Bool"_str).value(),
        .char0 = load("yama:Char"_str).value(),
    };
}

