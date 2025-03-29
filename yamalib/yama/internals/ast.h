

#pragma once


#include <memory>
#include <vector>
#include <variant>

#include <taul/source_code.h>
#include <taul/symbols.h>
#include <taul/listener.h>

#include "../core/general.h"
#include "../core/scalars.h"
#include "../core/res.h"


namespace yama::internal {


    // quick-n'-dirty Yama syntax AST

    // later on we may clean-up/replace this w/ something more *official*


    using ast_id_t = size_t;


    class ast_visitor;

    class ast_formatter;

    class ast_node;

    class ast_Chunk;
    class ast_DeclOrDir;
    class ast_ImportDir;
    class ast_ImportPath;
    class ast_VarDecl;
    class ast_FnDecl;
    class ast_CallSig;
    class ast_ParamDecl;
    class ast_Result;
    class ast_Block;
    class ast_Stmt;
    class ast_ExprStmt;
    class ast_IfStmt;
    class ast_LoopStmt;
    class ast_BreakStmt;
    class ast_ContinueStmt;
    class ast_ReturnStmt;
    class ast_Expr;
    class ast_PrimaryExpr;
    class ast_Lit;
    class ast_IntLit;
    class ast_UIntLit;
    class ast_FloatLit;
    class ast_BoolLit;
    class ast_CharLit;
    class ast_Assign;
    class ast_Args;
    class ast_TypeAnnot;
    class ast_TypeSpec;

    class ast_parser;


    class ast_visitor : public std::enable_shared_from_this<ast_visitor> {
    public:
        ast_visitor() = default;


        // visit_begin is fired before propagating to children

        virtual void visit_begin(res<ast_Chunk> x);
        virtual void visit_begin(res<ast_DeclOrDir> x);
        virtual void visit_begin(res<ast_ImportDir> x);
        virtual void visit_begin(res<ast_ImportPath> x);
        virtual void visit_begin(res<ast_VarDecl> x);
        virtual void visit_begin(res<ast_FnDecl> x);
        virtual void visit_begin(res<ast_CallSig> x);
        virtual void visit_begin(res<ast_ParamDecl> x);
        virtual void visit_begin(res<ast_Result> x);
        virtual void visit_begin(res<ast_Block> x);
        virtual void visit_begin(res<ast_Stmt> x);
        virtual void visit_begin(res<ast_ExprStmt> x);
        virtual void visit_begin(res<ast_IfStmt> x);
        virtual void visit_begin(res<ast_LoopStmt> x);
        virtual void visit_begin(res<ast_BreakStmt> x);
        virtual void visit_begin(res<ast_ContinueStmt> x);
        virtual void visit_begin(res<ast_ReturnStmt> x);
        virtual void visit_begin(res<ast_Expr> x);
        virtual void visit_begin(res<ast_PrimaryExpr> x);
        virtual void visit_begin(res<ast_Lit> x);
        virtual void visit_begin(res<ast_IntLit> x);
        virtual void visit_begin(res<ast_UIntLit> x);
        virtual void visit_begin(res<ast_FloatLit> x);
        virtual void visit_begin(res<ast_BoolLit> x);
        virtual void visit_begin(res<ast_CharLit> x);
        virtual void visit_begin(res<ast_Assign> x);
        virtual void visit_begin(res<ast_Args> x);
        virtual void visit_begin(res<ast_TypeAnnot> x);
        virtual void visit_begin(res<ast_TypeSpec> x);

        // visit_end is fired after propagating to children

