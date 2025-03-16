

#include "importer.h"

#include "../core/domain.h"

#include "util.h"


yama::internal::importer::importer(std::shared_ptr<debug> dbg, install_manager& install_manager, verifier& verifier, compiler& compiler)
    : api_component(dbg),
    _install_manager(&install_manager),
    _verifier(&verifier),
    _compiler(&compiler) {}

std::optional<yama::module> yama::internal::importer::import(const env& e, const str& path, res_state& upstream) {
    _state.set_upstream(upstream.modules);
    const auto resolved_path = _resolve_import_path(e, path);
    if (!resolved_path) return std::nullopt;
    const bool success = _import(e, *resolved_path);
    _state.commit_or_discard(success);
    return
        success
        ? std::make_optional(create_module(_state.pull(*resolved_path).get()))
        : std::nullopt;
}

yama::internal::install_manager& yama::internal::importer::_get_install_manager() const noexcept {
    return deref_assert(_install_manager);
}

yama::verifier& yama::internal::importer::_get_verifier() const noexcept {
    return deref_assert(_verifier);
}

yama::compiler& yama::internal::importer::_get_compiler() const noexcept {
    return deref_assert(_compiler);
}

bool yama::internal::importer::_import(const env& e, const import_path& path) {
    return
        _check_already_imported(path)
        ? true
        : _handle_fresh_import_and_memoize(e, path, _get_install_manager().get_installed(path.head()));
}

std::optional<yama::import_path> yama::internal::importer::_resolve_import_path(const env& e, const str& path) {
    bool head_was_bad{};
    const auto result = import_path::parse(e, path, head_was_bad);
    if (!result) {
        YAMA_RAISE(dbg(), dsignal::import_module_not_found);
        if (!head_was_bad) {
            YAMA_LOG(dbg(), import_c, "importing {}...", path); // <- error arises before main log of this
            YAMA_LOG(
                dbg(), import_error_c,
                "error: import {} failed; syntax error!",
                path);
        }
        else {
            YAMA_LOG(dbg(), import_c, "importing {}...", path); // <- error arises before main log of this
            YAMA_LOG(
                dbg(), import_error_c,
                "error: import {} failed; head specifies parcel not in env!",
                path);
        }
    }
    return result;
}

bool yama::internal::importer::_check_already_imported(const import_path& path) {
    return _state.exists(path);
}

bool yama::internal::importer::_handle_fresh_import_and_memoize(const env& e, const import_path& path, std::shared_ptr<parcel> p) {
    if (!p) return false; // fail quietly if outside arg query failed
    YAMA_LOG(dbg(), import_c, "importing {}...", path.fmt(e));
    if (auto imported = deref_assert(p).import(path.relative_path())) {
        if (imported->holds_module()) {
            if (!_verify(*imported->get_module(), path, deref_assert(p).id())) return false;
            _state.push(path, imported->get_module()); // memoize
        }
        else if (imported->holds_source()) {
            if (auto compiled = _get_compiler().compile(_acquire_cs(deref_assert(p).id()), imported->get_source(), path)) {
                for (auto& [key, value] : compiled->results) {
                    if (!_verify(value, key, deref_assert(p).id())) return false;
                    _state.push(key, make_res<module_info>(std::move(value))); // memoize
                }
            }
            else return false; // compilation fail
        }
        else YAMA_DEADEND;
    }
    else {
        _report_module_not_found(path);
        return false;
    }
    return true; // if we got this far, return successful
}

bool yama::internal::importer::_verify(const module_info& m, const import_path& path, parcel_id id) {
    return _do_verify(dbg(), _get_install_manager().parcel_env(id).value(), _get_verifier(), m, path, _get_install_manager().get_installed(id));
}

void yama::internal::importer::_report_module_not_found(const import_path& path) {
    _do_report_module_not_found(dbg(), _get_install_manager().domain_env(), path);
}

bool yama::internal::importer::_do_verify(const std::shared_ptr<debug>& dbg, const env& e, verifier& verifier, const module_info& m, const import_path& path, std::shared_ptr<parcel> p) {
    if (verifier.verify(m, deref_assert(p).metadata(), path.str(e))) return true;
    YAMA_RAISE(dbg, dsignal::import_invalid_module);
    YAMA_LOG(
        dbg, import_error_c,
        "error: module {} invalid!",
        path.fmt(e));
    return false;
}

void yama::internal::importer::_do_report_module_not_found(const std::shared_ptr<debug>& dbg, const env& e, const import_path& path) {
    YAMA_RAISE(dbg, dsignal::import_module_not_found);
    YAMA_LOG(
        dbg, import_error_c,
        "error: module {} not found!",
        path.fmt(e));
}

yama::res<yama::compiler_services> yama::internal::importer::_acquire_cs(parcel_id compilation_env_parcel_id) {
    return make_res<_compiler_services>(dbg(), compilation_env_parcel_id, *this);
}

yama::internal::importer::_compiler_services::_compiler_services(std::shared_ptr<debug> dbg, parcel_id compilation_env_parcel_id, importer& upstream)
    : compiler_services(dbg),
    _upstream(&upstream),
    _env(upstream._get_install_manager().parcel_env(compilation_env_parcel_id).value()) {}

yama::env yama::internal::importer::_compiler_services::env() const {
    return _env;
}

std::optional<yama::import_result_ext> yama::internal::importer::_compiler_services::import(const yama::import_path& path) {
    auto& upstream = deref_assert(_upstream);
    const auto parcel = upstream._get_install_manager().get_installed(path.head());
    if (!parcel) {
        YAMA_LOG(dbg(), import_c, "importing {}...", path.fmt(env())); // <- error arises before main log of this
        _report_module_not_found(path);
        return std::nullopt;
    }
    const yama::env result_e = upstream._get_install_manager().parcel_env(parcel->id()).value();
    if (const auto result = upstream._state.pull(path)) {
        return import_result_ext{
            .result = res(result),
            .e      = result_e,
        };
    }
    YAMA_LOG(dbg(), import_c, "importing {}...", path.fmt(env()));
    auto imported = deref_assert(parcel).import(path.relative_path());
    if (!imported) {
        _report_module_not_found(path);
        return std::nullopt;
    }
    if (imported->holds_module()) {
        if (!_verify(*imported->get_module(), path, parcel)) return std::nullopt;
        upstream._state.push(path, imported->get_module());
    }
    return import_result_ext{
        .result = std::move(*imported),
        .e      = result_e,
    };
}

bool yama::internal::importer::_compiler_services::_verify(const module_info& m, const import_path& path, std::shared_ptr<parcel> p) {
    return _do_verify(dbg(), env(), deref_assert(_upstream)._get_verifier(), m, path, p);
}

void yama::internal::importer::_compiler_services::_report_module_not_found(const import_path& path) {
    _do_report_module_not_found(dbg(), env(), path);
}

