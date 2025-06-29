

#include "second_pass.h"

#include "../core/kind-features.h"

#include "compiler.h"
#include "util.h"


using namespace yama::string_literals;


yama::internal::second_pass::second_pass(translation_unit& tu)
    : tu(tu) {}

void yama::internal::second_pass::visit_begin(res<ast_FnDecl> x) {
    if (tu->err.is_fatal()) {
        return;
    }
    _begin_fn_like(*x);
}

void yama::internal::second_pass::visit_begin(res<ast_StructDecl> x) {
    if (tu->err.is_fatal()) {
        return;
    }
    _begin_struct(*x);
}

void yama::internal::second_pass::visit_begin(res<ast_Block> x) {
    if (tu->err.is_fatal()) {
        return;
    }
    _begin_block(*x);
}

void yama::internal::second_pass::visit_begin(res<ast_IfStmt> x) {
    if (tu->err.is_fatal()) {
        return;
    }
    _begin_if_stmt(*x);
}

void yama::internal::second_pass::visit_begin(res<ast_LoopStmt> x) {
    if (tu->err.is_fatal()) {
        return;
    }
    _begin_loop_stmt(*x);
}

void yama::internal::second_pass::visit_begin(res<ast_TypeSpec> x) {
    if (tu->err.is_fatal()) {
        return;
    }
    _type_spec(*x);
}

void yama::internal::second_pass::visit_end(res<ast_VarDecl> x) {
    if (tu->err.is_fatal()) {
        return;
    }
    _localvar(*x);
}

void yama::internal::second_pass::visit_end(res<ast_FnDecl> x) {
    if (tu->err.is_fatal()) {
        return;
    }
    _end_fn_like(*x);
}

void yama::internal::second_pass::visit_end(res<ast_StructDecl> x) {
    if (tu->err.is_fatal()) {
        return;
    }
    _end_struct(*x);
}

void yama::internal::second_pass::visit_end(res<ast_Block> x) {
    if (tu->err.is_fatal()) {
        return;
    }
    _end_block(*x);
}

void yama::internal::second_pass::visit_end(res<ast_ExprStmt> x) {
    if (tu->err.is_fatal()) {
        return;
    }
    _expr_or_assign_stmt(*x);
}

void yama::internal::second_pass::visit_end(res<ast_IfStmt> x) {
    if (tu->err.is_fatal()) {
        return;
    }
    _end_if_stmt(*x);
}

void yama::internal::second_pass::visit_end(res<ast_LoopStmt> x) {
    if (tu->err.is_fatal()) {
        return;
    }
    _end_loop_stmt(*x);
}

void yama::internal::second_pass::visit_end(res<ast_BreakStmt> x) {
    if (tu->err.is_fatal()) {
        return;
    }
    _break_stmt(*x);
}

void yama::internal::second_pass::visit_end(res<ast_ContinueStmt> x) {
    if (tu->err.is_fatal()) {
        return;
    }
    _continue_stmt(*x);
}

void yama::internal::second_pass::visit_end(res<ast_ReturnStmt> x) {
    if (tu->err.is_fatal()) {
        return;
    }
    _return_stmt(*x);
}

void yama::internal::second_pass::_localvar(ast_VarDecl& x) {
    if (x.assign)   _localvar_with_init(x);
    else            _localvar_with_no_init(x);
}

void yama::internal::second_pass::_localvar_with_init(ast_VarDecl& x) {
    // codegen initializer expr
    tu->cs->ea.codegen(*x.assign->expr, newtop);
    if (x.type) { // type annot (so type check initializer w/ it)
        if (const auto type = x.type->type->get_type(*tu->cs)) {
            // if explicit type annot, type check it against temporary of the var decl
            if (!tu->rs.type_check_reg(-1, *type)) {
                tu->err.error(
                    x,
                    dsignal::compile_type_mismatch,
                    "var decl initializer expr is type {0}, but expected {1}!",
                    tu->rs.top_reg().type.fmt(tu->e()),
                    type->fmt(tu->e()));
                return;
            }
        }
        else return;
    }
    // promote our temporary into an actual local var
    tu->rs.promote_to_localvar(x);
}

