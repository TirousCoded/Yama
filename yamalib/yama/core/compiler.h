

#pragma once


#include <optional>
#include <vector>

#include <taul/source_code.h>

#include "res.h"
#include "api_component.h"
#include "module_info.h"
#include "module.h"


namespace yama {


    class domain;


    // NOTE: services classes are unit tested as part of domain impl unit tests

    class compiler_services final {
    public:
        compiler_services() = delete;


        // importing/loading occur in parcel env of parcel performing the compilation

        std::optional<yama::module> import(const str& import_path);
        std::optional<type> load(const str& fullname);
        type load_none();
        type load_int();
        type load_uint();
        type load_float();
        type load_bool();
        type load_char();


    private:
        friend class domain;


        compiler_services(domain& client, const str& parcel_env); // for use in domain


        domain* _client;
        str _parcel_env;
    };


    class compiler : public api_component {
    public:
        compiler(std::shared_ptr<debug> dbg = nullptr);


        // compile attempts to compile src into a module

        virtual std::shared_ptr<const module_info> compile(compiler_services services, const taul::source_code& src) = 0;
    };


    class default_compiler final : public compiler {
    public:
        default_compiler(std::shared_ptr<debug> dbg = nullptr);


        std::shared_ptr<const module_info> compile(compiler_services services, const taul::source_code& src) override final;
    };
}