        virtual void visit_end(res<ast_Chunk> x);
        virtual void visit_end(res<ast_DeclOrDir> x);
        virtual void visit_end(res<ast_ImportDir> x);
        virtual void visit_end(res<ast_ImportPath> x);
        virtual void visit_end(res<ast_VarDecl> x);
        virtual void visit_end(res<ast_FnDecl> x);
        virtual void visit_end(res<ast_CallSig> x);
        virtual void visit_end(res<ast_ParamDecl> x);
        virtual void visit_end(res<ast_Result> x);
        virtual void visit_end(res<ast_Block> x);
        virtual void visit_end(res<ast_Stmt> x);
        virtual void visit_end(res<ast_ExprStmt> x);
        virtual void visit_end(res<ast_IfStmt> x);
        virtual void visit_end(res<ast_LoopStmt> x);
        virtual void visit_end(res<ast_BreakStmt> x);
        virtual void visit_end(res<ast_ContinueStmt> x);
        virtual void visit_end(res<ast_ReturnStmt> x);
        virtual void visit_end(res<ast_Expr> x);
        virtual void visit_end(res<ast_PrimaryExpr> x);
        virtual void visit_end(res<ast_Lit> x);
        virtual void visit_end(res<ast_IntLit> x);
        virtual void visit_end(res<ast_UIntLit> x);
        virtual void visit_end(res<ast_FloatLit> x);
        virtual void visit_end(res<ast_BoolLit> x);
        virtual void visit_end(res<ast_CharLit> x);
        virtual void visit_end(res<ast_Assign> x);
        virtual void visit_end(res<ast_Args> x);
        virtual void visit_end(res<ast_TypeAnnot> x);
        virtual void visit_end(res<ast_TypeSpec> x);
    };


    class ast_formatter final {
    public:
        std::string output = "";


        inline ast_formatter(str src, const char* tab)
            : _src(src),
            _tab(tab) {
            YAMA_ASSERT(tab);
        }


        inline ast_formatter& next(std::string_view name) {
            _print_tabs();
            output += std::format("({0})\n", std::string(name));
            return *this;
        }

        inline ast_formatter& next(std::string_view name, taul::token x) {
            _print_tabs();
            if (x.is_normal()) {
                output += std::format("{0}: {1} \"{2}\"\n", std::string(name), x, x.str(_src));
            }
            else {
                output += std::format("{0}: {1}\n", std::string(name), x);
            }
            return *this;
        }

        inline ast_formatter& open(std::string_view name, taul::source_pos low_pos, taul::source_pos high_pos, ast_id_t id) {
            _print_tabs();
            output += std::format("{0} {1} (id={2}) {{\n", std::string(name), taul::fmt_pos_and_len(low_pos, high_pos - low_pos), id);
            _tabs++;
            return *this;
        }

        inline ast_formatter& close() {
            YAMA_ASSERT(_tabs > 0);
            _tabs--;
            _print_tabs();
            output += "}\n";
            return *this;
        }


    private:
        str _src;
        const char* _tab;
        size_t _tabs = 0;


        inline void _print_tabs() {
            for (size_t i = 0; i < _tabs; i++) {
                output += _tab;
            }
        }
    };


    class ast_node : public std::enable_shared_from_this<ast_node> { // base class
    public:
        const ast_id_t id;
        std::weak_ptr<ast_node> parent;


        inline ast_node(taul::source_pos pos, ast_id_t id)
            : id(id),
            parent(std::weak_ptr<ast_node>{}),
            _low_pos(pos),
            _high_pos(pos) {}


        inline taul::source_pos low_pos() const noexcept { return _low_pos; }
        inline taul::source_pos high_pos() const noexcept { return _high_pos; }


        // the way we build ASTs is that we build a node, and then we populate it
        // w/ info as the TAUL listener encounters it, invoking these 'give' methods

        inline void give(taul::token x) {
            _high_pos = std::max(high_pos(), x.high_pos());
            do_give(x);
        }

        template<std::derived_from<ast_node> T>
        inline void give(res<T> x) {
            _high_pos = std::max(high_pos(), x->high_pos());
            do_give(x);
        }

        // need this type erased way of calling the correct do_give overload, given
        // some input node we don't otherwise know the type of

        virtual void give_to(ast_node& target) = 0; // TODO: maybe move to protected

        inline void give(res<ast_node> x) {
            x->give_to(*this);
        }


        virtual void fmt(ast_formatter& x) = 0;

        inline std::string fmt_tree(str src, const char* tab = "    ") {
            YAMA_ASSERT(tab);
            ast_formatter a(src, tab);
            fmt(a);
            return a.output;
        }


        virtual void accept(ast_visitor& x) = 0;


