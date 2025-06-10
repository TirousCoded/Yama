

#pragma once


#include <memory>
#include <vector>
#include <variant>

#include <taul/source_code.h>
#include <taul/symbols.h>
#include <taul/listener.h>

#include "../core/macros.h"
#include "../core/general.h"
#include "../core/scalars.h"
#include "../core/res.h"


namespace yama::internal {


    class ctype;
    class compiler;


    // quick-n'-dirty Yama syntax AST

    // later on we may clean-up/replace this w/ something more *official*


    using ast_id_t = size_t;

    enum class ast_type : uint8_t {
        Chunk,
        Decl,
        ImportDecl,
        RelativePath,
        VarDecl,
        FnDecl,
        StructDecl,
        CallSig,
        ParamDecl,
        Result,
        Block,
        Stmt,
        ExprStmt,
        IfStmt,
        LoopStmt,
        BreakStmt,
        ContinueStmt,
        ReturnStmt,
        Expr,
        PrimaryExpr,
        Lit,
        IntLit,
        UIntLit,
        FloatLit,
        BoolLit,
        CharLit,
        Assign,
        Args,
        TypeAnnot,
        TypeSpec,

        num, // not a valid AST node type
    };

    constexpr size_t ast_types = (size_t)ast_type::num;

    std::string fmt_ast_type(ast_type x);
}

YAMA_SETUP_FORMAT(yama::internal::ast_type, yama::internal::fmt_ast_type(x));

namespace yama::internal {


    template<typename T>
    concept ast_type_provider =
        requires
    {
        { T::ast_type_value } noexcept -> std::convertible_to<ast_type>;
    };

    template<ast_type_provider T>
    constexpr ast_type ast_type_of() noexcept {
        return T::ast_type_value;
    }
    template<ast_type_provider T>
    constexpr ast_type ast_type_of(const T&) noexcept {
        return ast_type_of<T>();
    }


    class ast_visitor;

    class ast_formatter;

    class ast_node;

    class ast_expr;
    class ast_base_expr;
    class ast_suffix_expr;

    static_assert(ast_types == 30); // reminder

    class ast_Chunk;
    class ast_Decl;
    class ast_ImportDecl;
    class ast_RelativePath;
    class ast_VarDecl;
    class ast_FnDecl;
    class ast_StructDecl;
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

        static_assert(ast_types == 30); // reminder

        inline virtual void visit_begin(res<ast_Chunk> x) {}
        inline virtual void visit_begin(res<ast_Decl> x) {}
        inline virtual void visit_begin(res<ast_ImportDecl> x) {}
        inline virtual void visit_begin(res<ast_RelativePath> x) {}
        inline virtual void visit_begin(res<ast_VarDecl> x) {}
        inline virtual void visit_begin(res<ast_FnDecl> x) {}
        inline virtual void visit_begin(res<ast_StructDecl> x) {}
        inline virtual void visit_begin(res<ast_CallSig> x) {}
        inline virtual void visit_begin(res<ast_ParamDecl> x) {}
        inline virtual void visit_begin(res<ast_Result> x) {}
        inline virtual void visit_begin(res<ast_Block> x) {}
        inline virtual void visit_begin(res<ast_Stmt> x) {}
        inline virtual void visit_begin(res<ast_ExprStmt> x) {}
        inline virtual void visit_begin(res<ast_IfStmt> x) {}
        inline virtual void visit_begin(res<ast_LoopStmt> x) {}
        inline virtual void visit_begin(res<ast_BreakStmt> x) {}
        inline virtual void visit_begin(res<ast_ContinueStmt> x) {}
        inline virtual void visit_begin(res<ast_ReturnStmt> x) {}
        inline virtual void visit_begin(res<ast_Expr> x) {}
        inline virtual void visit_begin(res<ast_PrimaryExpr> x) {}
        inline virtual void visit_begin(res<ast_Lit> x) {}
        inline virtual void visit_begin(res<ast_IntLit> x) {}
        inline virtual void visit_begin(res<ast_UIntLit> x) {}
        inline virtual void visit_begin(res<ast_FloatLit> x) {}
        inline virtual void visit_begin(res<ast_BoolLit> x) {}
        inline virtual void visit_begin(res<ast_CharLit> x) {}
        inline virtual void visit_begin(res<ast_Assign> x) {}
        inline virtual void visit_begin(res<ast_Args> x) {}
        inline virtual void visit_begin(res<ast_TypeAnnot> x) {}
        inline virtual void visit_begin(res<ast_TypeSpec> x) {}

