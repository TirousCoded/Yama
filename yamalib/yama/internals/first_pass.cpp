

#include "first_pass.h"

#include <ranges>

#include "util.h"
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
    _process_decl(x);
}

void yama::internal::first_pass::visit_begin(res<ast_FnDecl> x) {
    bool no_name_conflict{};
    _process_decl(x, no_name_conflict);
    _begin_fn(x, no_name_conflict);
}

void yama::internal::first_pass::visit_begin(res<ast_StructDecl> x) {
    _process_decl(x);
}

void yama::internal::first_pass::visit_begin(res<ast_ParamDecl> x) {
    _process_decl(x);
}

void yama::internal::first_pass::visit_begin(res<ast_Result> x) {
    tu->cs->ea.add_root(*deref_assert(x->type).expr);
}

void yama::internal::first_pass::visit_begin(res<ast_Block> x) {
    // only if NOT the body block of a fn (otherwise fn handles this)
    if (!x->is_fn_body_block) _add_csymtab(x);
    _cfg.begin_block();
}

void yama::internal::first_pass::visit_begin(res<ast_ExprStmt> x) {
    tu->cs->ea.add_root(deref_assert(x->expr));
}

void yama::internal::first_pass::visit_begin(res<ast_IfStmt> x) {
    _cfg.begin_if_stmt();
    tu->cs->ea.add_root(deref_assert(x->cond));
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
    if (x->expr) {
        tu->cs->ea.add_root(*x->expr);
    }
}

void yama::internal::first_pass::visit_begin(res<ast_Expr> x) {
    tu->cs->ea.acknowledge(*tu, *x);
}

void yama::internal::first_pass::visit_begin(res<ast_PrimaryExpr> x) {
    tu->cs->ea.acknowledge(*tu, *x);
}

void yama::internal::first_pass::visit_begin(res<ast_Assign> x) {
    tu->cs->ea.add_root(deref_assert(x->expr));
}

void yama::internal::first_pass::visit_begin(res<ast_Args> x) {
    tu->cs->ea.acknowledge(*tu, *x);
}

void yama::internal::first_pass::visit_begin(res<ast_TypeMemberAccess> x) {
    tu->cs->ea.acknowledge(*tu, *x);
}

void yama::internal::first_pass::visit_begin(res<ast_ObjectMemberAccess> x) {
    tu->cs->ea.acknowledge(*tu, *x);
}

