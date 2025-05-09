

#pragma once


#include "safeptr.h"
#include "ast.h"
#include "ctypesys.h"
#include "cvalue.h"


namespace yama::internal {


    class translation_unit;
    class compiler;


    // constexpr_solver is specifically for precomputing constexprs which we
    // 100% know do not have any behaviour at runtime (eg. the crvalues which
    // define explicit type annots do not have any runtime behaviour)

    // at the moment, we don't have constexpr precomputing for constexprs other
    // than these mandatory, purely compile-time only, ones

    // constexpr_solver is used like ctype_resolver: first we 'add' during
    // first_pass, and then we 'solve' before second_pass, and only therafter
    // may get/operator[] be used

    // constexpr_solver::solve must be called AFTER ctype_resolver::resolve

    class constexpr_solver final {
    public:
        safeptr<compiler> cs;


        constexpr_solver(compiler& cs);


        // IMPORTANT:
        //      when adding to the solver:
        //          1) adding ast_PrimaryExpr means we want the *base* expr
        //          2) adding ast_Args (or other *suffixes*) means we want the expr
        //             of the suffix, which will have nested inside of it the exprs
        //             of all preceding suffixes, and the *base* expr
        //              * ast_Args is also added for things like constexpr guarantee exprs
        //          3) adding ast_Expr itself means we want the *entire expr*, including
        //             the *base* expr, and the exprs of all *suffixes*

        // fails quietly if x == nullptr

        std::optional<cvalue> get(const ast_PrimaryExpr* x) const noexcept;
        std::optional<cvalue> get(const ast_Args* x) const noexcept;
        std::optional<cvalue> get(const ast_Expr* x) const noexcept;
        inline auto operator[](const ast_PrimaryExpr* x) const noexcept { return get(x); }
        inline auto operator[](const ast_Args* x) const noexcept { return get(x); }
        inline auto operator[](const ast_Expr* x) const noexcept { return get(x); }

        void add(translation_unit& tu, const ast_PrimaryExpr& x, bool mandatory);
        void add(translation_unit& tu, const ast_Args& x, bool mandatory);
        void add(translation_unit& tu, const ast_Expr& x, bool mandatory);

        void solve();
        void cleanup();


    private:
        enum class _mode : uint8_t {
            primary_expr,
            args,
            expr,
        };

        struct _entry_t final {
            safeptr<translation_unit> tu;
            _mode mode;
            std::optional<cvalue> v;
            bool mandatory;
        };


        std::unordered_map<const ast_node*, _entry_t> _mappings;


        std::optional<cvalue> _get(const ast_node* x) const noexcept;

        // TODO: sinces constexprs are pure fns, that means identical constexprs will have
        //       identical results, so maybe look into trying to exploit that in future
        //       revisions to constexpr solving

        std::optional<cvalue> _solve(const ast_node& x, _mode mode, translation_unit& tu);
        std::optional<cvalue> _solve(const ast_PrimaryExpr& x, translation_unit& tu);
        std::optional<cvalue> _solve(const ast_Args& x, translation_unit& tu);
        std::optional<cvalue> _solve(const ast_Expr& x, translation_unit& tu);

        std::optional<cvalue> _solve_call_expr_args(const ast_Args& x, translation_unit& tu);
        std::optional<cvalue> _solve_constexpr_guarantee_expr_args(const ast_Args& x, translation_unit& tu);
    };
}

