

#pragma once



#include "../core/bcode.h"
#include "../core/type_info.h"

#include "safeptr.h"
#include "ast.h"
#include "csymtab.h"


namespace yama::internal {


    class translation_unit;


    // IMPORTANT:
    //      second pass responsibilities:
    //          - resolve deduced types of local var symbols
    //          - resolve register indices of local var symbols
    //          - code generation
    //              - type_info
    //              - constant tables
    //              - bcode
    //              - register alloc
    //          - detect any error not detected during first pass


    class second_pass final : public ast_visitor {
    public:
        safeptr<translation_unit> tu;


        second_pass(translation_unit& tu);


        //void visit_begin(res<ast_Chunk> x) override final;
        //void visit_begin(res<ast_Decl> x) override final;
        //void visit_begin(res<ast_ImportDecl> x) override final;
        //void visit_begin(res<ast_RelativePath> x) override final;
        //void visit_begin(res<ast_VarDecl> x) override final;
        void visit_begin(res<ast_FnDecl> x) override final;
        //void visit_begin(res<ast_CallSig> x) override final;
        //void visit_begin(res<ast_ParamDecl> x) override final;
        //void visit_begin(res<ast_Result> x) override final;
        void visit_begin(res<ast_Block> x) override final;
        //void visit_begin(res<ast_Stmt> x) override final;
        //void visit_begin(res<ast_ExprStmt> x) override final;
        void visit_begin(res<ast_IfStmt> x) override final;
        void visit_begin(res<ast_LoopStmt> x) override final;
        //void visit_begin(res<ast_BreakStmt> x) override final;
        //void visit_begin(res<ast_ContinueStmt> x) override final;
        //void visit_begin(res<ast_ReturnStmt> x) override final;
        void visit_begin(res<ast_Expr> x) override final;
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
        //void visit_end(res<ast_ImportDecl> x) override final;
        //void visit_end(res<ast_RelativePath> x) override final;
        void visit_end(res<ast_VarDecl> x) override final;
        void visit_end(res<ast_FnDecl> x) override final;
        //void visit_end(res<ast_CallSig> x) override final;
        //void visit_end(res<ast_ParamDecl> x) override final;
        //void visit_end(res<ast_Result> x) override final;
        void visit_end(res<ast_Block> x) override final;
        //void visit_end(res<ast_Stmt> x) override final;
        void visit_end(res<ast_ExprStmt> x) override final;
        void visit_end(res<ast_IfStmt> x) override final;
        void visit_end(res<ast_LoopStmt> x) override final;
        void visit_end(res<ast_BreakStmt> x) override final;
        void visit_end(res<ast_ContinueStmt> x) override final;
        void visit_end(res<ast_ReturnStmt> x) override final;
        void visit_end(res<ast_Expr> x) override final;
        //void visit_end(res<ast_PrimaryExpr> x) override final;
        //void visit_end(res<ast_Lit> x) override final;
        //void visit_end(res<ast_IntLit> x) override final;
        //void visit_end(res<ast_UIntLit> x) override final;
        //void visit_end(res<ast_FloatLit> x) override final;
        //void visit_end(res<ast_BoolLit> x) override final;
        //void visit_end(res<ast_CharLit> x) override final;
        //void visit_end(res<ast_Assign> x) override final;
        void visit_end(res<ast_Args> x) override final;
        //void visit_end(res<ast_TypeAnnot> x) override final;
        //void visit_end(res<ast_TypeSpec> x) override final;


    private:
        // IMPORTANT: if fatal error, the compilation is not to perform further code gen


        // TODO: at present, ALL custom types are fn types
        
        // _current_target gets set by _begin_target at start of type def, and then after
        // fully defined gets consumed by _end_target, which uploads it to results

        std::optional<yama::type_info> _current_target = std::nullopt;

        yama::type_info& _target() noexcept;

        void _begin_target(const str& unqualified_name);
        void _end_target();

        void _apply_bcode_to_target(const ast_FnDecl& x);
        void _update_target_locals();

        const fn_csym& _target_csym();

        std::optional<size_t> _target_param_index(const str& name);


        using label_id_t = bc::code_writer::label_id_t;

        label_id_t _next_label = 0;

        inline auto _gen_label() noexcept { return _next_label++; } // used to gen code_writer label IDs


        bc::code_writer cw; // gets applied onto current code gen target in fn decl's visit_end call
        bc::syms syms;

        void _add_sym(taul::source_pos pos); // call this AFTER cw.add_# method call


        // as the AST is traversed, an oprand stack-like 'register stack' is used to
        // perform type checking, and register allocation

