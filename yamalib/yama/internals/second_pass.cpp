

#include "second_pass.h"

#include "compiler.h"
#include "util.h"


using namespace yama::string_literals;


yama::internal::second_pass::second_pass(translation_unit& tu)
    : tu(tu) {}

void yama::internal::second_pass::visit_begin(res<ast_FnDecl> x) {
    if (tu->err.is_fatal()) {
        return;
    }
    const str name = x->name.str(tu->src);
    auto& symbol = deref_assert(tu->syms.lookup(*x, name, 0)).as<fn_csym>();
    // begin new fn type for codegen
    tu->cgt.gen_new_target(name);
    // resolve if fn type is None returning
    const ctype return_type = _query_type_or_none(tu->cgt.target_csym().return_type);
    symbol.is_none_returning = return_type == tu->types.none_type();
    // if return type is not None, then control-flow error if not all control paths have
    // explicit return stmts (or enter infinite loops)
    if (!symbol.is_none_returning.value() && !symbol.all_paths_return_or_loop.value()) {
        tu->err.error(
            *x,
            dsignal::compile_no_return_stmt,
            "for {}, not all control paths end with a return stmt!",
            name);
    }
}

void yama::internal::second_pass::visit_begin(res<ast_Block> x) {
    if (tu->err.is_fatal()) {
        return;
    }
    if (_is_if_stmt_body(*x)) { // if we're the if stmt's body block
        _mark_as_if_stmt_cond_reg(-1); // mark as if stmt's cond expr reg
        if (!tu->rs.type_check_reg(-1, tu->types.bool_type())) { // if stmt cond expr
            tu->err.error(
                *x,
                dsignal::compile_type_mismatch,
                "if stmt condition expr is type {0}, but expected {1}!",
                tu->rs.top_reg().type.fmt(tu->e()),
                tu->types.bool_type().fmt(tu->e()));
            return; // abort
        }
        // write branch to else-part or exit if cond is false
        const auto selected_label =
            _if_stmt_scope.top().has_else_part
            ? _if_stmt_scope.top().else_part_label
            : _if_stmt_scope.top().exit_label;
        tu->cgt.cw.add_jump_false(1, selected_label);
        tu->cgt.add_sym(x->low_pos());
        // pops the Bool cond temporary of the if stmt
        tu->rs.pop_temp(1, false); // jump_false handles popping, so no pop instr
    }
    tu->rs.push_scope();
}

void yama::internal::second_pass::visit_begin(res<ast_IfStmt> x) {
    if (tu->err.is_fatal()) {
        return;
    }
    _if_stmt_scope.push(_mk_if_stmt(*x));
}

void yama::internal::second_pass::visit_begin(res<ast_LoopStmt> x) {
    if (tu->err.is_fatal()) {
        return;
    }
    _loop_stmt_scope.push(_mk_loop_stmt(*x));
    // write continue label
    tu->cgt.cw.add_label(_loop_stmt_scope.top().continue_label);
}