    protected:
        virtual void do_give(taul::token x);
        virtual void do_give(res<ast_Chunk> x);
        virtual void do_give(res<ast_DeclOrDir> x);
        virtual void do_give(res<ast_ImportDir> x);
        virtual void do_give(res<ast_ImportPath> x);
        virtual void do_give(res<ast_VarDecl> x);
        virtual void do_give(res<ast_FnDecl> x);
        virtual void do_give(res<ast_CallSig> x);
        virtual void do_give(res<ast_ParamDecl> x);
        virtual void do_give(res<ast_Result> x);
        virtual void do_give(res<ast_Block> x);
        virtual void do_give(res<ast_Stmt> x);
        virtual void do_give(res<ast_ExprStmt> x);
        virtual void do_give(res<ast_IfStmt> x);
        virtual void do_give(res<ast_LoopStmt> x);
        virtual void do_give(res<ast_BreakStmt> x);
        virtual void do_give(res<ast_ContinueStmt> x);
        virtual void do_give(res<ast_ReturnStmt> x);
        virtual void do_give(res<ast_Expr> x);
        virtual void do_give(res<ast_PrimaryExpr> x);
        virtual void do_give(res<ast_Lit> x);
        virtual void do_give(res<ast_IntLit> x);
        virtual void do_give(res<ast_UIntLit> x);
        virtual void do_give(res<ast_FloatLit> x);
        virtual void do_give(res<ast_BoolLit> x);
        virtual void do_give(res<ast_CharLit> x);
        virtual void do_give(res<ast_Assign> x);
        virtual void do_give(res<ast_Args> x);
        virtual void do_give(res<ast_TypeAnnot> x);
        virtual void do_give(res<ast_TypeSpec> x);


    private:
        taul::source_pos _low_pos, _high_pos;
    };


    class ast_Chunk final : public ast_node {
    public:
        std::vector<res<ast_DeclOrDir>> decls_and_dirs;


        inline ast_Chunk(taul::source_pos pos, ast_id_t id)
            : ast_node(pos, id) {}


        inline void give_to(ast_node& target) override final { target.give(res<ast_Chunk>(shared_from_this())); }
        void fmt(ast_formatter& x) override final;
        void accept(ast_visitor& x) override final;


    protected:
        void do_give(res<ast_DeclOrDir> x) override final;
    };

    class ast_DeclOrDir final : public ast_node {
    public:
        std::variant<
            std::shared_ptr<ast_ImportDir>,
            std::shared_ptr<ast_VarDecl>,
            std::shared_ptr<ast_FnDecl>
            > decl_or_dir;


        inline ast_DeclOrDir(taul::source_pos pos, ast_id_t id)
            : ast_node(pos, id) {}


        std::shared_ptr<ast_node> get_decl_or_dir();


        inline void give_to(ast_node& target) override final { target.give(res<ast_DeclOrDir>(shared_from_this())); }
        void fmt(ast_formatter& x) override final;
        void accept(ast_visitor& x) override final;


    protected:
        void do_give(res<ast_ImportDir> x) override final;
        void do_give(res<ast_VarDecl> x) override final;
        void do_give(res<ast_FnDecl> x) override final;
    };
    
    class ast_ImportDir final : public ast_node {
    public:
        std::shared_ptr<ast_ImportPath> import_path;


        inline ast_ImportDir(taul::source_pos pos, ast_id_t id)
            : ast_node(pos, id) {}


        std::optional<std::string> path(const str& src) const;


        inline void give_to(ast_node& target) override final { target.give(res<ast_ImportDir>(shared_from_this())); }
        void fmt(ast_formatter& x) override final;
        void accept(ast_visitor& x) override final;


    protected:
        void do_give(res<ast_ImportPath> x) override final;
    };
    
    class ast_ImportPath final : public ast_node {
    public:
        std::vector<taul::token> ids_and_dots;


        inline ast_ImportPath(taul::source_pos pos, ast_id_t id)
            : ast_node(pos, id) {}


        std::optional<std::string> path(const str& src) const;


        inline void give_to(ast_node& target) override final { target.give(res<ast_ImportPath>(shared_from_this())); }
        void fmt(ast_formatter& x) override final;
        void accept(ast_visitor& x) override final;


    protected:
        void do_give(taul::token x) override final;
    };
    
    class ast_VarDecl final : public ast_node {
    public:
        taul::token name;
        std::shared_ptr<ast_TypeAnnot> type;
        std::shared_ptr<ast_Assign> assign;


        inline ast_VarDecl(taul::source_pos pos, ast_id_t id)
            : ast_node(pos, id) {}


        inline void give_to(ast_node& target) override final { target.give(res<ast_VarDecl>(shared_from_this())); }
        void fmt(ast_formatter& x) override final;
        void accept(ast_visitor& x) override final;


