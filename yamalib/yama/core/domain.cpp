

#include "domain.h"

#include <taul/hashing.h>

#include "../internals/builtin_type_info.h"
#include "../internals/util.h"


using namespace yama::string_literals;


size_t yama::dep_mapping_name::hash() const noexcept {
    return taul::hash(install_name, dep_name);
}

std::string yama::dep_mapping_name::fmt() const {
    return std::format("{}/{}", install_name, dep_name);
}

yama::install_batch& yama::install_batch::install(str install_name, res<parcel> x) {
    installs.insert(std::pair{ install_name, std::move(x) });
    return *this;
}

yama::install_batch& yama::install_batch::map_dep(str install_name, str dep_name, str mapped_to) {
    dep_mappings[{ install_name, dep_name }] = mapped_to;
    return *this;
}

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

bool yama::domain::upload(type_info&& x) {
    return do_upload(std::forward<type_info>(x));
}

bool yama::domain::upload(std::span<const type_info> x) {
    return do_upload(x);
}

bool yama::domain::upload(const std::vector<type_info>& x) {
    return upload(std::span(x.begin(), x.end()));
}

bool yama::domain::upload(std::initializer_list<type_info> x) {
    return upload(std::span(x.begin(), x.end()));
}

bool yama::domain::upload(res<const module_info> x) {
    return do_upload(std::move(x));
}

bool yama::domain::upload(const taul::source_code& src) {
    if (auto result = do_compile(src)) {
        return upload(res(std::move(result)));
    }
    return false;
}

bool yama::domain::upload(const str& src) {
    taul::source_code s{};
    s.add_str("<src>"_str, src);
    return upload(s);
}

bool yama::domain::upload(const std::filesystem::path& src_path) {
    taul::source_code s{};
    if (!s.add_file(src_path)) {
        YAMA_RAISE(dbg(), dsignal::compile_file_not_found);
        YAMA_LOG(
            dbg(), compile_error_c,
            "error: file {} not found!",
            src_path.string());
        return false;
    }
    return upload(s);
}

void yama::domain::finish_setup() {
    _quick_access = do_preload_builtins();
}

yama::parcel_services yama::domain::create_parcel_services(const str& install_name) {
    return parcel_services(*this, install_name);
}

yama::compiler_services yama::domain::create_compiler_services(const str& parcel_env) {
    return compiler_services(*this, parcel_env);
}

yama::default_domain::default_domain(const domain_config& config, std::shared_ptr<debug> dbg)
    : domain(dbg),
    _other_dbg(proxy_dbg(dbg, ~instant_error_c)),
    _verif(config.verifier ? res(config.verifier) : (res<verifier>)make_res<default_verifier>(_other_dbg)),
    _compiler(config.compiler ? res(config.compiler) : (res<yama::compiler>)make_res<default_compiler>(_other_dbg)),
    _type_info_db(),
    _type_info_db_batch(_type_info_db),
    _type_db(),
    _type_db_batch(_type_db),
    _type_batch_db(),
    _instant(_type_info_db, _type_db, _type_batch_db, std::allocator<void>{}, _other_dbg),
    _instant_batch(_type_info_db_batch, _type_db_batch, _type_batch_db, std::allocator<void>{}, _other_dbg) {
    finish_setup();
}

yama::default_domain::default_domain(std::shared_ptr<debug> dbg)
    : default_domain(domain_config{}, dbg) {}

bool yama::default_domain::install(install_batch&& x) {
    install_batch temp(std::forward<install_batch>(x));
    return _try_install(temp);
}

size_t yama::default_domain::install_count() const noexcept {
    return _installs.size();
}

bool yama::default_domain::is_installed(const str& install_name) const noexcept {
    return _installs.contains(install_name);
}

std::optional<yama::module> yama::default_domain::import(const str& import_path) {
    const auto result = _import(import_path, std::nullopt);
    _commit_or_discard_batch((bool)result);
    return result;
}

std::optional<yama::type> yama::default_domain::load(const str& fullname) {
    if (const auto first_attempt = _type_db.pull(fullname)) {
        return type(**first_attempt);
    }
    if (const size_t number = _instant.instantiate(fullname); number == 0) {
        return std::nullopt;
    }
    if (const auto second_attempt = _type_db.pull(fullname)) {
        return type(**second_attempt);
    }
    return std::nullopt;
}

yama::domain::quick_access yama::default_domain::do_preload_builtins() {
    if (!upload(make_res<module_info>(internal::get_builtin_type_info()))) {
        YAMA_DEADEND;
        abort(); // for release builds
    }
    return quick_access{
        .none = load("None"_str).value(),
        .int0 = load("Int"_str).value(),
        .uint = load("UInt"_str).value(),
        .float0 = load("Float"_str).value(),
        .bool0 = load("Bool"_str).value(),
        .char0 = load("Char"_str).value(),
    };
}

