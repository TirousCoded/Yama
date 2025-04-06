

#pragma once


#include <variant>

#include <taul/source_code.h>

#include "safeptr.h"
#include "ast.h"
#include "csymtab.h"
#include "ctypesys.h"
#include "error_reporter.h"


namespace yama::internal {


    class translation_unit;
    class compiler;


    // ctype_resolver is *populated* via 'add' calls in first passes, and then 'resolve' is
    // to be used immediately before second passes to resolve ctypes, and only thereafter
    // may get/operator[] be used to query them

    class ctype_resolver final {
    public:
        safeptr<compiler> cs;


        ctype_resolver(compiler& cs);


        void add(translation_unit& tu, const ast_TypeSpec& x);
        void add(translation_unit& tu, const ast_PrimaryExpr& x); // handles if x is not a type spec (ie. need-not check in first_pass)

        void resolve();

        std::optional<ctype> get(const ast_TypeSpec* x) const noexcept; // fails quietly if x == nullptr
        std::optional<ctype> get(const ast_PrimaryExpr* x) const noexcept; // fails quietly if x == nullptr
        inline auto operator[](const ast_TypeSpec* x) const noexcept { return get(x); }
        inline auto operator[](const ast_PrimaryExpr* x) const noexcept { return get(x); }


    private:
        struct _entry_t final {
            safeptr<translation_unit> tu;
            std::optional<ctype> t;
        };


        std::unordered_map<const ast_TypeSpec*, _entry_t> _type_spec_mappings;
        std::unordered_map<const ast_PrimaryExpr*, _entry_t> _primary_expr_mappings;


        void _resolve(translation_unit& tu, const ast_TypeSpec& x, std::optional<ctype>& target);
        void _resolve(translation_unit& tu, const ast_PrimaryExpr& x, std::optional<ctype>& target);
        bool _is_type_spec_id_expr(translation_unit& tu, const ast_PrimaryExpr& x);
        void _report(translation_unit& tu, bool ambiguous, bool bad_qualifier, const ast_node& where, std::optional<std::string_view> qualifier, const std::string& name);
    };
}

