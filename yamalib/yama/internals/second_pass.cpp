

#include "second_pass.h"

#include "compiler.h"


using namespace yama::string_literals;


#define _LOG_REG_AND_SCOPE_STK_MANIP 0


yama::internal::second_pass::second_pass(translation_unit& tu)
    : tu(tu) {}

void yama::internal::second_pass::visit_begin(res<ast_FnDecl> x) {
    if (tu->er.is_fatal()) {
        return;
    }
    const str name = x->name.str(tu->src);
    auto& symbol = deref_assert(tu->syms.lookup(*x, name, 0)).as<fn_csym>();
    // begin new fn type for code gen
    _begin_target(name);
    // resolve if fn type is None returning
    const ctype return_type = tu->types.default_none(tu->cs->resolver[_target_csym().return_type]);
    symbol.is_none_returning = return_type == tu->types.none_type();
    // if return type is not None, then control-flow error if not all control paths have
    // explicit return stmts (or enter infinite loops)
    if (!symbol.is_none_returning.value() && !symbol.all_paths_return_or_loop.value()) {
        tu->er.error(
            *x,
            dsignal::compile_no_return_stmt,
            "for {}, not all control paths end with a return stmt!",
            name);
    }
}

void yama::internal::second_pass::visit_begin(res<ast_Block> x) {
    if (tu->er.is_fatal()) {
        return;
    }
    if (_is_if_stmt_body(*x)) { // if we're the if stmt's body block
        _mark_as_if_stmt_cond_reg(-1); // mark as if stmt's cond expr reg
        if (!_reg_type_check(-1, tu->types.bool_type())) { // if stmt cond expr
            tu->er.error(
                *x,
                dsignal::compile_type_mismatch,
                "if stmt condition expr is type {0}, but expected {1}!",
                _top_reg().type.fmt(tu->e()),
                tu->types.bool_type().fmt(tu->e()));
            return; // abort
        }
        // write branch to else-part or exit if cond is false
        const auto selected_label =
            _top_if_stmt().has_else_part
            ? _top_if_stmt().else_part_label
            : _top_if_stmt().exit_label;
        cw.add_jump_false(1, selected_label);
        _add_sym(x->low_pos());
        // pops the Bool cond temporary of the if stmt
        _pop_temp(1, false); // jump_false handles popping, so no pop instr
    }
    _push_scope();
}

void yama::internal::second_pass::visit_begin(res<ast_IfStmt> x) {
    if (tu->er.is_fatal()) {
        return;
    }
    _push_if_stmt(*x);
}