    protected:
        void do_give(taul::token x) override final;
        void do_give(res<ast_TypeAnnot> x) override final;
        void do_give(res<ast_Assign> x) override final;
    };
    
    class ast_FnDecl final : public ast_node {
    public:
        taul::token name;
        std::shared_ptr<ast_CallSig> callsig;
        std::shared_ptr<ast_Block> block;


        inline ast_FnDecl(taul::source_pos pos, ast_id_t id)
            : ast_node(pos, id) {}


        inline void give_to(ast_node& target) override final { target.give(res<ast_FnDecl>(shared_from_this())); }
        void fmt(ast_formatter& x) override final;
        void accept(ast_visitor& x) override final;


    protected:
        void do_give(taul::token x) override final;
        void do_give(res<ast_CallSig> x) override final;
        void do_give(res<ast_Block> x) override final;
    };

    class ast_CallSig final : public ast_node {
    public:
        std::vector<res<ast_ParamDecl>> params;
        std::shared_ptr<ast_Result> result;


        inline ast_CallSig(taul::source_pos pos, ast_id_t id)
            : ast_node(pos, id) {}


        inline void give_to(ast_node& target) override final { target.give(res<ast_CallSig>(shared_from_this())); }
        void fmt(ast_formatter& x) override final;
        void accept(ast_visitor& x) override final;


    protected:
        void do_give(res<ast_ParamDecl> x) override final;
        void do_give(res<ast_Result> x) override final;
    };
    
    class ast_ParamDecl final : public ast_node {
    public:
        taul::token name;
        std::shared_ptr<ast_TypeAnnot> type;


        inline ast_ParamDecl(taul::source_pos pos, ast_id_t id)
            : ast_node(pos, id) {}


        inline void give_to(ast_node& target) override final { target.give(res<ast_ParamDecl>(shared_from_this())); }
        void fmt(ast_formatter& x) override final;
        void accept(ast_visitor& x) override final;


    protected:
        void do_give(taul::token x) override final;
        void do_give(res<ast_TypeAnnot> x) override final;
    };
    
    class ast_Result final : public ast_node {
    public:
        std::shared_ptr<ast_TypeSpec> type;


        inline ast_Result(taul::source_pos pos, ast_id_t id)
            : ast_node(pos, id) {}


        inline void give_to(ast_node& target) override final { target.give(res<ast_Result>(shared_from_this())); }
        void fmt(ast_formatter& x) override final;
        void accept(ast_visitor& x) override final;


    protected:
        void do_give(res<ast_TypeSpec> x) override final;
    };
    
    class ast_Block final : public ast_node {
    public:
        std::vector<res<ast_Stmt>> stmts;
        bool is_fn_body_block = false;


        inline ast_Block(taul::source_pos pos, ast_id_t id)
            : ast_node(pos, id) {}


        inline void give_to(ast_node& target) override final { target.give(res<ast_Block>(shared_from_this())); }
        void fmt(ast_formatter& x) override final;
        void accept(ast_visitor& x) override final;


    protected:
        void do_give(res<ast_Stmt> x) override final;
    };
    
    class ast_Stmt final : public ast_node {
    public:
        std::variant<
            std::shared_ptr<ast_DeclOrDir>,
            std::shared_ptr<ast_ExprStmt>,
            std::shared_ptr<ast_IfStmt>,
            std::shared_ptr<ast_LoopStmt>,
            std::shared_ptr<ast_BreakStmt>,
            std::shared_ptr<ast_ContinueStmt>,
            std::shared_ptr<ast_ReturnStmt>
            > stmt_decl_or_dir;


        inline ast_Stmt(taul::source_pos pos, ast_id_t id)
            : ast_node(pos, id) {}


        std::shared_ptr<ast_node> get_stmt_decl_or_dir();


        inline void give_to(ast_node& target) override final { target.give(res<ast_Stmt>(shared_from_this())); }
        void fmt(ast_formatter& x) override final;
        void accept(ast_visitor& x) override final;


    protected:
        void do_give(res<ast_DeclOrDir> x) override final;
        void do_give(res<ast_ExprStmt> x) override final;
        void do_give(res<ast_IfStmt> x) override final;
        void do_give(res<ast_LoopStmt> x) override final;
        void do_give(res<ast_BreakStmt> x) override final;
        void do_give(res<ast_ContinueStmt> x) override final;
        void do_give(res<ast_ReturnStmt> x) override final;
    };
    