void yama::internal::second_pass::visit_begin(res<ast_Expr> x) {
    // IMPORTANT:
    //      expr eval operates as follows:
    //          1) this occurs, when evals the primary expr at the base of the expr,
    //             pushing a temporary for it
    //          2) the subexprs of the first suffix are recursively evaluated, if any,
    //             pushing a temporary for each
    //              * #1 and #2 above occur in visit_begin calls
    //          3) the first suffix is evaluated, consuming the temporaries mentioned
    //             above, and pushing a new one
    //              * this occurs in a visit_end call
    //          4) #2 and #3 above occur for each subsequent suffix, w/ the final
    //             temporary remaining being the temporary of the whole expr
    if (tu->err.is_fatal()) {
        return;
    }
    _lvalue_scope.push_if(x->is_assign_stmt_lvalue);
    // skip if the lvalue
    if (_lvalue_scope) {
        return;
    }
    // skip if in type spec
    if (_type_spec_scope) {
        return;
    }
    YAMA_ASSERT(x->has_const_kw || x->primary);
    if (x->has_const_kw) {
        //
    }
    else if (x->primary) {
        if (x->primary->name) {
            const auto name = x->primary->name->str(tu->src);
            const auto symbol = tu->syms.lookup(*x, name, x->low_pos());
            const bool is_param = symbol && symbol->is<param_csym>();
            const bool is_var = symbol && symbol->is<var_csym>();
            // param, local var, prim or fn
            if (is_param) {
                const ctype param_type = _query_type(symbol->as<param_csym>().type).value();
                tu->rs.push_temp(*x, param_type);
                // IMPORTANT: remember that the indices of the args of a call include the callobj,
                //            so we gotta incr the fn param index to account for this
                // write put_arg loading param into temporary
                tu->cgt.cw.add_put_arg(yama::newtop, uint8_t(tu->cgt.target_param_index(name).value() + 1));
                tu->cgt.add_sym(x->primary->name->low_pos());
            }
            else if (is_var) {
                auto& info = symbol->as<var_csym>();
                // gotta account for the edge case where the code is something like 'var a = a;', in which
                // the var 'a' exists, but it's not been fully setup yet
                if (info.is_localvar()) {
                    const ctype localvar_type = info.deduced_type.value();
                    tu->rs.push_temp(*x, localvar_type);
                    // write copy from local var into temporary
                    tu->cgt.cw.add_copy(uint8_t(info.reg.value()), yama::newtop);
                    tu->cgt.add_sym(x->primary->name->low_pos());
                }
                else {
                    tu->err.error(
                        *x,
                        dsignal::compile_undeclared_name,
                        "undeclared name {}!",
                        name);
                    return; // abort
                }
            }
            // below check is used *instead of* checking if symbol is a prim, fn, etc.
            // as it handles both that and cases of another module defining a type
            else if (const auto type = tu->cs->resolver[x->primary.get()]) {
                if (type->kind() == kind::primitive) {
                    tu->rs.push_temp(*x, tu->types.type_type());
                    // write put_type_const loading type object into temporary
                    tu->cgt.cw.add_put_type_const(yama::newtop, uint8_t(tu->ctp.pull_type(*type)));
                    tu->cgt.add_sym(x->primary->name->low_pos());
                }
                else if (type->kind() == kind::function) {
                    tu->rs.push_temp(*x, *type);
                    // write put_const loading fn object into temporary
                    tu->cgt.cw.add_put_const(yama::newtop, uint8_t(tu->ctp.pull_fn_type(*type)));
                    tu->cgt.add_sym(x->primary->name->low_pos());
                }
                else YAMA_DEADEND;
            }
            else { // found nothing
                tu->err.error(
                    *x,
                    dsignal::compile_undeclared_name,
                    "undeclared name {}!",
                    name);
                return; // abort
            }
        }
        else if (x->primary->lit) {
            if (x->primary->lit->is_int()) {
                const auto lit_nd = res(x->primary->lit->as_int());
                const parsed_int v = parse_int(lit_nd->lit.str(tu->src)).value();
                if (v.overflow) { // overflow error
                    tu->err.error(
                        *x,
                        dsignal::compile_numeric_overflow,
                        "numeric overflow ({})!",
                        lit_nd->lit.str(tu->src));
                    return; // abort
                }
                else if (v.underflow) { // underflow error
                    tu->err.error(
                        *x,
                        dsignal::compile_numeric_underflow,
                        "numeric underflow ({})!",
                        lit_nd->lit.str(tu->src));
                    return; // abort
                }
                else { // valid
                    tu->rs.push_temp(*x, tu->types.int_type());
                    tu->cgt.cw.add_put_const(yama::newtop, uint8_t(tu->ctp.pull_int(v.v)));
                    tu->cgt.add_sym(x->primary->lit->low_pos());
                }
            }
            else if (x->primary->lit->is_uint()) {
                const auto lit_nd = res(x->primary->lit->as_uint());
                const parsed_uint v = parse_uint(lit_nd->lit.str(tu->src)).value();
                if (v.overflow) { // overflow error
                    tu->err.error(
                        *x,
                        dsignal::compile_numeric_overflow,
                        "numeric overflow ({})!",
                        lit_nd->lit.str(tu->src));
                    return; // abort
                }
                else { // valid
                    tu->rs.push_temp(*x, tu->types.uint_type());
                    tu->cgt.cw.add_put_const(yama::newtop, uint8_t(tu->ctp.pull_uint(v.v)));
                    tu->cgt.add_sym(x->primary->lit->low_pos());
                }
            }
            else if (x->primary->lit->is_float()) {
                const auto lit_nd = res(x->primary->lit->as_float());
                const parsed_float v = parse_float(lit_nd->lit.str(tu->src)).value();
                // below can handle cases of overflow/underflow
                tu->rs.push_temp(*x, tu->types.float_type());
                tu->cgt.cw.add_put_const(yama::newtop, uint8_t(tu->ctp.pull_float(v.v)));
                tu->cgt.add_sym(x->primary->lit->low_pos());
            }
            else if (x->primary->lit->is_bool()) {
                const auto lit_nd = res(x->primary->lit->as_bool());
                const parsed_bool v = parse_bool(lit_nd->lit.str(tu->src)).value();
                tu->rs.push_temp(*x, tu->types.bool_type());
                tu->cgt.cw.add_put_const(yama::newtop, uint8_t(tu->ctp.pull_bool(v.v)));
                tu->cgt.add_sym(x->primary->lit->low_pos());
            }
            else if (x->primary->lit->is_char()) {
                const auto lit_nd = res(x->primary->lit->as_char());
                const auto txt = lit_nd->lit.str(tu->src); // <- text w/ single-quotes
                const auto txt_corrected = txt.substr(1, txt.length() - 2); // <- text w/out single-quotes
                const parsed_char v = parse_char(txt_corrected).value();
                if (v.bytes < txt_corrected.length()) { // illegal multi-codepoint char literal error
                    tu->err.error(
                        *x,
                        dsignal::compile_malformed_literal,
                        "illegal multi-codepoint char literal ({})!",
                        fmt_string_literal(txt_corrected));
                    return; // abort
                }
                else if (!taul::is_unicode(v.v)) { // illegal Unicode error
                    tu->err.error(
                        *x,
                        dsignal::compile_illegal_unicode,
                        "illegal Unicode ({})!",
                        fmt_string_literal(txt_corrected));
                    return; // abort
                }
                else { // valid
                    tu->rs.push_temp(*x, tu->types.char_type());
                    tu->cgt.cw.add_put_const(yama::newtop, uint8_t(tu->ctp.pull_char(v.v)));
                    tu->cgt.add_sym(x->primary->lit->low_pos());
                }
            }
            else YAMA_DEADEND;
        }
        else YAMA_DEADEND;
    }
    else YAMA_DEADEND;
}

