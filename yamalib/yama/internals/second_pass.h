

#pragma once


#include "safeptr.h"
#include "scope_counter.h"
#include "scope_stack.h"
#include "ast.h"
#include "codegen_target.h"


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


        static_assert(ast_types == 35); // reminder

        //void visit_begin(res<ast_Chunk> x) override final;
        //void visit_begin(res<ast_Decl> x) override final;
        //void visit_begin(res<ast_ImportDecl> x) override final;
        //void visit_begin(res<ast_RelativePath> x) override final;
        //void visit_begin(res<ast_VarDecl> x) override final;
        void visit_begin(res<ast_FnDecl> x) override final;
        void visit_begin(res<ast_StructDecl> x) override final;
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
        //void visit_begin(res<ast_Expr> x) override final;
        //void visit_begin(res<ast_PrimaryExpr> x) override final;
        //void visit_begin(res<ast_ParenthesizedExpr> x) override final;
        //void visit_begin(res<ast_ConstexprGuaranteeExpr> x) override final;
        //void visit_begin(res<ast_Lit> x) override final;
        //void visit_begin(res<ast_IntLit> x) override final;
        //void visit_begin(res<ast_UIntLit> x) override final;
        //void visit_begin(res<ast_FloatLit> x) override final;
        //void visit_begin(res<ast_BoolLit> x) override final;
        //void visit_begin(res<ast_CharLit> x) override final;
        //void visit_begin(res<ast_Assign> x) override final;
        //void visit_begin(res<ast_Args> x) override final;
        //void visit_begin(res<ast_Conv> x) override final;
        //void visit_begin(res<ast_TypeMember> x) override final;
        //void visit_begin(res<ast_ObjectMember> x) override final;
        //void visit_begin(res<ast_TypeAnnot> x) override final;
        void visit_begin(res<ast_TypeSpec> x) override final;

        static_assert(ast_types == 35); // reminder

        //void visit_end(res<ast_Chunk> x) override final;
        //void visit_end(res<ast_Decl> x) override final;
        //void visit_end(res<ast_ImportDecl> x) override final;
        //void visit_end(res<ast_RelativePath> x) override final;
        void visit_end(res<ast_VarDecl> x) override final;
        void visit_end(res<ast_FnDecl> x) override final;
        void visit_end(res<ast_StructDecl> x) override final;
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
        //void visit_end(res<ast_Expr> x) override final;
        //void visit_end(res<ast_PrimaryExpr> x) override final;
        //void visit_end(res<ast_ParenthesizedExpr> x) override final;
        //void visit_end(res<ast_ConstexprGuaranteeExpr> x) override final;
        //void visit_end(res<ast_Lit> x) override final;
        //void visit_end(res<ast_IntLit> x) override final;
        //void visit_end(res<ast_UIntLit> x) override final;
        //void visit_end(res<ast_FloatLit> x) override final;
        //void visit_end(res<ast_BoolLit> x) override final;
        //void visit_end(res<ast_CharLit> x) override final;
        //void visit_end(res<ast_Assign> x) override final;
        //void visit_end(res<ast_Args> x) override final;
        //void visit_end(res<ast_Conv> x) override final;
        //void visit_end(res<ast_TypeMember> x) override final;
        //void visit_end(res<ast_ObjectMember> x) override final;
        //void visit_end(res<ast_TypeAnnot> x) override final;
        //void visit_end(res<ast_TypeSpec> x) override final;


    private:
        // IMPORTANT: if fatal error, the compilation is not to perform further codegen


        void _localvar(ast_VarDecl& x);
        void _localvar_with_init(ast_VarDecl& x);
        void _localvar_with_no_init(ast_VarDecl& x);

        void _begin_fn_like(ast_FnDecl& x);
        void _end_fn_like(ast_FnDecl& x);

        void _begin_struct(ast_StructDecl& x);
        void _end_struct(ast_StructDecl& x);

        void _begin_block(ast_Block& x);
        void _end_block(ast_Block& x);

        void _expr_or_assign_stmt(ast_ExprStmt& x);
        void _assign_stmt(ast_ExprStmt& x);
        void _expr_stmt(ast_ExprStmt& x);

        void _begin_if_stmt(ast_IfStmt& x);
        void _end_if_stmt(ast_IfStmt& x);

        void _begin_loop_stmt(ast_LoopStmt& x);
        void _end_loop_stmt(ast_LoopStmt& x);

        void _break_stmt(ast_BreakStmt& x);
        void _continue_stmt(ast_ContinueStmt& x);

        void _return_stmt(ast_ReturnStmt& x);
        void _return_stmt_with_val(ast_ReturnStmt& x);
        void _return_stmt_with_no_val(ast_ReturnStmt& x);

        void _type_spec(ast_TypeSpec& x);


        struct _if_stmt_t final {
            // TODO: cond_reg_index is currently unused, so maybe remove it?
            std::optional<size_t> cond_reg_index = std::nullopt;
            ast_id_t body_block;
            bool has_else_part;
            label_id_t else_part_label, exit_label;
        };

        struct _loop_stmt_t final {
            label_id_t break_label, continue_label;
            size_t first_reg; // the index where the first reg in the loop body's scope will be located on the stack
        };

        scope_stack<_if_stmt_t> _if_stmt_scope;
        scope_stack<_loop_stmt_t> _loop_stmt_scope;

        _if_stmt_t _mk_if_stmt(const ast_IfStmt& x);
        _loop_stmt_t _mk_loop_stmt(const ast_LoopStmt& x);
        
        void _mark_as_if_stmt_cond_reg(ssize_t index);
        bool _is_if_stmt_body(const ast_Block& x) const;
    };
}

