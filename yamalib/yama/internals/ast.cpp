

#include "ast.h"

#include <taul/source_reader.h>
#include <taul/lexer.h>
#include <taul/parser.h>
#include <taul/no_recovery_error_handler.h>

#include "../core/yama_gram.h"

#include "compiler.h"


using namespace yama::string_literals;


std::string yama::internal::fmt_ast_type(ast_type x) {
    static_assert(ast_types == 30); // reminder
    switch (x) {
    case ast_type::Chunk:           return "Chunk";
    case ast_type::Decl:            return "Decl";
    case ast_type::ImportDecl:      return "ImportDecl";
    case ast_type::RelativePath:    return "RelativePath";
    case ast_type::VarDecl:         return "VarDecl";
    case ast_type::FnDecl:          return "FnDecl";
    case ast_type::StructDecl:      return "StructDecl";
    case ast_type::CallSig:         return "CallSig";
    case ast_type::ParamDecl:       return "ParamDecl";
    case ast_type::Result:          return "Result";
    case ast_type::Block:           return "Block";
    case ast_type::Stmt:            return "Stmt";
    case ast_type::ExprStmt:        return "ExprStmt";
    case ast_type::IfStmt:          return "IfStmt";
    case ast_type::LoopStmt:        return "LoopStmt";
    case ast_type::BreakStmt:       return "BreakStmt";
    case ast_type::ContinueStmt:    return "ContinueStmt";
    case ast_type::ReturnStmt:      return "ReturnStmt";
    case ast_type::Expr:            return "Expr";
    case ast_type::PrimaryExpr:     return "PrimaryExpr";
    case ast_type::Lit:             return "Lit";
    case ast_type::IntLit:          return "IntLit";
    case ast_type::UIntLit:         return "UIntLit";
    case ast_type::FloatLit:        return "FloatLit";
    case ast_type::BoolLit:         return "BoolLit";
    case ast_type::CharLit:         return "CharLit";
    case ast_type::Assign:          return "Assign";
    case ast_type::Args:            return "Args";
    case ast_type::TypeAnnot:       return "TypeAnnot";
    case ast_type::TypeSpec:        return "TypeSpec";
    default: YAMA_DEADEND; break;
    }
    return std::string{};
}

yama::res<yama::internal::ast_Expr> yama::internal::ast_expr::root_expr() const {
    return _root_expr_helper(*this);
}

std::optional<yama::internal::ctype> yama::internal::ast_expr::get_type(compiler& cs) const {
    return cs.ea[*this].type;
}

yama::res<yama::internal::ast_Expr> yama::internal::ast_expr::_root_expr_helper(const ast_node& current) const {
    // if there's no Expr to find, this should recurse to AST root, then crash
    const auto p = parent();
    if (const auto result = p->as<ast_Expr>()) {
        return res(result);
    }
    return _root_expr_helper(*p);
}

std::shared_ptr<yama::internal::ast_expr> yama::internal::ast_base_expr::get_primary_subexpr() const noexcept {
    return nullptr;
}

std::shared_ptr<yama::internal::ast_expr> yama::internal::ast_suffix_expr::get_primary_subexpr() const noexcept {
    const res p = parent()->expect<ast_Expr>();
    return
        index == 0
        ? p->base
        : std::static_pointer_cast<ast_expr>(p->suffixes[index - 1].base());
}

void yama::internal::ast_Chunk::fmt(ast_formatter& x) {
    x.open("Chunk", low_pos(), high_pos(), id);
    x.next("decls");
    for (const auto& I : decls) {
        I->fmt(x);
    }
    x.close();
}

void yama::internal::ast_Chunk::accept(ast_visitor& x) {
    x.visit_begin(expect<ast_Chunk>());
    for (const auto& I : decls) {
        I->accept(x);
    }
    x.visit_end(expect<ast_Chunk>());
}

void yama::internal::ast_Chunk::do_give(res<ast_Decl> x) {
    decls.push_back(x);
}

std::shared_ptr<yama::internal::ast_node> yama::internal::ast_Decl::get_decl() {
    switch (decl.index()) {
    case 0: return std::get<0>(decl);
    case 1: return std::get<1>(decl);
    case 2: return std::get<2>(decl);
    case 3: return std::get<3>(decl);
    default: return nullptr;
    }
}