void yama::internal::second_pass::_localvar_with_no_init(ast_VarDecl& x) {
    if (!x.type) { // no type annot, nor assign
        tu->err.error(
            x,
            dsignal::compile_invalid_local_var,
            "cannot declare local var with no type annotation or initializer!");
        return;
    }
    // lookup type we're initializing w/
    if (const auto type = x.type->type->get_type(*tu->cs)) {
        tu->cgt.autosym(x.low_pos());
        // write bcode which inits our local var w/ default value
        tu->cgt.cw.add_default_init(newtop, uint8_t(tu->ctp.pull_type(*type)));
        // push local var w/ type
        tu->rs.push_temp(x, *type);
        tu->rs.promote_to_localvar(x);
    }
}

void yama::internal::second_pass::_begin_fn_like(ast_FnDecl& x) {
    const str unqualified_name = str(x.fmt_unqualified_name(tu->src).value());
    // gen new codegen target
    tu->cgt.gen_target_fn_like(unqualified_name, x.is_method());
    auto& targsym = tu->cgt.target_csym<fn_like_csym>();
    // resolve if fn type is None returning
    const ctype return_type = targsym.get_return_type_or_none();
    targsym.is_none_returning = return_type == tu->types.none_type();
    // if return type is not None, then control-flow error if not all control paths have
    // explicit return stmts (or enter infinite loops)
    if (!targsym.is_none_returning.value() && !targsym.all_paths_return_or_loop.value()) {
        tu->err.error(
            x,
            dsignal::compile_no_return_stmt,
            "for {}, not all control paths end with a return stmt!",
            unqualified_name);
    }
    // if we're a method, then assert that the owner type exists IN THE SAME MODULE
    if (x.is_method()) {
        const str owner_uqn = x.name.value().str(tu->src);
        const fullname owner_fln = qualified_name(tu->src_path, owner_uqn);
        const bool owner_type_exists = tu->types.load(owner_fln).has_value();
        if (!owner_type_exists) {
            tu->err.error(
                x,
                dsignal::compile_nonexistent_owner,
                "non-existent owner type {}!",
                owner_fln.fmt(tu->e()));
        }
    }
}

void yama::internal::second_pass::_end_fn_like(ast_FnDecl& x) {
    tu->cgt.upload_target(x);
}

void yama::internal::second_pass::_begin_struct(ast_StructDecl& x) {
    const str name = x.name.str(tu->src);
    // gen new codegen target
    tu->cgt.gen_target_struct(name);
}

void yama::internal::second_pass::_end_struct(ast_StructDecl& x) {
    tu->cgt.upload_target(x);
}

void yama::internal::second_pass::_begin_block(ast_Block& x) {
    if (_is_if_stmt_body(x)) { // if we're the if stmt's body block
        _mark_as_if_stmt_cond_reg(-1); // mark as if stmt's cond expr reg
        if (!tu->rs.type_check_reg(-1, tu->types.bool_type())) { // if stmt cond expr
            tu->err.error(
                x,
                dsignal::compile_type_mismatch,
                "if stmt condition expr is type {0}, but expected {1}!",
                tu->rs.top_reg().type.fmt(tu->e()),
                tu->types.bool_type().fmt(tu->e()));
            return;
        }
        // write branch to else-part or exit if cond is false
        const auto selected_label =
            _if_stmt_scope.top().has_else_part
            ? _if_stmt_scope.top().else_part_label
            : _if_stmt_scope.top().exit_label;
        tu->cgt.autosym(x.low_pos());
        tu->cgt.cw.add_jump_false(1, selected_label);
        // pops the Bool cond temporary of the if stmt
        tu->rs.pop_temp(1, false); // jump_false handles popping, so no pop instr
    }
    tu->rs.push_scope();
}

void yama::internal::second_pass::_end_block(ast_Block& x) {
    tu->cgt.autosym(x.high_pos());
    if (x.is_fn_body_block) {
        // if we're None returning, and not all control paths have an explicit return
        // stmt or enter infinite loops, write a ret instr
        const auto& targsym = tu->cgt.target_csym<fn_like_csym>();
        if (targsym.is_none_returning.value() && !targsym.all_paths_return_or_loop.value()) {
            tu->rs.push_temp(x, tu->types.none_type());
            tu->cgt.cw.add_put_none(yama::newtop);
            tu->cgt.cw.add_ret(uint8_t(tu->rs.top_reg().index));
            tu->rs.pop_temp(1, false); // pop instr after ret would just be dead code
        }
    }
    // pop instr after ret would just be dead code, and so too would pop instr
    // if we 100% know block will never exit via *fallthrough* (ie. it'll only
    // exit via break/continue/return)
    tu->rs.pop_scope(!x.is_fn_body_block && !x.will_never_exit_via_fallthrough);
    if (_is_if_stmt_body(x) && _if_stmt_scope.top().has_else_part) {
        // forgo writing jump to exit label if block fallthrough will never happen
        if (!x.will_never_exit_via_fallthrough) {
            // write unconditional branch to the exit label to skip the else-part (if any)
            tu->cgt.cw.add_jump(_if_stmt_scope.top().exit_label);
        }
        // write else-part label
        tu->cgt.cw.add_label(_if_stmt_scope.top().else_part_label);
    }
}