std::shared_ptr<const yama::module_info> yama::default_domain::do_compile(const taul::source_code& src) {
    const auto result = _compiler->compile(create_compiler_services("TODO"_str), src);
    _commit_or_discard_batch((bool)result);
    return result;
}

bool yama::default_domain::do_upload(type_info&& x) {
    type_info temp(std::forward<type_info>(x));
    return
        _verify(temp)
        ? (_upload(std::move(temp)), true)
        : false;
}

bool yama::default_domain::do_upload(std::span<const type_info> x) {
    return
        _verify(x)
        ? (_upload(std::move(x)), true)
        : false;
}

bool yama::default_domain::do_upload(res<const module_info> x) {
    return
        _verify(*x)
        ? (_upload(std::move(x)), true)
        : false;
}

std::shared_ptr<const yama::module_info> yama::default_domain::do_ps_compile(const taul::source_code& src, const str& parcel_env) {
    const auto result = _compiler->compile(create_compiler_services(parcel_env), src);
    _commit_or_discard_batch((bool)result);
    return result;
}

std::optional<yama::module> yama::default_domain::do_cs_import(const str& import_path, const str& parcel_env) {
    return _import(import_path, parcel_env);
}

std::optional<yama::type> yama::default_domain::do_cs_load(const str& fullname, const str& parcel_env) {
    if (const auto first_attempt = _type_db_batch.pull(fullname)) {
        return type(**first_attempt);
    }
    if (const size_t number = _instant_batch.instantiate(fullname); number == 0) {
        return std::nullopt;
    }
    if (const auto second_attempt = _type_db_batch.pull(fullname)) {
        return type(**second_attempt);
    }
    return std::nullopt;
}

void yama::default_domain::_commit_batch() {
    // these commit calls will reset our proxy for us
    const bool result =
        _type_info_db_batch.commit() &&
        _type_db_batch.commit();
    _modules.merge(_modules_batch);
    YAMA_ASSERT(result);
}

void yama::default_domain::_discard_batch() {
    _type_info_db_batch.reset();
    _type_db_batch.reset();
    _modules_batch.clear();
}

void yama::default_domain::_commit_or_discard_batch(bool commit) {
    if (commit) _commit_batch();
    else        _discard_batch();
}

bool yama::default_domain::_try_install(install_batch& batch) {
    if (!_check_no_install_batch_errors(batch)) return false;
    _commit_batch_to_main(batch);
    return true;
}

bool yama::default_domain::_check_no_install_batch_errors(const install_batch& batch) {
    return
        _check_no_install_name_conflicts(batch) &&
        _check_no_missing_dep_mappings(batch) &&
        _check_no_invalid_dep_mappings(batch) &&
        _check_no_dep_graph_cycles(batch);
}

bool yama::default_domain::_check_no_install_name_conflicts(const install_batch& batch) {
    bool success = true;
    for (const auto& [install_name, parcel] : batch.installs) {
        if (!_install_name_refs_main(install_name)) continue;
        YAMA_RAISE(dbg(), dsignal::install_install_name_conflict);
        YAMA_LOG(
            dbg(), install_error_c,
            "error: install name {} already taken by installed parcel!",
            install_name);
        success = false;
    }
    return success;
}

bool yama::default_domain::_check_no_missing_dep_mappings(const install_batch& batch) {
    bool success = true;
    for (const auto& [install_name, parcel] : batch.installs) {
        for (const auto& [dep_name, m] : parcel->deps()) {
            dep_mapping_name dmn{ .install_name = install_name, .dep_name = dep_name };
            if (batch.dep_mappings.contains(dmn)) continue;
            YAMA_RAISE(dbg(), dsignal::install_missing_dep_mapping);
            YAMA_LOG(
                dbg(), install_error_c,
                "error: required dep mapping {} not found!",
                dmn);
            success = false;
        }
    }
    return success;
}

