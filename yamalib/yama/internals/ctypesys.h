

#pragma once


#include <unordered_map>
#include <variant>

#include "../core/res.h"
#include "../core/module_info.h"

#include "env.h"
#include "specifiers.h"
#include "ast.h"


namespace yama::internal {


    // IMPORTANT: importing modules into a ctypesys should only happen in first_pass

    // IMPORTANT: loading types into a ctypesys should only happen in second_pass


    class csymtab;
    struct csymtab_entry;

    class compiler_services;

    class ctype_resolver;

    class ctype;
    class cmodule;
    class ctypesys;
    class ctypesys_local;


    class ctype final {
    public:
        ctype(ctypesys& s, res<csymtab_entry> x, const import_path& where, const env& e);
        ctype(ctypesys& s, const type_info& x, const import_path& where, const env& e);

        ctype(const ctype&) = default;
        ctype& operator=(const ctype&) = default;


        constexpr const env& e() const noexcept { return _e; }
        fullname fullname() const;

        kind kind() const noexcept;

        size_t param_count() const noexcept;
        std::optional<ctype> param_type(size_t param_index) const;
        std::optional<ctype> return_type() const;

        inline bool operator==(const ctype& other) const noexcept { return fullname() == other.fullname(); }
        inline bool operator!=(const ctype& other) const noexcept { return !(*this == other); }

        // for safety fmt will require caller to specify e, as I worry using e() will cause
        // issues w/ us using the wrong env

        std::string fmt(const env& e) const;


    private:
        using _info_t = std::variant<res<csymtab_entry>, const type_info*>;


        ctypesys* _s;
        _info_t _info;
        env _e;
        import_path _where;


        inline ctypesys& _sys() const noexcept { return deref_assert(_s); }

        bool _csymtab_entry_not_typeinf() const noexcept;
        const res<csymtab_entry>& _csymtab_entry() const;
        const type_info& _typeinf() const;
    };


    class cmodule final {
    public:
        cmodule(ctypesys& s, const res<csymtab>& root_csymtab, const internal::import_path& where, const env& e);
        cmodule(ctypesys& s, const res<module_info>& x, const internal::import_path& where, const env& e);


        constexpr const env& e() const noexcept { return _e; }
        constexpr const import_path& import_path() const noexcept { return _where; }

        std::optional<ctype> type(const str& unqualified_name);


    private:
        using _info_t = std::variant<res<csymtab>, res<module_info>>;


        ctypesys* _s;
        _info_t _info;
        env _e;
        internal::import_path _where;


        inline ctypesys& _sys() const noexcept { return deref_assert(_s); }

        bool _csymtab_not_modinf() const noexcept;
        const res<csymtab>& _csymtab() const;
        const res<module_info>& _modinf() const;
    };


    class ctypesys final {
    public:
        ctypesys(
            specifier_provider& sp,
            res<compiler_services> services,
            internal::ctype_resolver& ctype_resolver);


        inline specifier_provider& sp() const noexcept { return deref_assert(_sp); }
        constexpr const res<compiler_services>& services() const noexcept { return _services; }
        ctype_resolver& ctype_resolver() const noexcept;

        std::shared_ptr<cmodule> fetch_module(const import_path& x) const; // fetch w/out importing
        std::shared_ptr<cmodule> import(const import_path& x);
        std::optional<ctype> load(const fullname& x);

        bool register_module(const import_path& where, res<csymtab> x, const env& e);


    private:
        specifier_provider* _sp;
        res<compiler_services> _services;
        internal::ctype_resolver* _ctype_resolver;

        std::unordered_map<import_path, res<cmodule>> _modules;
    };


    // ctypesys_local is a layer over ctypesys which handles unqualified name lookup + shadowing
    
    // ctypesys_local exists as part of the translation unit of some compiling module

    class ctypesys_local final {
    public:
        // local_import_path is import path to the compiling module

        ctypesys_local(
            ctypesys& upstream,
            const taul::source_code& src,
            const import_path& local_import_path);


        inline ctypesys& upstream() const noexcept { return deref_assert(_upstream); }
        inline specifier_provider& sp() const noexcept { return upstream().sp(); }
        inline const res<compiler_services>& services() const noexcept { return upstream().services(); }

        inline std::shared_ptr<cmodule> fetch_module(const import_path& x) const { return upstream().fetch_module(x); }
        inline std::shared_ptr<cmodule> import(const import_path& x) { return upstream().import(x); }
        inline std::optional<ctype> load(const fullname& x) { return upstream().load(x); }

        // IMPORTANT: situations like a type spec identifier being shadowed by a parameter (ie. non-type)
        //            identifier are NOT covered by below 'load' methods

        // load via unqualified name lookup

        // ambiguous will be set to true if load fails due to ambiguity, and false in
        // every other case

        std::optional<ctype> load(const str& unqualified_name, bool& ambiguous);
        std::optional<ctype> load(const str& unqualified_name);

        std::optional<ctype> load(const ast_TypeSpec& x, bool& ambiguous);
        std::optional<ctype> load(const ast_TypeSpec& x);

        bool register_module(const import_path& where, res<csymtab> x, const env& e);

        // TODO: add_import DOES NOT perform import itself, just acknowledgement for shadowing,
        //       w/ me worrying this distinction may cause future confusion

        void add_import(const import_path& where); // adds to import set of compiling module


        ctype default_none(const std::optional<ctype>& x);

        // these lazy eval

        ctype none_type();
        ctype int_type();
        ctype uint_type();
        ctype float_type();
        ctype bool_type();
        ctype char_type();


    private:
        ctypesys* _upstream;
        const taul::source_code* _src;
        import_path _local_ip;
        std::unordered_set<import_path> _import_set;


        struct _builtin_cache final {
            ctype none, int0, uint, float0, bool0, char0;
        };
        std::optional<_builtin_cache> _builtin_cache_v;

        const _builtin_cache& _get_builtin_cache();
    };
}