void yama::internal::second_pass::_expr_or_assign_stmt(ast_ExprStmt& x) {
    if (x.assign)   _assign_stmt(x);
    else            _expr_stmt(x);
}

void yama::internal::second_pass::_assign_stmt(ast_ExprStmt& x) {
    YAMA_ASSERT(x.expr);
    const auto expr = x.expr->expect<ast_Expr>();
    const auto primary = res(expr->base)->expect<ast_PrimaryExpr>();
    const auto name = primary->name.value().str(tu->src);
    const auto symbol = tu->syms.lookup_as<var_csym>(x, name, x.low_pos());
    YAMA_ASSERT(symbol); // assignable
    // type check assignment before allowing it
    const ctype assigner_type = tu->cs->ea[*x.assign->expr].type.value();
    const ctype assignee_type = symbol->get_type().value();
    if (assigner_type != assignee_type) {
        tu->err.error(
            x,
            dsignal::compile_type_mismatch,
            "assigning expr is type {0}, but expected {1}!",
            assigner_type.fmt(tu->e()),
            assignee_type.fmt(tu->e()));
        return;
    }
    const size_t assignee_reg = symbol->reg.value();
    tu->cs->ea.codegen(*x.assign->expr, assignee_reg);
}

void yama::internal::second_pass::_expr_stmt(ast_ExprStmt& x) {
    tu->cs->ea.codegen_nr(*x.expr);
}

void yama::internal::second_pass::_begin_if_stmt(ast_IfStmt& x) {
    _if_stmt_scope.push(_mk_if_stmt(x));
    // codegen if stmt cond expr, pushing cond temporary
    tu->cs->ea.codegen(*x.cond, newtop);
}

void yama::internal::second_pass::_end_if_stmt(ast_IfStmt& x) {
    // write exit label
    tu->cgt.cw.add_label(_if_stmt_scope.top().exit_label);
    _if_stmt_scope.pop();
}

void yama::internal::second_pass::_begin_loop_stmt(ast_LoopStmt& x) {
    _loop_stmt_scope.push(_mk_loop_stmt(x));
    // write continue label
    tu->cgt.cw.add_label(_loop_stmt_scope.top().continue_label);
}

void yama::internal::second_pass::_end_loop_stmt(ast_LoopStmt& x) {
    tu->cgt.autosym(x.high_pos());
    // write unconditional branch to continue label
    tu->cgt.cw.add_jump(_loop_stmt_scope.top().continue_label);
    // write break label
    tu->cgt.cw.add_label(_loop_stmt_scope.top().break_label);
    _loop_stmt_scope.pop();
}

void yama::internal::second_pass::_break_stmt(ast_BreakStmt& x) {
    // IMPORTANT: we can't just pop all the registers of the current scope, as
    //            loop stmt blocks can have *multiple levels* of nested scope
    tu->cgt.autosym(x.low_pos());
    // pop all temporaries and local vars to *exit* loop block scope
    const auto total_regs_in_loop_stmt = tu->rs.regs() - _loop_stmt_scope.top().first_reg;
    if (total_regs_in_loop_stmt >= 1) { // don't write pop instr w/ 0 oprand
        tu->cgt.cw.add_pop(uint8_t(total_regs_in_loop_stmt));
    }
    // write unconditional branch to break label
    tu->cgt.cw.add_jump(_loop_stmt_scope.top().break_label);
}

