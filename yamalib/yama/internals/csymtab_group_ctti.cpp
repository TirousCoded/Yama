

#include "csymtab_group_ctti.h"

#include "../core/callsig.h"


#define _DUMP_LOG_1 0
#define _DUMP_LOG_2 0
#define _DUMP_LOG_3 0


yama::internal::csymtab_group_ctti::csymtab_group_ctti(
    res<compiler_services> services,
    const import_path& src_import_path,
    ast_Chunk& root,
    csymtab_group& csymtabs)
    : _services(services),
    _src_import_path(src_import_path),
    _root(&root),
    _csymtabs(&csymtabs) {}

bool yama::internal::csymtab_group_ctti::add_import(const import_path& path) {
#if _DUMP_LOG_3
    std::cerr << std::format("importing {}\n", path.fmt(_services->env()));
#endif
    if (path == _src_import_path) return true; // succeed up-front if importing compiling module
    if (query_import(path)) return true;
    if (const auto imported = _services->import(path)) {
        YAMA_ASSERT(imported->result.holds_module()); // TODO: add multi-source compiling later
#if _DUMP_LOG_3
        std::cerr << std::format("adding import for {}\n", path.fmt(_services->env()));
#endif
        _imported.insert({ path, _imported_entry{ .info = imported->result.get_module(), .e = imported->e } });
        return true;
    }
    return false;
}

std::optional<yama::internal::csymtab_group_ctti::queried_import> yama::internal::csymtab_group_ctti::query_import(const import_path& path) {
    const auto it = _imported.find(path);
    return
        it != _imported.end()
        ? std::make_optional(queried_import{ .ptr = it->second.info.get(), .e = it->second.e})
        : std::nullopt;
}

std::optional<yama::internal::csymtab_group_ctti::queried_type> yama::internal::csymtab_group_ctti::query_type(const str& name, bool& ambiguous) {
    ambiguous = false; // false by default, and true ONLY on specific error
    std::optional<queried_type> result{};
    // if we discern name is a qualified name
    if (const auto qn = qualified_name::parse(_services->env(), name)) {
        if (_imported.contains(qn->import_path())) {
            const auto& entry = _imported.at(qn->import_path());
            result = queried_type{
                .ptr = &(entry.info->type(qn->unqualified_name())),
                .e = entry.e,
                .from = qn->import_path(),
            };
        }
    }
    // if we discern name is NOT a qualified name (whether it's a reasonable unqualified name or not)
    else {
        size_t matches = 0;
#if _DUMP_LOG_2
        std::cerr << std::format("have {} imports\n", _imported.size());
#endif
        for (const auto& [key, value] : _imported) {
#if _DUMP_LOG_2
            std::cerr << std::format("trying {}\n", key.fmt(_services->env()));
            std::cerr << std::format("{}\n", value.info);
#endif
            if (!value.info->contains(name)) continue;
#if _DUMP_LOG_2
            std::cerr << "match!\n";
#endif
            result = queried_type{
                .ptr = &(value.info->type(name)),
                .e = value.e,
                .from = key,
            };
            matches++;
        }
        if (matches > 1) ambiguous = true; // ambiguity detected
        if (matches != 1) result.reset(); // fail if we find MULTIPLE possible matches
    }
    return result;
}

std::optional<yama::internal::csymtab_group_ctti::queried_type> yama::internal::csymtab_group_ctti::query_type(const str& name) {
    bool ambiguous{};
    return query_type(name, ambiguous);
}

yama::res<yama::internal::csymtab> yama::internal::csymtab_group_ctti::acquire(ast_id_t id) {
    return _get_csymtabs().acquire(id);
}

std::shared_ptr<yama::internal::ast_node> yama::internal::csymtab_group_ctti::node_of_inner_most(ast_node& x) const noexcept {
    return _get_csymtabs().node_of_inner_most(x);
}

std::shared_ptr<yama::internal::csymtab::entry> yama::internal::csymtab_group_ctti::lookup(ast_node& x, const str& name, taul::source_pos src_pos, bool& ambiguous) {
    ambiguous = false; // <- guarantee this gets set
#if _DUMP_LOG_1
    std::cerr << std::format("lookup {}\n", name);
#endif
    if (const auto result = _get_csymtabs().lookup(x, name, src_pos)) {
#if _DUMP_LOG_1
        std::cerr << "branch #1\n";
#endif
        return result;
    }
    // if couldn't find, see if name is qualified w/ _src_import_path, and if so, then
    // attempt w/out qualifier to see if that works (and if it doesn't, then fail, as
    // we 100% know below won't work if name is qualified w/ _src_import_path)
    else if (const auto qn = qualified_name::parse(_services->env(), name); qn && qn->import_path() == _src_import_path) {
#if _DUMP_LOG_1
        std::cerr << "branch #2\n";
#endif
        return _get_csymtabs().lookup(x, qn->unqualified_name(), src_pos); // retry
    }
    // if still couldn't find, see if there's a extern type under name
    else if (_externs.contains(name)) {
#if _DUMP_LOG_1
        std::cerr << "branch #3\n";
#endif
        return _externs.at(name);
    }
    // if still couldn't find, then try introducing a new extern type loaded
    // from some module and try one more time
    else if (const auto q = query_type(name, ambiguous)) {
#if _DUMP_LOG_1
        std::cerr << "branch #4\n";
#endif
        static_assert(kinds == 2);
        if (q->info().kind() == kind::primitive)        _insert_extern_prim(*q);
        else if (q->info().kind() == kind::function)    _insert_extern_fn(*q);
        else                                            YAMA_DEADEND;
        return _get_csymtabs().lookup(x, q->qualified_name(_services->env()), src_pos); // retry
    }
    else {
#if _DUMP_LOG_1
        std::cerr << "fail!\n";
#endif
        return nullptr; // fail
    }
}