        // visit_end is fired after propagating to children

        static_assert(ast_types == 30); // reminder

        inline virtual void visit_end(res<ast_Chunk> x) {}
        inline virtual void visit_end(res<ast_Decl> x) {}
        inline virtual void visit_end(res<ast_ImportDecl> x) {}
        inline virtual void visit_end(res<ast_RelativePath> x) {}
        inline virtual void visit_end(res<ast_VarDecl> x) {}
        inline virtual void visit_end(res<ast_FnDecl> x) {}
        inline virtual void visit_end(res<ast_StructDecl> x) {}
        inline virtual void visit_end(res<ast_CallSig> x) {}
        inline virtual void visit_end(res<ast_ParamDecl> x) {}
        inline virtual void visit_end(res<ast_Result> x) {}
        inline virtual void visit_end(res<ast_Block> x) {}
        inline virtual void visit_end(res<ast_Stmt> x) {}
        inline virtual void visit_end(res<ast_ExprStmt> x) {}
        inline virtual void visit_end(res<ast_IfStmt> x) {}
        inline virtual void visit_end(res<ast_LoopStmt> x) {}
        inline virtual void visit_end(res<ast_BreakStmt> x) {}
        inline virtual void visit_end(res<ast_ContinueStmt> x) {}
        inline virtual void visit_end(res<ast_ReturnStmt> x) {}
        inline virtual void visit_end(res<ast_Expr> x) {}
        inline virtual void visit_end(res<ast_PrimaryExpr> x) {}
        inline virtual void visit_end(res<ast_Lit> x) {}
        inline virtual void visit_end(res<ast_IntLit> x) {}
        inline virtual void visit_end(res<ast_UIntLit> x) {}
        inline virtual void visit_end(res<ast_FloatLit> x) {}
        inline virtual void visit_end(res<ast_BoolLit> x) {}
        inline virtual void visit_end(res<ast_CharLit> x) {}
        inline virtual void visit_end(res<ast_Assign> x) {}
        inline virtual void visit_end(res<ast_Args> x) {}
        inline virtual void visit_end(res<ast_TypeAnnot> x) {}
        inline virtual void visit_end(res<ast_TypeSpec> x) {}
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
        const ast_type node_type;
        const ast_id_t id;
        std::weak_ptr<ast_node> parent_node;


        inline ast_node(taul::source_pos pos, ast_id_t id, ast_type node_type)
            : node_type(node_type),
            id(id),
            parent_node(std::weak_ptr<ast_node>{}),
            _low_pos(pos),
            _high_pos(pos) {}


        inline auto try_parent() const noexcept {
            return parent_node.lock();
        }
        inline auto parent() const noexcept {
            return res(try_parent());
        }


        // below 'is', 'as' and 'expect' methods let us use AST node metadata to discern types

        inline bool is(ast_type x) const noexcept {
            return node_type == x;
        }
        template<ast_type_provider T>
        inline bool is() const noexcept {
            return is(ast_type_of<T>());
        }
        template<ast_type_provider T>
        inline std::shared_ptr<T> as() noexcept {
            return
                is<T>()
                ? std::static_pointer_cast<T>(shared_from_this())
                : nullptr;
        }
        template<ast_type_provider T>
        inline std::shared_ptr<const T> as() const noexcept {
            return
                is<const T>()
                ? std::static_pointer_cast<const T>(shared_from_this())
                : nullptr;
        }
        template<ast_type_provider T>
        inline res<T> expect() {
            return res(as<T>());
        }
        template<ast_type_provider T>
        inline res<const T> expect() const {
            return res(as<T>());
        }


        inline taul::source_pos low_pos() const noexcept { return _low_pos; }
        inline taul::source_pos high_pos() const noexcept { return _high_pos; }


        // the way we build ASTs is that we build a node, and then we populate it
        // w/ info as the TAUL listener encounters it, invoking these 'dispatch_give'
        // methods