bool yama::default_domain::_check_no_invalid_dep_mappings(const install_batch& batch) {
    bool success = true;
    for (const auto& [dmn, mapped_to] : batch.dep_mappings) {
        if (!_install_name_refs_batch(dmn.install_name, batch)) {
            YAMA_RAISE(dbg(), dsignal::install_invalid_dep_mapping);
            YAMA_LOG(
                dbg(), install_error_c,
                "error: invalid dep mapping {}; {} not found in install batch!",
                dmn, dmn.install_name);
            success = false;
        }
        else { // putting below in else stmt as we NEED a parcel to use
            const auto& our_parcel = batch.installs.at(dmn.install_name);
            const auto& our_deps = our_parcel->deps();
            if (!our_deps.exists(dmn.dep_name)) {
                YAMA_RAISE(dbg(), dsignal::install_invalid_dep_mapping);
                YAMA_LOG(
                    dbg(), install_error_c,
                    "error: invalid dep mapping {}; {} has no dep named {}!",
                    dmn, dmn.install_name, dmn.dep_name);
                success = false;
            }
        }
        if (!_install_name_refs_batch_or_main(mapped_to, batch)) {
            YAMA_RAISE(dbg(), dsignal::install_invalid_dep_mapping);
            YAMA_LOG(
                dbg(), install_error_c,
                "error: invalid dep mapping {}; parcel {} mapped-to not found (in installed parcels or batch)!",
                dmn, mapped_to);
            success = false;
        }
    }
    return success;
}

bool yama::default_domain::_check_no_dep_graph_cycles(const install_batch& batch) {
    return _dep_graph_cycle_detect(batch) == 0;
}

bool yama::default_domain::_install_name_refs_main(str install_name) const noexcept {
    return _installs.contains(install_name);
}

bool yama::default_domain::_install_name_refs_batch(str install_name, const install_batch& batch) const noexcept {
    return batch.installs.contains(install_name);
}

bool yama::default_domain::_install_name_refs_batch_or_main(str install_name, const install_batch& batch) const noexcept {
    return
        _install_name_refs_main(install_name) ||
        _install_name_refs_batch(install_name, batch);
}

void yama::default_domain::_commit_batch_to_main(install_batch& batch) {
    _installs.merge(batch.installs);
    _dep_mappings.merge(batch.dep_mappings);
}

size_t yama::default_domain::_dep_graph_cycle_detect(const install_batch& batch) {
    _dep_graph_process_init(batch);
    size_t cycles = 0;
    while (!_unprocessed_nodes.empty()) {
        str selected = _dep_graph_select_arbitrary_unprocessed_node();
        // recursively process selected and any unprocessed nodes reachable from it,
        // counting the cycles detected by it
        cycles += _dep_graph_node_visit(selected, batch);
        _dep_graph_merge_visited_into_island();
    }
    return cycles;
}

size_t yama::default_domain::_dep_graph_node_visit(const str& install_name, const install_batch& batch) {
    // if cycle detected, return as further traversal will be infinite recursion
    if (_dep_graph_detect_and_report_cycle(install_name)) return 1;
    _dep_graph_move_unprocessed_to_visited(install_name);
    _dep_graph_node_stk_push(install_name);
    size_t cycles = _dep_graph_traverse_outgoing_edges(install_name, batch);
    _dep_graph_node_stk_pop();
    return cycles;
}

size_t yama::default_domain::_dep_graph_traverse_outgoing_edges(const str& install_name, const install_batch& batch) {
    size_t cycles = 0;
    for (const auto& [dmn, mapped_to] : batch.dep_mappings) {
        if (dmn.install_name != install_name) continue; // skip if dep mapping isn't for install_name
        if (_island_nodes.contains(mapped_to)) continue; // skip if island node
        if (_install_name_refs_main(mapped_to)) continue; // skip if already installed parcel
        cycles += _dep_graph_node_visit(mapped_to, batch);
    }
    return cycles;
}

void yama::default_domain::_dep_graph_process_init(const install_batch& batch) {
    _unprocessed_nodes.clear();
    _visited_nodes.clear();
    _island_nodes.clear();
    _node_stk.clear();
    // populate _unprocessed_nodes
    for (const auto& [install_name, parcel] : batch.installs) {
        _unprocessed_nodes.insert(install_name);
    }
}

yama::str yama::default_domain::_dep_graph_select_arbitrary_unprocessed_node() {
    YAMA_ASSERT(!_unprocessed_nodes.empty());
    return *_unprocessed_nodes.begin();
}

void yama::default_domain::_dep_graph_merge_visited_into_island() {
    _island_nodes.merge(_visited_nodes);
}

bool yama::default_domain::_dep_graph_detect_and_report_cycle(const str& install_name) {
    // iterate upwards through _node_stk, looking for an instance of install_name already
    // on the stack, and if detected, report the dep graph cycle found
    for (size_t i = 0; i < _node_stk.size(); i++) {
        if (_node_stk[i] != install_name) continue;
        // detected dep graph cycle, so report it and return true
        YAMA_RAISE(dbg(), dsignal::install_dep_graph_cycle);
        YAMA_LOG(dbg(), install_error_c, "error: dep graph cycle!");
        YAMA_LOG(dbg(), install_error_c, "logging cycle...");
        for (size_t j = i; j < _node_stk.size(); j++) {
            YAMA_LOG(dbg(), install_error_c, "    > {}", _node_stk[j]);
        }
        YAMA_LOG(dbg(), install_error_c, "    > {} (back ref)", install_name);
        return true;
    }
    return false;
}