void yama::internal::second_pass::visit_begin(res<ast_LoopStmt> x) {
    if (tu->er.is_fatal()) {
        return;
    }
    _push_loop_stmt(*x);
    // write continue label
    cw.add_label(_top_loop_stmt().continue_label);
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
    if (tu->er.is_fatal()) {
        return;
    }
    if (x->is_assign_stmt_lvalue) _push_lvalue();
    // skip if the lvalue
    if (_is_in_lvalue()) {
        return;
    }
    YAMA_ASSERT(x->primary);
    if (x->primary->name) {
        const auto name = x->primary->name->str(tu->src);
        // param, local var, or fn
        if (const auto& symbol = tu->syms.lookup(*x, name, x->low_pos())) {
            if (symbol->is<param_csym>()) {
                _push_temp(*x, tu->cs->resolver[symbol->as<param_csym>().type].value());
                // IMPORTANT: remember that the indices of the args of an ll_call include the callobj,
                //            so we gotta incr the fn param index to account for this
                // write put_arg loading param into temporary
                cw.add_put_arg(yama::newtop, uint8_t(_target_param_index(name).value() + 1));
                _add_sym(x->primary->name->low_pos());
            }
            else if (symbol->is<var_csym>()) {
                auto& info = symbol->as<var_csym>();
                // gotta account for the edge case where the code is something like 'var a = a;', in which
                // the var 'a' exists, but it's not been fully setup yet
                if (info.is_localvar()) {
                    _push_temp(*x, info.deduced_type.value());
                    // write copy from local var into temporary
                    cw.add_copy(uint8_t(info.reg.value()), yama::newtop);
                    _add_sym(x->primary->name->low_pos());
                }
                else {
                    tu->er.error(
                        *x,
                        dsignal::compile_undeclared_name,
                        "undeclared name {}!",
                        name);
                    return; // abort
                }
            }
            else if (symbol->is<fn_csym>()) {
                if (const auto type = tu->cs->resolver[x->primary.get()]) {
                    _push_temp(*x, *type);
                    // write put_const loading fn object into temporary
                    cw.add_put_const(yama::newtop, uint8_t(tu->ctp.pull_fn_type(*type)));
                    _add_sym(x->primary->name->low_pos());
                }
                else return; // abort
            }
            else { // not a param, local var, or fn
                tu->er.error(
                    *x,
                    dsignal::compile_not_an_expr,
                    "invalid expr!",
                    name);
                return; // abort
            }
        }
        // TODO: below code is a duplicate of the fn_csym code in the 'true' branch path above
        // if lookup for symbol failed, then try resolving a fn ctype
        else if (const auto type = tu->cs->resolver[x->primary.get()]; type && type->kind() == kind::function) {
            _push_temp(*x, *type);
            // write put_const loading fn object into temporary
            cw.add_put_const(yama::newtop, uint8_t(tu->ctp.pull_fn_type(*type)));
            _add_sym(x->primary->name->low_pos());
        }
        else { // not a param, local var, or fn
            tu->er.error(
                *x,
                dsignal::compile_not_an_expr,
                "invalid expr!",
                name);
            return; // abort
        }
    }
    else if (x->primary->lit) {
        if (x->primary->lit->is_int()) {
            const auto lit_nd = res(x->primary->lit->as_int());
            const parsed_int v = parse_int(lit_nd->lit.str(tu->src)).value();
            if (v.overflow) { // overflow error
                tu->er.error(
                    *x,
                    dsignal::compile_numeric_overflow,
                    "numeric overflow ({})!",
                    lit_nd->lit.str(tu->src));
                return; // abort
            }
            else if (v.underflow) { // underflow error
                tu->er.error(
                    *x,
                    dsignal::compile_numeric_underflow,
                    "numeric underflow ({})!",
                    lit_nd->lit.str(tu->src));
                return; // abort
            }
            else { // valid
                _push_temp(*x, tu->types.int_type());
                cw.add_put_const(yama::newtop, uint8_t(tu->ctp.pull_int(v.v)));
                _add_sym(x->primary->lit->low_pos());
            }
        }
        else if (x->primary->lit->is_uint()) {
            const auto lit_nd = res(x->primary->lit->as_uint());
            const parsed_uint v = parse_uint(lit_nd->lit.str(tu->src)).value();
            if (v.overflow) { // overflow error
                tu->er.error(
                    *x,
                    dsignal::compile_numeric_overflow,
                    "numeric overflow ({})!",
                    lit_nd->lit.str(tu->src));
                return; // abort
            }
            else { // valid
                _push_temp(*x, tu->types.uint_type());
                cw.add_put_const(yama::newtop, uint8_t(tu->ctp.pull_uint(v.v)));
                _add_sym(x->primary->lit->low_pos());
            }
        }
        else if (x->primary->lit->is_float()) {
            const auto lit_nd = res(x->primary->lit->as_float());
            const parsed_float v = parse_float(lit_nd->lit.str(tu->src)).value();
            // below can handle cases of overflow/underflow
            _push_temp(*x, tu->types.float_type());
            cw.add_put_const(yama::newtop, uint8_t(tu->ctp.pull_float(v.v)));
            _add_sym(x->primary->lit->low_pos());
        }
        else if (x->primary->lit->is_bool()) {
            const auto lit_nd = res(x->primary->lit->as_bool());
            const parsed_bool v = parse_bool(lit_nd->lit.str(tu->src)).value();
            _push_temp(*x, tu->types.bool_type());
            cw.add_put_const(yama::newtop, uint8_t(tu->ctp.pull_bool(v.v)));
            _add_sym(x->primary->lit->low_pos());
        }
        else if (x->primary->lit->is_char()) {
            const auto lit_nd = res(x->primary->lit->as_char());
            const auto txt = lit_nd->lit.str(tu->src); // <- text w/ single-quotes
            const auto txt_corrected = txt.substr(1, txt.length() - 2); // <- text w/out single-quotes
            const parsed_char v = parse_char(txt_corrected).value();
            if (!taul::is_unicode(v.v)) { // illegal Unicode error
                tu->er.error(
                    *x,
                    dsignal::compile_illegal_unicode,
                    "illegal Unicode ({})!",
                    lit_nd->lit.str(tu->src));
                return; // abort
            }
            else { // valid
                _push_temp(*x, tu->types.char_type());
                cw.add_put_const(yama::newtop, uint8_t(tu->ctp.pull_char(v.v)));
                _add_sym(x->primary->lit->low_pos());
            }
        }
        else YAMA_DEADEND;
    }
    else YAMA_DEADEND;
}

