

#include "install_manager.h"

#include "../core/res.h"


yama::internal::install_manager::install_manager(std::shared_ptr<debug> dbg)
    : api_component(dbg) {}

bool yama::internal::install_manager::install(install_batch&& x) {
    install_batch temp(std::forward<install_batch>(x));
    return _try_install(temp);
}

size_t yama::internal::install_manager::install_count() const noexcept {
    return domain_env().count();
}

bool yama::internal::install_manager::is_installed(const str& install_name) const noexcept {
    return domain_env().exists(install_name);
}

bool yama::internal::install_manager::is_installed(parcel_id id) const noexcept {
    return domain_env().exists(id);
}

std::shared_ptr<yama::parcel> yama::internal::install_manager::get_installed(const str& install_name) const noexcept {
    const auto id = domain_env().id(install_name);
    return id ? get_installed(*id) : nullptr;
}

std::shared_ptr<yama::parcel> yama::internal::install_manager::get_installed(parcel_id id) const noexcept {
    return is_installed(id) ? _get_install(id).prcl.base() : nullptr;
}

const yama::env& yama::internal::install_manager::domain_env() const noexcept {
    return _domain_env_instance.get();
}

std::optional<yama::env> yama::internal::install_manager::parcel_env(const str& install_name) const noexcept {
    const auto id = domain_env().id(install_name);
    return id ? parcel_env(*id) : std::nullopt;
}

std::optional<yama::env> yama::internal::install_manager::parcel_env(parcel_id id) const noexcept {
    return is_installed(id) ? std::make_optional(_get_install(id).e.get()) : std::nullopt;
}

yama::internal::install_manager::_install_t& yama::internal::install_manager::_get_install(parcel_id id) noexcept {
    YAMA_ASSERT(_installs.contains(id));
    return _installs.at(id);
}

const yama::internal::install_manager::_install_t& yama::internal::install_manager::_get_install(parcel_id id) const noexcept {
    YAMA_ASSERT(_installs.contains(id));
    return _installs.at(id);
}

bool yama::internal::install_manager::_try_install(install_batch& batch) {
    if (!_check_no_install_batch_errors(batch)) return false;
    _populate_domain_env(batch);
    _create_and_populate_parcel_envs(batch);
    return true;
}

bool yama::internal::install_manager::_check_no_install_batch_errors(const install_batch& batch) {
    return
        _check_no_invalid_parcel_errors(batch) &&
        _check_no_install_name_conflicts(batch) &&
        _check_no_missing_dep_mappings(batch) &&
        _check_no_invalid_dep_mappings(batch) &&
        _check_no_dep_graph_cycles(batch);
}

bool yama::internal::install_manager::_check_no_invalid_parcel_errors(const install_batch& batch) {
    bool success = true;
    for (const auto& [install_name, parcel] : batch.installs) {
        const auto our_self_name = parcel->metadata().self_name;
        if (parcel->metadata().is_dep_name(our_self_name)) {
            success = false;
            YAMA_RAISE(dbg(), dsignal::install_invalid_parcel);
            YAMA_LOG(
                dbg(), install_error_c,
                "error: invalid parcel {}; dep cannot use self-name {}!",
                install_name,
                our_self_name);
        }
    }
    return success;
}