        // this is transacted like a stack machine, w/ composite exprs operating on
        // it during their visit_end method call

        // the stack holds two types of registers: 'temporaries' and 'local vars'

        struct _reg_t final {
            ctype                           type;               // type encapsulated by the register
            std::optional<str>              localvar;           // name of the local var of this register (or std::nullopt if a temporary)
            size_t                          index       = 0;    // the stack index of this register


            inline bool is_temp() const noexcept { return !localvar.has_value(); }
            inline bool is_localvar() const noexcept { return !is_temp(); }
        };

        struct _scope_t final {
            size_t                          regs        = 0;    // how many temporaries/local vars this scope has
            size_t                          first_reg   = 0;    // the first reg on stack which belongs to this scope
        };

        // IMPORTANT:
        //      we impose a limit of 254 registers allocated at a time, w/ us raising an
        //      error if this limit is exceeded
        //
        //      the rest of the system, however, doesn't see this limit, and will simply
        //      continue to operate w/out regards for this limit
        //
        //      when using cw to write bcode, writing stack indices into register fields
        //      A, B, and C, which are >254, will result in a mangled value being written
        //      as a result of the overflow from casting to uint8_t
        //
        //      the reason 254 was chosen instead of 255 is to ensure that no issues occur
        //      w/ the yama::newtop constant

        static constexpr size_t _reg_limit = size_t((uint8_t)yama::newtop) - 1;

        std::vector<_reg_t> _regstk;
        std::vector<_scope_t> _scopestk;

        size_t _regs() const noexcept;

        size_t _reg_abs_index(ssize_t index); // takes relative index x and return abs index

        _reg_t& _reg_abs(size_t index);
        _reg_t& _reg(ssize_t index); // negative values index from top downward (in Lua style)
        _reg_t& _top_reg();

        _scope_t& _top_scope();

        // IMPORTANT: _pop_temp can auto-write pop instr, but _push_temp does not auto-write
        //            instr pushing the temporary

        // IMPORTANT: _pop_temp can pop local var registers, but it WILL NOT REMOVE THE METADATA
        //            FOR THE LOCAL VAR, meaning that if you pop a local var register, and then
        //            lookup the local var, you'll get its entry even though it shouldn't be
        //            valid anymore

        void _push_temp(const ast_node& x, ctype type);
        void _pop_temp(size_t n, bool write_pop_instr, taul::source_pos pop_instr_pos = 0);

        // _reinit_temp is used to change the type of existing temporaries/local vars

        void _reinit_temp(ssize_t index, ctype new_type); // allows negative indices, like _reg

        void _push_scope();
        void _pop_scope(const ast_Block& x, bool write_pop_instr); // unwinds local vars

        // for local vars, we're gonna have it be that we *promote* temporaries to local vars

        // this updates the symbol table entry w/

        void _promote_to_localvar(ast_VarDecl& x);

        // for type checking, the following is used to check the type expect of a register

        bool _reg_type_check(ssize_t index, ctype expected); // allows negative indices, like _reg


        // for ast_ExprStmt(s) w/ an assignment, the expr on the left-hand-size of this assignment
        // is called the 'lvalue', and is special in that normal eval semantics don't apply to it

        size_t _lvalue_scopes = 0;

        inline bool _is_in_lvalue() const noexcept { return _lvalue_scopes >= 1; }
        inline void _push_lvalue() { _lvalue_scopes++; }
        inline void _pop_lvalue() { YAMA_ASSERT(_is_in_lvalue()); _lvalue_scopes--; }


        struct _if_stmt_t final {
            std::optional<size_t> cond_reg_index = std::nullopt;
            ast_id_t body_block;
            bool has_else_part;
            label_id_t else_part_label, exit_label;
        };

        std::vector<_if_stmt_t> _if_stmts;

        bool _in_if_stmt() const noexcept;
        const _if_stmt_t& _top_if_stmt() const;

        void _push_if_stmt(const ast_IfStmt& x);
        void _pop_if_stmt();

        void _mark_as_if_stmt_cond_reg(ssize_t index);
        bool _is_if_stmt_body(const ast_Block& x) const;


        struct _loop_stmt_t final {
            label_id_t break_label, continue_label;
            size_t first_reg; // the index where the first reg in the loop body's scope will be located on the stack
        };

        std::vector<_loop_stmt_t> _loop_stmts;

        bool _in_loop_stmt() const noexcept;
        const _loop_stmt_t& _top_loop_stmt() const;

        void _push_loop_stmt(const ast_LoopStmt& x);
        void _pop_loop_stmt();
    };
}