void yama::internal::ast_Decl::fmt(ast_formatter& x) {
    x.open("Decl", low_pos(), high_pos(), id);
    if (auto a = get_decl()) {
        x.next("decl");
        a->fmt(x);
    }
    x.close();
}

void yama::internal::ast_Decl::accept(ast_visitor& x) {
    x.visit_begin(expect<ast_Decl>());
    if (auto a = get_decl()) {
        a->accept(x);
    }
    x.visit_end(expect<ast_Decl>());
}

void yama::internal::ast_Decl::do_give(res<ast_ImportDecl> x) {
    decl = decltype(decl)(std::in_place_index<0>, x);
}

void yama::internal::ast_Decl::do_give(res<ast_VarDecl> x) {
    decl = decltype(decl)(std::in_place_index<1>, x);
}

void yama::internal::ast_Decl::do_give(res<ast_FnDecl> x) {
    decl = decltype(decl)(std::in_place_index<2>, x);
}

void yama::internal::ast_Decl::do_give(res<ast_StructDecl> x) {
    decl = decltype(decl)(std::in_place_index<3>, x);
}

std::optional<std::string> yama::internal::ast_ImportDecl::path(const str& src) const {
    return
        head
        ? std::make_optional(std::format("{}{}", head->str(src), relative_path ? relative_path->relative_path(src).value_or("") : ""))
        : std::nullopt;
}

void yama::internal::ast_ImportDecl::fmt(ast_formatter& x) {
    x.open("ImportDecl", low_pos(), high_pos(), id);
    if (relative_path) {
        x.next("relative_path");
        relative_path->fmt(x);
    }
    x.close();
}

void yama::internal::ast_ImportDecl::accept(ast_visitor& x) {
    x.visit_begin(expect<ast_ImportDecl>());
    if (relative_path) {
        relative_path->accept(x);
    }
    x.visit_end(expect<ast_ImportDecl>());
}

void yama::internal::ast_ImportDecl::do_give(taul::token x) {
    // first identifier is presumed to be head, and if second one is found,
    // then we know the first was actually the name, and the second one is
    // the head
    if (x.is_normal() && x.lpr->name() == "IDENTIFIER"_str) {
        if (head) {
            name = head;
        }
        head = x;
    }
}

void yama::internal::ast_ImportDecl::do_give(res<ast_RelativePath> x) {
    relative_path = x;
}

std::optional<std::string> yama::internal::ast_RelativePath::relative_path(const str& src) const {
    if (ids_and_dots.empty()) return std::nullopt;
    std::string result{};
    for (const auto& I : ids_and_dots) {
        result += I.str(src);
    }
    return result;
}

void yama::internal::ast_RelativePath::fmt(ast_formatter& x) {
    x.open("RelativePath", low_pos(), high_pos(), id);
    for (const auto& I : ids_and_dots) {
        x.next("ids_and_dots", I);
    }
    x.close();
}

void yama::internal::ast_RelativePath::accept(ast_visitor& x) {
    x.visit_begin(expect<ast_RelativePath>());
    x.visit_end(expect<ast_RelativePath>());
}

void yama::internal::ast_RelativePath::do_give(taul::token x) {
    if (x.is_normal() && x.lpr->name() == "IDENTIFIER"_str) {
        ids_and_dots.push_back(x);
    }
    else if (x.is_normal() && x.lpr->name() == "DOT"_str) {
        ids_and_dots.push_back(x);
    }
}

void yama::internal::ast_VarDecl::fmt(ast_formatter& x) {
    x.open("VarDecl", low_pos(), high_pos(), id);
    x.next("name", name);
    if (type) {
        x.next("type");
        type->fmt(x);
    }
    if (assign) {
        x.next("assign");
        assign->fmt(x);
    }
    x.close();
}

void yama::internal::ast_VarDecl::accept(ast_visitor& x) {
    x.visit_begin(expect<ast_VarDecl>());
    if (type) {
        type->accept(x);
    }
    if (assign) {
        assign->accept(x);
    }
    x.visit_end(expect<ast_VarDecl>());
}

void yama::internal::ast_VarDecl::do_give(taul::token x) {
    if (x.is_normal() && x.lpr->name() == "IDENTIFIER"_str) {
        name = x;
    }
}

void yama::internal::ast_VarDecl::do_give(res<ast_TypeAnnot> x) {
    type = x;
}