    class ast_ExprStmt final : public ast_node {
    public:
        std::shared_ptr<ast_Expr> expr;
        std::shared_ptr<ast_Assign> assign;


        inline ast_ExprStmt(taul::source_pos pos, ast_id_t id)
            : ast_node(pos, id) {}


        inline void give_to(ast_node& target) override final { target.give(res<ast_ExprStmt>(shared_from_this())); }
        void fmt(ast_formatter& x) override final;
        void accept(ast_visitor& x) override final;


    protected:
        void do_give(res<ast_Expr> x) override final;
        void do_give(res<ast_Assign> x) override final;
    };
    
    class ast_IfStmt final : public ast_node {
    public:
        std::shared_ptr<ast_Expr> cond;
        std::shared_ptr<ast_Block> block;
        std::variant<
            std::shared_ptr<ast_Block>,
            std::shared_ptr<ast_IfStmt>
            > else_block_or_stmt;


        inline ast_IfStmt(taul::source_pos pos, ast_id_t id)
            : ast_node(pos, id) {}


        std::shared_ptr<ast_node> get_else_block_or_stmt() const noexcept;


        inline void give_to(ast_node& target) override final { target.give(res<ast_IfStmt>(shared_from_this())); }
        void fmt(ast_formatter& x) override final;
        void accept(ast_visitor& x) override final;


    protected:
        void do_give(res<ast_Expr> x) override final;
        void do_give(res<ast_Block> x) override final;
        void do_give(res<ast_IfStmt> x) override final;
    };
    
    class ast_LoopStmt final : public ast_node {
    public:
        std::shared_ptr<ast_Block> block;


        inline ast_LoopStmt(taul::source_pos pos, ast_id_t id)
            : ast_node(pos, id) {}


        inline void give_to(ast_node& target) override final { target.give(res<ast_LoopStmt>(shared_from_this())); }
        void fmt(ast_formatter& x) override final;
        void accept(ast_visitor& x) override final;


    protected:
        void do_give(res<ast_Block> x) override final;
    };
    
    class ast_BreakStmt final : public ast_node {
    public:
        inline ast_BreakStmt(taul::source_pos pos, ast_id_t id)
            : ast_node(pos, id) {}


        inline void give_to(ast_node& target) override final { target.give(res<ast_BreakStmt>(shared_from_this())); }
        void fmt(ast_formatter& x) override final;
        void accept(ast_visitor& x) override final;
    };
    
    class ast_ContinueStmt final : public ast_node {
    public:
        inline ast_ContinueStmt(taul::source_pos pos, ast_id_t id)
            : ast_node(pos, id) {}


        inline void give_to(ast_node& target) override final { target.give(res<ast_ContinueStmt>(shared_from_this())); }
        void fmt(ast_formatter& x) override final;
        void accept(ast_visitor& x) override final;
    };
    
    class ast_ReturnStmt final : public ast_node {
    public:
        std::shared_ptr<ast_Expr> expr;


        inline ast_ReturnStmt(taul::source_pos pos, ast_id_t id)
            : ast_node(pos, id) {}


        inline void give_to(ast_node& target) override final { target.give(res<ast_ReturnStmt>(shared_from_this())); }
        void fmt(ast_formatter& x) override final;
        void accept(ast_visitor& x) override final;


    protected:
        void do_give(res<ast_Expr> x) override final;
    };
    
    class ast_Expr final : public ast_node {
    public:
        std::shared_ptr<ast_PrimaryExpr> primary;
        std::vector<res<ast_Args>> args;
        bool is_assign_stmt_lvalue = false;


        inline ast_Expr(taul::source_pos pos, ast_id_t id)
            : ast_node(pos, id) {}


        inline void give_to(ast_node& target) override final { target.give(res<ast_Expr>(shared_from_this())); }
        void fmt(ast_formatter& x) override final;
        void accept(ast_visitor& x) override final;


    protected:
        void do_give(res<ast_PrimaryExpr> x) override final;
        void do_give(res<ast_Args> x) override final;
    };
    
    class ast_PrimaryExpr final : public ast_node {
    public:
        std::optional<taul::token> name;
        std::shared_ptr<ast_Lit> lit;


