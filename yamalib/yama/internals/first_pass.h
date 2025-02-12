

#pragma once


#include <unordered_set>

#include "ast.h"
#include "csymtab.h"
#include "csymtab_group_ctti.h"

#include "../core/debug.h"
#include "../core/domain.h"


namespace yama::internal {


    // IMPORTANT:
    //      first pass responsibilities:
    //          - build symbol table(s)
    //              - detect name conflicts
    //              - predeclare types defined prior to compilation
    //                  * which types to predeclare is decided by which are referenced by type
    //                    specifiers and/or identifier exprs
    //                  * for declared types in code which conflict w/ those in domain, either the
    //                    conflict will be detected by name conflict w/ the above, or by checking
    //                    the domain itself
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
            compiler_services services,
            ast_Chunk& root,
            const taul::source_code& src,
            csymtab_group_ctti& csymtabs);


        // returns if the pass succeeded
        inline bool good() const noexcept { return !err; }


        void visit_begin(res<ast_Chunk> x) override final;
        //void visit_begin(res<ast_Decl> x) override final;
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
        //void visit_end(res<ast_Decl> x) override final;
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
        compiler_services _services;
        ast_Chunk* _root;
        const taul::source_code* _src;
        csymtab_group_ctti* _csymtabs;

        // upon encounterng an ast_FnDecl, we store its name here for its ast_ParamDecl(s)
        // to access, alongside storing an index (incr for each ast_ParamDecl) so they
        // can know their index in the ast_FnDecl's param list
        bool _has_last_fn_decl = false; // <- disables param decl behaviour if fn failed to declare due to name conflict
        str _last_fn_decl;
        size_t _param_index = size_t(-1);


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


        void _add_csymtab(res<ast_node> x);
        void _insert_vardecl(res<ast_VarDecl> x);
        void _insert_fndecl(res<ast_FnDecl> x);
        void _insert_paramdecl(res<ast_ParamDecl> x);


        // this method checks if x is in global block, and if so, returns whether or not
        // a type under name exists in dm, returning false otherwise

        // if this method will return true, a name conflict error will be raised

        // this is used in type decl visit method impls to judge whether or not to raise
        // a name conflict error due to conflict w/ predeclared type (as csymtab code
        // doesn't check for this)

        bool _check_predeclared_type_name_conflict(res<ast_node> x, str name);


        // this set is used to record the body blocks of fn decls

        std::unordered_set<ast_id_t> _fn_body_blocks;

        void _mark_as_fn_body_block(const ast_Block& x);
        bool _is_fn_body_block(const ast_Block& x) const noexcept;


        // the below counter is used to discern when the AST traversal is in the context
        // of the interior of a fn

        size_t _fn_ctx = 0;

        inline bool _in_fn_ctx() const noexcept { return _fn_ctx >= 1; }
        inline void _enter_fn_ctx() { _fn_ctx++; }
        inline void _exit_fn_ctx() { YAMA_ASSERT(_in_fn_ctx()); _fn_ctx--; }


        // the below counter is used to discern when the AST traversal is in a context
        // where break/continue stmts are okay to use

        size_t _loop_ctx = 0;

        inline bool _in_loop_ctx() const noexcept { return _loop_ctx >= 1; }
        inline void _enter_loop_ctx() { _loop_ctx++; }
        inline void _exit_loop_ctx() { YAMA_ASSERT(_in_loop_ctx()); _loop_ctx--; }


        // TODO: the below control-flow analysis I find to be somewhat confusing

        // below, 'control path' refers to 'control paths reachable from the entrypoint'

        // below, stmts are conceptualized as bundling together >=1 'subpath' control paths
        // subordinated by the overall control path of the stmt

        // as the pass depth-first traverses the AST, the below stack is used to analyse
        // the control-flow of the code

        // the below stack gets an entry for each block or composite stmt encountered, and a
        // special base entry for the fn decl itself (w/ this also meaning that local fns will
        // have a local 'base', and then entries about it relative to this base, but w/ these
        // being semantically disconnected from anything below the local base)

        // each entry records counters which count observed properties of the control paths
        // within a given stmt

        // each entry, upon the visit_end method for its corresponding AST node being
        // called will, according to its semantics, propagate its info upstream

        struct _cfg_paths_t final {
            bool    is_block;               // if entry is for a block
            size_t  block_endpoints = 0;    // control paths in which all subpaths exit block w/ break/continue/return stmts
            size_t  fn_endpoints    = 0;    // control paths in which all subpaths exit fn w/ return stmts, or enters infinite loops
            size_t  breaks          = 0;    // control paths containing >=1 subpaths which exit via break stmts
        };

        std::vector<_cfg_paths_t> _cfg_paths;

        bool _cfg_has_current();
        _cfg_paths_t& _cfg_current();
        void _cfg_push(bool is_block);
        _cfg_paths_t _cfg_pop(); // pops top _cfg_paths entry, returning it

        bool _cfg_is_dead_code();

        void _cfg_break_stmt();
        void _cfg_continue_stmt();
        void _cfg_return_stmt();
        
        void _cfg_block_begin();
        void _cfg_block_end();
        
        void _cfg_if_stmt_begin();
        void _cfg_if_stmt_end();

        void _cfg_loop_stmt_begin();
        void _cfg_loop_stmt_end();

        void _cfg_fn_decl_begin();
        bool _cfg_fn_decl_end(); // returns if can guarantee all control paths either reach return stmt, or enter infinite loop


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

