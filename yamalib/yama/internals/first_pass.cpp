

#include "first_pass.h"

#include "compiler.h"


using namespace yama::string_literals;


yama::internal::first_pass::first_pass(translation_unit& tu)
    : tu(tu) {}

void yama::internal::first_pass::visit_begin(res<ast_Chunk> x) {
    _add_csymtab(x);
    tu->types.register_module(*tu);
    _implicitly_import_yama_module();
}

void yama::internal::first_pass::visit_begin(res<ast_ImportDecl> x) {
    _explicitly_import_module(x);
}

void yama::internal::first_pass::visit_begin(res<ast_VarDecl> x) {
    _check_var_is_local(x);
    _insert_vardecl(x);
}

void yama::internal::first_pass::visit_begin(res<ast_FnDecl> x) {
    _begin_fn(x);
}

void yama::internal::first_pass::visit_begin(res<ast_ParamDecl> x) {
    _insert_paramdecl(x);
}

void yama::internal::first_pass::visit_begin(res<ast_Block> x) {
    // only if NOT the body block of a fn (otherwise fn handles this)
    if (!x->is_fn_body_block) _add_csymtab(x);
    _cfg.begin_block();
}

void yama::internal::first_pass::visit_begin(res<ast_IfStmt> x) {
    _cfg.begin_if_stmt();
}

void yama::internal::first_pass::visit_begin(res<ast_LoopStmt> x) {
    _cfg.begin_loop_stmt();
}

void yama::internal::first_pass::visit_begin(res<ast_BreakStmt> x) {
    _check_break_is_in_loop_stmt(x);
    _cfg.break_stmt();
}

void yama::internal::first_pass::visit_begin(res<ast_ContinueStmt> x) {
    _check_continue_is_in_loop_stmt(x);
    _cfg.continue_stmt();
}

void yama::internal::first_pass::visit_begin(res<ast_ReturnStmt> x) {
    _cfg.return_stmt();
}

void yama::internal::first_pass::visit_begin(res<ast_Expr> x) {
    _enter_expr(x);
}

void yama::internal::first_pass::visit_begin(res<ast_PrimaryExpr> x) {
    tu->cs->resolver.add(*tu, *x);
}

void yama::internal::first_pass::visit_begin(res<ast_TypeSpec> x) {
    _enter_type_spec(x);
}

void yama::internal::first_pass::visit_end(res<ast_FnDecl> x) {
    _end_fn();
}

void yama::internal::first_pass::visit_end(res<ast_Block> x) {
    x->will_never_exit_via_fallthrough = _cfg.is_in_dead_code();
    _cfg.end_block();
}

void yama::internal::first_pass::visit_end(res<ast_IfStmt> x) {
    _cfg.end_if_stmt();
}

void yama::internal::first_pass::visit_end(res<ast_LoopStmt> x) {
    _cfg.end_loop_stmt();
}

void yama::internal::first_pass::visit_end(res<ast_Expr> x) {
    _exit_expr(x);
}

void yama::internal::first_pass::visit_end(res<ast_TypeSpec> x) {
    _exit_type_spec(x);
}

void yama::internal::first_pass::_add_csymtab(res<ast_node> x) {
    tu->syms.acquire(x->id); // begin new block
}

void yama::internal::first_pass::_implicitly_import_yama_module() {
    YAMA_ASSERT(_import_decls_are_legal());
    const auto import_path = tu->cs->sp.pull_ip(tu->e(), "yama"_str);
    if (!import_path) {
        tu->err.error(
            tu->root(),
            dsignal::compile_invalid_env,
            "compilation env has no available 'yama' module!");
        return; // abort
    }
    if (tu->types.import(*import_path)) {
        tu->types.bind_import(*import_path);
        _insert_importdecl_for_implicit_yama_import(*import_path);
    }
    else {
        tu->err.error(
            tu->root(),
            dsignal::compile_invalid_env,
            "compilation env has no available 'yama' module!");
    }
}

void yama::internal::first_pass::_explicitly_import_module(const res<ast_ImportDecl>& x) {
    if (_import_decls_are_legal()) {
        const str import_path_s = str(x->path(tu->src).value());
        const auto import_path = tu->cs->sp.pull_ip(tu->e(), import_path_s);
        if (!import_path) {
            tu->err.error(
                *x,
                dsignal::compile_invalid_import,
                "cannot import {}!",
                import_path_s);
            return; // abort
        }
        if (tu->types.import(*import_path)) {
            tu->types.bind_import(*import_path);
            _insert_importdecl(x, *import_path);
        }
        else {
            tu->err.error(
                *x,
                dsignal::compile_invalid_import,
                "cannot import {}!",
                import_path_s);
        }
    }
    else {
        if (_is_in_fn()) {
            tu->err.error(
                *x,
                dsignal::compile_misplaced_import,
                "illegal local import!");
        }
        else {
            tu->err.error(
                *x,
                dsignal::compile_misplaced_import,
                "illegal import appearing after first type decl!");
        }
    }
}