        inline void dispatch_give(taul::token x) {
            _high_pos = std::max(high_pos(), x.high_pos());
            do_give(x);
        }
        template<typename T>
        inline void dispatch_give(res<T> x) {
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

        inline std::string fmt_tree(str src, const char* tab = default_tab) {
            YAMA_ASSERT(tab);
            ast_formatter a(src, tab);
            fmt(a);
            return a.output;
        }


        virtual void accept(ast_visitor& x) = 0;


    protected:
        static_assert(ast_types == 30); // reminder

        inline virtual void do_give(taul::token x) {}
        inline virtual void do_give(res<ast_Chunk> x) {}
        inline virtual void do_give(res<ast_Decl> x) {}
        inline virtual void do_give(res<ast_ImportDecl> x) {}
        inline virtual void do_give(res<ast_RelativePath> x) {}
        inline virtual void do_give(res<ast_VarDecl> x) {}
        inline virtual void do_give(res<ast_FnDecl> x) {}
        inline virtual void do_give(res<ast_StructDecl> x) {}
        inline virtual void do_give(res<ast_CallSig> x) {}
        inline virtual void do_give(res<ast_ParamDecl> x) {}
        inline virtual void do_give(res<ast_Result> x) {}
        inline virtual void do_give(res<ast_Block> x) {}
        inline virtual void do_give(res<ast_Stmt> x) {}
        inline virtual void do_give(res<ast_ExprStmt> x) {}
        inline virtual void do_give(res<ast_IfStmt> x) {}
        inline virtual void do_give(res<ast_LoopStmt> x) {}
        inline virtual void do_give(res<ast_BreakStmt> x) {}
        inline virtual void do_give(res<ast_ContinueStmt> x) {}
        inline virtual void do_give(res<ast_ReturnStmt> x) {}
        inline virtual void do_give(res<ast_Expr> x) {}
        inline virtual void do_give(res<ast_PrimaryExpr> x) {}
        inline virtual void do_give(res<ast_Lit> x) {}
        inline virtual void do_give(res<ast_IntLit> x) {}
        inline virtual void do_give(res<ast_UIntLit> x) {}
        inline virtual void do_give(res<ast_FloatLit> x) {}
        inline virtual void do_give(res<ast_BoolLit> x) {}
        inline virtual void do_give(res<ast_CharLit> x) {}
        inline virtual void do_give(res<ast_Assign> x) {}
        inline virtual void do_give(res<ast_Args> x) {}
        inline virtual void do_give(res<ast_TypeAnnot> x) {}
        inline virtual void do_give(res<ast_TypeSpec> x) {}


    private:
        taul::source_pos _low_pos, _high_pos;
    };


    class ast_expr : public ast_node {
    public:
        inline ast_expr(taul::source_pos pos, ast_id_t id, ast_type node_type)
            : ast_node(pos, id, node_type) {}


        // returns root of the expr tree (not the AST)

        res<ast_Expr> root_expr() const;

        // returns compile-time type this expr corresponds to, if any

        std::optional<ctype> get_type(compiler& cs) const;
        
        // IMPORTANT:
        //      herein, a 'primary subexpr' refers to something vary specific:
        //          1) for base exprs, it refers to nothing
        //          2) for suffix exprs, it refers to suffix expr immediately prior to this
        //             expr in the suffix chain, if any, or the base expr if no prior suffixes,
        //             or nothing if no base expr
        //          3) for Expr nodes, it refers to the last suffix of suffix chain, or the
        //             base expr if suffix chain is empty
        // 
        //      in the AST, left-associative exprs are structured in a way that makes it somewhat
        //      unintuitive to discern what expr is *nested within* a suffix expr, and so this
        //      and other complexities arising from this motivated the creation of a notion of
        //      a 'primary subexpr' to resolve these nuances automatically
        //
        //      to this end, get_primary_subexpr provides a standardized way of querying this
        
        // returns primary subexpr of this expr, if any

        virtual std::shared_ptr<ast_expr> get_primary_subexpr() const noexcept = 0;


    private:
        res<ast_Expr> _root_expr_helper(const ast_node& current) const;
    };

    class ast_base_expr : public ast_expr {
    public:
        inline ast_base_expr(taul::source_pos pos, ast_id_t id, ast_type node_type)
            : ast_expr(pos, id, node_type) {}


        std::shared_ptr<ast_expr> get_primary_subexpr() const noexcept override final;
    };

    class ast_suffix_expr : public ast_expr {
    public:
        size_t index = 0; // the index of this node in suffix array of parent