void yama::internal::ast_VarDecl::do_give(res<ast_Assign> x) {
    assign = x;
}

void yama::internal::ast_FnDecl::fmt(ast_formatter& x) {
    x.open("FnDecl", low_pos(), high_pos(), id);
    x.next("name", name);
    if (callsig) {
        x.next("callsig");
        callsig->fmt(x);
    }
    if (block) {
        x.next("block");
        block->fmt(x);
    }
    x.close();
}

void yama::internal::ast_FnDecl::accept(ast_visitor& x) {
    x.visit_begin(expect<ast_FnDecl>());
    if (callsig) {
        callsig->accept(x);
    }
    if (block) {
        block->accept(x);
    }
    x.visit_end(expect<ast_FnDecl>());
}

void yama::internal::ast_FnDecl::do_give(taul::token x) {
    if (x.is_normal() && x.lpr->name() == "IDENTIFIER"_str) {
        name = x;
    }
}

void yama::internal::ast_FnDecl::do_give(res<ast_CallSig> x) {
    callsig = x;
}

void yama::internal::ast_FnDecl::do_give(res<ast_Block> x) {
    block = x;
    x->is_fn_body_block = true;
}

void yama::internal::ast_StructDecl::fmt(ast_formatter& x) {
    x.open("StructDecl", low_pos(), high_pos(), id);
    x.next("name", name);
    x.close();
}

void yama::internal::ast_StructDecl::accept(ast_visitor& x) {
    x.visit_begin(expect<ast_StructDecl>());
    x.visit_end(expect<ast_StructDecl>());
}

void yama::internal::ast_StructDecl::do_give(taul::token x) {
    if (x.is_normal() && x.lpr->name() == "IDENTIFIER"_str) {
        name = x;
    }
}

void yama::internal::ast_CallSig::fmt(ast_formatter& x) {
    x.open("CallSig", low_pos(), high_pos(), id);
    x.next("params");
    for (const auto& I : params) {
        I->fmt(x);
    }
    if (result) {
        x.next("result");
        result->fmt(x);
    }
    x.close();
}

void yama::internal::ast_CallSig::accept(ast_visitor& x) {
    x.visit_begin(expect<ast_CallSig>());
    for (const auto& I : params) {
        I->accept(x);
    }
    if (result) {
        result->accept(x);
    }
    x.visit_end(expect<ast_CallSig>());
}

void yama::internal::ast_CallSig::do_give(res<ast_ParamDecl> x) {
    x->callsig = std::static_pointer_cast<ast_CallSig>(shared_from_this());
    x->index = params.size();
    params.push_back(x);
}

void yama::internal::ast_CallSig::do_give(res<ast_Result> x) {
    x->callsig = std::static_pointer_cast<ast_CallSig>(shared_from_this());
    result = x;
}

yama::res<yama::internal::ast_CallSig> yama::internal::ast_ParamDecl::get_callsig() const noexcept {
    return res(callsig.lock());
}

void yama::internal::ast_ParamDecl::fmt(ast_formatter& x) {
    x.open("ParamDecl", low_pos(), high_pos(), id);
    x.next("name", name);
    if (type) {
        x.next("type");
        type->fmt(x);
    }
    x.close();
}

void yama::internal::ast_ParamDecl::accept(ast_visitor& x) {
    x.visit_begin(expect<ast_ParamDecl>());
    if (type) {
        type->accept(x);
    }
    x.visit_end(expect<ast_ParamDecl>());
}

void yama::internal::ast_ParamDecl::do_give(taul::token x) {
    if (x.is_normal() && x.lpr->name() == "IDENTIFIER"_str) {
        name = x;
    }
}

void yama::internal::ast_ParamDecl::do_give(res<ast_TypeAnnot> x) {
    type = x;
}

yama::res<yama::internal::ast_CallSig> yama::internal::ast_Result::get_callsig() const noexcept {
    return res(callsig.lock());
}

void yama::internal::ast_Result::fmt(ast_formatter& x) {
    x.open("Result", low_pos(), high_pos(), id);
    if (type) {
        x.next("type");
        type->fmt(x);
    }
    x.close();
}

void yama::internal::ast_Result::accept(ast_visitor& x) {
    x.visit_begin(expect<ast_Result>());
    if (type) {
        type->accept(x);
    }
    x.visit_end(expect<ast_Result>());
}

