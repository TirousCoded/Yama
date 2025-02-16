

#pragma once


#include <optional>
#include <unordered_set>

#include <taul/source_code.h>

#include "res.h"
#include "api_component.h"
// NOTE: this is so things like yama::primitive_info are available to end-user
//       w/out forcing them to include type_info.h first
#include "type_info.h"
#include "type.h"
#include "module_info.h"
#include "module.h"
#include "parcel.h"
#include "verifier.h"
#include "compiler.h"

#include "../internals/type_instance.h"
#include "../internals/res_db.h"
#include "../internals/type_instantiator.h"


namespace yama {


    // TODO: later on, a feature that might be worth looking into is maybe incorporating
    //       something like ARC to dealloc instantiated types when there's no demand for
    //       the type in question

    // TODO: our unit tests don't cover the detail of how, when compiling, the domain
    //       passes a proxy to the compiler, and the contents loaded into this proxy only
    //       get *committed* to the domain's main loaded type data store if compilation was
    //       successful, being discarded otherwise
    //
    //       I'm really not sure how this behaviour should be unit tested


    // IMPORTANT:
    //      domains operate by the end-user 'uploading' type_info to it, and then the
    //      domain lazy loading (here called 'instantiating') actual types (accessed
    //      via type objects) on demand by the end-user
    //
    //      the memory underlying type objects is guaranteed to be valid for the
    //      lifetime of the domain, and to be entirely thread-safe, due said type
    //      info memory being immutable (take note that this doesn't mean that domain
    //      impl's methods are guaranteed to be thread-safe)


    // IMPORTANT: import paths are period-seperated lists of identifiers specifying
    //            where a module is found, and what it's called, w/ each module having
    //            a unique import path
    //
    //            the first identifier in this list is called the 'head', and specifies
    //            the parcel from which the module is imported
    //
    //            the meaning of the head identifier depends upon the 'environment', or
    //            'env', in which importing is occurring
    // 
    //            two types of environments exist:
    //                  1) 'domain envs' identify parcels by their install names
    //                  2) 'parcel envs' identify parcels by their dep names (in the
    //                     context of a particular parcel and its deps)
    //
    //            removing the head identifier from the import path yields the 'relative
    //            path' of the import path, which is used within parcels to specify
    //            modules w/out exposing info about said parcel existing within the
    //            context of a broader environment of parcels
    // 
    //            this lack of broader context is important as parcels are not supposed
    //            to be coupled to domains
    // 
    //            a relative path which is an empty string refers to the 'root' module
    //            of a parcel, w/ root modules having the notable characteristic that
    //            import paths to them only contain a head identifier
    //
    //            given an import path 'std.io.net.http', 'std' is the head, and
    //            '.io.net.http' is the relative path


    struct dep_mapping_name;
    struct install_batch;

    class domain;
    class default_domain;


    struct dep_mapping_name final {
        str install_name, dep_name;


        bool operator==(const dep_mapping_name&) const noexcept = default;

        size_t hash() const noexcept;
        std::string fmt() const;
    };
}

YAMA_SETUP_HASH(yama::dep_mapping_name, x.hash());
YAMA_SETUP_FORMAT(yama::dep_mapping_name, x.fmt());

namespace yama {


    struct install_batch final {
        // installs maps install names to parcels to install

        std::unordered_map<str, res<parcel>> installs;

        // maps dep mapping names to the install names of the parcels mapped to

        std::unordered_map<dep_mapping_name, str> dep_mappings;


        // TODO: these have not been unit tested

        install_batch& install(str install_name, res<parcel> x);
        install_batch& map_dep(str install_name, str dep_name, str mapped_to);
    };


    // TODO: behaviour about compiler/verifier being injected properly, and being generated
    //       if nullptr is injected

    // domain impls are expected to accept domain_config, and respond as expected

    struct domain_config final {
        std::shared_ptr<compiler> compiler = nullptr; // if compiler == nullptr domain impl will create one
        std::shared_ptr<verifier> verifier = nullptr; // if verifier == nullptr domain impl will create one
    };


    class domain : public api_component {
    public:
        domain(std::shared_ptr<debug> dbg = nullptr);

        virtual ~domain() noexcept = default;


        // IMPORTANT: install, install_count, and is_installed are unit tested together

        // install attempts to install x, returning if successful

        virtual bool install(install_batch&& x) = 0;

        virtual size_t install_count() const noexcept = 0;
        virtual bool is_installed(const str& install_name) const noexcept = 0;


        // import attempts to import a module at import_path (in domain env)

        virtual std::optional<yama::module> import(const str& import_path) = 0;


        // load returns the type under fullname, if any

        virtual std::optional<type> load(const str& fullname) = 0;

        // load_# methods below provide quick access to preloaded 
        // built-in Yama types, w/ as little overhead as possible

        type load_none();
        type load_int();
        type load_uint();
        type load_float();
        type load_bool();
        type load_char();


        // TODO: I don't think we've really unit tested failing due to a group
        //       of uploaded types having things like name conflicts BETWEEN
        //       said group's members

        // upload attempts to upload new type information to the domain,
        // returning if successful

        // upload will fail if uploaded type_info(s) use names already
        // taken by already uploaded type_info(s), or one another

        bool upload(type_info&& x);
        bool upload(std::span<const type_info> x);
        bool upload(const std::vector<type_info>& x);
        bool upload(std::initializer_list<type_info> x);
        bool upload(res<const module_info> x);

        // these overloads upload source code which gets compiled

        bool upload(const taul::source_code& src);
        bool upload(const str& src);
        bool upload(const std::filesystem::path& src_path);