        inline ast_suffix_expr(taul::source_pos pos, ast_id_t id, ast_type node_type)
            : ast_expr(pos, id, node_type) {}


        std::shared_ptr<ast_expr> get_primary_subexpr() const noexcept override final;
    };


    class ast_Chunk final : public ast_node {
    public:
        static constexpr auto ast_type_value = ast_type::Chunk;


        std::vector<res<ast_Decl>> decls;


        inline ast_Chunk(taul::source_pos pos, ast_id_t id)
            : ast_node(pos, id, ast_type_value) {}


        inline void give_to(ast_node& target) override final { target.dispatch_give(res<ast_Chunk>(shared_from_this())); }
        void fmt(ast_formatter& x) override final;
        void accept(ast_visitor& x) override final;


    protected:
        void do_give(res<ast_Decl> x) override final;
    };

    class ast_Decl final : public ast_node {
    public:
        static constexpr auto ast_type_value = ast_type::Decl;


        std::variant<
            std::shared_ptr<ast_ImportDecl>,
            std::shared_ptr<ast_VarDecl>,
            std::shared_ptr<ast_FnDecl>,
            std::shared_ptr<ast_StructDecl>
            > decl;


        inline ast_Decl(taul::source_pos pos, ast_id_t id)
            : ast_node(pos, id, ast_type_value) {}


        std::shared_ptr<ast_node> get_decl();


        inline void give_to(ast_node& target) override final { target.dispatch_give(res<ast_Decl>(shared_from_this())); }
        void fmt(ast_formatter& x) override final;
        void accept(ast_visitor& x) override final;


    protected:
        void do_give(res<ast_ImportDecl> x) override final;
        void do_give(res<ast_VarDecl> x) override final;
        void do_give(res<ast_FnDecl> x) override final;
        void do_give(res<ast_StructDecl> x) override final;
    };
    
    class ast_ImportDecl final : public ast_node {
    public:
        static constexpr auto ast_type_value = ast_type::ImportDecl;


        std::optional<taul::token> name;
        std::optional<taul::token> head;
        std::shared_ptr<ast_RelativePath> relative_path;


        inline ast_ImportDecl(taul::source_pos pos, ast_id_t id)
            : ast_node(pos, id, ast_type_value) {}


        std::optional<std::string> path(const str& src) const;


        inline void give_to(ast_node& target) override final { target.dispatch_give(res<ast_ImportDecl>(shared_from_this())); }
        void fmt(ast_formatter& x) override final;
        void accept(ast_visitor& x) override final;


    protected:
        void do_give(taul::token x) override final;
        void do_give(res<ast_RelativePath> x) override final;
    };
    
    class ast_RelativePath final : public ast_node {
    public:
        static constexpr auto ast_type_value = ast_type::RelativePath;


        std::vector<taul::token> ids_and_dots;


        inline ast_RelativePath(taul::source_pos pos, ast_id_t id)
            : ast_node(pos, id, ast_type_value) {}


        std::optional<std::string> relative_path(const str& src) const;


        inline void give_to(ast_node& target) override final { target.dispatch_give(res<ast_RelativePath>(shared_from_this())); }
        void fmt(ast_formatter& x) override final;
        void accept(ast_visitor& x) override final;


    protected:
        void do_give(taul::token x) override final;
    };
    
    class ast_VarDecl final : public ast_node {
    public:
        static constexpr auto ast_type_value = ast_type::VarDecl;


        taul::token name;
        std::shared_ptr<ast_TypeAnnot> type;
        std::shared_ptr<ast_Assign> assign;


        inline ast_VarDecl(taul::source_pos pos, ast_id_t id)
            : ast_node(pos, id, ast_type_value) {}


        inline void give_to(ast_node& target) override final { target.dispatch_give(res<ast_VarDecl>(shared_from_this())); }
        void fmt(ast_formatter& x) override final;
        void accept(ast_visitor& x) override final;


    protected:
        void do_give(taul::token x) override final;
        void do_give(res<ast_TypeAnnot> x) override final;
        void do_give(res<ast_Assign> x) override final;
    };
    
    class ast_FnDecl final : public ast_node {
    public:
        static constexpr auto ast_type_value = ast_type::FnDecl;


        taul::token name;
        std::shared_ptr<ast_CallSig> callsig;
        std::shared_ptr<ast_Block> block;