void yama::internal::ast_Result::do_give(res<ast_TypeSpec> x) {
    type = x;
}

void yama::internal::ast_Block::fmt(ast_formatter& x) {
    x.open("Block", low_pos(), high_pos(), id);
    x.next("stmts");
    for (const auto& I : stmts) {
        I->fmt(x);
    }
    x.close();
}

void yama::internal::ast_Block::accept(ast_visitor& x) {
    x.visit_begin(expect<ast_Block>());
    for (const auto& I : stmts) {
        I->accept(x);
    }
    x.visit_end(expect<ast_Block>());
}

void yama::internal::ast_Block::do_give(res<ast_Stmt> x) {
    stmts.push_back(x);
}

std::shared_ptr<yama::internal::ast_node> yama::internal::ast_Stmt::get_stmt_or_decl() {
    switch (stmt_or_decl.index()) {
    case 0: return std::get<0>(stmt_or_decl);
    case 1: return std::get<1>(stmt_or_decl);
    case 2: return std::get<2>(stmt_or_decl);
    case 3: return std::get<3>(stmt_or_decl);
    case 4: return std::get<4>(stmt_or_decl);
    case 5: return std::get<5>(stmt_or_decl);
    case 6: return std::get<6>(stmt_or_decl);
    default: return nullptr;
    }
}

void yama::internal::ast_Stmt::fmt(ast_formatter& x) {
    x.open("Stmt", low_pos(), high_pos(), id);
    if (auto a = get_stmt_or_decl()) {
        x.next("stmt_or_decl");
        a->fmt(x);
    }
    x.close();
}

void yama::internal::ast_Stmt::accept(ast_visitor& x) {
    x.visit_begin(expect<ast_Stmt>());
    if (auto a = get_stmt_or_decl()) {
        a->accept(x);
    }
    x.visit_end(expect<ast_Stmt>());
}

void yama::internal::ast_Stmt::do_give(res<ast_Decl> x) {
    stmt_or_decl = decltype(stmt_or_decl)(std::in_place_index<0>, x);
}

void yama::internal::ast_Stmt::do_give(res<ast_ExprStmt> x) {
    stmt_or_decl = decltype(stmt_or_decl)(std::in_place_index<1>, x);
}

void yama::internal::ast_Stmt::do_give(res<ast_IfStmt> x) {
    stmt_or_decl = decltype(stmt_or_decl)(std::in_place_index<2>, x);
}

void yama::internal::ast_Stmt::do_give(res<ast_LoopStmt> x) {
    stmt_or_decl = decltype(stmt_or_decl)(std::in_place_index<3>, x);
}

void yama::internal::ast_Stmt::do_give(res<ast_BreakStmt> x) {
    stmt_or_decl = decltype(stmt_or_decl)(std::in_place_index<4>, x);
}

void yama::internal::ast_Stmt::do_give(res<ast_ContinueStmt> x) {
    stmt_or_decl = decltype(stmt_or_decl)(std::in_place_index<5>, x);
}

void yama::internal::ast_Stmt::do_give(res<ast_ReturnStmt> x) {
    stmt_or_decl = decltype(stmt_or_decl)(std::in_place_index<6>, x);
}

void yama::internal::ast_ExprStmt::fmt(ast_formatter& x) {
    x.open("ExprStmt", low_pos(), high_pos(), id);
    if (expr) {
        x.next("expr");
        expr->fmt(x);
    }
    if (assign) {
        x.next("assign");
        assign->fmt(x);
    }
    x.close();
}

void yama::internal::ast_ExprStmt::accept(ast_visitor& x) {
    x.visit_begin(expect<ast_ExprStmt>());
    if (expr) {
        expr->accept(x);
    }
    if (assign) {
        assign->accept(x);
    }
    x.visit_end(expect<ast_ExprStmt>());
}

void yama::internal::ast_ExprStmt::do_give(res<ast_Expr> x) {
    expr = x;
}

void yama::internal::ast_ExprStmt::do_give(res<ast_Assign> x) {
    assign = x;
    if (expr) {
        expr->is_assign_stmt_lvalue = true;
    }
}

