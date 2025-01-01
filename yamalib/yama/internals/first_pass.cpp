

#include "first_pass.h"


using namespace yama::string_literals;


yama::internal::first_pass::first_pass(
    std::shared_ptr<debug> dbg,
    domain& dm,
    ast_Chunk& root,
    const taul::source_code& src,
    csymtab_group_ctti& csymtabs)
    : _dbg(dbg),
    _dm(&dm),
    _root(&root),
    _src(&src),
    _csymtabs(&csymtabs) {}

void yama::internal::first_pass::visit_begin(res<ast_Chunk> x) {
    _add_csymtab(x);
}

void yama::internal::first_pass::visit_begin(res<ast_VarDecl> x) {
    const auto name = x->name.str(_get_src());
    if (!_in_fn_ctx()) {
        _compile_error(
            *x,
            dsignal::compile_nonlocal_var,
            "illegal non-local var {}!",
            name);
    }
    _insert_vardecl(x);
}

void yama::internal::first_pass::visit_begin(res<ast_FnDecl> x) {
    const auto name = x->name.str(_get_src());
    if (_in_fn_ctx()) {
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
    _mark_as_fn_body_block(*x->block);
    _enter_fn_ctx();
    _insert_fndecl(x);
    // for fn decls, since we need to have params and local vars be in the same
    // block, we add the csymtab to the fn decl, and suppress adding one for the
    // body block AST node
    _add_csymtab(x);
    _cfg_fn_decl_begin();
}

void yama::internal::first_pass::visit_begin(res<ast_ParamDecl> x) {
    _insert_paramdecl(x);
}

void yama::internal::first_pass::visit_begin(res<ast_Block> x) {
    if (!_is_fn_body_block(*x)) { // suppress if we're the body block of a fn (see visit_begin for ast_FnDecl above)
        _add_csymtab(x);
    }
    _cfg_block_begin();
}

void yama::internal::first_pass::visit_begin(res<ast_IfStmt> x) {
    _cfg_if_stmt_begin();
}

void yama::internal::first_pass::visit_begin(res<ast_LoopStmt> x) {
    _enter_loop_ctx();
    _cfg_loop_stmt_begin();
}

void yama::internal::first_pass::visit_begin(res<ast_BreakStmt> x) {
    if (!_in_loop_ctx()) { // illegal break stmt if not directly/indirectly in loop stmt block
        _compile_error(
            *x,
            dsignal::compile_not_in_loop,
            "cannot use break outside of a loop stmt!");
    }
    _cfg_break_stmt();
}

void yama::internal::first_pass::visit_begin(res<ast_ContinueStmt> x) {
    if (!_in_loop_ctx()) { // illegal continue stmt if not directly/indirectly in loop stmt block
        _compile_error(
            *x,
            dsignal::compile_not_in_loop,
            "cannot use continue outside of a loop stmt!");
    }
    _cfg_continue_stmt();
}

void yama::internal::first_pass::visit_begin(res<ast_ReturnStmt> x) {
    _cfg_return_stmt();
}

void yama::internal::first_pass::visit_end(res<ast_FnDecl> x) {
    const auto name = x->name.str(_get_src());
    const bool all_paths_have_explicit_returns_or_infinite_loops = _cfg_fn_decl_end();
    // TODO: while it shouldn't cause issues, I think if there's a name conflict the below code
    //       might assign to the wrong fn decl
    // propagate above to symbol (+ fail quietly if no symbol due to compiling code being in error)
    if (const auto symbol = _get_csymtabs().lookup(_get_root(), name, 0)) {
        if (symbol->is<fn_csym>()) { // <- if name conflict error, symbol could be not a fn
            symbol->as<fn_csym>().all_paths_return_or_loop = all_paths_have_explicit_returns_or_infinite_loops;
        }
    }
    const bool requires_explicit_returns = !_fn_decl_return_type_is_none(*x);
    // if type annot, and type annot is not None, then control-flow error
    // if not all control paths have explicit return stmt (or infinite loop)
    if (requires_explicit_returns && !all_paths_have_explicit_returns_or_infinite_loops) {
        _compile_error(
            *x,
            dsignal::compile_no_return_stmt,
            "for {}, not all control paths end with a return stmt!",
            x->name.str(_get_src()));
    }
    _exit_fn_ctx();
}

void yama::internal::first_pass::visit_end(res<ast_Block> x) {
    _cfg_block_end();
}

void yama::internal::first_pass::visit_end(res<ast_IfStmt> x) {
    _cfg_if_stmt_end();
}

void yama::internal::first_pass::visit_end(res<ast_LoopStmt> x) {
    _cfg_loop_stmt_end();
    _exit_loop_ctx();
}

yama::domain& yama::internal::first_pass::_get_dm() const noexcept {
    return deref_assert(_dm);
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

void yama::internal::first_pass::_insert_vardecl(res<ast_VarDecl> x) {
    const auto name = x->name.str(_get_src());
    // check if name conflict w/ predeclared type
    if (_check_predeclared_type_name_conflict(x, name)) {
        return;
    }
    // prepare symbol
    var_csym sym{};
    if (x->type) { // get annotated type, if any
        sym.type = x->type->type->type.str(_get_src());
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

void yama::internal::first_pass::_insert_fndecl(res<ast_FnDecl> x) {
    const auto name = x->name.str(_get_src());
    // disable relevant param decl behaviour if we never setup the fn decl properly
    _has_last_fn_decl = false;
    // check if name conflict w/ predeclared type
    if (_check_predeclared_type_name_conflict(x, name)) {
        return;
    }
    // prepare symbol
    fn_csym sym{};
    if (x->callsig->result) {
        sym.return_type = x->callsig->result->type->type.str(_get_src());
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
        const size_t index = params_in.size() - 1 - i;
        const auto& in = params_in[index];
        auto& out = params_out[index];
        if (in->type) {
            annot = in->type; // encountered new type annot
        }
        if (annot) { // output type info from last encountered type annot
            out.type = annot->type->type.str(_get_src());
        }
    }
    // insert symbol
    bool no_table_found = false;
    const bool success = _get_csymtabs().insert(
        *x,
        name,
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
            name);
        return; // abort
    }
    // update _last_fn_decl, and reset _param_index, for reasons explained
    _has_last_fn_decl = true;
    _last_fn_decl = name;
    _param_index = 0;
}

void yama::internal::first_pass::_insert_paramdecl(res<ast_ParamDecl> x) {
    const auto name = x->name.str(_get_src());
    auto& our_fn = deref_assert(_get_csymtabs().lookup(_get_root(), _last_fn_decl, 0));
    const auto our_fn_return_type =
        _has_last_fn_decl
        ? our_fn.as<fn_csym>().params[_param_index].type
        : std::nullopt;
    // prepare symbol
    param_csym sym{
        // lookup fn decl of this param decl, then use _param_index to get type for this param decl
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
    // incr _param_index for reasons explained
    _param_index++;
}

bool yama::internal::first_pass::_check_predeclared_type_name_conflict(res<ast_node> x, str name) {
    const auto table_nd = _get_csymtabs().node_of_inner_most(*x);
    YAMA_ASSERT(table_nd);
    if (&*table_nd != &_get_root()) {
        return false;
    }
    const bool result = _get_dm().load(name).has_value();
    if (result) {
        _compile_error(
            *x,
            dsignal::compile_name_conflict,
            "name {} already in use by another decl!",
            name);
    }
    return result;
}

void yama::internal::first_pass::_mark_as_fn_body_block(const ast_Block& x) {
    _fn_body_blocks.insert(x.id);
}

bool yama::internal::first_pass::_is_fn_body_block(const ast_Block& x) const noexcept {
    return _fn_body_blocks.contains(x.id);
}

bool yama::internal::first_pass::_cfg_has_current() {
    return !_cfg_paths.empty();
}

yama::internal::first_pass::_cfg_paths_t& yama::internal::first_pass::_cfg_current() {
    YAMA_ASSERT(_cfg_has_current());
    return _cfg_paths.back();
}

void yama::internal::first_pass::_cfg_push(bool is_block) {
    _cfg_paths.push_back(_cfg_paths_t{ .is_block = is_block });
}

yama::internal::first_pass::_cfg_paths_t yama::internal::first_pass::_cfg_pop() {
    YAMA_ASSERT(_cfg_has_current());
    const _cfg_paths_t result = _cfg_current();
    _cfg_paths.pop_back();
    return result;
}

bool yama::internal::first_pass::_cfg_is_dead_code() {
    YAMA_ASSERT(_cfg_has_current());
    return 
        _cfg_current().is_block &&
        _cfg_current().block_endpoints >= 1;
}

void yama::internal::first_pass::_cfg_break_stmt() {
    if (!_cfg_has_current()) return;
    if (_cfg_is_dead_code()) return; // skip dead code
    if (!_in_loop_ctx()) return; // ignore illegal break stmts
    _cfg_current().block_endpoints++;
    _cfg_current().breaks++;
}

void yama::internal::first_pass::_cfg_continue_stmt() {
    if (!_cfg_has_current()) return;
    if (_cfg_is_dead_code()) return; // skip dead code
    if (!_in_loop_ctx()) return; // ignore illegal continue stmts
    _cfg_current().block_endpoints++;
    _cfg_current().fn_endpoints++;
}

void yama::internal::first_pass::_cfg_return_stmt() {
    if (!_cfg_has_current()) return;
    if (_cfg_is_dead_code()) return; // skip dead code
    _cfg_current().block_endpoints++;
    _cfg_current().fn_endpoints++;
}

void yama::internal::first_pass::_cfg_block_begin() {
    if (!_cfg_has_current()) return;
    _cfg_push(true);
}

void yama::internal::first_pass::_cfg_block_end() {
    if (!_cfg_has_current()) return;
    const auto top = _cfg_pop();
    YAMA_ASSERT(_cfg_has_current());
    if (_cfg_is_dead_code()) return; // skip dead code
    if (top.block_endpoints >= 1) {
        _cfg_current().block_endpoints++;
    }
    if (top.fn_endpoints >= 1) {
        _cfg_current().fn_endpoints++;
    }
    if (top.breaks >= 1) {
        _cfg_current().breaks++;
    }
}

void yama::internal::first_pass::_cfg_if_stmt_begin() {
    if (!_cfg_has_current()) return;
    _cfg_push(false);
}

void yama::internal::first_pass::_cfg_if_stmt_end() {
    if (!_cfg_has_current()) return;
    const auto top = _cfg_pop();
    YAMA_ASSERT(_cfg_has_current());
    if (_cfg_is_dead_code()) return; // skip dead code
    if (top.block_endpoints >= 2) { // propagate only if both if/else parts have
        _cfg_current().block_endpoints++;
    }
    if (top.fn_endpoints >= 2) { // propagate only if both if/else parts have
        _cfg_current().fn_endpoints++;
    }
    if (top.breaks >= 1) { // propagate if either part has
        _cfg_current().breaks++;
    }
}

void yama::internal::first_pass::_cfg_loop_stmt_begin() {
    if (!_cfg_has_current()) return;
    _cfg_push(false);
}

void yama::internal::first_pass::_cfg_loop_stmt_end() {
    if (!_cfg_has_current()) return;
    const auto top = _cfg_pop();
    YAMA_ASSERT(_cfg_has_current());
    if (_cfg_is_dead_code()) return; // skip dead code
    if (top.breaks == 0) { // implicit continue-like behaviour if no subpaths exit to code outside loop
        _cfg_current().block_endpoints++; // w/out breaks, loop stmt guarantees code after loop is dead code
        _cfg_current().fn_endpoints++; // w/out breaks, loop stmt guarantees its code will return from fn, or enter infinite loop
    }
}

void yama::internal::first_pass::_cfg_fn_decl_begin() {
    _cfg_push(false); // push 'base' entry for this fn decl
}

bool yama::internal::first_pass::_cfg_fn_decl_end() {
    YAMA_ASSERT(_cfg_has_current());
    const auto top = _cfg_pop();
    return top.fn_endpoints >= 1;
}

bool yama::internal::first_pass::_fn_decl_return_type_is_none(const ast_FnDecl& x) {
    return
        !x.callsig->result || // either no type annot
        x.callsig->result->type->type.str(_get_src()) == "None"_str; // or type annot is 'None'
}