void yama::internal::second_pass::visit_begin(res<ast_TypeSpec> x) {
    if (tu->err.is_fatal()) {
        return;
    }
    _type_spec_scope.push();
}

void yama::internal::second_pass::visit_end(res<ast_VarDecl> x) {
    if (tu->err.is_fatal()) {
        return;
    }
    if (x->assign) { // initializer expr
        if (x->type) { // type annot (check it)
            if (const auto type = _query_type(x->type->type.get())) {
                // if explicit type annot, type check it against temporary of the var decl
                if (!tu->rs.type_check_reg(-1, *type)) {
                    tu->err.error(
                        *x,
                        dsignal::compile_type_mismatch,
                        "var decl initializer expr is type {0}, but expected {1}!",
                        tu->rs.top_reg().type.fmt(tu->e()),
                        type->fmt(tu->e()));
                    return; // abort
                }
            }
            else return; // abort
        }
        // promote temporary within the var decl into local var
        tu->rs.promote_to_localvar(*x);
    }
    else { // no initializer expr
        if (x->type) { // type annot (so default init)
            // lookup type we're initializing w/
            if (const auto type = _query_type(x->type->type.get())) {
                // push local var w/ type
                tu->rs.push_temp(*x, *type);
                tu->rs.promote_to_localvar(*x);
                static_assert(kinds == 2);
                if (type->kind() == kind::primitive) { // primitive type
                    // write bcode which inits our local var w/ default value
                    if (*type == tu->types.none_type())         tu->cgt.cw.add_put_none(yama::newtop);
                    else if (*type == tu->types.int_type())     tu->cgt.cw.add_put_const(yama::newtop, uint8_t(tu->ctp.pull_int(0)));
                    else if (*type == tu->types.uint_type())    tu->cgt.cw.add_put_const(yama::newtop, uint8_t(tu->ctp.pull_uint(0u)));
                    else if (*type == tu->types.float_type())   tu->cgt.cw.add_put_const(yama::newtop, uint8_t(tu->ctp.pull_float(0.0)));
                    else if (*type == tu->types.bool_type())    tu->cgt.cw.add_put_const(yama::newtop, uint8_t(tu->ctp.pull_bool(false)));
                    else if (*type == tu->types.char_type())    tu->cgt.cw.add_put_const(yama::newtop, uint8_t(tu->ctp.pull_char(U'\0')));
                    else if (*type == tu->types.type_type())    tu->cgt.cw.add_put_type_const(yama::newtop, uint8_t(tu->ctp.pull_type(tu->types.none_type())));
                    else                                        YAMA_DEADEND;
                    tu->cgt.add_sym(x->low_pos()); // add sym for instr
                }
                else if (type->kind() == kind::function) { // fn type
                    // write bcode which inits our local var w/ default value
                    tu->cgt.cw.add_put_const(yama::newtop, uint8_t(tu->ctp.pull_fn_type(*type)));
                    tu->cgt.add_sym(x->low_pos());
                }
                else YAMA_DEADEND;
            }
            else return; // abort
        }
        else { // no type annot, nor assign
            tu->err.error(
                *x,
                dsignal::compile_invalid_local_var,
                "cannot declare local var with no type annotation or initializer!");
            return; // abort
        }
    }
}

