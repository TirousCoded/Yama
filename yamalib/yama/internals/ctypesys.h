

#pragma once


#include <unordered_map>
#include <variant>

#include "../core/res.h"
#include "../core/module_info.h"

#include "safeptr.h"
#include "env.h"
#include "specifiers.h"
#include "ast.h"


namespace yama::internal {


    // IMPORTANT: importing modules into a ctypesys should only happen in first_pass
    
    // IMPORTANT: loading types into a ctypesys should only happen in second_pass
    //              * it should also be fine in expr_analyzer too


    class compiler;
    class translation_unit;

    class csymtab;
    class csym;

    class compiler_services;

    class ctype;
    class cmodule;
    class ctypesys;
    class ctypesys_local;


    class ctype final {
    public:
        ctype(ctypesys& s, res<csym> x, const import_path& where);
        ctype(ctypesys& s, const type_info& x, const import_path& where);

        ctype(const ctype&) = default;
        ctype& operator=(const ctype&) = default;


        inline env e() const { return _e(); }

        unqualified_name uqn() const;
        fullname fln() const;

        kind kind() const noexcept;

        std::optional<ctype> owner_type() const;
        size_t param_count() const noexcept;
        std::optional<ctype> param_type(size_t param_index, compiler& cs) const;
        std::optional<ctype> return_type(compiler& cs) const;

        inline bool operator==(const ctype& other) const noexcept { return fln() == other.fln(); }
        inline bool operator!=(const ctype& other) const noexcept { return !(*this == other); }

        // for safety fmt will require caller to specify e, as I worry using e() will cause
        // issues w/ us using the wrong env

        std::string fmt(const env& e) const;


    private:
        using _info_t = std::variant<res<csym>, safeptr<const type_info>>;


        safeptr<ctypesys> _s;
        _info_t _info;
        import_path _where;


        env _e() const;

        bool _csymtab_entry_not_typeinf() const noexcept;
        const res<csym>& _csymtab_entry() const;
        const type_info& _typeinf() const;
    };


    class cmodule final {
    public:
        cmodule(ctypesys& s, translation_unit& tu, const internal::import_path& where);
        cmodule(ctypesys& s, const res<module_info>& x, const internal::import_path& where);


        inline env e() const { return _e(); }
        constexpr const import_path& ip() const noexcept { return _where; }

        std::optional<ctype> type(const str& unqualified_name);


    private:
        // TODO: _dummy_t use involves a dirty C-style pointer cast
        struct _dummy_t {}; // <- helps avoid possible template instantiation issues w/ translation_unit in _info_t
        using _info_t = std::variant<safeptr<_dummy_t>, res<module_info>>;


        safeptr<ctypesys> _s;
        _info_t _info;
        internal::import_path _where;


        env _e() const;

        bool _tu_not_modinf() const noexcept;
        const translation_unit& _tu() const;
        const res<module_info>& _modinf() const;
    };


    class ctypesys final {
    public:
        safeptr<compiler> cs;


        ctypesys(compiler& cs);


        std::shared_ptr<cmodule> fetch_module(const import_path& x) const; // fetch w/out importing
        std::shared_ptr<cmodule> import(const import_path& x);
        std::optional<ctype> load(const fullname& x);

        std::shared_ptr<cmodule> register_module(const import_path& where, res<module_info> x);
        std::shared_ptr<cmodule> register_module(translation_unit& x);

        void cleanup();


    private:
        std::unordered_map<import_path, res<cmodule>> _modules;
    };


    // ctypesys_local is a layer over ctypesys which handles unqualified name lookup + shadowing
    
    // ctypesys_local exists as part of the translation unit of some compiling module

    class ctypesys_local final {
    public:
        safeptr<translation_unit> tu;


        ctypesys_local(translation_unit& tu);


        std::shared_ptr<cmodule> fetch_module(const import_path& x) const;
        std::shared_ptr<cmodule> import(const import_path& x);
        std::optional<ctype> load(const fullname& x);

        // IMPORTANT: situations like a type spec identifier being shadowed by a parameter (ie. non-type)
        //            identifier are NOT covered by below 'load' methods

        // load via unqualified name lookup

        // ambiguous will be set to true if load fails due to ambiguity, and false in
        // every other case

        std::optional<ctype> load(const str& unqualified_name, bool& ambiguous);
        std::optional<ctype> load(const str& unqualified_name);

        // load via qualified name lookup

        // bad_qualifier will be set to true if load fails due to qualifier being undeclared
        // name, and false in every other case

        std::optional<ctype> load(const str& qualifier, const str& unqualified_name, bool& bad_qualifier);
        std::optional<ctype> load(const str& qualifier, const str& unqualified_name);

        // load via type spec

        std::optional<ctype> load(const ast_TypeSpec& x, bool& ambiguous, bool& bad_qualifier);
        std::optional<ctype> load(const ast_TypeSpec& x);

        // load via identifier expr primary expr

        std::optional<ctype> load(const ast_PrimaryExpr& x, bool& ambiguous, bool& bad_qualifier);
        std::optional<ctype> load(const ast_PrimaryExpr& x);

        std::shared_ptr<cmodule> register_module(translation_unit& x);

        // bind_import adds where to import set for shadowing

        void bind_import(const import_path& where);


        ctype default_none(const std::optional<ctype>& x);

        // these lazy eval

        ctype none_type();
        ctype int_type();
        ctype uint_type();
        ctype float_type();
        ctype bool_type();
        ctype char_type();
        ctype type_type();


    private:
        std::unordered_set<import_path> _import_set;


        struct _builtin_cache final {
            ctype none, int0, uint, float0, bool0, char0, type;
        };
        std::optional<_builtin_cache> _builtin_cache_v;

        const _builtin_cache& _get_builtin_cache();
    };
}