        inline ast_FnDecl(taul::source_pos pos, ast_id_t id)
            : ast_node(pos, id, ast_type_value) {}


        inline void give_to(ast_node& target) override final { target.dispatch_give(res<ast_FnDecl>(shared_from_this())); }
        void fmt(ast_formatter& x) override final;
        void accept(ast_visitor& x) override final;


    protected:
        void do_give(taul::token x) override final;
        void do_give(res<ast_CallSig> x) override final;
        void do_give(res<ast_Block> x) override final;
    };
    
    class ast_StructDecl final : public ast_node {
    public:
        static constexpr auto ast_type_value = ast_type::StructDecl;


        taul::token name;


        inline ast_StructDecl(taul::source_pos pos, ast_id_t id)
            : ast_node(pos, id, ast_type_value) {}


        inline void give_to(ast_node& target) override final { target.dispatch_give(res<ast_StructDecl>(shared_from_this())); }
        void fmt(ast_formatter& x) override final;
        void accept(ast_visitor& x) override final;


    protected:
        void do_give(taul::token x) override final;
    };

    class ast_CallSig final : public ast_node {
    public:
        static constexpr auto ast_type_value = ast_type::CallSig;


        std::vector<res<ast_ParamDecl>> params;
        std::shared_ptr<ast_Result> result;


        inline ast_CallSig(taul::source_pos pos, ast_id_t id)
            : ast_node(pos, id, ast_type_value) {}


        inline void give_to(ast_node& target) override final { target.dispatch_give(res<ast_CallSig>(shared_from_this())); }
        void fmt(ast_formatter& x) override final;
        void accept(ast_visitor& x) override final;


    protected:
        void do_give(res<ast_ParamDecl> x) override final;
        void do_give(res<ast_Result> x) override final;
    };
    
    class ast_ParamDecl final : public ast_node {
    public:
        static constexpr auto ast_type_value = ast_type::ParamDecl;


        std::weak_ptr<ast_CallSig> callsig; // back-ref
        size_t index = 0; // index of param in callsig
        taul::token name;
        std::shared_ptr<ast_TypeAnnot> type;


        inline ast_ParamDecl(taul::source_pos pos, ast_id_t id)
            : ast_node(pos, id, ast_type_value) {}


        res<ast_CallSig> get_callsig() const noexcept;


        inline void give_to(ast_node& target) override final { target.dispatch_give(res<ast_ParamDecl>(shared_from_this())); }
        void fmt(ast_formatter& x) override final;
        void accept(ast_visitor& x) override final;


    protected:
        void do_give(taul::token x) override final;
        void do_give(res<ast_TypeAnnot> x) override final;
    };
    
    class ast_Result final : public ast_node {
    public:
        static constexpr auto ast_type_value = ast_type::Result;


        std::weak_ptr<ast_CallSig> callsig; // back-ref
        std::shared_ptr<ast_TypeSpec> type;


        inline ast_Result(taul::source_pos pos, ast_id_t id)
            : ast_node(pos, id, ast_type_value) {}


        res<ast_CallSig> get_callsig() const noexcept;


        inline void give_to(ast_node& target) override final { target.dispatch_give(res<ast_Result>(shared_from_this())); }
        void fmt(ast_formatter& x) override final;
        void accept(ast_visitor& x) override final;


    protected:
        void do_give(res<ast_TypeSpec> x) override final;
    };
    
    class ast_Block final : public ast_node {
    public:
        static constexpr auto ast_type_value = ast_type::Block;


        std::vector<res<ast_Stmt>> stmts;
        bool is_fn_body_block = false;
        bool will_never_exit_via_fallthrough = false;


        inline ast_Block(taul::source_pos pos, ast_id_t id)
            : ast_node(pos, id, ast_type_value) {}


        inline void give_to(ast_node& target) override final { target.dispatch_give(res<ast_Block>(shared_from_this())); }
        void fmt(ast_formatter& x) override final;
        void accept(ast_visitor& x) override final;


    protected:
        void do_give(res<ast_Stmt> x) override final;
    };
    
    class ast_Stmt final : public ast_node {
    public:
        static constexpr auto ast_type_value = ast_type::Stmt;