void yama::internal::second_pass::visit_end(res<ast_FnDecl> x) {
    if (tu->err.is_fatal()) {
        return;
    }
    tu->cgt.upload_target(*x);
}

void yama::internal::second_pass::visit_end(res<ast_Block> x) {
    if (tu->err.is_fatal()) {
        return;
    }
    if (x->is_fn_body_block) {
        // if we're None returning, and not all control paths have an explicit return
        // stmt or enter infinite loops, write a ret instr
        if (tu->cgt.target_csym().is_none_returning.value() && !tu->cgt.target_csym().all_paths_return_or_loop.value()) {
            tu->rs.push_temp(*x, tu->types.none_type());
            tu->cgt.cw.add_put_none(yama::newtop);
            tu->cgt.add_sym(x->high_pos());
            tu->cgt.cw.add_ret(uint8_t(tu->rs.top_reg().index));
            tu->cgt.add_sym(x->high_pos());
            tu->rs.pop_temp(1, false); // pop instr after ret would just be dead code
        }
    }
    // pop instr after ret would just be dead code, and so too would pop instr
    // if we 100% know block will never exit via *fallthrough* (ie. it'll only
    // exit via break/continue/return)
    tu->rs.pop_scope(*x, !x->is_fn_body_block && !x->will_never_exit_via_fallthrough);
    if (_is_if_stmt_body(*x) && _if_stmt_scope.top().has_else_part) {
        // write unconditional branch to the exit label to skip the else-part (if any)
        tu->cgt.cw.add_jump(_if_stmt_scope.top().exit_label);
        tu->cgt.add_sym(x->high_pos());
        // write else-part label
        tu->cgt.cw.add_label(_if_stmt_scope.top().else_part_label);
    }
}

