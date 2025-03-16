

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
#include "env.h"
#include "specifiers.h"
#include "verifier.h"
#include "compiler.h"
#include "install_batch.h"

#include "../internals/type_instance.h"
#include "../internals/res_state.h"
#include "../internals/install_manager.h"
#include "../internals/importer.h"
#include "../internals/loader.h"


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


    // IMPORTANT: the memory underlying type objects is guaranteed to be valid for the
    //            lifetime of the domain, and to be entirely thread-safe, due said type
    //            info memory being immutable (take note that this doesn't mean that domain
    //            impl's methods are guaranteed to be thread-safe)

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
    //                          * also, the 'self-name' can be used in parcel envs to
    //                            access the parcel of the env itself
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


    class domain;
    class default_domain;


    // TODO: below behaviour about compiler gen has not been unit tested

    struct domain_config final {
        std::shared_ptr<compiler> compiler = nullptr; // nullptr tells domain impl to generate a compiler to use
    };


    class domain : public api_component {
    public:
        domain(std::shared_ptr<debug> dbg = nullptr);

        virtual ~domain() noexcept = default;


        // TODO: add unit tests for get_installed
        // TODO: add unit tests for parcel_id overloads of below

        virtual bool install(install_batch&& x) = 0;
        virtual size_t install_count() const noexcept = 0;
        virtual bool is_installed(const str& install_name) const noexcept = 0;
        virtual bool is_installed(parcel_id id) const noexcept = 0;
        virtual std::shared_ptr<parcel> get_installed(const str& install_name) const noexcept = 0;
        virtual std::shared_ptr<parcel> get_installed(parcel_id id) const noexcept = 0;


        virtual std::optional<yama::module> import(const str& import_path) = 0;


        virtual std::optional<type> load(const str& fullname) = 0;

        // provide quick access to built-in Yama types, w/ as little overhead as possible

        type load_none();
        type load_int();
        type load_uint();
        type load_float();
        type load_bool();
        type load_char();


        // TODO: unit test these

        virtual env domain_env() = 0;
        virtual std::optional<env> parcel_env(const str& install_name) = 0;


    protected:
        // finish_setup is called in domain impl ctors after the impl has initialized
        // to the point where type uploading/loading can be performed

        // finish_setup invokes do_preload_builtins

        // finish_setup is expected not to be able to fail

        // domain behaviour is undefined if finish_setup is not used properly

        void finish_setup();


        // quick_access is a struct containing yama::type(s) for each of the primitive
        // types the domain provides quick access to

        struct quick_access final {
            type none, int0, uint, float0, bool0, char0;
        };


        // do_preload_builtins installs built-in 'yama' parcel, and prepares the
        // domain's quick-access data

        virtual quick_access do_preload_builtins() = 0;


    private:
        std::optional<quick_access> _quick_access;
    };


    class default_domain final : public domain {
    public:
        default_domain(domain_config config, std::shared_ptr<debug> dbg = nullptr);
        default_domain(std::shared_ptr<debug> dbg = nullptr);

        virtual ~default_domain() noexcept = default;


        bool install(install_batch&& x) override final;
        size_t install_count() const noexcept override final;
        bool is_installed(const str& install_name) const noexcept override final;
        bool is_installed(parcel_id id) const noexcept override final;
        std::shared_ptr<parcel> get_installed(const str& install_name) const noexcept override final;
        std::shared_ptr<parcel> get_installed(parcel_id id) const noexcept override final;
        std::optional<yama::module> import(const str& import_path) override final;
        std::optional<type> load(const str& fullname) override final;
        env domain_env() override final;
        std::optional<env> parcel_env(const str& install_name) override final;


    protected:
        quick_access do_preload_builtins() override final;


    private:
        default_verifier _verif;
        res<compiler> _compiler;

        internal::res_state _state;

        internal::install_manager _install_manager;
        internal::importer _importer;
        internal::loader _loader;


        void _install_builtin_yama_parcel();
        quick_access _prepare_quick_access_data();
    };
}