void yama::internal::second_pass::visit_end(res<ast_VarDecl> x) {
    if (tu->er.is_fatal()) {
        return;
    }
    if (x->assign) { // initializer expr
        if (x->type) { // type annot (check it)
            if (const auto type = tu->cs->resolver[x->type->type.get()]) {
                // if explicit type annot, type check it against temporary of the var decl
                if (!_reg_type_check(-1, *type)) {
                    tu->er.error(
                        *x,
                        dsignal::compile_type_mismatch,
                        "var decl initializer expr is type {0}, but expected {1}!",
                        _top_reg().type.fmt(tu->e()),
                        type->fmt(tu->e()));
                    return; // abort
                }
            }
            else return; // abort
        }
        // promote temporary within the var decl into local var
        _promote_to_localvar(*x);
    }
    else { // no initializer expr
        if (x->type) { // type annot (so default init)
            // lookup type we're initializing w/
            if (const auto type = tu->cs->resolver[x->type->type.get()]) {
                // push local var w/ type
                _push_temp(*x, *type);
                _promote_to_localvar(*x);
                static_assert(kinds == 2);
                if (type->kind() == kind::primitive) { // primitive type
                    // write bcode which inits our local var w/ default value
                    if (*type == tu->types.none_type())        cw.add_put_none(yama::newtop);
                    else if (*type == tu->types.int_type())    cw.add_put_const(yama::newtop, uint8_t(tu->ctp.pull_int(0)));
                    else if (*type == tu->types.uint_type())   cw.add_put_const(yama::newtop, uint8_t(tu->ctp.pull_uint(0u)));
                    else if (*type == tu->types.float_type())  cw.add_put_const(yama::newtop, uint8_t(tu->ctp.pull_float(0.0)));
                    else if (*type == tu->types.bool_type())   cw.add_put_const(yama::newtop, uint8_t(tu->ctp.pull_bool(false)));
                    else if (*type == tu->types.char_type())   cw.add_put_const(yama::newtop, uint8_t(tu->ctp.pull_char(U'\0')));
                    else                                        YAMA_DEADEND;
                    _add_sym(x->low_pos()); // add sym for instr
                }
                else if (type->kind() == kind::function) { // fn type
                    // write bcode which inits our local var w/ default value
                    cw.add_put_const(yama::newtop, uint8_t(tu->ctp.pull_fn_type(*type)));
                    _add_sym(x->low_pos());
                }
                else YAMA_DEADEND;
            }
            else return; // abort
        }
        else { // no type annot, nor assign
            tu->er.error(
                *x,
                dsignal::compile_invalid_local_var,
                "cannot declare local var with no type annotation or initializer!");
            return; // abort
        }
    }
}

void yama::internal::second_pass::visit_end(res<ast_FnDecl> x) {
    if (tu->er.is_fatal()) {
        return;
    }
    _apply_bcode_to_target(*x);
    _end_target();
}