void yama::internal::second_pass::visit_end(res<ast_ExprStmt> x) {
    if (tu->err.is_fatal()) {
        return;
    }
    if (x->assign) { // assignment stmt
        YAMA_ASSERT(x->expr);
        if (x->expr->has_const_kw) {
            if (x->expr->is_assign_stmt_lvalue) {
                tu->err.error(
                    *x,
                    dsignal::compile_nonassignable_expr,
                    "non-assignable expr!");
                return; // abort
            }
        }
        else if (x->expr->primary) {
            if (x->expr->primary->name) {
                const auto name = x->expr->primary->name->str(tu->src);
                const auto symbol = tu->syms.lookup(*x, name, x->low_pos());
                const bool is_param = symbol && symbol->is<param_csym>();
                const bool is_var = symbol && symbol->is<var_csym>();
                if (is_param) { // non-assignable
                    tu->err.error(
                        *x,
                        dsignal::compile_nonassignable_expr,
                        "non-assignable expr!");
                    return; // abort
                }
                else if (is_var) { // assignable
                    const auto& info = symbol->as<var_csym>();
                    // type check assignment before allowing it
                    if (!tu->rs.type_check_reg(-1, info.deduced_type.value())) {
                        tu->err.error(
                            *x,
                            dsignal::compile_type_mismatch,
                            "assigning expr is type {0}, but expected {1}!",
                            tu->rs.top_reg().type.fmt(tu->e()),
                            info.deduced_type.value().fmt(tu->e()));
                        return; // abort
                    }
                    else {
                        // write copy of temporary assigned w/ onto local var, performing the assignment
                        tu->cgt.cw.add_copy(uint8_t(tu->rs.top_reg().index), uint8_t(info.reg.value()));
                        tu->cgt.add_sym(x->low_pos());
                        tu->rs.pop_temp(1, true, x->low_pos()); // consume temporary assigned w/
                    }
                }
                // below check is used *instead of* checking if symbol is a prim, fn, etc.
                // as it handles both that and cases of another module defining a type
                else if (const auto type = tu->cs->resolver[x->expr->primary.get()]) { // non-assignable
                    tu->err.error(
                        *x,
                        dsignal::compile_nonassignable_expr,
                        "non-assignable expr!");
                    return; // abort
                }
                else { // nothing found
                    tu->err.error(
                        *x,
                        dsignal::compile_undeclared_name,
                        "undeclared name {}!",
                        name);
                    return; // abort
                }
            }
            else { // lvalue isn't even an identifier expr, so non-assignable
                tu->err.error(
                    *x,
                    dsignal::compile_nonassignable_expr,
                    "non-assignable expr!");
                return; // abort
            }
        }
        else YAMA_DEADEND;
    }
    else { // expr stmt
        // consume temporary from our expr
        tu->rs.pop_temp(1, true, x->low_pos());
    }
}

void yama::internal::second_pass::visit_end(res<ast_IfStmt> x) {
    if (tu->err.is_fatal()) {
        return;
    }
    // write exit label
    tu->cgt.cw.add_label(_if_stmt_scope.top().exit_label);
    _if_stmt_scope.pop();
}

void yama::internal::second_pass::visit_end(res<ast_LoopStmt> x) {
    if (tu->err.is_fatal()) {
        return;
    }
    // write unconditional branch to continue label
    tu->cgt.cw.add_jump(_loop_stmt_scope.top().continue_label);
    tu->cgt.add_sym(x->high_pos());
    // write break label
    tu->cgt.cw.add_label(_loop_stmt_scope.top().break_label);
    _loop_stmt_scope.pop();
}

