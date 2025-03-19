

#pragma once


#include <unordered_set>

#include "../core/debug.h"
#include "../core/domain.h"

#include "ast.h"
#include "csymtab.h"
#include "csymtab_group_ctti.h"
#include "cfg_analyzer.h"


namespace yama::internal {


    // IMPORTANT:
    //      first pass responsibilities:
    //          - build table of imported modules
    //              - detect if 'yama' module unavailable
    //              - detect non-local import dirs
    //              - detect import dirs appearing after first type decl
    //              * includes implicitly imported 'yama' module
    //              * includes explicit imports by import dirs
    //          - build symbol table(s)
    //              - detect name conflicts
    //              - load extern types from imported modules
    //                  * which types to load is decided by which are referenced by type
    //                    specifiers and/or identifier exprs
    //                  * unqualified reference to extern type which could name a type in
    //                    one of multiple modules will be detected as a name conflict
    //                  * types declared in code shadow extern ones
    //              - resolution of explicitly annotated type info
    //                  * type deduction is performed in second pass
    //          - detect non-local vars
    //          - detect local fns
    //          - detect unannotated trailing param decls (ie. the 'd' in 'a, b, c: Int, d')
    //          - detect fns w/ >24 params
    //          - detect break stmts used outside of loop stmt blocks
    //          - detect continue stmts used outside of loop stmt blocks
    //          - detect fns w/ non-None return types w/ code w/ control paths which aren't punctuated
    //            by return stmts, and which do not enter infinite loops


    class first_pass final : public ast_visitor {
    public:

        first_pass(
            std::shared_ptr<debug> dbg,
            res<compiler_services> services,
            ast_Chunk& root,
            const taul::source_code& src,
            csymtab_group_ctti& csymtabs);


        // returns if the pass succeeded
        inline bool good() const noexcept { return !err; }


        void visit_begin(res<ast_Chunk> x) override final;
        //void visit_begin(res<ast_DeclOrDir> x) override final;
        void visit_begin(res<ast_ImportDir> x) override final;
        //void visit_begin(res<ast_ImportPath> x) override final;
        void visit_begin(res<ast_VarDecl> x) override final;
        void visit_begin(res<ast_FnDecl> x) override final;
        //void visit_begin(res<ast_CallSig> x) override final;
        void visit_begin(res<ast_ParamDecl> x) override final;
        //void visit_begin(res<ast_Result> x) override final;
        void visit_begin(res<ast_Block> x) override final;
        //void visit_begin(res<ast_Stmt> x) override final;
        //void visit_begin(res<ast_ExprStmt> x) override final;
        void visit_begin(res<ast_IfStmt> x) override final;
        void visit_begin(res<ast_LoopStmt> x) override final;
        void visit_begin(res<ast_BreakStmt> x) override final;
        void visit_begin(res<ast_ContinueStmt> x) override final;
        void visit_begin(res<ast_ReturnStmt> x) override final;
        //void visit_begin(res<ast_Expr> x) override final;
        //void visit_begin(res<ast_PrimaryExpr> x) override final;
        //void visit_begin(res<ast_Lit> x) override final;
        //void visit_begin(res<ast_IntLit> x) override final;
        //void visit_begin(res<ast_UIntLit> x) override final;
        //void visit_begin(res<ast_FloatLit> x) override final;
        //void visit_begin(res<ast_BoolLit> x) override final;
        //void visit_begin(res<ast_CharLit> x) override final;
        //void visit_begin(res<ast_Assign> x) override final;
        //void visit_begin(res<ast_Args> x) override final;
        //void visit_begin(res<ast_TypeAnnot> x) override final;
        //void visit_begin(res<ast_TypeSpec> x) override final;