void yama::internal::first_pass::_insert_importdecl(res<ast_ImportDecl> x, const import_path& path) {
    if (!x->name) return; // if unnamed, don't insert anything
    const auto name = x->name->str(tu->src);
    // prepare symbol
    import_csym sym{
        .path = path,
    };
    // insert symbol
    bool no_table_found = false;
    const bool success = tu->syms.insert(
        *x,
        name,
        x,
        0,
        sym,
        &no_table_found);
    YAMA_ASSERT(!no_table_found);
    if (!success) { // if failed insert, report name conflict
        tu->err.error(
            *x,
            dsignal::compile_name_conflict,
            // add the 'import' bit as only other import decls can conflict w/ this
            "name {} already in use by another import decl!",
            name);
    }
}

void yama::internal::first_pass::_insert_importdecl_for_implicit_yama_import(const import_path& path) {
    const auto name = "yama"_str;
    // prepare symbol
    import_csym sym{
        .path = path,
    };
    // insert symbol
    bool no_table_found = false;
    const bool success = tu->syms.insert(
        tu->root(),
        name,
        nullptr, // <- no associated AST node
        0,
        sym,
        &no_table_found);
    YAMA_ASSERT(!no_table_found);
    // NOTE: below failure should NEVER occur, but is here for *completeness*
    if (!success) { // if failed insert, report name conflict
        tu->err.error(
            tu->root(),
            dsignal::compile_name_conflict,
            // add the 'import' bit as only other import decls can conflict w/ this
            "name {} already in use by another import decl!",
            name);
    }
}

void yama::internal::first_pass::_insert_vardecl(res<ast_VarDecl> x) {
    const auto name = x->name.str(tu->src);
    // prepare symbol
    var_csym sym{};
    if (x->type) { // get annotated type, if any
        sym.annot_type = x->type->type.get();
    }
    // insert symbol
    bool no_table_found = false;
    const bool success = tu->syms.insert(
        *x,
        name,
        x,
        x->high_pos(), // only valid AFTER fully introduced
        sym,
        &no_table_found);
    YAMA_ASSERT(!no_table_found);
    if (!success) { // if failed insert, report name conflict
        tu->err.error(
            *x,
            dsignal::compile_name_conflict,
            "name {} already in use by another decl!",
            name);
    }
}

bool yama::internal::first_pass::_insert_fndecl(res<ast_FnDecl> x) {
    const auto unqualified_name = x->name.str(tu->src);
    // prepare symbol
    fn_csym sym{};
    if (x->callsig->result) {
        sym.return_type = x->callsig->result->type.get();
    }
    for (const auto& I : x->callsig->params) {
        sym.params.push_back(fn_csym::param{
            .name = I->name.str(tu->src),
            .type = nullptr, // <- will be filled in below
            });
    }
    // given some form like 'a: Int, b, c: Char', the below iterates backwards over
    // this part of the AST (below called 'in'), recording each type annotation found,
    // and propagating this type info to otherwise unannotated param decls, w/ this
    // info output to the corresponding parts of the fn decl symbol's param list (below
    // called 'out'), w/ us also detecting unannotated trailing param decls
    std::shared_ptr<ast_TypeAnnot> annot{}; // latest encountered type annot
    const auto& params_in = x->callsig->params;
    auto& params_out = sym.params;
    for (size_t i = 0; i < params_in.size(); i++) {
        const size_t index = params_in.size() - 1 - i; // <- iterate backwards
        const auto& in = params_in[index];
        auto& out = params_out[index];
        if (in->type) {
            annot = in->type; // encountered new type annot
        }
        if (annot) { // output type info from last encountered type annot
            out.type = annot->type.get();
        }
    }
    // insert symbol
    bool no_table_found = false;
    const bool success = tu->syms.insert(
        *x,
        unqualified_name,
        x,
        0, // TODO: if we ever add local fns, this'll need to conditionally be either '0' or 'x->high_pos()'
        sym,
        &no_table_found);
    YAMA_ASSERT(!no_table_found);
    if (!success) { // if failed insert, report name conflict
        tu->err.error(
            *x,
            dsignal::compile_name_conflict,
            "name {} already in use by another decl!",
            unqualified_name);
    }
    return success;
}

void yama::internal::first_pass::_insert_paramdecl(res<ast_ParamDecl> x) {
    const auto name = x->name.str(tu->src);
    const auto our_param_type =
        _is_in_fn() && _current_fn().symbol
        ? _current_fn().symbol->as<fn_csym>().params[_current_fn().next_param_index()].type
        : nullptr;
    // prepare symbol
    param_csym sym{
        .type = our_param_type,
    };
    // report if no type annotation
    if (!sym.type) {
        tu->err.error(
            *x,
            dsignal::compile_invalid_param_list,
            "param {} has no type annotation!",
            name);
    }
    // insert symbol
    bool no_table_found = false;
    const bool success = tu->syms.insert(
        *x,
        name,
        x,
        x->high_pos(), // only valid AFTER fully introduced
        sym,
        &no_table_found);
    YAMA_ASSERT(!no_table_found);
    if (!success) { // if failed insert, report name conflict
        tu->err.error(
            *x,
            dsignal::compile_name_conflict,
            "name {} already in use by another decl!",
            name);
    }
}