std::shared_ptr<yama::internal::ast_node> yama::internal::ast_IfStmt::get_else_block_or_stmt() const noexcept {
    switch (else_block_or_stmt.index()) {
    case 0: return std::get<0>(else_block_or_stmt);
    case 1: return std::get<1>(else_block_or_stmt);
    default: return nullptr;
    }
}

void yama::internal::ast_IfStmt::fmt(ast_formatter& x) {
    x.open("IfStmt", low_pos(), high_pos(), id);
    if (cond) {
        x.next("cond");
        cond->fmt(x);
    }
    if (block) {
        x.next("block");
        block->fmt(x);
    }
    if (auto a = get_else_block_or_stmt()) {
        x.next("else_block_or_stmt");
        a->fmt(x);
    }
    x.close();
}

void yama::internal::ast_IfStmt::accept(ast_visitor& x) {
    x.visit_begin(expect<ast_IfStmt>());
    if (cond) {
        cond->accept(x);
    }
    if (block) {
        block->accept(x);
    }
    if (auto a = get_else_block_or_stmt()) {
        a->accept(x);
    }
    x.visit_end(expect<ast_IfStmt>());
}

void yama::internal::ast_IfStmt::do_give(res<ast_Expr> x) {
    cond = x;
}

void yama::internal::ast_IfStmt::do_give(res<ast_Block> x) {
    if (!block) { // first is for true block
        block = x;
    }
    else { // second, if any, is for false block
        else_block_or_stmt = decltype(else_block_or_stmt)(std::in_place_index<0>, x);
    }
}

void yama::internal::ast_IfStmt::do_give(res<ast_IfStmt> x) {
    else_block_or_stmt = decltype(else_block_or_stmt)(std::in_place_index<1>, x);
}

void yama::internal::ast_LoopStmt::fmt(ast_formatter& x) {
    x.open("LoopStmt", low_pos(), high_pos(), id);
    if (block) {
        x.next("block");
        block->fmt(x);
    }
    x.close();
}

void yama::internal::ast_LoopStmt::accept(ast_visitor& x) {
    x.visit_begin(expect<ast_LoopStmt>());
    if (block) {
        block->accept(x);
    }
    x.visit_end(expect<ast_LoopStmt>());
}

void yama::internal::ast_LoopStmt::do_give(res<ast_Block> x) {
    block = x;
}

void yama::internal::ast_BreakStmt::fmt(ast_formatter& x) {
    x.open("BreakStmt", low_pos(), high_pos(), id);
    x.close();
}

void yama::internal::ast_BreakStmt::accept(ast_visitor& x) {
    x.visit_begin(expect<ast_BreakStmt>());
    x.visit_end(expect<ast_BreakStmt>());
}

void yama::internal::ast_ContinueStmt::fmt(ast_formatter& x) {
    x.open("ContinueStmt", low_pos(), high_pos(), id);
    x.close();
}

void yama::internal::ast_ContinueStmt::accept(ast_visitor& x) {
    x.visit_begin(expect<ast_ContinueStmt>());
    x.visit_end(expect<ast_ContinueStmt>());
}

void yama::internal::ast_ReturnStmt::fmt(ast_formatter& x) {
    x.open("ReturnStmt", low_pos(), high_pos(), id);
    if (expr) {
        x.next("expr");
        expr->fmt(x);
    }
    x.close();
}

void yama::internal::ast_ReturnStmt::accept(ast_visitor& x) {
    x.visit_begin(expect<ast_ReturnStmt>());
    if (expr) {
        expr->accept(x);
    }
    x.visit_end(expect<ast_ReturnStmt>());
}

void yama::internal::ast_ReturnStmt::do_give(res<ast_Expr> x) {
    expr = x;
}

void yama::internal::ast_Expr::fmt(ast_formatter& x) {
    x.open("Expr", low_pos(), high_pos(), id);
    if (base) {
        x.next("base");
        base->fmt(x);
    }
    x.next("args");
    for (const auto& I : suffixes) {
        I->fmt(x);
    }
    x.close();
}

void yama::internal::ast_Expr::accept(ast_visitor& x) {
    x.visit_begin(expect<ast_Expr>());
    if (base) {
        base->accept(x);
    }
    for (const auto& suffix : suffixes) {
        suffix->accept(x);
    }
    x.visit_end(expect<ast_Expr>());
}

