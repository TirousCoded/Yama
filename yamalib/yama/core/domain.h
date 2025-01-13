

#pragma once


#include <optional>

#include <taul/source_code.h>

#include "res.h"
#include "api_component.h"
#include "type.h"
// NOTE: this is so things like yama::primitive_info are available to end-user
//       w/out forcing them to include type_info.h first
#include "type_info.h"
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


    class domain;
    class default_domain;


    class domain : public api_component {
    public:

        domain(std::shared_ptr<debug> dbg = nullptr);

        virtual ~domain() noexcept = default;


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

        bool upload(type_info x);
        bool upload(std::span<const type_info> x);
        bool upload(const std::vector<type_info>& x);
        bool upload(std::initializer_list<type_info> x);

        // these overloads upload source code which gets compiled

        bool upload(const taul::source_code& src);
        bool upload(const str& src);
        bool upload(const std::filesystem::path& src_path);


    protected:

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
        virtual std::optional<std::vector<type_info>> do_compile(const taul::source_code& src) = 0;
        virtual bool do_upload(type_info x) = 0;
        virtual bool do_upload(std::span<const type_info> x) = 0;


    private:

        std::optional<quick_access> _quick_access;
    };


    class default_domain final : public domain {
    public:

        default_domain(std::shared_ptr<debug> dbg = nullptr);

        virtual ~default_domain() noexcept = default;


        std::optional<type> load(const str& fullname) override final;


    protected:

        quick_access do_preload_builtins() override final;
        std::optional<std::vector<type_info>> do_compile(const taul::source_code& src) override final;
        bool do_upload(type_info x) override final;
        bool do_upload(std::span<const type_info> x) override final;


    private:

        struct _state_t final {
            default_verifier verif;
            default_compiler compiler;

            // when we compile, we sent the compiler a *proxy* domain which shares the same
            // state as this one, but uses the below ***_proxy variants of things like instant
            
            // if compilation succeeds, we *commit* the contents of our proxy to the domain proper,
            // w/ us otherwise *discarding* said contents instead

            internal::res_db<res<type_info>> type_info_db;
            internal::res_db<res<type_info>> type_info_db_proxy;
            internal::res_db<res<internal::type_instance<std::allocator<void>>>> type_db;
            internal::res_db<res<internal::type_instance<std::allocator<void>>>> type_db_proxy;
            internal::res_db<res<internal::type_instance<std::allocator<void>>>> type_batch_db;
            internal::type_instantiator<std::allocator<void>> instant;
            internal::type_instantiator<std::allocator<void>> instant_proxy;


            _state_t(std::shared_ptr<debug> dbg);


            void commit_proxy();
            void discard_proxy();
        } _state;


        std::shared_ptr<domain> _compiler_services; // the proxy we send to our compiler

        void _setup_compiler_services();
        res<domain> _get_compiler_services();


        bool _verify(const type_info& x);
        bool _verify(std::span<const type_info> x);

        void _upload(type_info&& x);
        void _upload(std::span<const type_info> x);


        class _compiler_services_t final : public domain {
        public:

            _compiler_services_t(default_domain& upstream);

            virtual ~_compiler_services_t() noexcept = default;


            std::optional<type> load(const str& fullname) override final;


        protected:

            quick_access do_preload_builtins() override final;
            std::optional<std::vector<type_info>> do_compile(const taul::source_code& src) override final;
            bool do_upload(type_info x) override final;
            bool do_upload(std::span<const type_info> x) override final;


        private:

            default_domain* _upstream_ptr = nullptr;

            inline default_domain& _get_upstream() const noexcept { return deref_assert(_upstream_ptr); }
            inline _state_t& _get_state() const noexcept { return _get_upstream()._state; }


            bool _verify(const type_info& x);
            bool _verify(std::span<const type_info> x);

            void _upload(type_info&& x);
            void _upload(std::span<const type_info> x);
        };
    };
}

