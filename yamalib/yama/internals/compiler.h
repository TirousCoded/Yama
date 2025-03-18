

#pragma once


#include <optional>
#include <vector>

#include <taul/source_code.h>

#include "../core/general.h"
#include "../core/res.h"
#include "../core/api_component.h"
#include "../core/parcel.h"

#include "env.h"
#include "specifiers.h"


namespace yama::internal {


    struct import_result_ext final {
        import_result result;
        env e;
    };

    class compiler_services : public api_component {
    public:
        compiler_services(std::shared_ptr<debug> dbg = nullptr);
        virtual ~compiler_services() noexcept = default;


        // TODO: write impl unit tests for env
        
        // TODO: our import unit tests don't cover what 'e' is returned
        // TODO: our import unit tests are generally a bit of a mess...

        virtual env env() const = 0;
        virtual std::optional<import_result_ext> import(const import_path& path) = 0;
    };


    struct compile_result final {
        std::unordered_map<import_path, module_info> results;


        void add(const import_path& path, module_info&& x);
    };

    class compiler final : public api_component {
    public:
        compiler(std::shared_ptr<debug> dbg = nullptr);


        // returns a compile_result containing newly compiled modules

        // the returned compile_result excludes any imported module which didn't require
        // the compiler to compile it from source code

        // src_import_path is the import path which the compilation is to consider to the
        // import path of the module which src is being compiled into

        // src_import_path exists to help the compiler avoid needless (or even problematic)
        // attempts to import from the module originally being compiled

        std::optional<compile_result> compile(
            res<compiler_services> services,
            const taul::source_code& src,
            const import_path& src_import_path);
    };
}

