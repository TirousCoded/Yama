

#pragma once


#include <taul/source_code.h>

#include "../core/general.h"
#include "../core/api_component.h"

#include "specifiers.h"
#include "compilation_state.h"


namespace yama::internal {


    class domain_data;


    class compiler final : public api_component {
    public:
        compiler(std::shared_ptr<debug> dbg, domain_data& dd);


        // src_path is the import path of the original compiling module

        bool compile(const taul::source_code& src, const import_path& src_path);


    private:
        compilation_state _cs;
    };
}

