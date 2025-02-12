

#pragma once


#include "csymtab.h"

#include "../core/domain.h"


namespace yama::internal {


    // TODO: maybe revise code around csymtabs to use an interface
    
    // csymtab_group_ctti is a layer over a csymtab_group which makes available 'predeclared
    // types' (ie. types existing prior to compilation) by lazy loading them into the root
    // csymtab on demand

    class csymtab_group_ctti final {
    public:

        csymtab_group_ctti(
            compiler_services services,
            ast_Chunk& root,
            csymtab_group& csymtabs);


        // TODO: pull in other methods when we need them

        res<csymtab> acquire(ast_id_t id);
        std::shared_ptr<ast_node> node_of_inner_most(ast_node& x) const noexcept;
        std::shared_ptr<csymtab::entry> lookup(ast_node& x, const str& name, taul::source_pos src_pos);
        template<typename Info>
        inline bool insert(ast_node& x, const str& name, std::shared_ptr<ast_node> node, taul::source_pos starts, Info&& info, bool* no_table_found = nullptr);

        std::string fmt(size_t tabs = 0, const char* tab = "    ");


    private:
        compiler_services _services;
        ast_Chunk* _root;
        csymtab_group* _csymtabs;


        ast_Chunk& _get_root() const noexcept;
        csymtab_group& _get_csymtabs() const noexcept;


        void _insert_predeclared_prim(res<ast_node> x, type t);
        void _insert_predeclared_fn(res<ast_node> x, type t);
    };


    template<typename Info>
    inline bool csymtab_group_ctti::insert(ast_node& x, const str& name, std::shared_ptr<ast_node> node, taul::source_pos starts, Info&& info, bool* no_table_found) {
        // if inserting into root, check that there's no predeclared type that should block insert
        if (&x == &_get_root() && _services.load(name).has_value()) {
            return false;
        }
        return _get_csymtabs().insert(x, name, node, starts, std::forward<Info>(info), no_table_found);
    }
}