        std::variant<
            std::shared_ptr<ast_Decl>,
            std::shared_ptr<ast_ExprStmt>,
            std::shared_ptr<ast_IfStmt>,
            std::shared_ptr<ast_LoopStmt>,
            std::shared_ptr<ast_BreakStmt>,
            std::shared_ptr<ast_ContinueStmt>,
            std::shared_ptr<ast_ReturnStmt>
            > stmt_or_decl;


        inline ast_Stmt(taul::source_pos pos, ast_id_t id)
            : ast_node(pos, id, ast_type_value) {}


        std::shared_ptr<ast_node> get_stmt_or_decl();


        inline void give_to(ast_node& target) override final { target.dispatch_give(res<ast_Stmt>(shared_from_this())); }
        void fmt(ast_formatter& x) override final;
        void accept(ast_visitor& x) override final;


    protected:
        void do_give(res<ast_Decl> x) override final;
        void do_give(res<ast_ExprStmt> x) override final;
        void do_give(res<ast_IfStmt> x) override final;
        void do_give(res<ast_LoopStmt> x) override final;
        void do_give(res<ast_BreakStmt> x) override final;
        void do_give(res<ast_ContinueStmt> x) override final;
        void do_give(res<ast_ReturnStmt> x) override final;
    };
    
    class ast_ExprStmt final : public ast_node {
    public:
        static constexpr auto ast_type_value = ast_type::ExprStmt;


        std::shared_ptr<ast_Expr> expr;
        std::shared_ptr<ast_Assign> assign;


        inline ast_ExprStmt(taul::source_pos pos, ast_id_t id)
            : ast_node(pos, id, ast_type_value) {}


        inline void give_to(ast_node& target) override final { target.dispatch_give(res<ast_ExprStmt>(shared_from_this())); }
        void fmt(ast_formatter& x) override final;
        void accept(ast_visitor& x) override final;


    protected:
        void do_give(res<ast_Expr> x) override final;
        void do_give(res<ast_Assign> x) override final;
    };
    
    class ast_IfStmt final : public ast_node {
    public:
        static constexpr auto ast_type_value = ast_type::IfStmt;


        std::shared_ptr<ast_Expr> cond;
        std::shared_ptr<ast_Block> block;
        std::variant<
            std::shared_ptr<ast_Block>,
            std::shared_ptr<ast_IfStmt>
            > else_block_or_stmt;


        inline ast_IfStmt(taul::source_pos pos, ast_id_t id)
            : ast_node(pos, id, ast_type_value) {}


        std::shared_ptr<ast_node> get_else_block_or_stmt() const noexcept;


        inline void give_to(ast_node& target) override final { target.dispatch_give(res<ast_IfStmt>(shared_from_this())); }
        void fmt(ast_formatter& x) override final;
        void accept(ast_visitor& x) override final;


    protected:
        void do_give(res<ast_Expr> x) override final;
        void do_give(res<ast_Block> x) override final;
        void do_give(res<ast_IfStmt> x) override final;
    };
    
    class ast_LoopStmt final : public ast_node {
    public:
        static constexpr auto ast_type_value = ast_type::LoopStmt;


        std::shared_ptr<ast_Block> block;


        inline ast_LoopStmt(taul::source_pos pos, ast_id_t id)
            : ast_node(pos, id, ast_type_value) {}


        inline void give_to(ast_node& target) override final { target.dispatch_give(res<ast_LoopStmt>(shared_from_this())); }
        void fmt(ast_formatter& x) override final;
        void accept(ast_visitor& x) override final;


    protected:
        void do_give(res<ast_Block> x) override final;
    };
    
    class ast_BreakStmt final : public ast_node {
    public:
        static constexpr auto ast_type_value = ast_type::BreakStmt;


        inline ast_BreakStmt(taul::source_pos pos, ast_id_t id)
            : ast_node(pos, id, ast_type_value) {}


        inline void give_to(ast_node& target) override final { target.dispatch_give(res<ast_BreakStmt>(shared_from_this())); }
        void fmt(ast_formatter& x) override final;
        void accept(ast_visitor& x) override final;
    };
    
    class ast_ContinueStmt final : public ast_node {
    public:
        static constexpr auto ast_type_value = ast_type::ContinueStmt;


        inline ast_ContinueStmt(taul::source_pos pos, ast_id_t id)
            : ast_node(pos, id, ast_type_value) {}