void yama::internal::second_pass::_continue_stmt(ast_ContinueStmt& x) {
    // IMPORTANT: we can't just pop all the registers of the current scope, as
    //            loop stmt blocks can have *multiple levels* of nested scope
    tu->cgt.autosym(x.low_pos());
    // pop all temporaries and local vars to *restart* loop block scope
    const auto total_regs_in_loop_stmt = tu->rs.regs() - _loop_stmt_scope.top().first_reg;
    if (total_regs_in_loop_stmt >= 1) { // don't write pop instr w/ 0 oprand
        tu->cgt.cw.add_pop(uint8_t(total_regs_in_loop_stmt));
    }
    // write unconditional branch to continue label
    tu->cgt.cw.add_jump(_loop_stmt_scope.top().continue_label);
}

void yama::internal::second_pass::_return_stmt(ast_ReturnStmt& x) {
    if (x.expr) _return_stmt_with_val(x);       // form 'return x;'
    else        _return_stmt_with_no_val(x);    // form 'return;'
}

void yama::internal::second_pass::_return_stmt_with_val(ast_ReturnStmt& x) {
    const auto& targsym = tu->cgt.target_csym<fn_like_csym>();
    // codegen returned value expr, pushing temporary
    tu->cs->ea.codegen(*x.expr, newtop);
    // type check return value expr, w/ return type None if no explicit return type
    const ctype return_type = targsym.get_return_type_or_none();
    const ctype returned_value_type = tu->cs->ea[*x.expr].type.value();
    if (returned_value_type != return_type) {
        tu->err.error(
            x,
            dsignal::compile_type_mismatch,
            "return stmt expr is type {0}, but expected {1}!",
            returned_value_type.fmt(tu->e()),
            return_type.fmt(tu->e()));
        return;
    }
    const size_t returned_value_reg = tu->rs.top_reg().index;
    tu->cgt.autosym(x.low_pos());
    tu->cgt.cw.add_ret(uint8_t(returned_value_reg));
    tu->rs.pop_temp(1, false); // consume return value temporary + pop instr after ret would just be dead code
}

void yama::internal::second_pass::_return_stmt_with_no_val(ast_ReturnStmt& x) {
    const auto& targsym = tu->cgt.target_csym<fn_like_csym>();
    // this form may not be used if return type isn't None
    const ctype actual_return_type = targsym.get_return_type_or_none();
    const ctype expected_return_type = tu->types.none_type();
    if (actual_return_type != expected_return_type) {
        tu->err.error(
            x,
            dsignal::compile_type_mismatch,
            "return stmt returns {0} object but return type is {1}!",
            expected_return_type.fmt(tu->e()),
            actual_return_type.fmt(tu->e()));
        return;
    }
    tu->cgt.autosym(x.low_pos());
    tu->rs.push_temp(x, expected_return_type);
    tu->cgt.cw.add_put_none(yama::newtop);
    tu->cgt.cw.add_ret(uint8_t(tu->rs.top_reg().index));
    tu->rs.pop_temp(1, false); // pop instr after ret would just be dead code
}

void yama::internal::second_pass::_type_spec(ast_TypeSpec& x) {
    // expect a type to be able to be discerned
    if (!x.get_type(*tu->cs)) {
        tu->err.error(
            x.low_pos(),
            dsignal::compile_not_a_type,
            "expr does not specify a type!");
    }
}

yama::internal::second_pass::_if_stmt_t yama::internal::second_pass::_mk_if_stmt(const ast_IfStmt& x) {
    return _if_stmt_t{
        .body_block         = x.block->id,
        .has_else_part      = (bool)x.get_else_block_or_stmt(),
        .else_part_label    = tu->cgt.gen_label(),
        .exit_label         = tu->cgt.gen_label(),
    };
}

yama::internal::second_pass::_loop_stmt_t yama::internal::second_pass::_mk_loop_stmt(const ast_LoopStmt& x) {
    return _loop_stmt_t{
        .break_label        = tu->cgt.gen_label(),
        .continue_label     = tu->cgt.gen_label(),
        .first_reg          = tu->rs.regs(),
    };
}

void yama::internal::second_pass::_mark_as_if_stmt_cond_reg(ssize_t index) {
    YAMA_ASSERT(!_if_stmt_scope.top().cond_reg_index);
    _if_stmt_scope.top().cond_reg_index = tu->rs.reg_abs_index(index);
}

bool yama::internal::second_pass::_is_if_stmt_body(const ast_Block& x) const {
    return _if_stmt_scope && _if_stmt_scope.top().body_block == x.id;
}