    protected:
        friend class parcel_services;
        friend class compiler_services;


        // finish_setup is called in domain impl ctors after the impl has initialized
        // to the point where type uploading/loading can be performed

        // finish_setup invokes do_preload_builtins and prepares quick access data

        // finish_setup is expected not to be able to fail

        // domain behaviour is undefined if finish_setup is not used properly

        void finish_setup();


        // quick_access is a struct containing yama::type(s) for each of the primitive
        // types the domain provides quick access to

        struct quick_access final {
            type none, int0, uint, float0, bool0, char0;
        };


        virtual quick_access do_preload_builtins() = 0;
        virtual std::shared_ptr<const module_info> do_compile(const taul::source_code& src) = 0;
        virtual bool do_upload(type_info&& x) = 0;
        virtual bool do_upload(std::span<const type_info> x) = 0;
        virtual bool do_upload(res<const module_info> x) = 0;


        parcel_services create_parcel_services(const str& install_name);
        compiler_services create_compiler_services(const str& parcel_env);


        virtual std::shared_ptr<const module_info> do_ps_compile(const taul::source_code& src, const str& parcel_env) = 0;

        virtual std::optional<yama::module> do_cs_import(const str& import_path, const str& parcel_env) = 0;
        virtual std::optional<type> do_cs_load(const str& fullname, const str& parcel_env) = 0;


    private:
        std::optional<quick_access> _quick_access;
    };


    class default_domain final : public domain {
    public:
        default_domain(const domain_config& config, std::shared_ptr<debug> dbg = nullptr);
        default_domain(std::shared_ptr<debug> dbg = nullptr);

        virtual ~default_domain() noexcept = default;


        bool install(install_batch&& x) override final;
        size_t install_count() const noexcept override final;
        bool is_installed(const str& install_name) const noexcept override final;
        std::optional<yama::module> import(const str& import_path) override final;
        std::optional<type> load(const str& fullname) override final;


    protected:
        quick_access do_preload_builtins() override final;
        std::shared_ptr<const module_info> do_compile(const taul::source_code& src) override final;
        bool do_upload(type_info&& x) override final;
        bool do_upload(std::span<const type_info> x) override final;
        bool do_upload(res<const module_info> x) override final;

        std::shared_ptr<const module_info> do_ps_compile(const taul::source_code& src, const str& parcel_env) override final;

        std::optional<yama::module> do_cs_import(const str& import_path, const str& parcel_env) override final;
        std::optional<type> do_cs_load(const str& fullname, const str& parcel_env) override final;


    private:
        // TODO: replace _other_dbg w/ methods like import/load being given 'quiet' params which
        //       suppress error/warning msgs, w/ us making use of these where needed to impl the
        //       desired avoiding of annoying internal error msgs

        // we're using a proxy for dbg for certain things to cut out annoying internal
        // instantiation error messages which will arise during successful compilation

        std::shared_ptr<debug> _other_dbg;


        res<verifier> _verif;
        res<compiler> _compiler;


        // TODO: our impl code uses the term 'batch' for multiple different concepts, w/ these
        //       usages being similar to one another, but different, so try and figure out a
        //       way to harmonize and/or rename stuff to avoid confusion

        // when we compile we send the compiler a compiler services proxy domain which shares
        // the same state as this one, but uses the ***_batch variants of the below fields

        // if compilation succeeds, we *commit* the contents of our proxy to the domain proper,
        // w/ us otherwise *discarding* said contents instead

        internal::res_db<res<type_info>> _type_info_db, _type_info_db_batch;
        internal::res_db<res<internal::type_instance<std::allocator<void>>>> _type_db, _type_db_batch, _type_batch_db;
        internal::type_instantiator<std::allocator<void>> _instant, _instant_batch;

        // TODO: at present, _modules_batch is COMPLETELY UNUSED, as NONE of our loading stuff
        //       currently knows about, nor interacts w/, modules
        //
        //       this means that, in truth, our impl is *not entirely correct* in how it works,
        //       so when we do add loader usage of modules, please do keep this in mind

        std::unordered_map<str, res<const module_info>> _modules, _modules_batch;

        void _commit_batch();
        void _discard_batch();
        void _commit_or_discard_batch(bool commit);


        // the below record the installed parcels of the domain, and record the established
        // dep mappings between them, using their install names

        std::unordered_map<str, res<parcel>> _installs;
        std::unordered_map<dep_mapping_name, str> _dep_mappings; // includes special self-name mappings

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
        void _add_self_name_mappings(install_batch& batch);
        void _commit_batch_to_main_and_add_self_name_mappings(install_batch& batch);

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


        // NOTE: head is AFTER parcel to domain env conversion

        struct _resolved_import_path_t final {
            str import_path, head, relative_path;
        };

        void _assert_parcel_env_parcel_is_okay(const std::optional<str>& parcel);
        std::optional<_resolved_import_path_t> _resolve_import_path(const str& import_path, const std::optional<str>& parcel);
        std::optional<yama::module> _query_already_memoized_parcel(const str& import_path);
        std::shared_ptr<parcel> _query_installed_parcel(const str& install_name);
        std::optional<yama::module> _handle_fresh_import_and_memoize(const _resolved_import_path_t& rip, parcel& p);
        // if parcel == std::nullopt, then domain env, otherwise, its parcel env of *parcel
        std::optional<yama::module> _import(const str& import_path, std::optional<str> parcel);


        bool _verify(const type_info& x);
        bool _verify(std::span<const type_info> x);
        bool _verify(const module_info& x);

        void _upload(type_info&& x);
        void _upload(std::span<const type_info> x);
        void _upload(res<const module_info> x);
    };
}

