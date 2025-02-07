

#pragma once


#include <optional>
#include <vector>

#include <taul/source_code.h>

#include "res.h"
#include "api_component.h"
#include "module_info.h"


namespace yama {


    // TODO: right now we're just gonna expose ALL types in the domain to
    //       the compiler to use
    //
    //       later on, however, we're gonna need to revise it to instead take
    //       in some kind of 'environment' object which mediates access to
    //       some namespace of types
    //
    //       this system will need to be able to handle both compiling modules
    //       which see only a limited set of named dependencies, and scripts
    //       which might be allowed to see EVERYTHING


    class domain;


    class compiler : public api_component {
    public:
        using services = domain;


        compiler(std::shared_ptr<debug> dbg = nullptr);


        // IMPORTANT: it's important for compiler impls to avoid taking ownership of services's
        //            object for longer than the execution of compile, as compiler is used
        //            within domain impls, so it's best to avoid potential ref cycles

        // IMPORTANT: the services object injected need-not allow compiler to install parcels

        // compile takes in a taul::source_code and compiles it into a module

        // services exposes compile to existing type information

        virtual std::shared_ptr<const module_info> compile(
            res<services> services,
            const taul::source_code& src) = 0;
    };


    class default_compiler final : public compiler {
    public:
        default_compiler(std::shared_ptr<debug> dbg = nullptr);


        std::shared_ptr<const module_info> compile(
            res<services> services,
            const taul::source_code& src) override final;


    private:
        // we use the services param in compile to keep domain alive during compilation,
        // and then we'll assign this ptr services.get() to access it

        services* _services_ptr;


        services& _services() const noexcept;
    };
}

