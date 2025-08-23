

#include "importer.h"

#include "util.h"
#include "domain_data.h"

#include "../internals/imported_module.h"


yama::internal::importer::importer(std::shared_ptr<debug> dbg, domain_data& dd)
    : api_component(dbg),
    _dd(dd) {}

std::optional<yama::module_ref> yama::internal::importer::import(const env& e, const str& path, res_state& upstream) {
    state.set_upstream(upstream.modules);
    const auto resolved_path = _resolve_import_path(e, path);
    if (!resolved_path) return std::nullopt;
    _last_was_success = _import(e, *resolved_path);
    return
        _last_was_success
        ? std::make_optional(create_module(deref_assert(state.pull(*resolved_path))))
        : std::nullopt;
}

void yama::internal::importer::commit_or_discard() {
    state.commit_or_discard(_last_was_success);
    mids.commit_or_discard(_last_was_success);
    _unlock(); // can't forget
}

void yama::internal::importer::commit_or_discard(std::shared_mutex& protects_upstream) {
    state.commit_or_discard(_last_was_success, protects_upstream);
    mids.commit_or_discard(_last_was_success);
    _unlock(); // can't forget
}

std::optional<yama::import_result> yama::internal::importer::import_for_import_dir(const import_path& path) {
    const env e = _dd->installs.domain_env();
    const auto parcel = _dd->installs.get_installed(path.head());
    if (!parcel) {
        YAMA_LOG(dbg(), import_c, "importing {}...", path.fmt(e)); // <- error arises before main log of this
        _report_module_not_found(path);
        return std::nullopt;
    }
    if (const auto result = state.pull(path)) {
        return import_result(res(result->m));
    }
    YAMA_LOG(dbg(), import_c, "importing {}...", path.fmt(e));
    auto imported = parcel->import(path.relative_path());
    if (!imported) {
        _report_module_not_found(path);
        return std::nullopt;
    }
    if (imported->holds_module()) {
        if (!_verify_and_memoize(imported->get_module(), mids.pull(), path)) {
            return std::nullopt;
        }
    }
    return *imported;
}

bool yama::internal::importer::upload_compiled_module(const import_path& path, res<module> new_module) {
    return _verify_and_memoize(new_module, mids.pull(), path);
}

void yama::internal::importer::_lock() {
    _dd->new_data_mtx.lock();
    YAMA_ASSERT(!_holds_lock); // top-level importer use shouldn't be recursive
    _holds_lock = true;
}

void yama::internal::importer::_unlock() {
    if (!_holds_lock) return;
    _holds_lock = false;
    _dd->new_data_mtx.unlock();
}

bool yama::internal::importer::_import(const env& e, const import_path& path) {
    return
        _check_already_imported(path)
        ? true
        : _handle_fresh_import_and_memoize(e, path, _dd->installs.get_installed(path.head()));
}

std::optional<yama::internal::import_path> yama::internal::importer::_resolve_import_path(const env& e, const str& path) {
    bool head_was_bad{};
    // TODO: be careful replacing import_path::parse w/ a cache, as the usage of this in importer::import
    //       will, when called in domain, only be protected by inclusive lock meant for READ ONLY STUFF
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
    _lock(); // <- lock for new data gen
    YAMA_LOG(dbg(), import_c, "importing {}...", path.fmt(e));
    if (auto imported = deref_assert(p).import(path.relative_path())) {
        if (imported->holds_module()) {
            if (!_verify_and_memoize(imported->get_module(), mids.pull(), path)) {
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

bool yama::internal::importer::_verify_and_memoize(const res<module>& m, mid_t mid, const import_path& path) {
    if (!_verify(*m, path)) return false;
    _memoize(m, mid, path);
    return true;
}

bool yama::internal::importer::_verify(const module& m, const import_path& path) {
    const auto e = _dd->installs.parcel_env(path.head()).value();
    const auto parcel = res(_dd->installs.get_installed(path.head()));
    if (_dd->verif.verify(m, parcel->metadata(), path.str(e))) return true;
    YAMA_RAISE(dbg(), dsignal::import_invalid_module);
    YAMA_LOG(
        dbg(), import_error_c,
        "error: module {} invalid!",
        path.fmt(e));
    return false;
}

void yama::internal::importer::_report_module_not_found(const import_path& path) {
    const auto e = _dd->installs.domain_env();
    YAMA_RAISE(dbg(), dsignal::import_module_not_found);
    YAMA_LOG(
        dbg(), import_error_c,
        "error: module {} not found!",
        path.fmt(e));
}

void yama::internal::importer::_memoize(const res<module>& m, mid_t mid, const import_path& path) {
    imported_module im{
        .m = m,
        .id = mid,
    };
    state.push(path, make_res<imported_module>(std::move(im)));
}