std::shared_ptr<yama::internal::csymtab::entry> yama::internal::csymtab_group_ctti::lookup(ast_node& x, const str& name, taul::source_pos src_pos) {
    bool ambiguous{};
    return lookup(x, name, src_pos, ambiguous);
}

std::string yama::internal::csymtab_group_ctti::fmt(size_t tabs, const char* tab) {
    return _get_csymtabs().fmt(tabs, tab);
}

yama::str yama::internal::csymtab_group_ctti::ensure_qualified(const str& name) {
    // if name is unqualified AND we can discern an type symbol w/ UNQUALIFIED NAME
    // (meaning it's type defined in module, which shadows externs), then we want
    // to resolve qualified name as referring to this
    if (!qualified_name::parse(_services->env(), name) && _get_csymtabs().lookup(_get_root(), name, 0)) {
        return str(std::format("{}:{}", _src_import_path.fmt(_services->env()), name));
    }
    else if (const auto queried_type = query_type(name)) {
        return queried_type->qualified_name(_services->env());
    }
    else if (const auto qn = qualified_name::parse(_services->env(), name)) {
        return name;
    }
    // if name is unqualified, and it's not discerned to be referring to an extern
    // type, then it must be referring to a type being compiled, so prepend w/ import
    // path of compiling module
    else return str(std::format("{}:{}", _src_import_path.fmt(_services->env()), name));
}

void yama::internal::csymtab_group_ctti::_add_extern(const str& unqualified_name, const str& qualified_name) {
#if _DUMP_LOG_1
    std::cerr << std::format("add extern {} for {}\n", unqualified_name, qualified_name);
#endif
    _externs.insert({ unqualified_name, res(_get_csymtabs().lookup(_get_root(), qualified_name, 0)) });
}

yama::internal::ast_Chunk& yama::internal::csymtab_group_ctti::_get_root() const noexcept {
    return deref_assert(_root);
}

yama::internal::csymtab_group& yama::internal::csymtab_group_ctti::_get_csymtabs() const noexcept {
    return deref_assert(_csymtabs);
}

void yama::internal::csymtab_group_ctti::_insert_extern_prim(const queried_type& q) {
    prim_csym sym{
        //
    };
    const auto qualified_name = q.qualified_name(_services->env());
    // insert symbol
    bool no_table_found = false;
    const bool success = _get_csymtabs().insert(
        _get_root(), // extern types must be inserted into the ROOT node
        qualified_name, // query success == no ambiguity, and no new imports may occur by this point
        nullptr, // extern types have no corresponding AST node
        0, // extern types are in global scope
        sym,
        &no_table_found);
    YAMA_ASSERT(!no_table_found);
    YAMA_ASSERT(success); // shouldn't be able to fail
    _add_extern(q.info().unqualified_name, qualified_name);
}

void yama::internal::csymtab_group_ctti::_insert_extern_fn(const queried_type& q) {
    // convert_qn changes qn of constsym at index from parcel env of parcel of type in q
    // to parcel env of the compilation
    auto convert_qn =
        [this, &q](const_t index) -> str {
        const str constsym_qn = q.info().consts.qualified_name(index).value();
        const qualified_name qn = qualified_name::parse(q.e, constsym_qn).value();
        return qn.str(_services->env());
        };
    const callsig_info& callsig = deref_assert(q.info().callsig());
    fn_csym sym{
        // populate callsig return type of sym
        .return_type = convert_qn(callsig.ret),
    };
    // populate callsig params of sym
    for (size_t i = 0; i < callsig.params.size(); i++) {
        // TODO: 'v#' as we don't have access to actual param name (maybe add a way?)
        const str param_name = str(std::format("v{0}", i));
        sym.params.push_back(fn_csym::param{
            .name = param_name,
            .type = convert_qn(callsig.params[i]),
            });
    }
    const auto qualified_name = q.qualified_name(_services->env());
    // insert symbol
    bool no_table_found = false;
    const bool success = _get_csymtabs().insert(
        _get_root(), // extern types must be inserted into the ROOT node
        qualified_name, // query success == no ambiguity, and no new imports may occur by this point
        nullptr, // extern types have no corresponding AST node
        0, // extern types are in global scope
        sym,
        &no_table_found);
    YAMA_ASSERT(!no_table_found);
    YAMA_ASSERT(success); // shouldn't be able to fail
    _add_extern(q.info().unqualified_name, qualified_name);
}

