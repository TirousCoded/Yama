

#pragma once


#include <variant>

#include <taul/source_code.h>

#include "ast.h"
#include "csymtab.h"
#include "ctypesys.h"
#include "error_reporter.h"


namespace yama::internal {


    // ctype_resolver is *populated* via 'add' calls in first_pass, and then 'resolve' is
    // to be used at the vary start of second_pass to resolve ctypes, and only thereafter
    // may get/operator[] be used to query them

    class ctype_resolver final {
    public:
        ctype_resolver(
            csymtab_group& csymtabs,
            error_reporter& er,
            const taul::source_code& src);


        void add(const ast_TypeSpec& x);
        void add(const ast_PrimaryExpr& x); // handles if x is not a type spec (ie. need-not check in first_pass)

        void resolve(ctypesys_local& ctypesys);

        std::optional<ctype> get(const ast_TypeSpec* x) const noexcept; // fails quietly if x == nullptr
        std::optional<ctype> get(const ast_PrimaryExpr* x) const noexcept; // fails quietly if x == nullptr
        inline auto operator[](const ast_TypeSpec* x) const noexcept { return get(x); }
        inline auto operator[](const ast_PrimaryExpr* x) const noexcept { return get(x); }


    private:
        csymtab_group* _csymtabs;
        error_reporter* _er;
        const taul::source_code* _src;

        std::unordered_map<const ast_TypeSpec*, std::optional<ctype>> _type_spec_mappings;
        std::unordered_map<const ast_PrimaryExpr*, std::optional<ctype>> _primary_expr_mappings;


        void _resolve(ctypesys_local& ctypesys, const ast_TypeSpec& x, std::optional<ctype>& target);
        void _resolve(ctypesys_local& ctypesys, const ast_PrimaryExpr& x, std::optional<ctype>& target);
        bool _is_type_spec_id_expr(const ast_PrimaryExpr& x);
        void _report(bool ambiguous, const ast_node& where, const std::string& name);
    };
}

