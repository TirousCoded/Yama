

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


        // fails quietly if x == nullptr

        std::optional<ctype> get(const ast_PrimaryExpr* x) const noexcept;
        inline auto operator[](const ast_PrimaryExpr* x) const noexcept { return get(x); }

        // handles if x is not a type ref (ie. need-not check in first_pass)

        void add(translation_unit& tu, const ast_PrimaryExpr& x);

        void resolve();
        void cleanup();


    private:
        struct _entry_t final {
            safeptr<translation_unit> tu;
            std::optional<ctype> t;
        };


        std::unordered_map<const ast_PrimaryExpr*, _entry_t> _mappings;


        void _resolve(translation_unit& tu, const ast_PrimaryExpr& x, std::optional<ctype>& target);
        bool _is_type_ref_id_expr(translation_unit& tu, const ast_PrimaryExpr& x);
    };
}

