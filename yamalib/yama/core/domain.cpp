

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

std::optional<yama::module_ref> yama::domain::import(const str& import_path) {
    std::shared_lock lk(_dd.state_mtx);
    auto result = _dd.importer.import(_dd.installs.domain_env(), import_path, _dd.state);
    lk.unlock(); // release before commit_or_discard
    _dd.importer.commit_or_discard(_dd.state_mtx); // can't forget
    return result;
}

std::optional<yama::item_ref> yama::domain::load(const str& fullname) {
    std::shared_lock lk(_dd.state_mtx);
    auto result = _dd.loader.load(fullname);
    lk.unlock(); // release before commit_or_discard
    _dd.loader.commit_or_discard(_dd.state_mtx); // can't forget
    return result;
}

yama::item_ref yama::domain::none_type() {
    return deref_assert(_dd.quick_access).none;
}

yama::item_ref yama::domain::int_type() {
    return deref_assert(_dd.quick_access).int0;
}

yama::item_ref yama::domain::uint_type() {
    return deref_assert(_dd.quick_access).uint;
}

yama::item_ref yama::domain::float_type() {
    return deref_assert(_dd.quick_access).float0;
}

yama::item_ref yama::domain::bool_type() {
    return deref_assert(_dd.quick_access).bool0;
}

yama::item_ref yama::domain::char_type() {
    return deref_assert(_dd.quick_access).char0;
}

yama::item_ref yama::domain::type_type() {
    return deref_assert(_dd.quick_access).type;
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
        .type = load("yama:Type"_str).value(),
    };
}