std::shared_ptr<yama::internal::ast_expr> yama::internal::ast_Expr::get_primary_subexpr() const noexcept {
    return
        suffixes.empty()
        ? base
        : std::static_pointer_cast<ast_expr>(suffixes.back().base());
}

void yama::internal::ast_Expr::do_give(taul::token x) {
    if (x.is_normal() && x.lpr->name() == "CONST"_str) {
        has_const_kw = true;
    }
}

void yama::internal::ast_Expr::do_give(res<ast_PrimaryExpr> x) {
    YAMA_ASSERT(!base);
    base = x;
}

void yama::internal::ast_Expr::do_give(res<ast_Args> x) {
    x->index = suffixes.size();
    suffixes.push_back(x);
}

std::optional<std::string> yama::internal::ast_PrimaryExpr::fmt_name(const taul::source_code& src) const {
    if (!name) return std::nullopt;
    std::optional result =
        qualifier
        ? std::format("{}:{}", qualifier->str(src).fmt(), name->str(src).fmt())
        : name->str(src).fmt();
    return result;
}

void yama::internal::ast_PrimaryExpr::fmt(ast_formatter& x) {
    x.open("PrimaryExpr", low_pos(), high_pos(), id);
    if (qualifier) {
        x.next("qualifier", *qualifier);
    }
    if (name) {
        x.next("name", *name);
    }
    if (lit) {
        x.next("lit");
        lit->fmt(x);
    }
    x.close();
}

void yama::internal::ast_PrimaryExpr::accept(ast_visitor& x) {
    x.visit_begin(expect<ast_PrimaryExpr>());
    if (lit) {
        lit->accept(x);
    }
    x.visit_end(expect<ast_PrimaryExpr>());
}

void yama::internal::ast_PrimaryExpr::do_give(taul::token x) {
    if (x.is_normal() && x.lpr->name() == "IDENTIFIER"_str) {
        if (name) {
            // if we encounter second identifier, then first one was actually
            // for qualifier, not name
            qualifier = name;
        }
        name = x;
    }
}

void yama::internal::ast_PrimaryExpr::do_give(res<ast_Lit> x) {
    lit = x;
}

std::shared_ptr<yama::internal::ast_node> yama::internal::ast_Lit::get_lit() {
    switch (lit.index()) {
    case 0: return std::get<0>(lit);
    case 1: return std::get<1>(lit);
    case 2: return std::get<2>(lit);
    case 3: return std::get<3>(lit);
    case 4: return std::get<4>(lit);
    default: return nullptr;
    }
}

void yama::internal::ast_Lit::fmt(ast_formatter& x) {
    x.open("Lit", low_pos(), high_pos(), id);
    if (auto a = get_lit()) {
        x.next("lit");
        a->fmt(x);
    }
    x.close();
}

void yama::internal::ast_Lit::accept(ast_visitor& x) {
    x.visit_begin(expect<ast_Lit>());
    if (auto a = get_lit()) {
        a->accept(x);
    }
    x.visit_end(expect<ast_Lit>());
}

void yama::internal::ast_Lit::do_give(res<ast_IntLit> x) {
    lit = decltype(lit)(std::in_place_index<0>, x);
}

void yama::internal::ast_Lit::do_give(res<ast_UIntLit> x) {
    lit = decltype(lit)(std::in_place_index<1>, x);
}

void yama::internal::ast_Lit::do_give(res<ast_FloatLit> x) {
    lit = decltype(lit)(std::in_place_index<2>, x);
}

void yama::internal::ast_Lit::do_give(res<ast_BoolLit> x) {
    lit = decltype(lit)(std::in_place_index<3>, x);
}

void yama::internal::ast_Lit::do_give(res<ast_CharLit> x) {
    lit = decltype(lit)(std::in_place_index<4>, x);
}

void yama::internal::ast_IntLit::fmt(ast_formatter& x) {
    x.open("IntLit", low_pos(), high_pos(), id);
    x.next("lit", lit);
    x.close();
}

void yama::internal::ast_IntLit::accept(ast_visitor& x) {
    x.visit_begin(expect<ast_IntLit>());
    x.visit_end(expect<ast_IntLit>());
}

void yama::internal::ast_IntLit::do_give(taul::token x) {
    lit = x;
}