void yama::internal::second_pass::visit_end(res<ast_Block> x) {
    if (tu->er.is_fatal()) {
        return;
    }
    if (x->is_fn_body_block) {
        // if we're None returning, and not all control paths have an explicit return
        // stmt or enter infinite loops, write a ret instr
        if (_target_csym().is_none_returning.value() && !_target_csym().all_paths_return_or_loop.value()) {
            _push_temp(*x, tu->types.none_type());
            cw.add_put_none(yama::newtop);
            _add_sym(x->high_pos());
            cw.add_ret(uint8_t(_top_reg().index));
            _add_sym(x->high_pos());
            _pop_temp(1, false); // pop instr after ret would just be dead code
        }
    }
    _pop_scope(*x, !x->is_fn_body_block); // pop instr after ret would just be dead code
    if (_is_if_stmt_body(*x) && _top_if_stmt().has_else_part) {
        // write unconditional branch to the exit label to skip the else-part (if any)
        cw.add_jump(_top_if_stmt().exit_label);
        _add_sym(x->high_pos());
        // write else-part label
        cw.add_label(_top_if_stmt().else_part_label);
    }
}

void yama::internal::second_pass::visit_end(res<ast_ExprStmt> x) {
    if (tu->er.is_fatal()) {
        return;
    }
    if (x->assign) { // assignment stmt
        YAMA_ASSERT(x->expr && x->expr->primary);
        if (x->expr->primary->name) {
            const auto name = x->expr->primary->name->str(tu->src);
            if (const auto& symbol = tu->syms.lookup(*x, name, x->low_pos())) {
                if (symbol->is<var_csym>()) {
                    const auto& info = symbol->as<var_csym>();
                    // type check assignment before allowing it
                    if (!_reg_type_check(-1, info.deduced_type.value())) {
                        tu->er.error(
                            *x,
                            dsignal::compile_type_mismatch,
                            "assigning expr is type {0}, but expected {1}!",
                            _top_reg().type.fmt(tu->e()),
                            info.deduced_type.value().fmt(tu->e()));
                        return; // abort
                    }
                    else {
                        // write copy of temporary assigned w/ onto local var, performing the assignment
                        cw.add_copy(uint8_t(_top_reg().index), uint8_t(info.reg.value()));
                        _add_sym(x->low_pos());
                        _pop_temp(1, true, x->low_pos()); // consume temporary assigned w/
                    }
                }
                else if (symbol->is<param_csym>()) { // non-assignable
                    tu->er.error(
                        *x,
                        dsignal::compile_nonassignable_expr,
                        "cannot assign to non-assignable expr!");
                    return; // abort
                }
                else if (symbol->is<fn_csym>()) { // non-assignable
                    tu->er.error(
                        *x,
                        dsignal::compile_nonassignable_expr,
                        "cannot assign to non-assignable expr!");
                    return; // abort
                }
                else { // not a param, local var, or fn
                    tu->er.error(
                        *x,
                        dsignal::compile_not_an_expr,
                        "invalid expr!");
                    return; // abort
                }
            }
            else { // nothing found
                tu->er.error(
                    *x,
                    dsignal::compile_undeclared_name,
                    "undeclared name {}!",
                    name);
                return; // abort
            }
        }
        else { // lvalue isn't even an identifier expr, so non-assignable
            tu->er.error(
                *x,
                dsignal::compile_nonassignable_expr,
                "cannot assign to non-assignable expr!");
            return; // abort
        }
    }
    else { // expr stmt
        // consume temporary from our expr
        _pop_temp(1, true, x->low_pos());
    }
}

void yama::internal::second_pass::visit_end(res<ast_IfStmt> x) {
    if (tu->er.is_fatal()) {
        return;
    }
    // write exit label
    cw.add_label(_top_if_stmt().exit_label);
    _pop_if_stmt();
}

