

#pragma once


#include <unordered_set>

#include "../core/debug.h"

#include "specifiers.h"
#include "ast.h"
#include "csymtab.h"
#include "ctypesys.h"
#include "ctype_resolver.h"
#include "error_reporter.h"
#include "cfg_analyzer.h"


namespace yama::internal {


    class compiler_services;


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
    //          - detect fns w/ non-None return types containing control paths which aren't
    //            punctuated by return stmts, and which do not enter infinite loops
    //              * only control-flow analysis part is done here, w/ type info requiring part,
    //                and error raising, being deferred to second_pass


    class first_pass final : public ast_visitor {
    public:
        first_pass(
            std::shared_ptr<debug> dbg,
            res<compiler_services> services,
            const import_path& src_import_path,
            ast_Chunk& root,
            const taul::source_code& src,
            specifier_provider& sp,
            error_reporter& er,
            csymtab_group& csymtabs,
            ctypesys_local& ctypesys,
            ctype_resolver& ctype_resolver);


        // returns if the pass succeeded
        inline bool good() const noexcept { return !_get_er().is_fatal(); }


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
        void visit_begin(res<ast_PrimaryExpr> x) override final;
        //void visit_begin(res<ast_Lit> x) override final;
        //void visit_begin(res<ast_IntLit> x) override final;
        //void visit_begin(res<ast_UIntLit> x) override final;
        //void visit_begin(res<ast_FloatLit> x) override final;
        //void visit_begin(res<ast_BoolLit> x) override final;
        //void visit_begin(res<ast_CharLit> x) override final;
        //void visit_begin(res<ast_Assign> x) override final;
        //void visit_begin(res<ast_Args> x) override final;
        //void visit_begin(res<ast_TypeAnnot> x) override final;
        void visit_begin(res<ast_TypeSpec> x) override final;

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
        import_path _src_import_path;

        ast_Chunk* _root;
        const taul::source_code* _src;
        specifier_provider* _sp;
        error_reporter* _er;
        csymtab_group* _csymtabs;
        ctypesys_local* _ctypesys;
        ctype_resolver* _ctype_resolver;

        ast_Chunk& _get_root() const noexcept;
        const taul::source_code& _get_src() const noexcept;
        specifier_provider& _get_sp() const noexcept;
        error_reporter& _get_er() const noexcept;
        csymtab_group& _get_csymtabs() const noexcept;
        ctypesys_local& _get_ctypesys() const noexcept;
        ctype_resolver& _get_ctype_resolver() const noexcept;

        env _get_e() const;


        // this handles control-flow analysis for us, including providing info
        // to the rest of the system about what the CFG is like at those points

        cfg_analyzer _cfg;


        void _add_csymtab(res<ast_node> x);
        void _implicitly_import_yama_module();
        void _explicitly_import_module(const res<ast_ImportDir>& x);
        void _insert_vardecl(res<ast_VarDecl> x);
        bool _insert_fndecl(res<ast_FnDecl> x);
        void _insert_paramdecl(res<ast_ParamDecl> x);


        // this stack maintains what fn decl is currently being evaluated
        // (it's a stack for later when we add lambda fns)

        struct _fn_decl final {
            res<ast_FnDecl> node;
            std::shared_ptr<csymtab_entry> symbol; // nullptr if error
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


        void _check_var_is_local(res<ast_VarDecl> x);
        void _check_break_is_in_loop_stmt(const res<ast_BreakStmt>& x);
        void _check_continue_is_in_loop_stmt(const res<ast_ContinueStmt>& x);
    };
}

