

#pragma once


#include <unordered_set>

#include "../core/api_component.h"
#include "../core/general.h"
#include "../core/install_batch.h"

#include "../internals/env.h"


namespace yama::internal {


    class install_manager final : public api_component {
    public:
        install_manager(std::shared_ptr<debug> dbg);
        virtual ~install_manager() noexcept = default;


        bool install(install_batch&& x);
        size_t install_count() const noexcept;
        bool is_installed(const str& install_name) const noexcept;
        bool is_installed(parcel_id id) const noexcept;
        std::shared_ptr<parcel> get_installed(const str& install_name) const noexcept;
        std::shared_ptr<parcel> get_installed(parcel_id id) const noexcept;

        const env& domain_env() const noexcept;
        std::optional<env> parcel_env(const str& install_name) const noexcept;
        std::optional<env> parcel_env(parcel_id id) const noexcept;


    private:
        struct _install_t final {
            res<parcel> prcl;
            env_instance e; // these envs include mappings for self-names
        };

        std::unordered_map<parcel_id, _install_t> _installs;
        env_instance _domain_env_instance;

        _install_t& _get_install(parcel_id id) noexcept;
        const _install_t& _get_install(parcel_id id) const noexcept;


        // below are used to impl _check_no_dep_graph_cycles

        // we use a depth-first approach to detect dep graph cycles, w/ this approach having some repeat
        // graph node processing in the name of presenting end-user w/ a nice list of all of them

        // our method is inspired by *topological sorting* (https://en.wikipedia.org/wiki/Topological_sorting#)

        // _unprocessed_nodes starts off populated by ALL dep graph nodes, w/ us repeatedly selecting
        // one, then *processing it*, which traverses its graph and looks for cycles, and then it and
        // all nodes also in _unprocessed_nodes visited during traversal are moved to _island_nodes,
        // repeating this process until _unprocessed_nodes is empty

        std::unordered_set<str> _unprocessed_nodes;

        // _visited_nodes temporarily records nodes visited during a round a processing, so that they
        // can be added to _island_nodes afterwards

        // can't add these nodes to _island_nodes immediately as we can't be 100% sure they can't form
        // any (more) cycles until we've 100% traversed their region of the graph

        std::unordered_set<str> _visited_nodes;

        // given some node, and given the nodes reachable from it during its processing, after we're
        // done processing it, we 100% know that it, and nodes reachable from it, have NO OTHER OUTGOING
        // CONNECTIONS TO OTHER NODES

        // to this end, _island_nodes is used to let us *skip* certain outgoing traversal paths during
        // future processing as we 100% know that nodes in _island_nodes won't refer to nodes involved
        // in said future processing, and so can't form cycles w/ them

        std::unordered_set<str> _island_nodes;

        // upon start of a visit to a node, we push its name to _node_stk, popping it upon end of visit,
        // w/ this forming a kind of *scope stack* for our nodes during processing

        // prior to pushing, if _node_stk already contains an entry w/ the name of the visited node,
        // then we've detected a back reference, and we can use this info to get a really nice list
        // of node names specifying the nature of the dep graph cycle

        std::vector<str> _node_stk;


        bool _try_install(install_batch& batch);
        bool _check_no_install_batch_errors(const install_batch& batch);
        bool _check_no_invalid_parcel_errors(const install_batch& batch);
        bool _check_no_install_name_conflicts(const install_batch& batch);
        bool _check_no_missing_dep_mappings(const install_batch& batch);
        bool _check_no_invalid_dep_mappings(const install_batch& batch);
        bool _check_no_dep_graph_cycles(const install_batch& batch);
        bool _install_name_refs_main(str install_name) const noexcept;
        bool _install_name_refs_batch(str install_name, const install_batch& batch) const noexcept;
        bool _install_name_refs_batch_or_main(str install_name, const install_batch& batch) const noexcept;
        void _populate_domain_env(install_batch& batch);
        void _create_and_populate_parcel_envs(install_batch& batch);

        size_t _dep_graph_cycle_detect(const install_batch& batch);
        size_t _dep_graph_node_visit(const str& install_name, const install_batch& batch);
        size_t _dep_graph_traverse_outgoing_edges(const str& install_name, const install_batch& batch);
        void _dep_graph_process_init(const install_batch& batch);
        str _dep_graph_select_arbitrary_unprocessed_node();
        void _dep_graph_merge_visited_into_island();
        bool _dep_graph_detect_and_report_cycle(const str& install_name);
        void _dep_graph_move_unprocessed_to_visited(const str& install_name);
        void _dep_graph_node_stk_push(const str& install_name);
        void _dep_graph_node_stk_pop();
    };
}