void yama::internal::second_pass::visit_end(res<ast_BreakStmt> x) {
    if (tu->err.is_fatal()) {
        return;
    }
    // IMPORTANT: we can't just pop all the registers of the current scope, as
    //            loop stmt blocks can have *multiple levels* of nested scope
    // pop all temporaries and local vars to *exit* loop block scope
    const auto total_regs_in_loop_stmt = tu->rs.regs() - _loop_stmt_scope.top().first_reg;
    if (total_regs_in_loop_stmt >= 1) { // don't write pop instr w/ 0 oprand
        tu->cgt.cw.add_pop(uint8_t(total_regs_in_loop_stmt));
        tu->cgt.add_sym(x->low_pos());
    }
    // write unconditional branch to break label
    tu->cgt.cw.add_jump(_loop_stmt_scope.top().break_label);
    tu->cgt.add_sym(x->low_pos());
}

void yama::internal::second_pass::visit_end(res<ast_ContinueStmt> x) {
    if (tu->err.is_fatal()) {
        return;
    }
    // IMPORTANT: we can't just pop all the registers of the current scope, as
    //            loop stmt blocks can have *multiple levels* of nested scope
    // pop all temporaries and local vars to *restart* loop block scope
    const auto total_regs_in_loop_stmt = tu->rs.regs() - _loop_stmt_scope.top().first_reg;
    if (total_regs_in_loop_stmt >= 1) { // don't write pop instr w/ 0 oprand
        tu->cgt.cw.add_pop(uint8_t(total_regs_in_loop_stmt));
        tu->cgt.add_sym(x->low_pos());
    }
    // write unconditional branch to continue label
    tu->cgt.cw.add_jump(_loop_stmt_scope.top().continue_label);
    tu->cgt.add_sym(x->low_pos());
}

void yama::internal::second_pass::visit_end(res<ast_ReturnStmt> x) {
    if (tu->err.is_fatal()) {
        return;
    }
    if (x->expr) { // form 'return x;'
        // type check return value expr, w/ return type None if no explicit return type
        const ctype return_type = _query_type_or_none(tu->cgt.target_csym().return_type);
        if (!tu->rs.type_check_reg(-1, return_type)) {
            tu->err.error(
                *x,
                dsignal::compile_type_mismatch,
                "return stmt expr is type {0}, but expected {1}!",
                tu->rs.top_reg().type.fmt(tu->e()),
                return_type.fmt(tu->e()));
            return; // abort
        }
        tu->cgt.cw.add_ret(uint8_t(tu->rs.top_reg().index));
        tu->cgt.add_sym(x->low_pos());
        tu->rs.pop_temp(1, false); // consume return value temporary + pop instr after ret would just be dead code
    }
    else { // form 'return;'
        // this form may not be used if return type isn't None
        const ctype return_type = _query_type_or_none(tu->cgt.target_csym().return_type);
        const ctype expected_return_type = tu->types.none_type();
        if (return_type != expected_return_type) {
            tu->err.error(
                *x,
                dsignal::compile_type_mismatch,
                "return stmt returns {} object but return type is {}!",
                expected_return_type.fmt(tu->e()),
                return_type.fmt(tu->e()));
            return; // abort
        }
        tu->rs.push_temp(*x, expected_return_type);
        tu->cgt.cw.add_put_none(yama::newtop);
        tu->cgt.add_sym(x->low_pos());
        tu->cgt.cw.add_ret(uint8_t(tu->rs.top_reg().index));
        tu->cgt.add_sym(x->low_pos());
        tu->rs.pop_temp(1, false); // pop instr after ret would just be dead code
    }
}

void yama::internal::second_pass::visit_end(res<ast_Expr> x) {
    if (tu->err.is_fatal()) {
        return;
    }
    _lvalue_scope.pop_if(x->is_assign_stmt_lvalue);
}

