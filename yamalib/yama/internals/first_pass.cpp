

#include "first_pass.h"


using namespace yama::string_literals;


yama::internal::first_pass::first_pass(
    std::shared_ptr<debug> dbg,
    res<compiler_services> services,
    ast_Chunk& root,
    const taul::source_code& src,
    csymtab_group_ctti& csymtabs)
    : _dbg(dbg),
    _services(services),
    _root(&root),
    _src(&src),
    _csymtabs(&csymtabs) {}

void yama::internal::first_pass::visit_begin(res<ast_Chunk> x) {
    _add_csymtab(x);
    _implicitly_import_yama_module();
}

void yama::internal::first_pass::visit_begin(res<ast_ImportDir> x) {
    if (_import_dirs_are_legal()) {
        const str import_path_s = str(x->path(_get_src()).value());
        const auto import_path = import_path::parse(_services->env(), import_path_s);
        if (!import_path) {
            _compile_error(
                *x,
                dsignal::compile_invalid_import,
                "cannot import {}!",
                import_path_s);
            return; // abort
        }
        const bool valid_import = _get_csymtabs().add_import(deref_assert(import_path));
        if (!valid_import) {
            _compile_error(
                *x,
                dsignal::compile_invalid_import,
                "cannot import {}!",
                import_path_s);
        }
    }
    else {
        if (_is_in_fn()) {
            _compile_error(
                *x,
                dsignal::compile_misplaced_import,
                "illegal local import!");
        }
        else {
            _compile_error(
                *x,
                dsignal::compile_misplaced_import,
                "illegal import appearing after first type decl!");
        }
    }
}

void yama::internal::first_pass::visit_begin(res<ast_VarDecl> x) {
    const auto name = x->name.str(_get_src());
    if (!_is_in_fn()) {
        _compile_error(
            *x,
            dsignal::compile_nonlocal_var,
            "illegal non-local var {}!",
            name);
    }
    _insert_vardecl(x);
}

void yama::internal::first_pass::visit_begin(res<ast_FnDecl> x) {
    _begin_fn(x);
}

void yama::internal::first_pass::visit_begin(res<ast_ParamDecl> x) {
    _insert_paramdecl(x);
}

void yama::internal::first_pass::visit_begin(res<ast_Block> x) {
    if (!_is_fn_body_block(*x)) { // suppress if we're the body block of a fn (see visit_begin for ast_FnDecl above)
        _add_csymtab(x);
    }
    _cfg.begin_block();
}

void yama::internal::first_pass::visit_begin(res<ast_IfStmt> x) {
    _cfg.begin_if_stmt();
}

void yama::internal::first_pass::visit_begin(res<ast_LoopStmt> x) {
    _cfg.begin_loop_stmt();
}

void yama::internal::first_pass::visit_begin(res<ast_BreakStmt> x) {
    if (!_cfg.is_in_loop()) { // illegal break stmt if not directly/indirectly in loop stmt block
        _compile_error(
            *x,
            dsignal::compile_not_in_loop,
            "cannot use break outside of a loop stmt!");
    }
    _cfg.break_stmt();
}

void yama::internal::first_pass::visit_begin(res<ast_ContinueStmt> x) {
    if (!_cfg.is_in_loop()) { // illegal continue stmt if not directly/indirectly in loop stmt block
        _compile_error(
            *x,
            dsignal::compile_not_in_loop,
            "cannot use continue outside of a loop stmt!");
    }
    _cfg.continue_stmt();
}