bool yama::internal::install_manager::_check_no_install_name_conflicts(const install_batch& batch) {
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

bool yama::internal::install_manager::_check_no_missing_dep_mappings(const install_batch& batch) {
    bool success = true;
    for (const auto& [install_name, parcel] : batch.installs) {
        for (const auto& dep_name : parcel->metadata().dep_names) {
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

bool yama::internal::install_manager::_check_no_invalid_dep_mappings(const install_batch& batch) {
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
            if (!our_parcel->metadata().is_dep_name(dmn.dep_name)) {
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

bool yama::internal::install_manager::_check_no_dep_graph_cycles(const install_batch& batch) {
    return _dep_graph_cycle_detect(batch) == 0;
}

bool yama::internal::install_manager::_install_name_refs_main(str install_name) const noexcept {
    return is_installed(install_name);
}

bool yama::internal::install_manager::_install_name_refs_batch(str install_name, const install_batch& batch) const noexcept {
    return batch.installs.contains(install_name);
}

bool yama::internal::install_manager::_install_name_refs_batch_or_main(str install_name, const install_batch& batch) const noexcept {
    return
        _install_name_refs_main(install_name) ||
        _install_name_refs_batch(install_name, batch);
}

void yama::internal::install_manager::_populate_domain_env(install_batch& batch) {
    // hereafter, we can consider these parcels *installed* regarding access via install_manager methods
    for (const auto& [install_name, parcel] : batch.installs) {
        _installs.insert({ parcel->id(), _install_t{ .prcl = parcel, .e = env_instance{} } });
        _domain_env_instance.add(install_name, parcel->id());
    }
}

void yama::internal::install_manager::_create_and_populate_parcel_envs(install_batch& batch) {
    for (const auto& [install_name, parcel] : batch.installs) {
        auto& our_parcel_env = _get_install(parcel->id()).e;
        YAMA_ASSERT(!our_parcel_env.get().exists(install_name));
        // self-name mapping
        our_parcel_env.add(parcel->metadata().self_name, parcel->id());
        // dep mappings
        for (const auto& dep_name : parcel->metadata().dep_names) {
            dep_mapping_name dmn{ .install_name = install_name, .dep_name = dep_name };
            const auto& name_in_domain_env = batch.dep_mappings.at(dmn);
            our_parcel_env.add(dep_name, get_installed(name_in_domain_env)->id());
        }
    }
}

size_t yama::internal::install_manager::_dep_graph_cycle_detect(const install_batch& batch) {
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

size_t yama::internal::install_manager::_dep_graph_node_visit(const str& install_name, const install_batch& batch) {
    // if cycle detected, return as further traversal will be infinite recursion
    if (_dep_graph_detect_and_report_cycle(install_name)) return 1;
    _dep_graph_move_unprocessed_to_visited(install_name);
    _dep_graph_node_stk_push(install_name);
    size_t cycles = _dep_graph_traverse_outgoing_edges(install_name, batch);
    _dep_graph_node_stk_pop();
    return cycles;
}

size_t yama::internal::install_manager::_dep_graph_traverse_outgoing_edges(const str& install_name, const install_batch& batch) {
    size_t cycles = 0;
    for (const auto& [dmn, mapped_to] : batch.dep_mappings) {
        if (dmn.install_name != install_name) continue; // skip if dep mapping isn't for install_name
        if (_island_nodes.contains(mapped_to)) continue; // skip if island node
        if (_install_name_refs_main(mapped_to)) continue; // skip if already installed parcel
        cycles += _dep_graph_node_visit(mapped_to, batch);
    }
    return cycles;
}

void yama::internal::install_manager::_dep_graph_process_init(const install_batch& batch) {
    _unprocessed_nodes.clear();
    _visited_nodes.clear();
    _island_nodes.clear();
    _node_stk.clear();
    // populate _unprocessed_nodes
    for (const auto& [install_name, parcel] : batch.installs) {
        _unprocessed_nodes.insert(install_name);
    }
}

yama::str yama::internal::install_manager::_dep_graph_select_arbitrary_unprocessed_node() {
    YAMA_ASSERT(!_unprocessed_nodes.empty());
    return *_unprocessed_nodes.begin();
}

void yama::internal::install_manager::_dep_graph_merge_visited_into_island() {
    _island_nodes.merge(_visited_nodes);
}

bool yama::internal::install_manager::_dep_graph_detect_and_report_cycle(const str& install_name) {
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

void yama::internal::install_manager::_dep_graph_move_unprocessed_to_visited(const str& install_name) {
    _unprocessed_nodes.erase(install_name);
    _visited_nodes.insert(install_name);
}

void yama::internal::install_manager::_dep_graph_node_stk_push(const str& install_name) {
    _node_stk.push_back(install_name);
}

void yama::internal::install_manager::_dep_graph_node_stk_pop() {
    _node_stk.pop_back();
}

