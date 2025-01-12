

#include "second_pass.h"


using namespace yama::string_literals;


#define _LOG_REG_AND_SCOPE_STK_MANIP 0


yama::internal::second_pass::second_pass(
    std::shared_ptr<debug> dbg,
    ast_Chunk& root,
    const taul::source_code& src,
    csymtab_group_ctti& csymtabs)
    : _dbg(dbg),
    _root(&root),
    _src(&src),
    _csymtabs(&csymtabs) {}

void yama::internal::second_pass::visit_begin(res<ast_FnDecl> x) {
    if (err) {
        return;
    }
    _push_target(x->name.str(_get_src())); // push new fn type for code gen
    _mark_as_fn_body_block(*x->block);
}

void yama::internal::second_pass::visit_begin(res<ast_Block> x) {
    if (err) {
        return;
    }
    if (_is_if_stmt_body(*x)) { // if we're the if stmt's body block
        _mark_as_if_stmt_cond_reg(-1); // mark as if stmt's cond expr reg
        if (!_reg_type_check(-1, "Bool"_str)) { // if stmt cond expr
            _compile_error(
                *x,
                dsignal::compile_type_mismatch,
                "if stmt condition expr is type {0}, but expected {1}!",
                _top_reg().type, "Bool"_str);
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

void yama::internal::second_pass::visit_begin(res<ast_ExprStmt> x) {
    if (err) {
        return;
    }
    if (x->assign) _mark_as_assign_stmt_lvalue(*x->expr);
}

void yama::internal::second_pass::visit_begin(res<ast_IfStmt> x) {
    if (err) {
        return;
    }
    _push_if_stmt(*x);
}

void yama::internal::second_pass::visit_begin(res<ast_LoopStmt> x) {
    if (err) {
        return;
    }
    _push_loop_stmt(*x);
    // write continue label
    cw.add_label(_top_loop_stmt().continue_label);
}

void yama::internal::second_pass::visit_begin(res<ast_Expr> x) {
    // add entries to _args_node_to_expr_node_map
    for (const auto& I : x->args) {
        _args_node_to_expr_node_map[I->id] = x.get();
    }
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
    if (err) {
        return;
    }
    // suppress this and nested if the lvalue of an assignment stmt
    if (_is_assign_stmt_lvalue(*x)) {
        _push_suppress();
    }
    // skip if suppressed
    if (_is_suppressed()) {
        return;
    }
    YAMA_ASSERT(x->primary);
    if (x->primary->name) {
        const auto name = x->primary->name->str(_get_src());
        // param, local var, or fn
        if (const auto& symbol = _get_csymtabs().lookup(*x, name, x->low_pos())) {
            if (symbol->is<param_csym>()) {
                _push_temp(*x, symbol->as<param_csym>().type.value());
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
                    _push_temp(*x, info.type.value());
                    // write copy from local var into temporary
                    cw.add_copy(uint8_t(info.reg.value()), yama::newtop);
                    _add_sym(x->primary->name->low_pos());
                }
                else {
                    _compile_error(
                        *x,
                        dsignal::compile_undeclared_name,
                        "undeclared name {}!",
                        name);
                    return; // abort
                }
            }
            else if (symbol->is<fn_csym>()) {
                _push_temp(*x, name);
                // write put_const loading fn object into temporary
                cw.add_put_const(yama::newtop, uint8_t(_pull_fn_type_c(name)));
                _add_sym(x->primary->name->low_pos());
            }
            else { // not a param, local var, or fn
                _compile_error(
                    *x,
                    dsignal::compile_not_an_expr,
                    "invalid expr!",
                    name);
                return; // abort
            }
        }
        else { // nothing found
            _compile_error(
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
            const parsed_int v = parse_int(lit_nd->lit.str(_get_src())).value();
            if (v.overflow) { // overflow error
                _compile_error(
                    *x,
                    dsignal::compile_numeric_overflow,
                    "numeric overflow ({})!",
                    lit_nd->lit.str(_get_src()));
                return; // abort
            }
            else if (v.underflow) { // underflow error
                _compile_error(
                    *x,
                    dsignal::compile_numeric_underflow,
                    "numeric underflow ({})!",
                    lit_nd->lit.str(_get_src()));
                return; // abort
            }
            else { // valid
                _push_temp(*x, "Int"_str);
                cw.add_put_const(yama::newtop, uint8_t(_pull_int_c(v.v)));
                _add_sym(x->primary->lit->low_pos());
            }
        }
        else if (x->primary->lit->is_uint()) {
            const auto lit_nd = res(x->primary->lit->as_uint());
            const parsed_uint v = parse_uint(lit_nd->lit.str(_get_src())).value();
            if (v.overflow) { // overflow error
                _compile_error(
                    *x,
                    dsignal::compile_numeric_overflow,
                    "numeric overflow ({})!",
                    lit_nd->lit.str(_get_src()));
                return; // abort
            }
            else { // valid
                _push_temp(*x, "UInt"_str);
                cw.add_put_const(yama::newtop, uint8_t(_pull_uint_c(v.v)));
                _add_sym(x->primary->lit->low_pos());
            }
        }
        else if (x->primary->lit->is_float()) {
            const auto lit_nd = res(x->primary->lit->as_float());
            const parsed_float v = parse_float(lit_nd->lit.str(_get_src())).value();
            // below can handle cases of overflow/underflow
            _push_temp(*x, "Float"_str);
            cw.add_put_const(yama::newtop, uint8_t(_pull_float_c(v.v)));
            _add_sym(x->primary->lit->low_pos());
        }
        else if (x->primary->lit->is_bool()) {
            const auto lit_nd = res(x->primary->lit->as_bool());
            const parsed_bool v = parse_bool(lit_nd->lit.str(_get_src())).value();
            _push_temp(*x, "Bool"_str);
            cw.add_put_const(yama::newtop, uint8_t(_pull_bool_c(v.v)));
            _add_sym(x->primary->lit->low_pos());
        }
        else if (x->primary->lit->is_char()) {
            const auto lit_nd = res(x->primary->lit->as_char());
            const auto txt = lit_nd->lit.str(_get_src()); // <- text w/ single-quotes
            const auto txt_corrected = txt.substr(1, txt.length() - 2); // <- text w/out single-quotes
            const parsed_char v = parse_char(txt_corrected).value();
            if (!taul::is_unicode(v.v)) { // illegal Unicode error
                _compile_error(
                    *x,
                    dsignal::compile_illegal_unicode,
                    "illegal Unicode ({})!",
                    lit_nd->lit.str(_get_src()));
                return; // abort
            }
            else { // valid
                _push_temp(*x, "Char"_str);
                cw.add_put_const(yama::newtop, uint8_t(_pull_char_c(v.v)));
                _add_sym(x->primary->lit->low_pos());
            }
        }
        else YAMA_DEADEND;
    }
    else YAMA_DEADEND;
}

void yama::internal::second_pass::visit_end(res<ast_VarDecl> x) {
    if (err) {
        return;
    }
    if (x->assign) {
        if (x->type) {
            const auto type = x->type->type->type.str(_get_src());
            // if explicit type annot, type check it against temporary of the var decl
            if (!_reg_type_check(-1, type)) {
                _compile_error(
                    *x,
                    dsignal::compile_type_mismatch,
                    "var decl initializer expr is type {0}, but expected {1}!",
                    _top_reg().type, type);
                return; // abort
            }
        }
        // promote temporary within the var decl into local var
        _promote_to_localvar(*x);
    }
    else {
        if (x->type) {
            const auto type = x->type->type->type.str(_get_src());
            // push local var w/ type
            _push_temp(*x, type);
            _promote_to_localvar(*x);
            // lookup type we're initializing w/
            if (const auto symbol = _get_csymtabs().lookup(*x, type, x->low_pos())) {
                if (symbol->is<prim_csym>()) { // primitive type
                    // write bcode which inits our local var w/ default value
                    if (type == "None"_str) {
                        cw.add_put_none(yama::newtop);
                        _add_sym(x->low_pos());
                    }
                    else if (type == "Int"_str) {
                        cw.add_put_const(yama::newtop, uint8_t(_pull_int_c(0)));
                        _add_sym(x->low_pos());
                    }
                    else if (type == "UInt"_str) {
                        cw.add_put_const(yama::newtop, uint8_t(_pull_uint_c(0u)));
                        _add_sym(x->low_pos());
                    }
                    else if (type == "Float"_str) {
                        cw.add_put_const(yama::newtop, uint8_t(_pull_float_c(0.0)));
                        _add_sym(x->low_pos());
                    }
                    else if (type == "Bool"_str) {
                        cw.add_put_const(yama::newtop, uint8_t(_pull_bool_c(false)));
                        _add_sym(x->low_pos());
                    }
                    else if (type == "Char"_str) {
                        cw.add_put_const(yama::newtop, uint8_t(_pull_char_c(U'\0')));
                        _add_sym(x->low_pos());
                    }
                    else YAMA_DEADEND;
                }
                else if (symbol->is<fn_csym>()) { // fn type
                    // write bcode which inits our local var w/ default value
                    cw.add_put_const(yama::newtop, uint8_t(_pull_fn_type_c(type)));
                    _add_sym(x->low_pos());
                }
                else { // not a primitive or fn type
                    _compile_error(
                        *x,
                        dsignal::compile_not_a_type,
                        "invalid type specifier!");
                    return; // abort
                }
            }
            else { // nothing found
                _compile_error(
                    *x,
                    dsignal::compile_undeclared_name,
                    "undeclared name {}!",
                    type);
                return; // abort
            }
        }
        else { // no type annot, nor assign
            _compile_error(
                *x,
                dsignal::compile_invalid_local_var,
                "cannot declare local var with no type annotation or initializer!");
            return; // abort
        }
    }
}

void yama::internal::second_pass::visit_end(res<ast_FnDecl> x) {
    if (err) {
        return;
    }
    _apply_bcode_to_target(*x);
}

void yama::internal::second_pass::visit_end(res<ast_Block> x) {
    if (err) {
        return;
    }
    const bool this_is_fn_body_block = _is_fn_body_block(*x);
    if (this_is_fn_body_block) {
        // if we're None returning, and not all control paths have an explicit return
        // stmt or enter infinite loops, write a ret instr
        const bool is_none_returning = _target_csym().return_type.value_or("None"_str) == "None";
        const bool not_all_paths_return_or_loop = !_target_csym().all_paths_return_or_loop.value();
        if (is_none_returning && not_all_paths_return_or_loop) {
            _push_temp(*x, "None"_str);
            cw.add_put_none(yama::newtop);
            _add_sym(x->high_pos());
            cw.add_ret(uint8_t(_top_reg().index));
            _add_sym(x->high_pos());
            _pop_temp(1, false); // pop instr after ret would just be dead code
        }
    }
    _pop_scope(*x, !this_is_fn_body_block); // pop instr after ret would just be dead code
    if (_is_if_stmt_body(*x) && _top_if_stmt().has_else_part) {
        // write unconditional branch to the exit label to skip the else-part (if any)
        cw.add_jump(_top_if_stmt().exit_label);
        _add_sym(x->high_pos());
        // write else-part label
        cw.add_label(_top_if_stmt().else_part_label);
    }
}

void yama::internal::second_pass::visit_end(res<ast_ExprStmt> x) {
    if (err) {
        return;
    }
    if (x->assign) { // assignment stmt
        YAMA_ASSERT(x->expr && x->expr->primary);
        if (x->expr->primary->name) {
            const auto name = x->expr->primary->name->str(_get_src());
            if (const auto& symbol = _get_csymtabs().lookup(*x, name, x->low_pos())) {
                if (symbol->is<var_csym>()) {
                    const auto& info = symbol->as<var_csym>();
                    // type check assignment before allowing it
                    if (!_reg_type_check(-1, info.type.value())) {
                        _compile_error(
                            *x,
                            dsignal::compile_type_mismatch,
                            "assigning expr is type {0}, but expected {1}!",
                            _top_reg().type, info.type.value());
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
                    _compile_error(
                        *x,
                        dsignal::compile_nonassignable_expr,
                        "cannot assign to non-assignable expr!");
                    return; // abort
                }
                else if (symbol->is<fn_csym>()) { // non-assignable
                    _compile_error(
                        *x,
                        dsignal::compile_nonassignable_expr,
                        "cannot assign to non-assignable expr!");
                    return; // abort
                }
                else { // not a param, local var, or fn
                    _compile_error(
                        *x,
                        dsignal::compile_not_an_expr,
                        "invalid expr!");
                    return; // abort
                }
            }
            else { // nothing found
                _compile_error(
                    *x,
                    dsignal::compile_undeclared_name,
                    "undeclared name {}!",
                    name);
                return; // abort
            }
        }
        else { // lvalue isn't even an identifier expr, so non-assignable
            _compile_error(
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
    if (err) {
        return;
    }
    // write exit label
    cw.add_label(_top_if_stmt().exit_label);
    _pop_if_stmt();
}

void yama::internal::second_pass::visit_end(res<ast_LoopStmt> x) {
    if (err) {
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
    if (err) {
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
    if (err) {
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
    if (err) {
        return;
    }
    if (x->expr) { // form 'return x;'
        // type check return value expr, w/ return type None if no explicit return type
        if (!_reg_type_check(-1, _target_csym().return_type.value_or("None"_str))) {
            _compile_error(
                *x,
                dsignal::compile_type_mismatch,
                "return stmt expr is type {0}, but expected {1}!",
                _top_reg().type, _target_csym().return_type.value_or("None"_str));
            return; // abort
        }
        cw.add_ret(uint8_t(_top_reg().index));
        _add_sym(x->low_pos());
        _pop_temp(1, false); // consume return value temporary + pop instr after ret would just be dead code
    }
    else { // form 'return;'
        // this form may not be used if return type isn't None
        if (_target_csym().return_type.value_or("None"_str) != "None"_str) {
            _compile_error(
                *x,
                dsignal::compile_type_mismatch,
                "return stmt returns None object but return type is {}!",
                _target_csym().return_type.value_or("None"_str));
            return; // abort
        }
        _push_temp(*x, "None"_str);
        cw.add_put_none(yama::newtop);
        _add_sym(x->low_pos());
        cw.add_ret(uint8_t(_top_reg().index));
        _add_sym(x->low_pos());
        _pop_temp(1, false); // pop instr after ret would just be dead code
    }
}

void yama::internal::second_pass::visit_end(res<ast_Expr> x) {
    // unsuppress if we're the one that suppressed
    if (_is_assign_stmt_lvalue(*x)) {
        _pop_suppress();
    }
}

void yama::internal::second_pass::visit_end(res<ast_Args> x) {
    YAMA_ASSERT(_args_node_to_expr_node_map.contains(x->id));
    if (err) {
        return;
    }
    // skip if suppressed
    if (_is_suppressed()) {
        return;
    }
    // on the register stack will be the temporaries corresponding to the callobj
    // and args of the call this code will resolve
    const size_t arg_count = x->args.size();
    const size_t callobj_reg_index = _regs() - arg_count - 1;
    // lookup fn_csym for the callobj's type, searching from root of AST, in order
    // to verify that the callobj is actually callable, and to get needed info
    if (const auto symbol = _get_csymtabs().lookup(_get_root(), _reg_abs(callobj_reg_index).type, 0)) {
        // whatever the callobj is, it's not a callable type
        if (!symbol->is<fn_csym>()) {
            _compile_error(
                deref_assert(_args_node_to_expr_node_map.at(x->id)),
                dsignal::compile_invalid_operation,
                "cannot call non-callable type {}!",
                _reg(callobj_reg_index).type);
            return; // abort
        }
        const auto& info = symbol->as<fn_csym>();
        // check that arg count is correct
        if (arg_count != info.params.size()) {
            _compile_error(
                deref_assert(_args_node_to_expr_node_map.at(x->id)),
                dsignal::compile_wrong_arg_count,
                "call to {} given {} args, but expected {}!",
                _reg(callobj_reg_index).type, arg_count, info.params.size());
            return; // abort
        }
        // type check the args
        bool args_are_correct_types = true;
        for (size_t i = 0; i < arg_count; i++) {
            const auto arg_reg_index = callobj_reg_index + i + 1;
            const auto arg_number = i + 1;
            if (!_reg_type_check(arg_reg_index, info.params[i].type.value())) {
                _compile_error(
                    *x->args[i],
                    dsignal::compile_type_mismatch,
                    "arg {0} expr is type {1}, but expected {2}!",
                    arg_number, _reg(arg_reg_index).type, info.params[i].type.value());
                args_are_correct_types = false;
            }
        }
        if (!args_are_correct_types) { // don't return until we've type checked ALL args
            return; // abort
        }
        const taul::source_pos expr_pos = deref_assert(_args_node_to_expr_node_map.at(x->id)).low_pos();
        // return value should overwrite callobj
        const size_t return_value_reg_index = callobj_reg_index;
        // write the call instr
        cw.add_call(uint8_t(arg_count + 1), yama::newtop);
        _add_sym(expr_pos);
        // reinit our return value (formerly callobj) register to the correct type
        const auto return_type = info.return_type.value_or("None"_str);
        _reinit_temp(return_value_reg_index, return_type);
        // pop args, but NOT callobj, as that register is reused for our return value
        _pop_temp(arg_count, false);
    }
    else YAMA_DEADEND;
}

yama::internal::ast_Chunk& yama::internal::second_pass::_get_root() const noexcept {
    return deref_assert(_root);
}

const taul::source_code& yama::internal::second_pass::_get_src() const noexcept {
    return yama::deref_assert(_src);
}

yama::internal::csymtab_group_ctti& yama::internal::second_pass::_get_csymtabs() const noexcept {
    return deref_assert(_csymtabs);
}

yama::type_info& yama::internal::second_pass::_target() noexcept {
    YAMA_ASSERT(!results.empty());
    return results.back();
}

void yama::internal::second_pass::_push_target(str fullname) {
    // push bare-bones new_type to results
    yama::type_info new_type{
        .fullname = fullname,
        .info = yama::function_info{
            .callsig = callsig_info{}, // stub
            .call_fn = yama::bcode_call_fn,
            .max_locals = 0,
        },
    };
    results.push_back(std::move(new_type));
    auto& our_type = results.back(); // <- replaces new_type
    // build callsig of our_type (which has to be done after pushing it as
    // we need to populate its constant table)
    auto new_callsig = _build_callsig_for_fn_type(fullname);
    // patch new_callsig onto our_type
    std::get<function_info>(our_type.info).callsig = std::move(new_callsig);
}

void yama::internal::second_pass::_apply_bcode_to_target(const ast_FnDecl& x) {
    if (std::holds_alternative<function_info>(results.back().info)) {
        auto& info = std::get<function_info>(results.back().info);
        bool label_not_found{};
        if (auto fn_bcode = cw.done(&label_not_found)) {
            info.bcode = fn_bcode.value();
            info.bcodesyms = std::move(syms);
        }
        else {
            if (label_not_found) { // if this, then the compiler is broken
                _compile_error(
                    x,
                    dsignal::compile_impl_internal,
                    "internal error; label_not_found == true!");
            }
            else { // exceeded branch dist limit
                _compile_error(
                    x,
                    dsignal::compile_impl_limits,
                    "fn {} contains branch which exceeds max branch offset limits!",
                    results.back().fullname);
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
    const auto& symbol = _get_csymtabs().lookup(_get_root(), _target().fullname, 0);
    YAMA_ASSERT(symbol);
    return symbol->as<fn_csym>();
}

std::optional<size_t> yama::internal::second_pass::_target_param_index(str name) {
    const auto& params = _target_csym().params;
    for (size_t i = 0; i < params.size(); i++) {
        if (params[i].name == name) {
            return i;
        }
    }
    return std::nullopt;
}

yama::const_t yama::internal::second_pass::_pull_int_c(int_t x) {
    auto& _consts = _target().consts;
    // first try and find existing constant
    if (const auto found = _find_existing_c<int_const>(_consts, x)) {
        return found.value();
    }
    // add new constant and return it
    _consts.add_int(x);
    return _consts.consts.size() - 1;
}

yama::const_t yama::internal::second_pass::_pull_uint_c(uint_t x) {
    auto& _consts = _target().consts;
    // first try and find existing constant
    if (const auto found = _find_existing_c<uint_const>(_consts, x)) {
        return found.value();
    }
    // add new constant and return it
    _consts.add_uint(x);
    return _consts.consts.size() - 1;
}

yama::const_t yama::internal::second_pass::_pull_float_c(float_t x) {
    auto& _consts = _target().consts;
    // NOTE: as stated, we're not gonna bother trying to compare floats to
    //       avoid duplicates, as comparing floats is never consistent enough
    //       to not potentially cause problems
    _consts.add_float(x);
    return _consts.consts.size() - 1;
}

yama::const_t yama::internal::second_pass::_pull_bool_c(bool_t x) {
    auto& _consts = _target().consts;
    // first try and find existing constant
    if (const auto found = _find_existing_c<bool_const>(_consts, x)) {
        return found.value();
    }
    // add new constant and return it
    _consts.add_bool(x);
    return _consts.consts.size() - 1;
}

yama::const_t yama::internal::second_pass::_pull_char_c(char_t x) {
    auto& _consts = _target().consts;
    // first try and find existing constant
    if (const auto found = _find_existing_c<char_const>(_consts, x)) {
        return found.value();
    }
    // add new constant and return it
    _consts.add_char(x);
    return _consts.consts.size() - 1;
}

yama::const_t yama::internal::second_pass::_pull_type_c(str fullname) {
    // lookup symbol for this type (which should exist)
    const auto symbol = _get_csymtabs().lookup(_get_root(), fullname, 0);
    YAMA_ASSERT(symbol);
    // resolve based on what we found
    if (symbol->is<prim_csym>())    return _pull_prim_type_c(fullname);
    else if (symbol->is<fn_csym>()) return _pull_fn_type_c(fullname);
    else                            YAMA_DEADEND;
    return const_t{};
}

yama::const_t yama::internal::second_pass::_pull_prim_type_c(str fullname) {
    auto& _consts = _target().consts;
    // search for existing constant to use
    for (const_t i = 0; i < _consts.consts.size(); i++) {
        if (const auto ptr = _consts.get<yama::primitive_type_const>(i)) {
            if (deref_assert(ptr).fullname != fullname) continue;
            return i;
        }
    }
    // add new constant and return it
    _consts.add_primitive_type(fullname);
    return _consts.consts.size() - 1;
}

yama::const_t yama::internal::second_pass::_pull_fn_type_c(str fullname) {
    auto& _consts = _target().consts;
    // search for existing constant to use
    for (const_t i = 0; i < _consts.consts.size(); i++) {
        if (const auto ptr = _consts.get<yama::function_type_const>(i)) {
            if (deref_assert(ptr).fullname != fullname) continue;
            return i;
        }
    }
    // IMPORTANT: in order to allow for fn type constants' callsigs to have cyclical dependence
    //            w/ one another, we gonna first add a fn type constant w/ a *stub* callsig, then
    //            we're gonna build its proper callsig (recursively pulling on other constants),
    //            and then we're gonna *patch* our type constant w/ this new callsig
    // add new constant
    _consts.add_function_type(fullname, callsig_info{}); // <- now available for reference
    // get index of our type constant
    const const_t our_index = _consts.consts.size() - 1;
    // build our proper callsig
    callsig_info proper_callsig = _build_callsig_for_fn_type(fullname);
    // patch proper_callsig onto our type constant
    _consts._patch_function_type(our_index, std::move(proper_callsig));
    return our_index; // return index of our type constant
}

yama::callsig_info yama::internal::second_pass::_build_callsig_for_fn_type(str fullname) {
    const auto& symbol = _get_csymtabs().lookup(_get_root(), fullname, 0);
    YAMA_ASSERT(symbol);
    YAMA_ASSERT(symbol->is<fn_csym>());
    const auto& info = symbol->as<fn_csym>();
    // build callsig
    callsig_info result{};
    // resolve parameters
    for (const auto& I : info.params) {
        result.params.push_back(_pull_type_c(I.type.value()));
    }
    // resolve return type
    result.ret = _pull_type_c(info.return_type.value_or("None"_str));
    return result;
}

void yama::internal::second_pass::_add_sym(taul::source_pos pos) {
    const auto loc = _get_src().location_at(pos);
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

void yama::internal::second_pass::_push_temp(const ast_node& x, str type) {
    if (_regs() >= _reg_limit) { // register count exceeds impl limit
        _compile_error(
            x,
            dsignal::compile_impl_limits,
            "fn {} contains parts requiring >{} registers to store all temporaries and local vars, exceeding limit!",
            results.back().fullname, _reg_limit);
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
            list_of_regs += I.type.fmt();
            not_first = true;
        }
    }
    YAMA_LOG(
        _dbg, general_c,
        "*push* reg {: <15} ({} -> {}) ~> [ {} ]",
        type,
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
            list_of_regs += I.type.fmt();
            not_first = true;
        }
    }
    YAMA_LOG(
        _dbg, general_c,
        "*pop* reg {: <16} ({} -> {}) ~> [ {} ]",
        n,
        _top_scope().first_reg + _top_scope().regs + n,
        _top_scope().first_reg + _top_scope().regs,
        list_of_regs);
#endif
}

void yama::internal::second_pass::_reinit_temp(ssize_t index, str new_type) {
    _reg(index).type = new_type;
#if _LOG_REG_AND_SCOPE_STK_MANIP == 1
    YAMA_LOG(_dbg, general_c, "*reinit* reg {} {}", _reg_abs_index(index), new_type);
#endif
}

void yama::internal::second_pass::_push_scope() {
    _scopestk.push_back(_scope_t{ .regs = 0, .first_reg = _regstk.size() });
#if _LOG_REG_AND_SCOPE_STK_MANIP == 1
    YAMA_LOG(_dbg, general_c, "\n*enter* scope #{}\n", _scopestk.size());
#endif
}

void yama::internal::second_pass::_pop_scope(const ast_Block& x, bool write_pop_instr) {
    _pop_temp(_top_scope().regs, write_pop_instr, x.high_pos()); // unwind temporaries and local vars
    _scopestk.pop_back();
#if _LOG_REG_AND_SCOPE_STK_MANIP == 1
    YAMA_LOG(_dbg, general_c, "\n*exit* scope #{}\n", _scopestk.size() + 1);
#endif
}

void yama::internal::second_pass::_promote_to_localvar(ast_VarDecl& x) {
    YAMA_ASSERT(_top_reg().is_temp());
    const auto localvar_name = x.name.str(_get_src());
    const auto localvar_type = _top_reg().type; // *deduce* local var type from type of temporary
    // tell the temporary (now local var) register the name of its local var
    _top_reg().localvar = localvar_name;
    // update symbol table entry
    if (const auto symbol = _get_csymtabs().lookup(x, localvar_name, x.high_pos())) {
        if (symbol->is<var_csym>()) {
            auto& info = symbol->as<var_csym>();
            // if var decl has no type annotation, then its type is deduced, meaning that
            // right now its csymtab entry will have no stated type, so we need to update
            // the csymtab entry to say it has deduced type
            if (!info.type) info.type = localvar_type;
            // when we promote a temporary to a local var, its symbol is expected to not
            // yet have an associated register index, w/ this assigning one
            YAMA_ASSERT(!info.reg);
            info.reg = _top_reg().index;
        }
    }
    YAMA_ASSERT(_top_reg().is_localvar());
}

bool yama::internal::second_pass::_reg_type_check(ssize_t index, str expected) {
    return _reg(index).type == expected;
}

void yama::internal::second_pass::_mark_as_fn_body_block(const ast_Block& x) {
    _fn_body_blocks.insert(x.id);
}

bool yama::internal::second_pass::_is_fn_body_block(const ast_Block& x) const noexcept {
    return _fn_body_blocks.contains(x.id);
}

void yama::internal::second_pass::_mark_as_assign_stmt_lvalue(const ast_Expr& x) {
    _assign_stmt_lvalue_exprs.insert(x.id);
}

bool yama::internal::second_pass::_is_assign_stmt_lvalue(const ast_Expr& x) const noexcept {
    return _assign_stmt_lvalue_exprs.contains(x.id);
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

