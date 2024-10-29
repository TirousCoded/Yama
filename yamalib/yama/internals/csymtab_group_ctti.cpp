

#include "csymtab_group_ctti.h"

#include "../core/callsig.h"


yama::internal::csymtab_group_ctti::csymtab_group_ctti(domain& dm, ast_Chunk& root, csymtab_group& csymtabs)
    : _dm(&dm),
    _root(&root),
    _csymtabs(&csymtabs) {}

yama::res<yama::internal::csymtab> yama::internal::csymtab_group_ctti::acquire(ast_id_t id) {
    return _get_csymtabs().acquire(id);
}

std::shared_ptr<yama::internal::ast_node> yama::internal::csymtab_group_ctti::node_of_inner_most(ast_node& x) const noexcept {
    return _get_csymtabs().node_of_inner_most(x);
}

std::shared_ptr<yama::internal::csymtab::entry> yama::internal::csymtab_group_ctti::lookup(ast_node& x, const str& name, taul::source_pos src_pos) {
    auto result = _get_csymtabs().lookup(x, name, src_pos);
    if (!result) {
        // if couldn't find, then try loading a type from domain, and if successful,
        // add that to the root of the csymtabs
        if (const auto t = _get_dm().load(name)) {
            static_assert(kinds == 2);
            if (t->kind() == kind::primitive) {
                _insert_predeclared_prim(res(x.shared_from_this()), *t);
            }
            else if (t->kind() == kind::function) {
                _insert_predeclared_fn(res(x.shared_from_this()), *t);
            }
            else YAMA_DEADEND;
            result = _get_csymtabs().lookup(x, name, src_pos); // retry
        }
    }
    return result;
}

std::string yama::internal::csymtab_group_ctti::fmt(size_t tabs, const char* tab) {
    return _get_csymtabs().fmt(tabs, tab);
}

yama::domain& yama::internal::csymtab_group_ctti::_get_dm() const noexcept {
    return deref_assert(_dm);
}

yama::internal::ast_Chunk& yama::internal::csymtab_group_ctti::_get_root() const noexcept {
    return deref_assert(_root);
}

yama::internal::csymtab_group& yama::internal::csymtab_group_ctti::_get_csymtabs() const noexcept {
    return deref_assert(_csymtabs);
}

void yama::internal::csymtab_group_ctti::_insert_predeclared_prim(res<ast_node> x, type t) {
    prim_csym sym{
        //
    };
    // insert symbol
    bool no_table_found = false;
    const bool success = _get_csymtabs().insert(
        _get_root(), // predeclared types must be inserted into the ROOT node
        t.fullname(),
        nullptr, // predeclared types have no corresponding AST node
        0, // predeclared types are in global scope
        sym,
        &no_table_found);
    YAMA_ASSERT(!no_table_found);
    YAMA_ASSERT(success); // shouldn't be able to fail
}

void yama::internal::csymtab_group_ctti::_insert_predeclared_fn(res<ast_node> x, type t) {
    const callsig t_callsig = t.callsig().value();
    const type t_return_type = t_callsig.return_type().value();
    fn_csym sym{
        .return_type = t_return_type.fullname(),
    };
    // populate callsig params of sym
    for (size_t i = 0; i < t_callsig.params(); i++) {
        const type param_type = t_callsig.param_type(i).value();
        sym.params.push_back(fn_csym::param{
            .name = str(std::format("v{0}", i)), // generate name for param
            .type = t_callsig.param_type(i).value().fullname(),
            });
    }
    // insert symbol
    bool no_table_found = false;
    const bool success = _get_csymtabs().insert(
        _get_root(), // predeclared types must be inserted into the ROOT node
        t.fullname(),
        nullptr, // predeclared types have no corresponding AST node
        0, // predeclared types are in global scope
        sym,
        &no_table_found);
    YAMA_ASSERT(!no_table_found);
    YAMA_ASSERT(success); // shouldn't be able to fail
}

