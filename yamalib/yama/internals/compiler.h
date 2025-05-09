

#pragma once


#include <vector>

#include <taul/source_code.h>

#include "../core/res.h"

#include "safeptr.h"
#include "specifiers.h"
#include "env.h"
#include "ast.h"
#include "error_reporter.h"
#include "codegen_target.h"
#include "csymtab.h"
#include "ctypesys.h"
#include "ctype_resolver.h"
#include "constexpr_solver.h"
#include "const_table_populator.h"
#include "register_stk.h"
#include "first_pass.h"
#include "second_pass.h"


namespace yama::internal {


    class domain_data;

    class compiler;
    class translation_unit;


    class compiler final : public api_component {
    public:
        safeptr<domain_data> dd;

        specifier_provider sp;
        ctypesys types;
        ctype_resolver resolver;
        constexpr_solver solver;

        std::vector<res<translation_unit>> units;


        compiler(const std::shared_ptr<debug>& dbg, domain_data& dd);


        // compilation procedure

        // src_path is the import path of the original compiling module

        bool compile(const taul::source_code& src, const import_path& src_path);
        
        bool first_passes();
        bool second_passes();
        void cleanup();
        
        bool first_pass(translation_unit& unit);
        bool second_pass(translation_unit& unit);

        void push_new_unit(const res<translation_unit>& unit);
        void resolve_and_solve();
    };


    class translation_unit final {
    public:
        safeptr<compiler> cs;

        const taul::source_code src;
        import_path src_path;

        error_reporter err;
        codegen_target cgt;
        csymtab_group syms;
        ctypesys_local types;
        const_table_populator ctp;
        register_stk rs;
        first_pass fp;
        second_pass sp;

        std::shared_ptr<ast_Chunk> ast;
        module_factory output;


        translation_unit(compiler& cs, taul::source_code&& src, const import_path& src_path);


        // compilation procedure

        bool first_pass();
        bool second_pass();

        bool parse_ast();
        bool upload();


        // helpers

        inline parcel_id parcel() const noexcept { return src_path.head(); }
        env e() const;
        ast_Chunk& root() const;
    };
}

