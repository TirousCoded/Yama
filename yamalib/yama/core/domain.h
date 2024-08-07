

#pragma once


#include <optional>

#include "res.h"
#include "api_component.h"
#include "mas.h"
#include "type.h"

// NOTE: this is so things like yama::primitive_info are available to end-user
//       w/out forcing them to include type_info.h first

#include "type_info.h"


namespace yama {


    class domain_setup_error final : public std::runtime_error {
    public:
        explicit domain_setup_error(const std::string& msg) : runtime_error(msg.c_str()) {}
        explicit domain_setup_error(const char* msg) : runtime_error(msg) {}
    };


    class domain : public api_component {
    public:

        domain(std::shared_ptr<debug> dbg = nullptr);


        // get_mas returns the MAS used associated w/ this domain

        virtual res<mas> get_mas() = 0;


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


        // push attempts to push new type information to the domain,
        // returning if successful

        virtual bool push(type_info x) = 0;


    protected:

        // TODO: expected behaviour of domain impls which fail domain setup
        //       has not yet been unit tested

        // IMPORTANT:
        //      domain behaviour is undefined if setup fails but the impl does
        //      not adhere to the expected usage pattern detailed below

        // setup_domain is called in order to perform yama::domain setup
        // after all domain state as otherwise been initialized, w/ this
        // including doing:
        //      1) pushing built-in type info to the domain
        //      2) pre-loading built-in primitive types for quick access

        // setup_domain is called in the ctor of domain impls after they've
        // called yama::domain's ctor, and have setup their own state

        // setup_domain returns if it was able to 100% setup all that needed 
        // setting-up in the domain

        bool setup_domain();

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


    private:

        struct _quick_access_t final {
            type none, int0, uint, float0, bool0, char0;
        };

        std::optional<_quick_access_t> _quick_access;
    };
}