void yama::internal::first_pass::visit_begin(res<ast_ReturnStmt> x) {
    _cfg.return_stmt();
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

yama::internal::ast_Chunk& yama::internal::first_pass::_get_root() const noexcept {
    return deref_assert(_root);
}

const taul::source_code& yama::internal::first_pass::_get_src() const noexcept {
    return yama::deref_assert(_src);
}

yama::internal::csymtab_group_ctti& yama::internal::first_pass::_get_csymtabs() const noexcept {
    return deref_assert(_csymtabs);
}

void yama::internal::first_pass::_add_csymtab(res<ast_node> x) {
    _get_csymtabs().acquire(x->id); // begin new block
}

void yama::internal::first_pass::_implicitly_import_yama_module() {
    const auto import_path = import_path::parse(_services->env(), "yama"_str);
    if (!import_path) {
        _compile_error(
            _get_root(),
            dsignal::compile_invalid_env,
            "compilation env has no available 'yama' module!");
        return; // abort
    }
    const bool valid_import = _get_csymtabs().add_import(deref_assert(import_path));
    if (!valid_import) {
        _compile_error(
            _get_root(),
            dsignal::compile_invalid_env,
            "compilation env has no available 'yama' module!");
    }
}

void yama::internal::first_pass::_insert_vardecl(res<ast_VarDecl> x) {
    const auto name = x->name.str(_get_src());
    // prepare symbol
    var_csym sym{};
    if (x->type) { // get annotated type, if any
        const auto unqualified_name = x->type->type->type.str(_get_src());
        sym.type = _get_csymtabs().ensure_qualified(unqualified_name);
    }
    // insert symbol
    bool no_table_found = false;
    const bool success = _get_csymtabs().insert(
        *x,
        name,
        x,
        x->high_pos(), // only valid AFTER fully introduced
        sym,
        &no_table_found);
    YAMA_ASSERT(!no_table_found);
    if (!success) { // if failed insert, report name conflict
        _compile_error(
            *x,
            dsignal::compile_name_conflict,
            "name {} already in use by another decl!",
            name);
    }
}

bool yama::internal::first_pass::_insert_fndecl(res<ast_FnDecl> x) {
    const auto unqualified_name = x->name.str(_get_src());
    // prepare symbol
    fn_csym sym{};
    if (x->callsig->result) {
        const auto return_type_unqualified_name = x->callsig->result->type->type.str(_get_src());
        sym.return_type = _get_csymtabs().ensure_qualified(return_type_unqualified_name);
    }
    for (const auto& I : x->callsig->params) {
        sym.params.push_back(fn_csym::param{
            .name = I->name.str(_get_src()),
            .type = std::nullopt, // <- will be filled in below
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
            const auto param_type_unqualified_name = annot->type->type.str(_get_src());
            out.type = _get_csymtabs().ensure_qualified(param_type_unqualified_name);
        }
    }
    // insert symbol
    bool no_table_found = false;
    const bool success = _get_csymtabs().insert(
        *x,
        unqualified_name,
        x,
        0, // TODO: if we ever add local fns, this'll need to conditionally be either '0' or 'x->high_pos()'
        sym,
        &no_table_found);
    YAMA_ASSERT(!no_table_found);
    if (!success) { // if failed insert, report name conflict
        _compile_error(
            *x,
            dsignal::compile_name_conflict,
            "name {} already in use by another decl!",
            unqualified_name);
    }
    return success;
}

void yama::internal::first_pass::_insert_paramdecl(res<ast_ParamDecl> x) {
    const auto name = x->name.str(_get_src());
    const auto our_fn_return_type =
        _is_in_fn() && _current_fn().symbol
        ? _current_fn().symbol->as<fn_csym>().params[_current_fn().next_param_index()].type
        : std::nullopt;
    // prepare symbol
    param_csym sym{
        .type = our_fn_return_type,
    };
    // report if no type annotation
    if (!sym.type) {
        _compile_error(
            *x,
            dsignal::compile_invalid_param_list,
            "param {} has no type annotation!",
            name);
    }
    // insert symbol
    bool no_table_found = false;
    const bool success = _get_csymtabs().insert(
        *x,
        name,
        x,
        x->high_pos(), // only valid AFTER fully introduced
        sym,
        &no_table_found);
    YAMA_ASSERT(!no_table_found);
    if (!success) { // if failed insert, report name conflict
        _compile_error(
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
    const auto name = x->name.str(_get_src());
    if (_is_in_fn()) {
        _compile_error(
            *x,
            dsignal::compile_local_fn,
            "illegal local fn {}!",
            name);
    }
    if (x->callsig->params.size() > 24) {
        _compile_error(
            *x,
            dsignal::compile_invalid_param_list,
            "illegal fn {} with >24 params!",
            name);
    }
    _acknowledge_type_decl();
    _mark_as_fn_body_block(*x->block);
    _cfg.begin_fn();
    const bool no_name_conflict = _insert_fndecl(x);
    // for fn decls, since we need to have params and local vars be in the same
    // block, we add the csymtab to the fn decl, and suppress adding one for the
    // body block AST node
    _add_csymtab(x);
    _fn_decl details{
        .node = x,
        // if name conflict arose, then should be nullptr
        .symbol = no_name_conflict ? _get_csymtabs().lookup(_get_root(), x->name.str(_get_src()), 0) : nullptr,
    };
    _fn_decl_stk.push_back(std::move(details));
}

void yama::internal::first_pass::_end_fn() {
    const auto& nd = *_current_fn().node;
    const auto name = nd.name.str(_get_src());
    const bool all_paths_have_explicit_returns_or_infinite_loops = _cfg.check_fn();
    // propagate above to symbol (+ fail quietly if no symbol due to compiling code being in error)
    if (const auto symbol = _current_fn().symbol) {
        symbol->as<fn_csym>().all_paths_return_or_loop = all_paths_have_explicit_returns_or_infinite_loops;
    }
    const bool requires_explicit_returns = !_fn_decl_return_type_is_none(nd);
    // if type annot, and type annot is not None, then control-flow error
    // if not all control paths have explicit return stmt (or infinite loop)
    if (requires_explicit_returns && !all_paths_have_explicit_returns_or_infinite_loops) {
        _compile_error(
            nd,
            dsignal::compile_no_return_stmt,
            "for {}, not all control paths end with a return stmt!",
            name);
    }
    _cfg.end_fn();
}

void yama::internal::first_pass::_acknowledge_type_decl() {
    _reached_first_type_decl = true;
}

bool yama::internal::first_pass::_import_dirs_are_legal() const noexcept {
    return !_reached_first_type_decl;
}

void yama::internal::first_pass::_mark_as_fn_body_block(const ast_Block& x) {
    _fn_body_blocks.insert(x.id);
}

bool yama::internal::first_pass::_is_fn_body_block(const ast_Block& x) const noexcept {
    return _fn_body_blocks.contains(x.id);
}

bool yama::internal::first_pass::_fn_decl_return_type_is_none(const ast_FnDecl& x) {
    return
        !x.callsig->result || // either no type annot
        x.callsig->result->type->type.str(_get_src()) == "None"_str; // or type annot is 'None'
}