void yama::internal::second_pass::visit_end(res<ast_Args> x) {
    if (tu->err.is_fatal()) {
        return;
    }
    // skip if the lvalue
    if (_lvalue_scope) {
        return;
    }
    // skip if in type spec
    if (_type_spec_scope) {
        return;
    }
    // skip below call expr stuff if we're args of a constexpr guarantee expr
    if (x->is_constexpr_guarantee_expr_args()) {
        return;
    }
    // on the register stack will be the temporaries corresponding to the callobj
    // and args of the call this code will resolve
    const size_t arg_count = x->args.size();
    const size_t callobj_reg_index = tu->rs.regs() - arg_count - 1;
    const ctype callobj_type = tu->rs.reg_abs(callobj_reg_index).type;
    // whatever the callobj is, it's not a callable type
    if (callobj_type.kind() != kind::function) {
        tu->err.error(
            *x->get_expr(),
            dsignal::compile_invalid_operation,
            "cannot call non-callable type {}!",
            callobj_type.fmt(tu->e()));
        return; // abort
    }
    // check that arg count is correct
    if (arg_count != callobj_type.param_count()) {
        tu->err.error(
            *x->get_expr(),
            dsignal::compile_wrong_arg_count,
            "call to {} given {} args, but expected {}!",
            callobj_type.fmt(tu->e()),
            arg_count,
            callobj_type.param_count());
        return; // abort
    }
    // type check the args
    bool args_are_correct_types = true;
    for (size_t i = 0; i < arg_count; i++) {
        const auto arg_reg_index = callobj_reg_index + i + 1;
        const auto arg_number = i + 1;
        const ctype param_type = callobj_type.param_type(i, tu->cs->solver).value();
        if (!tu->rs.type_check_reg(arg_reg_index, param_type)) {
            tu->err.error(
                *x->args[i],
                dsignal::compile_type_mismatch,
                "arg {0} expr is type {1}, but expected {2}!",
                arg_number,
                tu->rs.reg(arg_reg_index).type.fmt(tu->e()),
                param_type.fmt(tu->e()));
            args_are_correct_types = false;
        }
    }
    if (!args_are_correct_types) { // don't return until we've type checked ALL args
        return; // abort
    }
    const taul::source_pos expr_pos = x->get_expr()->low_pos();
    // return value should overwrite callobj
    const size_t return_value_reg_index = callobj_reg_index;
    // write the call instr
    tu->cgt.cw.add_call(uint8_t(arg_count + 1), yama::newtop);
    tu->cgt.add_sym(expr_pos);
    // reinit our return value (formerly callobj) register to the correct type
    const auto return_type = tu->types.default_none(callobj_type.return_type(tu->cs->solver));
    tu->rs.reinit_temp(return_value_reg_index, return_type);
    // pop args, but NOT callobj, as that register is reused for our return value
    tu->rs.pop_temp(arg_count, false);
}

void yama::internal::second_pass::visit_end(res<ast_TypeSpec> x) {
    if (tu->err.is_fatal()) {
        return;
    }
    _type_spec_scope.pop();
    // if _query_type fails, even after solving, then that means that
    // the expr of x is not a valid type specifying expr
    if (!_query_type(x.get())) {
        tu->err.error(
            x->low_pos(),
            dsignal::compile_not_a_type,
            "expr does not specify a type!");
        return; // abort
    }
}

std::optional<yama::internal::ctype> yama::internal::second_pass::_query_type(const ast_TypeSpec* x) {
    if (!x) {
        return std::nullopt;
    }
    if (const auto result = tu->cs->solver[x->expr.get()]; result && result->to_type()) {
        return result->to_type();
    }
    if (!x->expr->primary) {
        return std::nullopt;
    }
    if (const auto result = tu->cs->solver[x->expr->primary.get()]; result && result->to_type()) {
        return result->to_type();
    }
    return std::nullopt;
}

yama::internal::ctype yama::internal::second_pass::_query_type_or_none(const ast_TypeSpec* x) {
    return tu->types.default_none(_query_type(x));
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