void yama::internal::second_pass::visit_end(res<ast_LoopStmt> x) {
    if (tu->er.is_fatal()) {
        return;
    }
    // write unconditional branch to continue label
    cw.add_jump(_top_loop_stmt().continue_label);
    _add_sym(x->high_pos());
    // write break label
    cw.add_label(_top_loop_stmt().break_label);
    _pop_loop_stmt();
}

void yama::internal::second_pass::visit_end(res<ast_BreakStmt> x) {
    if (tu->er.is_fatal()) {
        return;
    }
    // IMPORTANT: we can't just pop all the registers of the current scope, as
    //            loop stmt blocks can have *multiple levels* of nested scope
    // pop all temporaries and local vars to *exit* loop block scope
    const auto total_regs_in_loop_stmt = _regstk.size() - _top_loop_stmt().first_reg;
    cw.add_pop(uint8_t(total_regs_in_loop_stmt));
    _add_sym(x->low_pos());
    // write unconditional branch to break label
    cw.add_jump(_top_loop_stmt().break_label);
    _add_sym(x->low_pos());
}

void yama::internal::second_pass::visit_end(res<ast_ContinueStmt> x) {
    if (tu->er.is_fatal()) {
        return;
    }
    // IMPORTANT: we can't just pop all the registers of the current scope, as
    //            loop stmt blocks can have *multiple levels* of nested scope
    // pop all temporaries and local vars to *restart* loop block scope
    const auto total_regs_in_loop_stmt = _regstk.size() - _top_loop_stmt().first_reg;
    cw.add_pop(uint8_t(total_regs_in_loop_stmt));
    _add_sym(x->low_pos());
    // write unconditional branch to continue label
    cw.add_jump(_top_loop_stmt().continue_label);
    _add_sym(x->low_pos());
}

void yama::internal::second_pass::visit_end(res<ast_ReturnStmt> x) {
    if (tu->er.is_fatal()) {
        return;
    }
    if (x->expr) { // form 'return x;'
        // type check return value expr, w/ return type None if no explicit return type
        const ctype return_type = tu->types.default_none(tu->cs->resolver[_target_csym().return_type]);
        if (!_reg_type_check(-1, return_type)) {
            tu->er.error(
                *x,
                dsignal::compile_type_mismatch,
                "return stmt expr is type {0}, but expected {1}!",
                _top_reg().type.fmt(tu->e()),
                return_type.fmt(tu->e()));
                return; // abort
        }
        cw.add_ret(uint8_t(_top_reg().index));
        _add_sym(x->low_pos());
        _pop_temp(1, false); // consume return value temporary + pop instr after ret would just be dead code
    }
    else { // form 'return;'
        // this form may not be used if return type isn't None
        const ctype return_type = tu->types.default_none(tu->cs->resolver[_target_csym().return_type]);
        const ctype expected_return_type = tu->types.none_type();
        if (return_type != expected_return_type) {
            tu->er.error(
                *x,
                dsignal::compile_type_mismatch,
                "return stmt returns {} object but return type is {}!",
                expected_return_type.fmt(tu->e()),
                return_type.fmt(tu->e()));
            return; // abort
        }
        _push_temp(*x, expected_return_type);
        cw.add_put_none(yama::newtop);
        _add_sym(x->low_pos());
        cw.add_ret(uint8_t(_top_reg().index));
        _add_sym(x->low_pos());
        _pop_temp(1, false); // pop instr after ret would just be dead code
    }
}

void yama::internal::second_pass::visit_end(res<ast_Expr> x) {
    if (x->is_assign_stmt_lvalue) _pop_lvalue();
}