void yama::internal::ast_UIntLit::fmt(ast_formatter& x) {
    x.open("UIntLit", low_pos(), high_pos(), id);
    x.next("lit", lit);
    x.close();
}

void yama::internal::ast_UIntLit::accept(ast_visitor& x) {
    x.visit_begin(expect<ast_UIntLit>());
    x.visit_end(expect<ast_UIntLit>());
}

void yama::internal::ast_UIntLit::do_give(taul::token x) {
    lit = x;
}

void yama::internal::ast_FloatLit::fmt(ast_formatter& x) {
    x.open("FloatLit", low_pos(), high_pos(), id);
    x.next("lit", lit);
    x.close();
}

void yama::internal::ast_FloatLit::accept(ast_visitor& x) {
    x.visit_begin(expect<ast_FloatLit>());
    x.visit_end(expect<ast_FloatLit>());
}

void yama::internal::ast_FloatLit::do_give(taul::token x) {
    lit = x;
}

void yama::internal::ast_BoolLit::fmt(ast_formatter& x) {
    x.open("BoolLit", low_pos(), high_pos(), id);
    x.next("lit", lit);
    x.close();
}

void yama::internal::ast_BoolLit::accept(ast_visitor& x) {
    x.visit_begin(expect<ast_BoolLit>());
    x.visit_end(expect<ast_BoolLit>());
}

void yama::internal::ast_BoolLit::do_give(taul::token x) {
    lit = x;
}

void yama::internal::ast_CharLit::fmt(ast_formatter& x) {
    x.open("CharLit", low_pos(), high_pos(), id);
    x.next("lit", lit);
    x.close();
}

void yama::internal::ast_CharLit::accept(ast_visitor& x) {
    x.visit_begin(expect<ast_CharLit>());
    x.visit_end(expect<ast_CharLit>());
}

void yama::internal::ast_CharLit::do_give(taul::token x) {
    lit = x;
}

void yama::internal::ast_Assign::fmt(ast_formatter& x) {
    x.open("Assign", low_pos(), high_pos(), id);
    if (expr) {
        x.next("expr");
        expr->fmt(x);
    }
    x.close();
}

void yama::internal::ast_Assign::accept(ast_visitor& x) {
    x.visit_begin(expect<ast_Assign>());
    if (expr) {
        expr->accept(x);
    }
    x.visit_end(expect<ast_Assign>());
}

void yama::internal::ast_Assign::do_give(res<ast_Expr> x) {
    expr = x;
}

bool yama::internal::ast_Args::is_constexpr_guarantee_expr_args() const noexcept {
    return index == 0 && parent()->expect<ast_Expr>()->has_const_kw;
}

void yama::internal::ast_Args::fmt(ast_formatter& x) {
    x.open("Args", low_pos(), high_pos(), id);
    x.next("args");
    for (const auto& I : args) {
        I->fmt(x);
    }
    x.close();
}

void yama::internal::ast_Args::accept(ast_visitor& x) {
    x.visit_begin(expect<ast_Args>());
    for (const auto& I : args) {
        I->accept(x);
    }
    x.visit_end(expect<ast_Args>());
}

void yama::internal::ast_Args::do_give(res<ast_Expr> x) {
    args.push_back(x);
}

void yama::internal::ast_TypeAnnot::fmt(ast_formatter& x) {
    x.open("TypeAnnot", low_pos(), high_pos(), id);
    if (type) {
        x.next("type");
        type->fmt(x);
    }
    x.close();
}

void yama::internal::ast_TypeAnnot::accept(ast_visitor& x) {
    x.visit_begin(expect<ast_TypeAnnot>());
    if (type) {
        type->accept(x);
    }
    x.visit_end(expect<ast_TypeAnnot>());
}

void yama::internal::ast_TypeAnnot::do_give(res<ast_TypeSpec> x) {
    type = x;
}

std::optional<yama::internal::ctype> yama::internal::ast_TypeSpec::get_type(compiler& cs) const {
    return cs.ea.crvalue_to_type(*this);
}

void yama::internal::ast_TypeSpec::fmt(ast_formatter& x) {
    x.open("TypeSpec", low_pos(), high_pos(), id);
    if (expr) {
        x.next("expr");
        expr->fmt(x);
    }
    x.close();
}