        //void visit_end(res<ast_Chunk> x) override final;
        //void visit_end(res<ast_DeclOrDir> x) override final;
        //void visit_end(res<ast_ImportDir> x) override final;
        //void visit_end(res<ast_ImportPath> x) override final;
        //void visit_end(res<ast_VarDecl> x) override final;
        void visit_end(res<ast_FnDecl> x) override final;
        //void visit_end(res<ast_CallSig> x) override final;
        //void visit_end(res<ast_ParamDecl> x) override final;
        //void visit_end(res<ast_Result> x) override final;
        void visit_end(res<ast_Block> x) override final;
        //void visit_end(res<ast_Stmt> x) override final;
        //void visit_end(res<ast_ExprStmt> x) override final;
        void visit_end(res<ast_IfStmt> x) override final;
        void visit_end(res<ast_LoopStmt> x) override final;
        //void visit_end(res<ast_BreakStmt> x) override final;
        //void visit_end(res<ast_ContinueStmt> x) override final;
        //void visit_end(res<ast_ReturnStmt> x) override final;
        //void visit_end(res<ast_Expr> x) override final;
        //void visit_end(res<ast_PrimaryExpr> x) override final;
        //void visit_end(res<ast_Lit> x) override final;
        //void visit_end(res<ast_IntLit> x) override final;
        //void visit_end(res<ast_UIntLit> x) override final;
        //void visit_end(res<ast_FloatLit> x) override final;
        //void visit_end(res<ast_BoolLit> x) override final;
        //void visit_end(res<ast_CharLit> x) override final;
        //void visit_end(res<ast_Assign> x) override final;
        //void visit_end(res<ast_Args> x) override final;
        //void visit_end(res<ast_TypeAnnot> x) override final;
        //void visit_end(res<ast_TypeSpec> x) override final;


    private:
        std::shared_ptr<debug> _dbg;
        res<compiler_services> _services;

        ast_Chunk* _root;
        const taul::source_code* _src;
        csymtab_group_ctti* _csymtabs;

        ast_Chunk& _get_root() const noexcept;
        const taul::source_code& _get_src() const noexcept;
        csymtab_group_ctti& _get_csymtabs() const noexcept;


        // err indicates if the compilation has encountered a fatal error

        bool err = false;

        inline void _fatal() {
            err = true;
        }

        // helper for raising compile-time error msgs

        template<typename... Args>
        inline void _compile_error(
            const ast_node& where,
            dsignal dsig,
            std::format_string<Args...> fmt,
            Args&&... args);


        // this handles control-flow analysis for us, including providing info
        // to the rest of the system about what the CFG is like at those points

        cfg_analyzer _cfg;


        void _add_csymtab(res<ast_node> x);
        void _implicitly_import_yama_module();
        void _insert_vardecl(res<ast_VarDecl> x);
        bool _insert_fndecl(res<ast_FnDecl> x);
        void _insert_paramdecl(res<ast_ParamDecl> x);


        // this stack maintains what fn decl is currently being evaluated

        struct _fn_decl final {
            res<ast_FnDecl> node;
            std::shared_ptr<csymtab::entry> symbol; // nullptr if error
            size_t param_index = 0; // used to help params discern their index


            inline size_t next_param_index() { return param_index++; }
        };

        std::vector<_fn_decl> _fn_decl_stk;

        bool _is_in_fn();
        _fn_decl& _current_fn();
        
        void _begin_fn(res<ast_FnDecl> x);
        void _end_fn();


        // this flag is used to discern when we're at or beyond the first type decl, meaning
        // that no further import dirs may exist

        bool _reached_first_type_decl = false;

        void _acknowledge_type_decl();
        bool _import_dirs_are_legal() const noexcept;


        // this set is used to record the body blocks of fn decls

        std::unordered_set<ast_id_t> _fn_body_blocks;

        void _mark_as_fn_body_block(const ast_Block& x);
        bool _is_fn_body_block(const ast_Block& x) const noexcept;


        // returns if x either has no return type annot, or its return type is None

        bool _fn_decl_return_type_is_none(const ast_FnDecl& x);
    };

    template<typename... Args>
    inline void first_pass::_compile_error(
        const ast_node& where,
        dsignal dsig,
        std::format_string<Args...> fmt,
        Args&&... args) {
        YAMA_RAISE(_dbg, dsig);
        YAMA_LOG(
            _dbg, compile_error_c,
            "error: {} {}",
            _get_src().location_at(where.low_pos()),
            std::format(fmt, std::forward<Args&&>(args)...));
        _fatal();
    }
}