void yama::default_domain::_dep_graph_move_unprocessed_to_visited(const str& install_name) {
    _unprocessed_nodes.erase(install_name);
    _visited_nodes.insert(install_name);
}

void yama::default_domain::_dep_graph_node_stk_push(const str& install_name) {
    _node_stk.push_back(install_name);
}

void yama::default_domain::_dep_graph_node_stk_pop() {
    _node_stk.pop_back();
}

void yama::default_domain::_assert_parcel_env_parcel_is_okay(const std::optional<str>& parcel) {
    if (!parcel) return;
    YAMA_ASSERT(_installs.contains(parcel.value()));
}

std::optional<yama::default_domain::_resolved_import_path_t> yama::default_domain::_resolve_import_path(const str& import_path, const std::optional<str>& parcel) {
    const auto [head, relative_path] = internal::split(import_path, '.');
    if (!parcel) { // domain env
        return _resolved_import_path_t{
            .import_path = import_path,
            .head = head,
            .relative_path = relative_path,
        };
    }
    else { // parcel env
        dep_mapping_name dmn{ .install_name = parcel.value(), .dep_name = head };
        const auto found = _dep_mappings.find(dmn);
        if (found == _dep_mappings.end()) {
            YAMA_RAISE(dbg(), dsignal::import_module_not_found);
            YAMA_LOG(
                dbg(), import_error_c,
                "error: import failed; dep mapping {} not found!",
                dmn);
            return std::nullopt;
        }
        const auto& resolved_head = found->second;
        return _resolved_import_path_t{
            .import_path = resolved_head + relative_path,
            .head = resolved_head,
            .relative_path = relative_path,
        };
    }
}

std::optional<yama::module> yama::default_domain::_query_already_memoized_parcel(const str& import_path) {
    const auto found = _modules.find(import_path);
    return
        found != _modules.end()
        ? std::make_optional(yama::module(found->second.get()))
        : std::nullopt;
}

std::shared_ptr<yama::parcel> yama::default_domain::_query_installed_parcel(const str& install_name) {
    const auto found = _installs.find(install_name);
    if (found != _installs.end()) return found->second;
    YAMA_RAISE(dbg(), dsignal::import_module_not_found);
    YAMA_LOG(
        dbg(), import_error_c,
        "error: import failed; parcel {} not found!",
        install_name);
    return nullptr;
}

std::optional<yama::module> yama::default_domain::_handle_fresh_import_and_memoize(const _resolved_import_path_t& rip, parcel& p) {
    const auto imported = p.import(create_parcel_services(rip.head), rip.relative_path);
    if (!imported) {
        YAMA_RAISE(dbg(), dsignal::import_module_not_found);
        YAMA_LOG(
            dbg(), import_error_c,
            "error: import failed; module {} not found!",
            rip.import_path);
        return std::nullopt;
    }
    _modules.insert({ rip.import_path, res(imported) }); // memoize
    return yama::module(imported.get());
}

std::optional<yama::module> yama::default_domain::_import(const str& import_path, std::optional<str> parcel) {
    _assert_parcel_env_parcel_is_okay(parcel);
    const auto resolved = _resolve_import_path(import_path, parcel); // translates from parcel to domain env
    if (!resolved) return std::nullopt;
    if (const auto memoized = _query_already_memoized_parcel(resolved->import_path)) return memoized;
    const auto our_parcel = _query_installed_parcel(resolved->head);
    if (!our_parcel) return std::nullopt;
    return _handle_fresh_import_and_memoize(*resolved, *our_parcel);
}

bool yama::default_domain::_verify(const type_info& x) {
    return _verif->verify(x);
}

bool yama::default_domain::_verify(std::span<const type_info> x) {
    for (const auto& I : x) {
        if (!_verif->verify(I)) return false;
    }
    return true;
}

bool yama::default_domain::_verify(const module_info& x) {
    return _verif->verify(x);
}

void yama::default_domain::_upload(type_info&& x) {
    _type_info_db.push(make_res<type_info>(std::forward<type_info>(x)));
}

void yama::default_domain::_upload(std::span<const type_info> x) {
    for (const auto& I : x) _upload(type_info(I));
}

void yama::default_domain::_upload(res<const module_info> x) {
    for (const auto& I : *x) _upload(type_info(I.second));
}

