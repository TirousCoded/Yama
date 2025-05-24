

#pragma once


#include "../core/newtop.h"

#include "safeptr.h"
#include "ast.h"
#include "ctypesys.h"
#include "cvalue.h"


namespace yama::internal {


    class translation_unit;
    class compiler;


    // TODO: if we ever add things like allowing for constant local vars to be used to define
    //       the values of constexprs like type specs, we're gonna need to add in a ref cycle
    //       detector for constexpr eval

    // expr_analyzer is used to analyze expr trees in Yama code, doing things like discerning
    // their types, and precomputing constexprs when possible, as well as being responsible
    // for outputting the bcode associated w/ an expr tree

    class expr_analyzer final {
    public:
        enum class mode : uint8_t {
            rvalue,
            lvalue,

            num,
        };

        static constexpr size_t modes = size_t(mode::num);

        enum class category : uint8_t {
            param_id,
            var_id,
            type_id,
            int_lit,
            uint_lit,
            float_lit,
            bool_lit,
            char_lit,
            call,
            constexpr_guarantee,

            num,
        };

        static constexpr size_t categories = size_t(category::num);

        static std::string fmt_category(category x);

        // TODO: if process crashes, look into if the problem is us failing to account for
        //       if a composite expr is provided w/ invalid operand exprs

        struct metadata final {
            safeptr<translation_unit>   tu;
            taul::source_pos            where;                              // where in src is expr considered to exist
            mode                        mode;
            category                    category;
            std::optional<ctype>        type                = std::nullopt;
            std::optional<cvalue>       crvalue             = std::nullopt; // precomputed value, if any


            // metadata w/out type indicates expr was in error

            inline bool good() const noexcept { return type.has_value(); }


            inline bool rvalue() const noexcept { return mode == mode::rvalue; }
            inline bool lvalue() const noexcept { return mode == mode::lvalue; }

            inline bool is_constexpr() const noexcept { return crvalue.has_value(); }
        };


        safeptr<compiler> cs;


        expr_analyzer(compiler& cs);


        // IMPORTANT: during first passes, call acknowledge on all PrimaryExpr/Args/Expr nodes

        void acknowledge(translation_unit& tu, const ast_PrimaryExpr& x);
        void acknowledge(translation_unit& tu, const ast_Args& x);
        void acknowledge(translation_unit& tu, const ast_Expr& x);

        // IMPORTANT: during first passes, call add_root on all 'root' Expr nodes

        // root Expr nodes are the root nodes of expr trees, and adding them registers the
        // entire tree w/ the expr_analyzer

        void add_root(const ast_Expr& x);

        // IMPORTANT: query/codegen methods may only be used in second passes

        // when querying:
        //      1) querying ast_PrimaryExpr means we want the *base* expr
        //      2) querying ast_Args (or other *suffixes*) means we want the expr
        //         of the suffix, which will have nested inside of it the exprs
        //         of all preceding suffixes, and the *base* expr
        //          * ast_Args is also queried for things like constexpr guarantee exprs
        //      3) querying ast_Expr itself means we want the *entire tree*, including
        //         the *base* expr, and the exprs of all *suffixes*

        // operator[] queries expr metadata, w/ lazy loading

        metadata& operator[](const ast_PrimaryExpr& x);
        metadata& operator[](const ast_Args& x);
        metadata& operator[](const ast_Expr& x);
        metadata& operator[](const ast_TypeSpec& x); // forwards to expr

        // crvalue specifically tries to query the precomputed value of the expr

        std::optional<cvalue> crvalue(const ast_PrimaryExpr& x);
        std::optional<cvalue> crvalue(const ast_Args& x);
        std::optional<cvalue> crvalue(const ast_Expr& x);
        std::optional<cvalue> crvalue(const ast_TypeSpec& x); // forwards to expr, fails if missing
        std::optional<ctype> crvalue_to_type(const ast_PrimaryExpr& x);
        std::optional<ctype> crvalue_to_type(const ast_Args& x);
        std::optional<ctype> crvalue_to_type(const ast_Expr& x);
        std::optional<ctype> crvalue_to_type(const ast_TypeSpec& x); // forwards to expr, fails if missing

        // IMPORTANT: codegen[_nr] must ONLY be used within second passes, and in particular
        //            assumes that local var id exprs will be able to successfully query
        //            info about what register to use

        // codegen and codegen_nr write bcode to the current target (of the translation
        // unit of root) for the expr root, w/ the final value of the expr tree being
        // output to register at ret (which can be newtop)

        // codegen_nr is used for situations where the ultimate result of the expr tree
        // is not needed, enabling codegen optimization

        void codegen(const ast_Expr& root, size_t ret);
        void codegen_nr(const ast_Expr& root);

        // IMPORTANT: analyze MUST be called after first passes, but before second passes

        void analyze();
        void cleanup();


    private:
        struct _root final {
            bool must_be_constexpr;
        };

        enum class _ast_node_type : uint8_t {
            primary_expr,
            args,
            expr,
        };

        struct _first_nested_info final {
            _ast_node_type t;
            res<ast_node> node;


            template<typename T>
            inline res<T> as() const noexcept {
                return res<T>(node);
            }
        };


