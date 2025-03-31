

#include "importer.h"

#include "util.h"
#include "domain_data.h"


yama::internal::importer::importer(std::shared_ptr<debug> dbg, domain_data& dd)
    : api_component(dbg),
    _dd(dd) {}

std::optional<yama::module> yama::internal::importer::import(const env& e, const str& path, res_state& upstream) {
    state.set_upstream(upstream.modules);
    const auto resolved_path = _resolve_import_path(e, path);
    if (!resolved_path) return std::nullopt;
    const bool success = _import(e, *resolved_path);
    state.commit_or_discard(success);
    return
        success
        ? std::make_optional(create_module(state.pull(*resolved_path).get()))
        : std::nullopt;
}

std::optional<yama::import_result> yama::internal::importer::import_for_import_dir(const import_path& path) {
    const env e = _dd->install_manager.domain_env();
    const auto parcel = _dd->install_manager.get_installed(path.head());
    if (!parcel) {
        YAMA_LOG(dbg(), import_c, "importing {}...", path.fmt(e)); // <- error arises before main log of this
        _report_module_not_found(path);
        return std::nullopt;
    }
    if (const auto result = state.pull(path)) {
        return import_result(res(result));
    }
    YAMA_LOG(dbg(), import_c, "importing {}...", path.fmt(e));
    auto imported = parcel->import(path.relative_path());
    if (!imported) {
        _report_module_not_found(path);
        return std::nullopt;
    }
    if (imported->holds_module()) {
        if (!_verify_and_memoize(imported->get_module(), path)) {
            return std::nullopt;
        }
    }
    return *imported;
}

bool yama::internal::importer::upload_compiled_module(const import_path& path, res<module_info> new_module) {
    return _verify_and_memoize(new_module, path);
}

bool yama::internal::importer::_import(const env& e, const import_path& path) {
    return
        _check_already_imported(path)
        ? true
        : _handle_fresh_import_and_memoize(e, path, _dd->install_manager.get_installed(path.head()));
}

std::optional<yama::internal::import_path> yama::internal::importer::_resolve_import_path(const env& e, const str& path) {
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
    return state.exists(path);
}

bool yama::internal::importer::_handle_fresh_import_and_memoize(const env& e, const import_path& path, std::shared_ptr<parcel> p) {
    if (!p) return false; // fail quietly if outside arg query failed
    YAMA_LOG(dbg(), import_c, "importing {}...", path.fmt(e));
    if (auto imported = deref_assert(p).import(path.relative_path())) {
        if (imported->holds_module()) {
            if (!_verify_and_memoize(imported->get_module(), path)) {
                return false;
            }
        }
        else if (imported->holds_source()) {
            if (!_dd->compiler.compile(imported->get_source(), path)) {
                return false;
            }
        }
        else YAMA_DEADEND;
    }
    else {
        _report_module_not_found(path);
        return false;
    }
    return true; // if we got this far, return successful
}

bool yama::internal::importer::_verify_and_memoize(const res<module_info>& m, const import_path& path) {
    if (!_verify(*m, path)) return false;
    state.push(path, m); // memoize
    return true;
}

bool yama::internal::importer::_verify(const module_info& m, const import_path& path) {
    return _do_verify(dbg(), _dd->install_manager.parcel_env(path.head()).value(), _dd->verif, m, path, _dd->install_manager.get_installed(path.head()));
}

void yama::internal::importer::_report_module_not_found(const import_path& path) {
    _do_report_module_not_found(dbg(), _dd->install_manager.domain_env(), path);
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