void yama::internal::ast_TypeSpec::accept(ast_visitor& x) {
    x.visit_begin(expect<ast_TypeSpec>());
    if (expr) {
        expr->accept(x);
    }
    x.visit_end(expect<ast_TypeSpec>());
}

void yama::internal::ast_TypeSpec::do_give(res<ast_Expr> x) {
    x->is_type_spec_crvalue = true;
    expr = x;
}

void yama::internal::ast_parser::listener::on_startup() {
    client()._syntax_error = std::nullopt;
    client()._result = nullptr;
    client()._next_id = 0;
}

void yama::internal::ast_parser::listener::on_shutdown() {
    client()._stk.clear();
}

void yama::internal::ast_parser::listener::on_lexical(taul::token tkn) {
    if (!client()._stk.empty()) {
        client()._stk.back()->dispatch_give(tkn);
    }
}

void yama::internal::ast_parser::listener::on_syntactic(taul::ppr_ref ppr, taul::source_pos pos) {
#define _YAMA_UNIT_(nm) \
if (ppr.name() == #nm ""_str) stk().push_back(make_res<ast_ ## nm>(pos, client()._next_id))

    static_assert(ast_types == 30); // reminder

    _YAMA_UNIT_(Chunk);
    else _YAMA_UNIT_(Decl);
    else _YAMA_UNIT_(ImportDecl);
    else _YAMA_UNIT_(RelativePath);
    else _YAMA_UNIT_(VarDecl);
    else _YAMA_UNIT_(FnDecl);
    else _YAMA_UNIT_(StructDecl);
    else _YAMA_UNIT_(CallSig);
    else _YAMA_UNIT_(ParamDecl);
    else _YAMA_UNIT_(Result);
    else _YAMA_UNIT_(Block);
    else _YAMA_UNIT_(Stmt);
    else _YAMA_UNIT_(ExprStmt);
    else _YAMA_UNIT_(IfStmt);
    else _YAMA_UNIT_(LoopStmt);
    else _YAMA_UNIT_(BreakStmt);
    else _YAMA_UNIT_(ContinueStmt);
    else _YAMA_UNIT_(ReturnStmt);
    else _YAMA_UNIT_(Expr);
    else _YAMA_UNIT_(PrimaryExpr);
    else _YAMA_UNIT_(Lit);
    else _YAMA_UNIT_(IntLit);
    else _YAMA_UNIT_(UIntLit);
    else _YAMA_UNIT_(FloatLit);
    else _YAMA_UNIT_(BoolLit);
    else _YAMA_UNIT_(CharLit);
    else _YAMA_UNIT_(Assign);
    else _YAMA_UNIT_(Args);
    else _YAMA_UNIT_(TypeAnnot);
    else _YAMA_UNIT_(TypeSpec);

    else YAMA_DEADEND;

#undef _YAMA_UNIT_

    client()._next_id++;
}

void yama::internal::ast_parser::listener::on_close() {
    YAMA_ASSERT(!stk().empty());
    const res old_top = stk().back();
    stk().pop_back();
    
    if (!stk().empty()) { // give to next one down
        YAMA_ASSERT(old_top != stk().back());
        old_top->parent_node = stk().back().base();
        stk().back()->give(old_top);
    }
    else { // submit as parse result
        client()._result = old_top->as<ast_Chunk>();
    }
}

void yama::internal::ast_parser::listener::on_terminal_error(taul::token_range ids, taul::token input) {
    client()._syntax_error = input.low_pos();
}

void yama::internal::ast_parser::listener::on_nonterminal_error(taul::symbol_id id, taul::token input) {
    client()._syntax_error = input.low_pos();
}

yama::internal::ast_parser::result yama::internal::ast_parser::parse(const taul::source_code& src) {
    const auto gram = yama_gram();
    taul::source_reader rdr(src);
    taul::lexer lxr(gram);
    taul::parser psr(gram);
    taul::no_recovery_error_handler eh{};
    listener lstnr(*this);
    lxr.bind_source(&rdr);
    psr.bind_source(&lxr);
    psr.bind_error_handler(&eh);
    psr.bind_listener(&lstnr);
    psr.reset(); // flush pipeline before use
#if 1
    psr.parse_notree("Chunk"_str);
#else
    auto tree = psr.parse("Chunk"_str);
    println("{}", tree.fmt());
#endif
    return result{
        .syntax_error = _syntax_error,
        .root = _result,
    };
}