        std::unordered_map<safeptr<const ast_Expr>, _root> _roots;
        std::unordered_map<safeptr<const ast_node>, safeptr<translation_unit>> _tu_map;
        std::unordered_map<safeptr<const ast_node>, metadata> _metadata;


        bool _is_root_and_must_be_constexpr(const ast_Expr& x) const noexcept;


        translation_unit& _tu(const ast_node& x) const noexcept;

        metadata& _pull(const ast_PrimaryExpr& x);
        metadata& _pull(const ast_Args& x);
        metadata& _pull(const ast_Expr& x);
        metadata& _pull(const _first_nested_info& x);

        metadata& _fetch(const ast_node& x);

        bool _is_resolved(const ast_node& x) const noexcept;

        // IMPORTANT:
        //      for most composite exprs, their subexprs will ALWAYS be told that they're rvalues,
        //      as only the top-level expr of the tree can be an lvalue
        // 
        //      for example: array access operators are lvalues, but the array being accessed and
        //      the index are specified by nested rvalues
        //
        //      for ast_Expr, however, we're gonna propagate lvalue mode down one level to the
        //      ast_PrimaryExpr or suffix therein, as ast_Expr acts more as an alias for these
        //      than it does a seperate expr

        void _gen_metadata(const ast_node& x, taul::source_pos where, category category, mode mode);

        bool _resolve(const ast_PrimaryExpr& x);
        bool _resolve(const ast_Args& x);
        bool _resolve(const ast_Expr& x);

        bool _resolve_children(const ast_Args& x);
        bool _resolve_children(const ast_Expr& x);

        category _discern_category(const ast_PrimaryExpr& x);
        category _discern_category(const ast_Args& x);
        category _discern_category(const ast_Expr& x);
        
        bool _resolve_expr(const ast_PrimaryExpr& x);
        bool _resolve_expr(const ast_Args& x);
        bool _resolve_expr(const ast_Expr& x);

        // below, ret == std::nullopt means no result

        void _codegen(const ast_Expr& root, std::optional<size_t> ret);
        void _codegen_step(const ast_PrimaryExpr& x, std::optional<size_t> ret);
        void _codegen_step(const ast_Args& x, std::optional<size_t> ret);
        void _codegen_step(const ast_Expr& x, std::optional<size_t> ret);
        void _codegen_step(const _first_nested_info& x, std::optional<size_t> ret);

        // IMPORTANT:
        //      in the AST, left-associative exprs are structured in a way that makes it somewhat
        //      unintuitive to discern what their *left-most* nested subexpr is
        // 
        //      the below methods are used to discern this *first nested subexpr*
        // 
        //      for ast_PrimaryExpr, no first nested subexpr (so it gets no method overload)
        //
        //      for ast_Args, first nested subexpr is:
        //          1) if there's preceeding suffix, then that suffix
        //          2) if no preceeding suffix, and there's primary expr, then that primary expr
        //          3) if no preceeding suffix, and no primary expr, then nothing
        //
        //      for ast_Expr, first nested subexpr is:
        //          1) if there's a suffix, then that suffix
        //          2) if no suffix, then primary expr
        //
        //      for ast_Expr, it's more that we're aliasing the ast_Expr as either the ast_PrimaryExpr
        //      or suffix therein, rather than treating ast_Expr as a *seperate* expr

        std::optional<_first_nested_info> _first_nested_subexpr(const ast_Args& x);
        std::optional<_first_nested_info> _first_nested_subexpr(const ast_Expr& x);

        mode _discern_mode(const ast_PrimaryExpr& x) const noexcept;
        mode _discern_mode(const ast_Args& x) const noexcept;
        mode _discern_mode(const ast_Expr& x) const noexcept;

        bool _is_type_ref_id_expr(const ast_PrimaryExpr& x);

        bool _raise_numeric_overflow_if(bool x, metadata& md, const taul::token& lit);
        bool _raise_numeric_underflow_if(bool x, metadata& md, const taul::token& lit);
        bool _raise_malformed_literal_for_char_lit_if(bool x, metadata& md, const str& unquoted);
        bool _raise_illegal_unicode_if(bool x, metadata& md, const str& unquoted);
        bool _raise_undeclared_name_if(bool x, metadata& md, std::string_view name);
        bool _raise_ambiguous_name_if(bool x, metadata& md, std::string_view name);
        bool _raise_undeclared_qualifier_if(bool x, metadata& md, std::optional<taul::token> qualifier);
        bool _raise_nonassignable_expr_if(bool x, metadata& md);
        bool _raise_nonconstexpr_expr_if(bool x, metadata& md);
        bool _raise_wrong_arg_count_if(bool x, metadata& md, size_t expected_args);
        bool _raise_wrong_arg_count_if(bool x, metadata& md, ctype call_to, size_t expected_args);
        bool _raise_invalid_operation_due_to_noncallable_type_if(bool x, metadata& md, ctype t);
        bool _raise_type_mismatch_for_arg_if(bool x, metadata& md, size_t arg_display_number, ctype actual, ctype expected);


        static str _remove_quotes(const str& x) noexcept;


        static constexpr std::nullopt_t _runtime_only = std::nullopt;
    };
}