        inline ast_PrimaryExpr(taul::source_pos pos, ast_id_t id)
            : ast_node(pos, id) {}


        inline void give_to(ast_node& target) override final { target.give(res<ast_PrimaryExpr>(shared_from_this())); }
        void fmt(ast_formatter& x) override final;
        void accept(ast_visitor& x) override final;


    protected:
        void do_give(taul::token x) override final;
        void do_give(res<ast_Lit> x) override final;
    };
    
    class ast_Lit final : public ast_node {
    public:
        std::variant<
            std::shared_ptr<ast_IntLit>,
            std::shared_ptr<ast_UIntLit>,
            std::shared_ptr<ast_FloatLit>,
            std::shared_ptr<ast_BoolLit>,
            std::shared_ptr<ast_CharLit>
            > lit;


        inline ast_Lit(taul::source_pos pos, ast_id_t id)
            : ast_node(pos, id) {}


        std::shared_ptr<ast_node> get_lit();

        inline bool is_int() const noexcept { return std::holds_alternative<std::shared_ptr<ast_IntLit>>(lit); }
        inline bool is_uint() const noexcept { return std::holds_alternative<std::shared_ptr<ast_UIntLit>>(lit); }
        inline bool is_float() const noexcept { return std::holds_alternative<std::shared_ptr<ast_FloatLit>>(lit); }
        inline bool is_bool() const noexcept { return std::holds_alternative<std::shared_ptr<ast_BoolLit>>(lit); }
        inline bool is_char() const noexcept { return std::holds_alternative<std::shared_ptr<ast_CharLit>>(lit); }

        inline std::shared_ptr<ast_IntLit> as_int() const { YAMA_ASSERT(is_int()); return std::get<0>(lit); }
        inline std::shared_ptr<ast_UIntLit> as_uint() const { YAMA_ASSERT(is_uint()); return std::get<1>(lit); }
        inline std::shared_ptr<ast_FloatLit> as_float() const { YAMA_ASSERT(is_float()); return std::get<2>(lit); }
        inline std::shared_ptr<ast_BoolLit> as_bool() const { YAMA_ASSERT(is_bool()); return std::get<3>(lit); }
        inline std::shared_ptr<ast_CharLit> as_char() const { YAMA_ASSERT(is_char()); return std::get<4>(lit); }


        inline void give_to(ast_node& target) override final { target.give(res<ast_Lit>(shared_from_this())); }
        void fmt(ast_formatter& x) override final;
        void accept(ast_visitor& x) override final;


    protected:
        void do_give(res<ast_IntLit> x) override final;
        void do_give(res<ast_UIntLit> x) override final;
        void do_give(res<ast_FloatLit> x) override final;
        void do_give(res<ast_BoolLit> x) override final;
        void do_give(res<ast_CharLit> x) override final;
    };
    
    class ast_IntLit final : public ast_node {
    public:
        taul::token lit;


        inline ast_IntLit(taul::source_pos pos, ast_id_t id)
            : ast_node(pos, id) {}


        inline void give_to(ast_node& target) override final { target.give(res<ast_IntLit>(shared_from_this())); }
        void fmt(ast_formatter& x) override final;
        void accept(ast_visitor& x) override final;


    protected:
        void do_give(taul::token x) override final;
    };
    
    class ast_UIntLit final : public ast_node {
    public:
        taul::token lit;


        inline ast_UIntLit(taul::source_pos pos, ast_id_t id)
            : ast_node(pos, id) {}


        inline void give_to(ast_node& target) override final { target.give(res<ast_UIntLit>(shared_from_this())); }
        void fmt(ast_formatter& x) override final;
        void accept(ast_visitor& x) override final;


    protected:
        void do_give(taul::token x) override final;
    };
    
    class ast_FloatLit final : public ast_node {
    public:
        taul::token lit;


        inline ast_FloatLit(taul::source_pos pos, ast_id_t id)
            : ast_node(pos, id) {}


        inline void give_to(ast_node& target) override final { target.give(res<ast_FloatLit>(shared_from_this())); }
        void fmt(ast_formatter& x) override final;
        void accept(ast_visitor& x) override final;


    protected:
        void do_give(taul::token x) override final;
    };
    
    class ast_BoolLit final : public ast_node {
    public:
        taul::token lit;