bool yama::internal::first_pass::_is_in_fn() {
    //YAMA_ASSERT(_cfg.is_in_fn() == !_fn_decl_stk.empty());
    return _cfg.is_in_fn();
}

yama::internal::first_pass::_fn_decl& yama::internal::first_pass::_current_fn() {
    YAMA_ASSERT(_is_in_fn());
    return _fn_decl_stk.back();
}

void yama::internal::first_pass::_begin_fn(res<ast_FnDecl> x) {
    const auto name = x->name.str(tu->src);
    if (_is_in_fn()) {
        tu->err.error(
            *x,
            dsignal::compile_local_fn,
            "illegal local fn {}!",
            name);
    }
    if (x->callsig->params.size() > 24) {
        tu->err.error(
            *x,
            dsignal::compile_invalid_param_list,
            "illegal fn {} with >24 params!",
            name);
    }
    _acknowledge_type_decl();
    _cfg.begin_fn();
    const bool no_name_conflict = _insert_fndecl(x);
    // for fn decls, since we need to have params and local vars be in the same
    // block, we add the csymtab to the fn decl, and suppress adding one for the
    // body block AST node
    _add_csymtab(x);
    _fn_decl details{
        .node = x,
        // if name conflict arose, then should be nullptr
        .symbol = no_name_conflict ? tu->syms.lookup(tu->root(), name, 0) : nullptr,
    };
    _fn_decl_stk.push_back(std::move(details));
}

void yama::internal::first_pass::_end_fn() {
    // propagate above to symbol (+ fail quietly if no symbol due to compiling code being in error)
    if (const auto& symbol = _current_fn().symbol) {
        symbol->as<fn_csym>().all_paths_return_or_loop = _cfg.check_fn();
    }
    _cfg.end_fn();
}

void yama::internal::first_pass::_acknowledge_type_decl() {
    _reached_first_type_decl = true;
}

bool yama::internal::first_pass::_import_decls_are_legal() const noexcept {
    return !_reached_first_type_decl;
}

void yama::internal::first_pass::_check_var_is_local(res<ast_VarDecl> x) {
    if (_is_in_fn()) return;
    tu->err.error(
        *x,
        dsignal::compile_nonlocal_var,
        "illegal non-local var {}!",
        x->name.str(tu->src));
}

void yama::internal::first_pass::_check_break_is_in_loop_stmt(const res<ast_BreakStmt>& x) {
    if (_cfg.is_in_loop()) return;
    tu->err.error(
        *x,
        dsignal::compile_not_in_loop,
        "cannot use break outside of a loop stmt!");
}

void yama::internal::first_pass::_check_continue_is_in_loop_stmt(const res<ast_ContinueStmt>& x) {
    if (_cfg.is_in_loop()) return;
    tu->err.error(
        *x,
        dsignal::compile_not_in_loop,
        "cannot use continue outside of a loop stmt!");
}

void yama::internal::first_pass::_enter_expr(const res<ast_Expr>& x) {
    const bool can_output_constexpr = !already_in_solved_constexpr && !x->is_assign_stmt_lvalue;
    if (x->has_const_kw) {
        YAMA_ASSERT(x->args.size() != 0);
        if (size_t args = x->args[0]->args.size(); args != 1) {
            tu->err.error(
                *x,
                dsignal::compile_wrong_arg_count,
                "constexpr guarantee expr given {} args, but expected {}!",
                args,
                1);
        }
        else if (can_output_constexpr) {
            // to add the constexpr guarantee expr itself, we want to add
            // the ast_Args suffix used by it
            tu->cs->solver.add(*tu, *x->args[0], true);
        }
        already_in_solved_constexpr.push();
    }
    else {
        YAMA_ASSERT(x->primary);
        // if primary expr is not a ref to a param or local var, then add it as a constexpr
        auto _should_be_constexpr = [&]() -> bool {
            if (x->primary->name) {
                const auto name = x->primary->name->str(tu->src);
                const auto symbol = tu->syms.lookup(*x, name, x->low_pos());
                return
                    symbol &&
                    !symbol->is<param_csym>() &&
                    !symbol->is<var_csym>();
            }
            else if (x->primary->lit) {
                return true;
            }
            else return false;
            };
        if (can_output_constexpr && _should_be_constexpr()) {
            // TODO: we don't have any suffixes at the moment that can be constexpr,
            //       except for constexpr guarantee expr
            if (x->args.size() == 0) {
                tu->cs->solver.add(*tu, *x, false);
            }
            else {
                tu->cs->solver.add(*tu, *x->primary, false);
            }
        }
    }
}

void yama::internal::first_pass::_exit_expr(const res<ast_Expr>& x) {
    already_in_solved_constexpr.pop_if(x->has_const_kw);
}

void yama::internal::first_pass::_enter_type_spec(const res<ast_TypeSpec>& x) {
    tu->cs->solver.add(*tu, deref_assert(x->expr), true);
    already_in_solved_constexpr.push();
}

void yama::internal::first_pass::_exit_type_spec(const res<ast_TypeSpec>& x) {
    already_in_solved_constexpr.pop();
}