        inline void give_to(ast_node& target) override final { target.dispatch_give(res<ast_ContinueStmt>(shared_from_this())); }
        void fmt(ast_formatter& x) override final;
        void accept(ast_visitor& x) override final;
    };
    
    class ast_ReturnStmt final : public ast_node {
    public:
        static constexpr auto ast_type_value = ast_type::ReturnStmt;


        std::shared_ptr<ast_Expr> expr;


        inline ast_ReturnStmt(taul::source_pos pos, ast_id_t id)
            : ast_node(pos, id, ast_type_value) {}


        inline void give_to(ast_node& target) override final { target.dispatch_give(res<ast_ReturnStmt>(shared_from_this())); }
        void fmt(ast_formatter& x) override final;
        void accept(ast_visitor& x) override final;


    protected:
        void do_give(res<ast_Expr> x) override final;
    };
    
    class ast_Expr final : public ast_expr {
    public:
        static constexpr auto ast_type_value = ast_type::Expr;


        std::shared_ptr<ast_base_expr> base;
        std::vector<res<ast_suffix_expr>> suffixes;
        bool has_const_kw = false;
        bool is_type_spec_crvalue = false;
        bool is_assign_stmt_lvalue = false;


        inline ast_Expr(taul::source_pos pos, ast_id_t id)
            : ast_expr(pos, id, ast_type_value) {}


        inline void give_to(ast_node& target) override final { target.dispatch_give(res<ast_Expr>(shared_from_this())); }
        void fmt(ast_formatter& x) override final;
        void accept(ast_visitor& x) override final;

        std::shared_ptr<ast_expr> get_primary_subexpr() const noexcept override final;


    protected:
        void do_give(taul::token x) override final;
        void do_give(res<ast_PrimaryExpr> x) override final;
        void do_give(res<ast_Args> x) override final;
    };

    class ast_PrimaryExpr final : public ast_base_expr {
    public:
        static constexpr auto ast_type_value = ast_type::PrimaryExpr;


        std::optional<taul::token> qualifier;
        std::optional<taul::token> name;
        std::shared_ptr<ast_Lit> lit;


        inline ast_PrimaryExpr(taul::source_pos pos, ast_id_t id)
            : ast_base_expr(pos, id, ast_type_value) {}


        std::optional<std::string> fmt_name(const taul::source_code& src) const; // fmts name w/ or w/out qualifier


        inline void give_to(ast_node& target) override final { target.dispatch_give(res<ast_PrimaryExpr>(shared_from_this())); }
        void fmt(ast_formatter& x) override final;
        void accept(ast_visitor& x) override final;


    protected:
        void do_give(taul::token x) override final;
        void do_give(res<ast_Lit> x) override final;
    };
    
    class ast_Lit final : public ast_node {
    public:
        static constexpr auto ast_type_value = ast_type::Lit;


        std::variant<
            std::shared_ptr<ast_IntLit>,
            std::shared_ptr<ast_UIntLit>,
            std::shared_ptr<ast_FloatLit>,
            std::shared_ptr<ast_BoolLit>,
            std::shared_ptr<ast_CharLit>
            > lit;


        inline ast_Lit(taul::source_pos pos, ast_id_t id)
            : ast_node(pos, id, ast_type_value) {}


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


        inline void give_to(ast_node& target) override final { target.dispatch_give(res<ast_Lit>(shared_from_this())); }
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
        static constexpr auto ast_type_value = ast_type::IntLit;


        taul::token lit;


        inline ast_IntLit(taul::source_pos pos, ast_id_t id)
            : ast_node(pos, id, ast_type_value) {}


        inline void give_to(ast_node& target) override final { target.dispatch_give(res<ast_IntLit>(shared_from_this())); }
        void fmt(ast_formatter& x) override final;
        void accept(ast_visitor& x) override final;


    protected:
        void do_give(taul::token x) override final;
    };
    
    class ast_UIntLit final : public ast_node {
    public:
        static constexpr auto ast_type_value = ast_type::UIntLit;


        taul::token lit;


        inline ast_UIntLit(taul::source_pos pos, ast_id_t id)
            : ast_node(pos, id, ast_type_value) {}


        inline void give_to(ast_node& target) override final { target.dispatch_give(res<ast_UIntLit>(shared_from_this())); }
        void fmt(ast_formatter& x) override final;
        void accept(ast_visitor& x) override final;


