

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
    //
    //       this, as part of a larger revision, could even replace subdomains... maybe

    // TODO: maybe replace the undefined behaviour encountered if a subdomain is used
    //       when its upstream domain no longer exists w/ an exception throw, as that'll
    //       make release builds less likely to corrupt at runtime due to macroscopic
    //       usage patterns of domains by other code


    // IMPORTANT:
    //      domains operate by the end-user 'uploading' type_info to it, and then the
    //      domain lazy loading (here called 'instantiating') actual types (accessed
    //      via type objects) on demand by the end-user
    //
    //      the memory underlying type objects is guaranteed to be valid for the
    //      lifetime of the domain, and to be entirely thread-safe, due said type
    //      info memory being immutable (take note that this doesn't mean that domain
    //      impl's methods are guaranteed to be thread-safe)
    // 
    //      this guarantee extends to types instantiated within subdomains, if and
    //      only if, commit is called after its instantiation, w/ this type info
    //      otherwise being guaranteed to be lost upon subdomain destruction
    //
    //      take note that the above about subdomains even applies when the instantiated
    //      type's type_info was uploaded to the upstream domain (ie. where it's type_info
    //      was uploaded doesn't matter, only where the type was instantiated)


    class domain_setup_error;
    class domain;
    class subdomain;

    class default_domain;


    class domain_setup_error final : public std::runtime_error {
    public:
        inline explicit domain_setup_error(const std::string& msg) : runtime_error(msg.c_str()) {}
        inline explicit domain_setup_error(const char* msg) : runtime_error(msg) {}
    };


    class domain : public api_component {
    public:

        domain(std::shared_ptr<debug> dbg = nullptr);

        virtual ~domain() noexcept = default;


        // locked returns if the domain is locked due to the existence of a subdomain

        virtual bool locked() const noexcept = 0;

        // fork creates a subdomain downstream of this domain, locking this domain
        // while the subdomain exists

        // the debug layer of the domain will be propagated down to the subdomain

        // the forked subdomain will NOT take ownership of this domain's memory, w/
        // undefined behaviour if the subdomain outlives this domain

        // if locked() == true, the impl is free to either fail, or block the
        // calling thread until the domain becomes available again
        //      * impls should have unit tests for these

        std::shared_ptr<subdomain> fork();

        // an overload of fork where the debug layer of the subdomain is injected,
        // rather than being left up to the impl

        std::shared_ptr<subdomain> fork(std::shared_ptr<debug> dbg);


        // load returns the type under fullname, if any

        // if locked() == true, the impl is free to either fail, or block the
        // calling thread until the domain becomes available again
        //      * impls should have unit tests for these

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

        // if locked() == true, the impl is free to either fail, or block the
        // calling thread until the domain becomes available again
        //      * impls should have unit tests for these

        bool upload(type_info x);
        bool upload(std::span<const type_info> x);
        bool upload(const std::vector<type_info>& x);
        bool upload(std::initializer_list<type_info> x);

        // these overloads upload source code which gets compiled

        bool upload(const taul::source_code& src);
        bool upload(const str& src);
        bool upload(const std::filesystem::path& src_path);


    protected:

        // TODO: expected behaviour of domain impls which fail domain setup
        //       has not yet been unit tested

        // IMPORTANT:
        //      for subdomains, DO NOT use setup_domain, use setup_subdomain!
        //      (and likewise for non-subdomains use setup_domain)

        // IMPORTANT:
        //      domain behaviour is undefined if setup fails but the impl does
        //      not adhere to the expected usage pattern detailed below

        // setup_domain is called in order to perform yama::domain setup
        // after all domain state as otherwise been initialized, w/ this
        // including doing:
        //      1) uploading built-in type info to the domain
        //      2) pre-loading built-in primitive types for quick access

        // setup_domain is called in the ctor of domain impls after they've
        // called yama::domain's ctor, and have setup their own state

        // setup_domain returns if it was able to 100% setup all that needed 
        // setting-up in the domain

        bool setup_domain();

        // setup_subdomain is an alternative to setup_domain for when the domain
        // being initialized is a subdomain of some upstream domain

        // the main difference is that setup_subdomain does not attempt to upload
        // built-in type info, instead using load_# methods on the upstream domain
        // to acquire built-in types for quick access

        bool setup_subdomain(std::weak_ptr<domain> upstream);

        // fail_domain_setup throws yama::domain_setup_error to report a failure
        // to setup the domain properly

        // setup_domain and fail_domain_setup are expected to be used as follows:
        //
        //      if (!setup_domain()) {
        //          // cleanup code
        //          fail_domain_setup();
        //      }

        // the above pattern is intended to help the end-user avoid easy-to-make 
        // bugs in which a failure to make domain impl ctor code *exception safe*
        // could lead to silent bugs like memory leaks

        void fail_domain_setup();


        virtual std::optional<std::vector<type_info>> do_compile(const taul::source_code& src) = 0;
        virtual std::shared_ptr<subdomain> do_fork(std::shared_ptr<debug> dbg) = 0;
        virtual bool do_upload(type_info x) = 0;
        virtual bool do_upload(std::span<const type_info> x) = 0;


    private:

        struct _quick_access_t final {
            type none, int0, uint, float0, bool0, char0;
        };

        std::optional<_quick_access_t> _quick_access;
    };


    class subdomain : public domain {
    public:

        // NOTE: subdomain impls are expected to be only instantiatable by the
        //       internals of the domain/subdomain impl they are a part of

        subdomain(std::weak_ptr<domain> upstream);
        subdomain(std::weak_ptr<domain> upstream, std::shared_ptr<debug> dbg);

        // IMPORTANT: subdomain impls must be able to handle their upstream domain being destroyed
        //            BEFORE the subdomain is
        //
        //            it's still undefined behaviour to use a subdomain when upstream is destroyed,
        //            but to make usage easier (as dtor order is somewhat annoying), we'll allow
        //            this nuance in dtor behaviour
        //
        //            be sure to assert that method usage w/out upstream is still undefined behaviour

        virtual ~subdomain() noexcept;


        // upstream returns the upstream domain, which is guaranteed to be lockable if
        // called outside of subdomain impl dtor

        // subdomain behaviour is undefined if upstream().lock() == nullptr (see note above
        // about dtor behaviour however)

        std::weak_ptr<domain> upstream() const noexcept;


        // commit pushes all type info uploaded to, or instantiated by, this subdomain
        // to the domain upstream, making it available to it (when it unblocks)

        // uncommitted uploaded/instantiated type info will be lost upon subdomain destruction

        void commit();


    protected:

        virtual void do_commit() = 0;


    private:

        std::weak_ptr<domain> _upstream;
    };


    class default_domain final : public domain {
    public:

        default_domain(std::shared_ptr<debug> dbg = nullptr);

        virtual ~default_domain() noexcept = default;


        bool locked() const noexcept override final;
        std::optional<type> load(const str& fullname) override final;


    protected:

        std::optional<std::vector<type_info>> do_compile(const taul::source_code& src) override final;
        std::shared_ptr<subdomain> do_fork(std::shared_ptr<debug> dbg) override final;
        bool do_upload(type_info x) override final;
        bool do_upload(std::span<const type_info> x) override final;


    private:

        struct _state_t final {
            default_verifier verif;
            default_compiler compiler;
            internal::res_db<res<type_info>> type_info_db;
            internal::res_db<res<internal::type_instance<std::allocator<void>>>> type_db;
            internal::res_db<res<internal::type_instance<std::allocator<void>>>> type_batch_db;
            internal::type_instantiator<std::allocator<void>> instant;
            bool locked = false;
            bool upstream_is_default_domain = false;


            _state_t(std::shared_ptr<debug> dbg);
            _state_t(std::shared_ptr<debug> dbg, _state_t& upstream);
        };


        _state_t _state;


        bool _verify(const type_info& x);
        bool _verify(std::span<const type_info> x);

        void _upload(type_info&& x);
        void _upload(std::span<const type_info> x);


        class subdomain_t final : public subdomain {
        public:

            subdomain_t(std::weak_ptr<default_domain> upstream, std::shared_ptr<debug> dbg);
            subdomain_t(std::weak_ptr<subdomain_t> upstream, std::shared_ptr<debug> dbg);

            virtual ~subdomain_t() noexcept;


            bool locked() const noexcept override final;
            std::optional<type> load(const str& fullname) override final;


        protected:

            std::optional<std::vector<type_info>> do_compile(const taul::source_code& src) override final;
            std::shared_ptr<subdomain> do_fork(std::shared_ptr<debug> dbg) override final;
            bool do_upload(type_info x) override final;
            bool do_upload(std::span<const type_info> x) override final;
            void do_commit() override final;


        private:

            friend class yama::default_domain;


            _state_t _state;


            bool _verify(const type_info& x);
            bool _verify(std::span<const type_info> x);

            void _upload(type_info&& x);
            void _upload(std::span<const type_info> x);

            _state_t& _upstream_state() const;
        };
    };
}

