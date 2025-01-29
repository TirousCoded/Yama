

#pragma once


#include <unordered_set>

#include "ast.h"
#include "csymtab.h"
#include "csymtab_group_ctti.h"
#include "first_pass.h"

#include "../core/debug.h"
#include "../core/domain.h"


namespace yama::internal {


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

        module_factory results; // result of code gen


        second_pass(
            std::shared_ptr<debug> dbg,
            ast_Chunk& root,
            const taul::source_code& src,
            csymtab_group_ctti& csymtabs);


        // returns if the pass succeeded
        inline bool good() const noexcept { return !err; }


        //void visit_begin(res<ast_Chunk> x) override final;
        //void visit_begin(res<ast_Decl> x) override final;
        //void visit_begin(res<ast_VarDecl> x) override final;
        void visit_begin(res<ast_FnDecl> x) override final;
        //void visit_begin(res<ast_CallSig> x) override final;
        //void visit_begin(res<ast_ParamDecl> x) override final;
        //void visit_begin(res<ast_Result> x) override final;
        void visit_begin(res<ast_Block> x) override final;
        //void visit_begin(res<ast_Stmt> x) override final;
        void visit_begin(res<ast_ExprStmt> x) override final;
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
        std::shared_ptr<debug> _dbg;
        ast_Chunk* _root;
        const taul::source_code* _src;
        csymtab_group_ctti* _csymtabs;


        ast_Chunk& _get_root() const noexcept;
        const taul::source_code& _get_src() const noexcept;
        csymtab_group_ctti& _get_csymtabs() const noexcept;


        // err indicates if the compilation has encountered a fatal error

        // if so, the compilation is not to perform any further code gen

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


        // TODO: at present, ALL custom types are fn types
        
        // _current_target gets set by _begin_target at start of type def, and then after
        // fully defined gets consumed by _end_target, which uploads it to results

        std::optional<yama::type_info> _current_target = std::nullopt;

        yama::type_info& _target() noexcept;

        void _begin_target(str fullname);
        void _end_target();

        void _apply_bcode_to_target(const ast_FnDecl& x);
        void _update_target_locals();

        const fn_csym& _target_csym();

        std::optional<size_t> _target_param_index(str name);


        // these methods populate the current code gen target w/ constants, populating
        // it in a pull-based manner, w/ these methods also doing things like trying
        // to avoid having duplicates

        // the method for type constants DO NOT check if said types actually exist

        const_t _pull_int_c(int_t x);
        const_t _pull_uint_c(uint_t x);
        const_t _pull_float_c(float_t x); // not gonna try to avoid duplicates for floats, to avoid potential issues
        const_t _pull_bool_c(bool_t x);
        const_t _pull_char_c(char_t x);
        const_t _pull_type_c(str fullname);

        const_t _pull_prim_type_c(str fullname);
        const_t _pull_fn_type_c(str fullname);

        template<const_type C>
        inline std::optional<const_t> _find_existing_c(const const_table_info& consts, const const_data_of_t<C>& x) const noexcept;
        callsig_info _build_callsig_for_fn_type(str fullname);


        // when writing bcode, the below is used to gen code_writer label IDs

        using label_id_t = bc::code_writer::label_id_t;

        label_id_t _next_label = 0;

        inline label_id_t _gen_label() noexcept {
            _next_label++;
            return _next_label - 1;
        }


        // this is the code_writer used to write bcode, which'll get applied onto
        // the current code gen target upon the fn decl's visit_end method call

        bc::code_writer cw;


        // this is the bcode symbols we're outputting

        bc::syms syms;

        // _add_sym expects to be called AFTER the cw.add_# method call

        void _add_sym(taul::source_pos pos);


        // TODO: _args_node_to_expr_node_map is super rough, and there's probably a
        //       more *elegant* way of doing what it exists to do

        // this maps Args AST node IDs to the Expr AST node which corresponds to them,
        // w/ this info being used for _add_sym, and deciding the src pos for
        // compile errors

        std::unordered_map<ast_id_t, const ast_Expr*> _args_node_to_expr_node_map;


        // as the AST is traversed, an oprand stack-like 'register stack' is used to
        // perform type checking, and register allocation

        // this is transacted like a stack machine, w/ composite exprs operating on
        // it during their visit_end method call

        // the stack holds two types of registers: 'temporaries' and 'local vars'

        struct _reg_t final {
            str                             type;               // the name of the type encapsulated by the register
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

        void _push_temp(const ast_node& x, str type);
        void _pop_temp(size_t n, bool write_pop_instr, taul::source_pos pop_instr_pos = 0);

        // _reinit_temp is used to change the type of existing temporaries/local vars

        void _reinit_temp(ssize_t index, str new_type); // allows negative indices, like _reg

        void _push_scope();
        void _pop_scope(const ast_Block& x, bool write_pop_instr); // unwinds local vars

        // for local vars, we're gonna have it be that we *promote* temporaries to local vars

        // this updates the symbol table entry w/

        void _promote_to_localvar(ast_VarDecl& x);

        // for type checking, the following is used to check the type expect of a register

        bool _reg_type_check(ssize_t index, str expected); // allows negative indices, like _reg


        // this set is used to record the body blocks of fn decls

        std::unordered_set<ast_id_t> _fn_body_blocks;

        void _mark_as_fn_body_block(const ast_Block& x);
        bool _is_fn_body_block(const ast_Block& x) const noexcept;


        // this set is used to record the 'lvalue' exprs of assignment stmts, in order to
        // signal to them not to be evaluated as normal, as right now all the lvalue portion
        // does is specify a local var to assign to

        std::unordered_set<ast_id_t> _assign_stmt_lvalue_exprs;

        void _mark_as_assign_stmt_lvalue(const ast_Expr& x);
        bool _is_assign_stmt_lvalue(const ast_Expr& x) const noexcept;


        // for ast_Expr and ast_Args which comprise the 'lvalue' expr of assignment stmts,
        // the below suppress counter is used to let the outer-most ast_Expr signal to the
        // ast_Expr(s) and ast_Args(s) nested within it to skip eval

        size_t _suppress = 0;

        inline bool _is_suppressed() const noexcept { return _suppress >= 1; }
        inline void _push_suppress() { _suppress++; }
        inline void _pop_suppress() { YAMA_ASSERT(_is_suppressed()); _suppress--; }


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

    template<typename... Args>
    inline void second_pass::_compile_error(
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

    template<const_type C>
    inline std::optional<const_t> second_pass::_find_existing_c(const const_table_info& consts, const const_data_of_t<C>& x) const noexcept {
        for (const_t i = 0; i < consts.consts.size(); i++) {
            if (const auto c = consts.get<C>(i)) {
                if (c->v == x) {
                    return i;
                }
            }
        }
        return std::nullopt;
    }
}