    protected:
        void do_give(taul::token x) override final;
    };
    
    class ast_FloatLit final : public ast_node {
    public:
        static constexpr auto ast_type_value = ast_type::FloatLit;


        taul::token lit;


        inline ast_FloatLit(taul::source_pos pos, ast_id_t id)
            : ast_node(pos, id, ast_type_value) {}


        inline void give_to(ast_node& target) override final { target.dispatch_give(res<ast_FloatLit>(shared_from_this())); }
        void fmt(ast_formatter& x) override final;
        void accept(ast_visitor& x) override final;


    protected:
        void do_give(taul::token x) override final;
    };
    
    class ast_BoolLit final : public ast_node {
    public:
        static constexpr auto ast_type_value = ast_type::BoolLit;


        taul::token lit;


        inline ast_BoolLit(taul::source_pos pos, ast_id_t id)
            : ast_node(pos, id, ast_type_value) {}


        inline void give_to(ast_node& target) override final { target.dispatch_give(res<ast_BoolLit>(shared_from_this())); }
        void fmt(ast_formatter& x) override final;
        void accept(ast_visitor& x) override final;


    protected:
        void do_give(taul::token x) override final;
    };
    
    class ast_CharLit final : public ast_node {
    public:
        static constexpr auto ast_type_value = ast_type::CharLit;


        taul::token lit;


        inline ast_CharLit(taul::source_pos pos, ast_id_t id)
            : ast_node(pos, id, ast_type_value) {}


        inline void give_to(ast_node& target) override final { target.dispatch_give(res<ast_CharLit>(shared_from_this())); }
        void fmt(ast_formatter& x) override final;
        void accept(ast_visitor& x) override final;


    protected:
        void do_give(taul::token x) override final;
    };
    
    class ast_Assign final : public ast_node {
    public:
        static constexpr auto ast_type_value = ast_type::Assign;


        std::shared_ptr<ast_Expr> expr;


        inline ast_Assign(taul::source_pos pos, ast_id_t id)
            : ast_node(pos, id, ast_type_value) {}


        inline void give_to(ast_node& target) override final { target.dispatch_give(res<ast_Assign>(shared_from_this())); }
        void fmt(ast_formatter& x) override final;
        void accept(ast_visitor& x) override final;


    protected:
        void do_give(res<ast_Expr> x) override final;
    };
    
    class ast_Args final : public ast_suffix_expr {
    public:
        static constexpr auto ast_type_value = ast_type::Args;


        std::vector<res<ast_Expr>> args;


        inline ast_Args(taul::source_pos pos, ast_id_t id)
            : ast_suffix_expr(pos, id, ast_type_value) {}


        bool is_constexpr_guarantee_expr_args() const noexcept;


        inline void give_to(ast_node& target) override final { target.dispatch_give(res<ast_Args>(shared_from_this())); }
        void fmt(ast_formatter& x) override final;
        void accept(ast_visitor& x) override final;


    protected:
        void do_give(res<ast_Expr> x) override final;
    };
    
    class ast_TypeAnnot final : public ast_node {
    public:
        static constexpr auto ast_type_value = ast_type::TypeAnnot;


        std::shared_ptr<ast_TypeSpec> type;


        inline ast_TypeAnnot(taul::source_pos pos, ast_id_t id)
            : ast_node(pos, id, ast_type_value) {}


        inline void give_to(ast_node& target) override final { target.dispatch_give(res<ast_TypeAnnot>(shared_from_this())); }
        void fmt(ast_formatter& x) override final;
        void accept(ast_visitor& x) override final;


    protected:
        void do_give(res<ast_TypeSpec> x) override final;
    };
    
    class ast_TypeSpec final : public ast_node {
    public:
        static constexpr auto ast_type_value = ast_type::TypeSpec;


        std::shared_ptr<ast_Expr> expr;


        inline ast_TypeSpec(taul::source_pos pos, ast_id_t id)
            : ast_node(pos, id, ast_type_value) {}


        std::optional<ctype> get_type(compiler& cs) const;


        inline void give_to(ast_node& target) override final { target.dispatch_give(res<ast_TypeSpec>(shared_from_this())); }
        void fmt(ast_formatter& x) override final;
        void accept(ast_visitor& x) override final;


    protected:
        void do_give(res<ast_Expr> x) override final;
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

