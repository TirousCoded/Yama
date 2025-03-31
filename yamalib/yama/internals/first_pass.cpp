

#include "first_pass.h"

#include "compiler.h"

#include "compilation_state.h"


using namespace yama::string_literals;


yama::internal::first_pass::first_pass(translation_unit& tu)
    : tu(tu) {}

void yama::internal::first_pass::visit_begin(res<ast_Chunk> x) {
    _add_csymtab(x);
    tu->types.register_module(*tu);
    _implicitly_import_yama_module();
}

void yama::internal::first_pass::visit_begin(res<ast_ImportDir> x) {
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

void yama::internal::first_pass::visit_begin(res<ast_PrimaryExpr> x) {
    tu->cs->resolver.add(*tu, *x);
}

void yama::internal::first_pass::visit_begin(res<ast_TypeSpec> x) {
    tu->cs->resolver.add(*tu, *x);
}

void yama::internal::first_pass::visit_end(res<ast_FnDecl> x) {
    _end_fn();
}

void yama::internal::first_pass::visit_end(res<ast_Block> x) {
    _cfg.end_block();
}

void yama::internal::first_pass::visit_end(res<ast_IfStmt> x) {
    _cfg.end_if_stmt();
}

void yama::internal::first_pass::visit_end(res<ast_LoopStmt> x) {
    _cfg.end_loop_stmt();
}

void yama::internal::first_pass::_add_csymtab(res<ast_node> x) {
    tu->syms.acquire(x->id); // begin new block
}

void yama::internal::first_pass::_implicitly_import_yama_module() {
    YAMA_ASSERT(_import_dirs_are_legal());
    const auto import_path = tu->cs->sp.pull_ip(tu->e(), "yama"_str);
    if (!import_path) {
        tu->er.error(
            tu->root(),
            dsignal::compile_invalid_env,
            "compilation env has no available 'yama' module!");
        return; // abort
    }
    if (tu->types.import(import_path.value())) {
        tu->types.add_import(import_path.value());
    }
    else {
        tu->er.error(
            tu->root(),
            dsignal::compile_invalid_env,
            "compilation env has no available 'yama' module!");
    }
}

void yama::internal::first_pass::_explicitly_import_module(const res<ast_ImportDir>& x) {
    if (_import_dirs_are_legal()) {
        const str import_path_s = str(x->path(tu->src).value());
        const auto import_path = tu->cs->sp.pull_ip(tu->e(), import_path_s);
        if (!import_path) {
            tu->er.error(
                *x,
                dsignal::compile_invalid_import,
                "cannot import {}!",
                import_path_s);
            return; // abort
        }
        if (tu->types.import(import_path.value())) {
            tu->types.add_import(import_path.value());
        }
        else {
            tu->er.error(
                *x,
                dsignal::compile_invalid_import,
                "cannot import {}!",
                import_path_s);
        }
    }
    else {
        if (_is_in_fn()) {
            tu->er.error(
                *x,
                dsignal::compile_misplaced_import,
                "illegal local import!");
        }
        else {
            tu->er.error(
                *x,
                dsignal::compile_misplaced_import,
                "illegal import appearing after first type decl!");
        }
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
        tu->er.error(
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
        tu->er.error(
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
        tu->er.error(
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
        tu->er.error(
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
        tu->er.error(
            *x,
            dsignal::compile_local_fn,
            "illegal local fn {}!",
            name);
    }
    if (x->callsig->params.size() > 24) {
        tu->er.error(
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

bool yama::internal::first_pass::_import_dirs_are_legal() const noexcept {
    return !_reached_first_type_decl;
}

void yama::internal::first_pass::_check_var_is_local(res<ast_VarDecl> x) {
    if (_is_in_fn()) return;
    tu->er.error(
        *x,
        dsignal::compile_nonlocal_var,
        "illegal non-local var {}!",
        x->name.str(tu->src));
}

void yama::internal::first_pass::_check_break_is_in_loop_stmt(const res<ast_BreakStmt>& x) {
    if (_cfg.is_in_loop()) return;
    tu->er.error(
        *x,
        dsignal::compile_not_in_loop,
        "cannot use break outside of a loop stmt!");
}

void yama::internal::first_pass::_check_continue_is_in_loop_stmt(const res<ast_ContinueStmt>& x) {
    if (_cfg.is_in_loop()) return;
    tu->er.error(
        *x,
        dsignal::compile_not_in_loop,
        "cannot use continue outside of a loop stmt!");
}