void yama::internal::second_pass::visit_end(res<ast_Args> x) {
    if (tu->er.is_fatal()) {
        return;
    }
    // skip if the lvalue
    if (_is_in_lvalue()) {
        return;
    }
    // on the register stack will be the temporaries corresponding to the callobj
    // and args of the call this code will resolve
    const size_t arg_count = x->args.size();
    const size_t callobj_reg_index = _regs() - arg_count - 1;
    const ctype callobj_type = _reg_abs(callobj_reg_index).type;
    // whatever the callobj is, it's not a callable type
    if (callobj_type.kind() != kind::function) {
        tu->er.error(
            deref_assert(x->expr.lock()),
            dsignal::compile_invalid_operation,
            "cannot call non-callable type {}!",
            callobj_type.fmt(tu->e()));
        return; // abort
    }
    // check that arg count is correct
    if (arg_count != callobj_type.param_count()) {
        tu->er.error(
            deref_assert(x->expr.lock()),
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
        const ctype param_type = callobj_type.param_type(i, tu->cs->resolver).value();
        if (!_reg_type_check(arg_reg_index, param_type)) {
            tu->er.error(
                *x->args[i],
                dsignal::compile_type_mismatch,
                "arg {0} expr is type {1}, but expected {2}!",
                arg_number,
                _reg(arg_reg_index).type.fmt(tu->e()),
                param_type.fmt(tu->e()));
            args_are_correct_types = false;
        }
    }
    if (!args_are_correct_types) { // don't return until we've type checked ALL args
        return; // abort
    }
    const taul::source_pos expr_pos = deref_assert(x->expr.lock()).low_pos();
    // return value should overwrite callobj
    const size_t return_value_reg_index = callobj_reg_index;
    // write the call instr
    cw.add_call(uint8_t(arg_count + 1), yama::newtop);
    _add_sym(expr_pos);
    // reinit our return value (formerly callobj) register to the correct type
    const auto return_type = tu->types.default_none(callobj_type.return_type(tu->cs->resolver));
    _reinit_temp(return_value_reg_index, return_type);
    // pop args, but NOT callobj, as that register is reused for our return value
    _pop_temp(arg_count, false);
}

yama::type_info& yama::internal::second_pass::_target() noexcept {
    YAMA_ASSERT(_current_target);
    return *_current_target;
}

void yama::internal::second_pass::_begin_target(const str& unqualified_name) {
    YAMA_ASSERT(!_current_target);
    // bind bare-bones new_type to _current_target
    yama::type_info new_type{
        .unqualified_name = unqualified_name,
        .info = yama::function_info{
            .callsig = callsig_info{}, // stub
            .call_fn = yama::bcode_call_fn,
            .max_locals = 0,
        },
    };
    // bind new target
    _current_target = std::move(new_type);
    tu->ctp.bind(*_current_target);
    auto& our_type = _target(); // <- replaces new_type (which we moved from)
    // build callsig of our_type (which has to be done after binding it as
    // we need to populate its constant table)
    auto new_callsig = tu->ctp.build_callsig_for_fn_type(tu->types.load(unqualified_name).value());
    // patch new_callsig onto our_type
    std::get<function_info>(our_type.info).callsig = std::move(new_callsig);
}

void yama::internal::second_pass::_end_target() {
    YAMA_ASSERT(_current_target);
    tu->output.add_type(std::move(_target()));
    _current_target.reset();
}

void yama::internal::second_pass::_apply_bcode_to_target(const ast_FnDecl& x) {
    if (std::holds_alternative<function_info>(_target().info)) {
        auto& info = std::get<function_info>(_target().info);
        bool label_not_found{};
        if (auto fn_bcode = cw.done(&label_not_found)) {
            info.bcode = fn_bcode.value();
            info.bcodesyms = std::move(syms);
        }
        else {
            if (label_not_found) { // if this, then the compiler is broken
                tu->er.error(
                    x,
                    dsignal::compile_impl_internal,
                    "internal error; label_not_found == true!");
            }
            else { // exceeded branch dist limit
                tu->er.error(
                    x,
                    dsignal::compile_impl_limits,
                    "fn {} contains branch which exceeds max branch offset limits!",
                    _target().unqualified_name);
            }
        }
    }
}

void yama::internal::second_pass::_update_target_locals() {
    YAMA_ASSERT(std::holds_alternative<function_info>(_target().info));
    auto& max_locals = std::get<function_info>(_target().info).max_locals;
    max_locals = std::max(max_locals, _regstk.size());
}

const yama::internal::fn_csym& yama::internal::second_pass::_target_csym() {
    const auto& symbol = tu->syms.lookup(tu->root(), _target().unqualified_name, 0);
    return deref_assert(symbol).as<fn_csym>();
}

std::optional<size_t> yama::internal::second_pass::_target_param_index(const str& name) {
    const auto& params = _target_csym().params;
    for (size_t i = 0; i < params.size(); i++) {
        if (params[i].name == name) {
            return i;
        }
    }
    return std::nullopt;
}

void yama::internal::second_pass::_add_sym(taul::source_pos pos) {
    const auto loc = tu->src.location_at(pos);
    syms.add(cw.count() - 1, loc.origin, loc.chr, loc.line);
}

size_t yama::internal::second_pass::_regs() const noexcept {
    return _regstk.size();
}

size_t yama::internal::second_pass::_reg_abs_index(ssize_t index) {
    return index >= 0 ? size_t(index) : _regs() + index;
}

yama::internal::second_pass::_reg_t& yama::internal::second_pass::_reg_abs(size_t index) {
    return _regstk.at(index);
}

yama::internal::second_pass::_reg_t& yama::internal::second_pass::_reg(ssize_t index) {
    return _reg_abs(_reg_abs_index(index));
}

yama::internal::second_pass::_reg_t& yama::internal::second_pass::_top_reg() {
    return _regstk.back();
}

yama::internal::second_pass::_scope_t& yama::internal::second_pass::_top_scope() {
    return _scopestk.back();
}

void yama::internal::second_pass::_push_temp(const ast_node& x, ctype type) {
    if (_regs() >= _reg_limit) { // register count exceeds impl limit
        tu->er.error(
            x,
            dsignal::compile_impl_limits,
            "fn {} contains parts requiring >{} registers to store all temporaries and local vars, exceeding limit!",
            _target().unqualified_name,
            _reg_limit);
        return;
    }
    const auto ind = _regs();
    _regstk.push_back(_reg_t{ .type = type, .index = ind });
    _update_target_locals(); // can't forget to propagate this to code gen target
    _top_scope().regs++;
#if _LOG_REG_AND_SCOPE_STK_MANIP == 1
    std::string list_of_regs{};
    {
        bool not_first = false;
        for (const auto& I : _regstk) {
            if (not_first) {
                list_of_regs += ", ";
            }
            list_of_regs += I.type.fmt(tu->e());
            not_first = true;
        }
    }
    YAMA_LOG(
        tu->cs->dbg(), general_c,
        "*push* reg {: <15} ({} -> {}) ~> [ {} ]",
        type.fmt(tu->e()),
        _top_scope().first_reg + _top_scope().regs - 1,
        _top_scope().first_reg + _top_scope().regs,
        list_of_regs);
#endif
}

void yama::internal::second_pass::_pop_temp(size_t n, bool write_pop_instr, taul::source_pos pop_instr_pos) {
    if (n > _regstk.size()) n = _regstk.size(); // <- avoid undefined behaviour for pop_back
    if (write_pop_instr) {
        cw.add_pop(uint8_t(n));
        _add_sym(pop_instr_pos);
    }
    _top_scope().regs -= n;
    for (auto nn = n; nn >= 1; nn--) _regstk.pop_back();
#if _LOG_REG_AND_SCOPE_STK_MANIP == 1
    std::string list_of_regs{};
    {
        bool not_first = false;
        for (const auto& I : _regstk) {
            if (not_first) {
                list_of_regs += ", ";
            }
            list_of_regs += I.type.fmt(tu->e());
            not_first = true;
        }
    }
    YAMA_LOG(
        tu->cs->dbg(), general_c,
        "*pop* reg {: <16} ({} -> {}) ~> [ {} ]",
        n,
        _top_scope().first_reg + _top_scope().regs + n,
        _top_scope().first_reg + _top_scope().regs,
        list_of_regs);
#endif
}

void yama::internal::second_pass::_reinit_temp(ssize_t index, ctype new_type) {
    _reg(index).type = new_type;
#if _LOG_REG_AND_SCOPE_STK_MANIP == 1
    YAMA_LOG(tu->cs->dbg(), general_c, "*reinit* reg {} {}", _reg_abs_index(index), new_type.fmt(tu->e()));
#endif
}

void yama::internal::second_pass::_push_scope() {
    _scopestk.push_back(_scope_t{ .regs = 0, .first_reg = _regstk.size() });
#if _LOG_REG_AND_SCOPE_STK_MANIP == 1
    YAMA_LOG(tu->cs->dbg(), general_c, "\n*enter* scope #{}\n", _scopestk.size());
#endif
}

void yama::internal::second_pass::_pop_scope(const ast_Block& x, bool write_pop_instr) {
    _pop_temp(_top_scope().regs, write_pop_instr, x.high_pos()); // unwind temporaries and local vars
    _scopestk.pop_back();
#if _LOG_REG_AND_SCOPE_STK_MANIP == 1
    YAMA_LOG(tu->cs->dbg(), general_c, "\n*exit* scope #{}\n", _scopestk.size() + 1);
#endif
}

void yama::internal::second_pass::_promote_to_localvar(ast_VarDecl& x) {
    YAMA_ASSERT(_top_reg().is_temp());
    const auto localvar_name = x.name.str(tu->src);
    const ctype localvar_type = _top_reg().type; // *deduce* local var type from type of temporary
    // tell the temporary (now local var) register the name of its local var
    _top_reg().localvar = localvar_name;
    // update symbol table entry
    if (const auto symbol = tu->syms.lookup(x, localvar_name, x.high_pos())) {
        if (symbol->is<var_csym>()) {
            auto& info = symbol->as<var_csym>();
            // update symbol w/ info about deduced type
            if (!info.deduced_type) info.deduced_type = localvar_type;
            // when we promote a temporary to a local var, its symbol is expected to not
            // yet have an associated register index, w/ this assigning one
            YAMA_ASSERT(!info.reg);
            info.reg = _top_reg().index;
        }
    }
    YAMA_ASSERT(_top_reg().is_localvar());
}

bool yama::internal::second_pass::_reg_type_check(ssize_t index, ctype expected) {
    return _reg(index).type == expected;
}

bool yama::internal::second_pass::_in_if_stmt() const noexcept {
    return !_if_stmts.empty();
}

const yama::internal::second_pass::_if_stmt_t& yama::internal::second_pass::_top_if_stmt() const {
    return _if_stmts.back();
}

void yama::internal::second_pass::_push_if_stmt(const ast_IfStmt& x) {
    _if_stmts.push_back(_if_stmt_t{
        .body_block = x.block->id,
        .has_else_part = (bool)x.get_else_block_or_stmt(),
        .else_part_label = _gen_label(),
        .exit_label = _gen_label(),
        });
}

void yama::internal::second_pass::_pop_if_stmt() {
    _if_stmts.pop_back();
}

void yama::internal::second_pass::_mark_as_if_stmt_cond_reg(ssize_t index) {
    YAMA_ASSERT(!_top_if_stmt().cond_reg_index);
    _if_stmts.back().cond_reg_index = _reg_abs_index(index);
}

bool yama::internal::second_pass::_is_if_stmt_body(const ast_Block& x) const {
    return _in_if_stmt() && _top_if_stmt().body_block == x.id;
}

bool yama::internal::second_pass::_in_loop_stmt() const noexcept {
    return !_loop_stmts.empty();
}

const yama::internal::second_pass::_loop_stmt_t& yama::internal::second_pass::_top_loop_stmt() const {
    return _loop_stmts.back();
}

void yama::internal::second_pass::_push_loop_stmt(const ast_LoopStmt& x) {
    _loop_stmts.push_back(_loop_stmt_t{
        .break_label = _gen_label(),
        .continue_label = _gen_label(),
        .first_reg = _regs(),
        });
}

void yama::internal::second_pass::_pop_loop_stmt() {
    _loop_stmts.pop_back();
}

