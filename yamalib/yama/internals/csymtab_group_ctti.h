

#pragma once


#include "csymtab.h"

#include "../core/domain.h"


namespace yama::internal {


    // TODO: maybe revise code around csymtabs to use an interface
    
    // csymtab_group_ctti is a layer over a csymtab_group which makes extern types
    // available by lazy loading them into the root csymtab on demand

    class csymtab_group_ctti final {
    public:

        csymtab_group_ctti(
            res<compiler_services> services,
            const import_path& src_import_path,
            ast_Chunk& root,
            csymtab_group& csymtabs);


        bool add_import(const import_path& path); // fails quietly if already imported

        struct queried_import final {
            const module_info* ptr;
            env e;


            inline const module_info& info() const noexcept { return deref_assert(ptr); }
        };

        std::optional<queried_import> query_import(const import_path& path);

        // NOTE: if more than one imported module has unqualified_name, then w/out
        //       qualification no type can be accessed

        // NOTE: remember, we can assume import dirs only exist prior to first decl

        struct queried_type final {
            const type_info* ptr;
            env e;
            import_path from;


            inline const type_info& info() const noexcept { return deref_assert(ptr); }

            inline str qualified_name(const env& e) const { return str(std::format("{}:{}", from.fmt(e), info().unqualified_name)); }
        };

        // name may be qualified or unqualified

        // ambiguous is set to true if fails due to multiple imported modules having
        // a type under name (is set to false is ALL other cases, including success)

        std::optional<queried_type> query_type(const str& name, bool& ambiguous);
        std::optional<queried_type> query_type(const str& name);


        // TODO: pull in other methods when we need them

        // lookup is semantically extended to be able to handle w/ and w/out import path qualifier

        res<csymtab> acquire(ast_id_t id);
        std::shared_ptr<ast_node> node_of_inner_most(ast_node& x) const noexcept;
        std::shared_ptr<csymtab::entry> lookup(ast_node& x, const str& name, taul::source_pos src_pos, bool& ambiguous);
        std::shared_ptr<csymtab::entry> lookup(ast_node& x, const str& name, taul::source_pos src_pos);
        template<typename Info>
        inline bool insert(ast_node& x, const str& name, std::shared_ptr<ast_node> node, taul::source_pos starts, Info&& info, bool* no_table_found = nullptr);

        std::string fmt(size_t tabs = 0, const char* tab = "    ");


        // TODO: this whole class feels hacky, so figure out how to replace it

        // ensure_qualified takes in a unqualified or qualified name and returns an equiv
        // which is guaranteed to be qualified

        // the type does not need to exist (if it doesn't, it's presumed to be a type
        // in the compiling module which simply hasn't been declared yet)

        str ensure_qualified(const str& name);


    private:
        res<compiler_services> _services;
        import_path _src_import_path;
        ast_Chunk* _root;
        csymtab_group* _csymtabs;


        // TODO: refactor later

        struct _imported_entry final {
            res<module_info> info;
            env e;
        };

        std::unordered_map<import_path, _imported_entry> _imported;

        // this maps unqualified extern type names to symbol table entries placed
        // in the AST in order to provide info on them

        // this map is needed as the actual symbol table entry will be indexed
        // under the qualified name of the type, not its unqualified name

        // entries are not to be added to this if unqualified name collision is
        // detected between extern types

        // table entries are *shadowed* if their unqualified name is used to name
        // an entry in the root of the AST after this table's entry has been added
        // (this is important in case first pass were to do something like load
        // an extern type under some unqualified name, only for a Yama code type
        // w/ that same name appearing later on)

        std::unordered_map<str, res<csymtab::entry>> _externs;

        void _add_extern(const str& unqualified_name, const str& qualified_name);


        ast_Chunk& _get_root() const noexcept;
        csymtab_group& _get_csymtabs() const noexcept;


        void _insert_extern_prim(const queried_type& x);
        void _insert_extern_fn(const queried_type& x);
    };


    template<typename Info>
    inline bool csymtab_group_ctti::insert(ast_node& x, const str& name, std::shared_ptr<ast_node> node, taul::source_pos starts, Info&& info, bool* no_table_found) {
        // if inserting into root, check that there's no extern type that should block insertion
        if (&x == &_get_root() && query_type(name).has_value()) {
            return false;
        }
        return _get_csymtabs().insert(x, name, node, starts, std::forward<Info>(info), no_table_found);
    }
}