        inline ast_BoolLit(taul::source_pos pos, ast_id_t id)
            : ast_node(pos, id) {}


        inline void give_to(ast_node& target) override final { target.give(res<ast_BoolLit>(shared_from_this())); }
        void fmt(ast_formatter& x) override final;
        void accept(ast_visitor& x) override final;


    protected:
        void do_give(taul::token x) override final;
    };
    
    class ast_CharLit final : public ast_node {
    public:
        taul::token lit;


        inline ast_CharLit(taul::source_pos pos, ast_id_t id)
            : ast_node(pos, id) {}


        inline void give_to(ast_node& target) override final { target.give(res<ast_CharLit>(shared_from_this())); }
        void fmt(ast_formatter& x) override final;
        void accept(ast_visitor& x) override final;


    protected:
        void do_give(taul::token x) override final;
    };
    
    class ast_Assign final : public ast_node {
    public:
        std::shared_ptr<ast_Expr> expr;


        inline ast_Assign(taul::source_pos pos, ast_id_t id)
            : ast_node(pos, id) {}


        inline void give_to(ast_node& target) override final { target.give(res<ast_Assign>(shared_from_this())); }
        void fmt(ast_formatter& x) override final;
        void accept(ast_visitor& x) override final;


    protected:
        void do_give(res<ast_Expr> x) override final;
    };
    
    class ast_Args final : public ast_node {
    public:
        std::weak_ptr<ast_Expr> expr; // back-ref to the expr this Args exists within
        std::vector<res<ast_Expr>> args;


        inline ast_Args(taul::source_pos pos, ast_id_t id)
            : ast_node(pos, id) {}


        inline void give_to(ast_node& target) override final { target.give(res<ast_Args>(shared_from_this())); }
        void fmt(ast_formatter& x) override final;
        void accept(ast_visitor& x) override final;


    protected:
        void do_give(res<ast_Expr> x) override final;
    };
    
    class ast_TypeAnnot final : public ast_node {
    public:
        std::shared_ptr<ast_TypeSpec> type;


        inline ast_TypeAnnot(taul::source_pos pos, ast_id_t id)
            : ast_node(pos, id) {}


        inline void give_to(ast_node& target) override final { target.give(res<ast_TypeAnnot>(shared_from_this())); }
        void fmt(ast_formatter& x) override final;
        void accept(ast_visitor& x) override final;


    protected:
        void do_give(res<ast_TypeSpec> x) override final;
    };
    
    class ast_TypeSpec final : public ast_node {
    public:
        taul::token type;


        inline ast_TypeSpec(taul::source_pos pos, ast_id_t id)
            : ast_node(pos, id) {}


        // TODO: update later when we add import aliases

        std::string fmt_type(const taul::source_code& src) const;


        inline void give_to(ast_node& target) override final { target.give(res<ast_TypeSpec>(shared_from_this())); }
        void fmt(ast_formatter& x) override final;
        void accept(ast_visitor& x) override final;


    protected:
        void do_give(taul::token x) override final;
    };


    class ast_parser final {
    public:
        ast_parser() = default;


        struct result final {
            std::optional<taul::source_pos> syntax_error = std::nullopt;
            std::shared_ptr<ast_Chunk> root = nullptr;
        };

        result parse(const taul::source_code& src);


    private:
        class listener;


        std::optional<taul::source_pos> _syntax_error;
        std::shared_ptr<ast_Chunk> _result;
        ast_id_t _next_id = 0;
        std::vector<res<ast_node>> _stk;
    };

    class ast_parser::listener final : public taul::listener {
    public:
        ast_parser* client_ptr;


        inline listener(ast_parser& client) 
            : client_ptr(&client) {}


        inline ast_parser& client() noexcept { return deref_assert(client_ptr); }
        inline decltype(ast_parser::_stk)& stk() noexcept { return client()._stk; }


        void on_startup() override final;
        void on_shutdown() override final;
        void on_lexical(taul::token tkn) override final;
        void on_syntactic(taul::ppr_ref ppr, taul::source_pos pos) override final;
        void on_close() override final;
        inline void on_abort() override final {}
        void on_terminal_error(taul::token_range ids, taul::token input) override final;
        void on_nonterminal_error(taul::symbol_id id, taul::token input) override final;
    };
}