void yama::internal::first_pass::visit_begin(res<ast_TypeAnnot> x) {
    tu->cs->ea.add_root(*deref_assert(x->type).expr);
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

void yama::internal::first_pass::_add_csymtab(res<ast_node> x) {
    tu->syms.acquire(x->id); // begin new block
}

void yama::internal::first_pass::_implicitly_import_yama_module() {
    YAMA_ASSERT(_import_decls_are_legal());
    const auto import_path = tu->cs->sp.ip(tu->e(), "yama"_str);
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
        const auto import_path = tu->cs->sp.ip(tu->e(), import_path_s);
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
    // insert symbol
    bool no_table_found = false;
    const std::shared_ptr entry = tu->syms.insert(
        *x,
        name,
        x,
        0,
        import_csym{ .path = path },
        &no_table_found);
    YAMA_ASSERT(!no_table_found);
    _check_name_conflict_for_import((bool)entry, *x, name);
}

void yama::internal::first_pass::_insert_importdecl_for_implicit_yama_import(const import_path& path) {
    const auto name = "yama"_str;
    // insert symbol
    bool no_table_found = false;
    const std::shared_ptr entry = tu->syms.insert(
        tu->root(),
        name,
        nullptr, // <- no associated AST node
        0,
        import_csym{ .path = path },
        &no_table_found);
    YAMA_ASSERT(!no_table_found);
    // NOTE: below failure should NEVER occur, but is here for *completeness*
    _check_name_conflict_for_import((bool)entry, tu->root(), name);
    YAMA_ASSERT(entry);
}

void yama::internal::first_pass::_insert_vardecl(res<ast_VarDecl> x) {
    const auto name = x->name.str(tu->src);
    // insert symbol
    bool no_table_found = false;
    const std::shared_ptr entry = tu->syms.insert(
        *x,
        name,
        x,
        _vardecl_intro_point(*x),
        _mk_var_csym(*x),
        &no_table_found);
    YAMA_ASSERT(!no_table_found);
    _check_name_conflict((bool)entry, *x, name);
}

void yama::internal::first_pass::_insert_fndecl(res<ast_FnDecl> x, bool& no_name_conflict) {
    // NOTE: this fn must ALWAYS set no_name_conflict at some point
    const auto unqualified_name = str(x->fmt_unqualified_name(tu->src).value());
    // insert symbol
    bool no_table_found = false;
    const std::shared_ptr entry = tu->syms.insert(
        *x,
        unqualified_name,
        x,
        _fndecl_intro_point(*x),
        _mk_fn_like_csym(*x, unqualified_name),
        &no_table_found);
    YAMA_ASSERT(!no_table_found);
    _check_name_conflict((bool)entry, *x, unqualified_name);
    no_name_conflict = (bool)entry;
}

void yama::internal::first_pass::_insert_paramdecl(res<ast_ParamDecl> x) {
    const auto name = x->name.str(tu->src);
    // insert symbol
    bool no_table_found = false;
    const std::shared_ptr entry = tu->syms.insert(
        *x,
        name,
        x,
        _paramdecl_intro_point(*x),
        _mk_param_csym(*x, name),
        &no_table_found);
    YAMA_ASSERT(!no_table_found);
    _check_name_conflict((bool)entry, *x, name);
    if (entry && _is_in_fn() && _current_fn().symbol) {
        const auto& csym = entry->as<param_csym>();
        // check for no type annotation error
        if (!csym.type && !csym.self_param) {
            tu->err.error(
                *x,
                dsignal::compile_invalid_param_list,
                "param {} has no type annotation!",
                entry->name);
        }
        // add entry to param list of current fn
        _current_fn().symbol->as<fn_like_csym>().params.push_back(*entry);
    }
}

void yama::internal::first_pass::_insert_structdecl(res<ast_StructDecl> x) {
    const auto unqualified_name = x->name.str(tu->src);
    // insert symbol
    bool no_table_found = false;
    const std::shared_ptr entry = tu->syms.insert(
        *x,
        unqualified_name,
        x,
        _structdecl_intro_point(*x),
        _mk_struct_csym(*x),
        &no_table_found);
    YAMA_ASSERT(!no_table_found);
    _check_name_conflict((bool)entry, *x, unqualified_name);
}

void yama::internal::first_pass::_check_name_conflict(bool no_name_conflict, const ast_node& where, const str& name) {
    if (no_name_conflict) return;
    tu->err.error(
        where,
        dsignal::compile_name_conflict,
        "name {} already in use by another decl!",
        name);
}

void yama::internal::first_pass::_check_name_conflict_for_import(bool no_name_conflict, const ast_node& where, const str& name) {
    if (no_name_conflict) return;
    tu->err.error(
        where,
        dsignal::compile_name_conflict,
        // add the 'import' bit as only other import decls can conflict w/ this
        "name {} already in use by another import decl!",
        name);
}

yama::internal::var_csym yama::internal::first_pass::_mk_var_csym(const ast_VarDecl& x) {
    var_csym result{};
    if (x.type) { // get annotated type, if any
        result.annot_type = x.type->type.get();
    }
    if (x.assign) { // get initializer expr, if any
        result.initializer = x.assign->expr.get();
    }
    return result;
}

yama::internal::fn_like_csym yama::internal::first_pass::_mk_fn_like_csym(const ast_FnDecl& x, const str& unqualified_name) {
    // param stuff won't be resolved until later
    fn_like_csym result{
        .is_method = x.is_method(),
        .fln = qualified_name(tu->src_path, internal::unqualified_name(unqualified_name).owner_name()),
    };
    if (x.callsig->result) {
        result.return_type = x.callsig->result->type.get();
    }
    return result;
}

yama::internal::param_csym yama::internal::first_pass::_mk_param_csym(const ast_ParamDecl& x, const str& name) {
    if (!(_is_in_fn() && _current_fn().symbol)) {
        // dummy if invalid fn
        return param_csym{
            .fn = nullptr,
            .index = size_t(),
            .type = nullptr,
            .self_param = false,
        };
    }
    const auto& csym = _current_fn().symbol->as<fn_like_csym>();
    const size_t index = csym.params.size();
    const auto type_annot_info = _current_fn().symbol->node->as<ast_FnDecl>()->callsig->find_param_type_annot(index);
    const bool self_param =
        csym.is_method &&
        index == 0 &&
        name == "self"_str &&
        !type_annot_info.directly_attached;
    const auto type_spec =
        type_annot_info && !self_param
        ? type_annot_info.type_annot->type.get()
        : nullptr;
    return param_csym{
        .fn = _current_fn().symbol.get(),
        .index = index,
        .type = type_spec,
        .self_param = self_param,
    };
}

yama::internal::struct_csym yama::internal::first_pass::_mk_struct_csym(const ast_StructDecl& x) {
    return struct_csym{};
}

taul::source_pos yama::internal::first_pass::_vardecl_intro_point(const ast_VarDecl& x) {
    return x.high_pos(); // only valid AFTER fully introduced
}

taul::source_pos yama::internal::first_pass::_fndecl_intro_point(const ast_FnDecl& x) {
    // TODO: if we ever add local fns, this'll need to conditionally be either '0' or 'x.high_pos()'
    return 0;
}

taul::source_pos yama::internal::first_pass::_paramdecl_intro_point(const ast_ParamDecl& x) {
    if (!(_is_in_fn() && _current_fn().symbol)) { // abort if fn is in error
        return x.low_pos(); // dummy
    }
    // gotta account for the fact that some param decls will get their type
    // annot from a later param decl, so we gotta use the type annot to get
    // the correct high_pos value
    const auto type_annot_info = _current_fn().symbol->node->as<ast_FnDecl>()->callsig->find_param_type_annot(x.index);
    return
        (bool)type_annot_info
        ? type_annot_info.type_annot->high_pos() // only valid AFTER fully introduced
        : x.high_pos(); // we'll use x.high_pos() in edge case of no type annot
}

taul::source_pos yama::internal::first_pass::_structdecl_intro_point(const ast_StructDecl& x) {
    // TODO: if we ever add local structs, this'll need to conditionally be either '0' or 'x.high_pos()'
    return 0;
}

void yama::internal::first_pass::_process_decl(const res<ast_VarDecl>& x) {
    _check_var_is_local(x);
    _insert_vardecl(x);
}

void yama::internal::first_pass::_process_decl(const res<ast_FnDecl>& x, bool& no_name_conflict) {
    // NOTE: this fn must ALWAYS set no_name_conflict at some point
    const auto name = x->name.value().str(tu->src);
    if (_is_in_fn()) {
        tu->err.error(
            *x,
            dsignal::compile_illegal_local_decl,
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
    _insert_fndecl(x, no_name_conflict);
}

void yama::internal::first_pass::_process_decl(const res<ast_StructDecl>& x) {
    const auto name = x->name.str(tu->src);
    if (_is_in_fn()) {
        tu->err.error(
            *x,
            dsignal::compile_illegal_local_decl,
            "illegal local struct {}!",
            name);
    }
    _acknowledge_type_decl();
    _insert_structdecl(x);
}

void yama::internal::first_pass::_process_decl(const res<ast_ParamDecl>& x) {
    _insert_paramdecl(x);
}

bool yama::internal::first_pass::_is_in_fn() {
    //YAMA_ASSERT(_cfg.is_in_fn() == !_fn_decl_stk.empty());
    return _cfg.is_in_fn();
}

yama::internal::first_pass::_fn_decl& yama::internal::first_pass::_current_fn() {
    YAMA_ASSERT(_is_in_fn());
    return _fn_decl_stk.back();
}

void yama::internal::first_pass::_begin_fn(res<ast_FnDecl> x, bool no_name_conflict) {
    const auto name = str(x->fmt_unqualified_name(tu->src).value());
    _cfg.begin_fn();
    // for fn decls, since we need to have params and local vars be in the same
    // block, we add the csymtab to the fn decl, and suppress adding one for the
    // body block AST node
    _add_csymtab(x);
    _fn_decl details{
        .node = x,
        // if name conflict arose, then should be nullptr
        .symbol = no_name_conflict ? tu->syms.lookup(*x, name, 0) : nullptr,
    };
    _fn_decl_stk.push_back(std::move(details));
}

void yama::internal::first_pass::_end_fn() {
    // propagate above to symbol (+ fail quietly if no symbol due to compiling code being in error)
    if (const auto& symbol = _current_fn().symbol) {
        auto& fn_csym = symbol->as<fn_like_csym>();
        fn_csym.all_paths_return_or_loop = _cfg.check_fn();
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
        dsignal::compile_illegal_nonlocal_decl,
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

