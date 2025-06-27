

#include "ctypesys.h"

#include "../core/asserts.h"
#include "../core/general.h"
#include "../core/kind-features.h"

#include "domain_data.h"
#include "compiler.h"
#include "util.h"


using namespace yama::string_literals;


yama::internal::ctype::ctype(ctypesys& s, res<csymtab_entry> x, const import_path& where)
    : _s(s),
    _info(std::move(x)),
    _where(where) {
    YAMA_ASSERT(_csymtab_entry()->is_type());
}

yama::internal::ctype::ctype(ctypesys& s, const type_info& x, const import_path& where)
    : _s(s),
    _info(x),
    _where(where) {}

yama::internal::unqualified_name yama::internal::ctype::uqn() const {
    return
        _csymtab_entry_not_typeinf()
        ? _csymtab_entry()->name
        : _typeinf().unqualified_name();
}

yama::internal::fullname yama::internal::ctype::fln() const {
    return qualified_name(_where, uqn());
}

yama::kind yama::internal::ctype::kind() const noexcept {
    if (_csymtab_entry_not_typeinf()) {
        yama::kind result{};
        static_assert(kinds == 4);
        if (_csymtab_entry()->is<prim_csym>())          result = yama::kind::primitive;
        else if (_csymtab_entry()->is<fn_like_csym>())  result = _csymtab_entry()->as<fn_like_csym>().is_method ? yama::kind::method : yama::kind::function;
        else if (_csymtab_entry()->is<struct_csym>())   result = yama::kind::struct0;
        return result;
    }
    else {
        return _typeinf().kind();
    }
}

std::optional<yama::internal::ctype> yama::internal::ctype::owner_type() const {
    // assert that if is_member(kind()) == true, then we MUST have valid owner/member division
    YAMA_ASSERT(!is_member(kind()) || fln().uqn().is_member());
    return
        is_member(kind())
        ? _s->load(qualified_name(_where, uqn().owner_name()))
        : std::nullopt;
}

size_t yama::internal::ctype::param_count() const noexcept {
    if (_csymtab_entry_not_typeinf()) {
        return
            _csymtab_entry()->is<fn_like_csym>()
            ? _csymtab_entry()->as<fn_like_csym>().params.size()
            : 0;
    }
    else {
        return deref_assert(_typeinf().callsig()).params.size();
    }
}

std::optional<yama::internal::ctype> yama::internal::ctype::param_type(size_t param_index, compiler& cs) const {
    if (_csymtab_entry_not_typeinf()) {
        if (!_csymtab_entry()->is<fn_like_csym>()) {
            return std::nullopt;
        }
        const auto& our_csym = _csymtab_entry()->as<fn_like_csym>();
        if (param_index >= our_csym.params.size()) {
            return std::nullopt; // out-of-bounds
        }
        const auto& our_param = our_csym.params[param_index]->as<param_csym>();
        return
            our_param.self_param
            ? owner_type()
            : cs.ea.crvalue_to_type(deref_assert(deref_assert(our_param.type).expr));
    }
    else {
        const auto& our_params = deref_assert(_typeinf().callsig()).params;
        if (param_index >= our_params.size()) {
            return std::nullopt; // out-of-bounds
        }
        const auto& our_const = our_params[param_index];
        const str our_qn(_typeinf().consts().qualified_name(our_const).value());
        return _s->load(_s->cs->sp.qn(e(), our_qn).value());
    }
}

std::optional<yama::internal::ctype> yama::internal::ctype::return_type(compiler& cs) const {
    if (_csymtab_entry_not_typeinf()) {
        if (_csymtab_entry()->is<fn_like_csym>()) {
            if (const auto return_type = _csymtab_entry()->as<fn_like_csym>().return_type) {
                return cs.ea.crvalue_to_type(deref_assert(return_type->expr));
            }
        }
        return std::nullopt;
    }
    else {
        const auto& our_const = deref_assert(_typeinf().callsig()).ret;
        const str our_qn(_typeinf().consts().qualified_name(our_const).value());
        return _s->load(_s->cs->sp.qn(e(), our_qn).value());
    }
}

std::string yama::internal::ctype::fmt(const env& e) const {
    return fln().fmt(e);
}

yama::internal::env yama::internal::ctype::_e() const {
    return _s->cs->dd->installs.parcel_env(fln().ip().head()).value();
}

bool yama::internal::ctype::_csymtab_entry_not_typeinf() const noexcept {
    YAMA_ASSERT(std::holds_alternative<res<csymtab_entry>>(_info) != std::holds_alternative<safeptr<const type_info>>(_info));
    return std::holds_alternative<res<csymtab_entry>>(_info);
}

const yama::res<yama::internal::csymtab_entry>& yama::internal::ctype::_csymtab_entry() const {
    YAMA_ASSERT(_csymtab_entry_not_typeinf());
    return std::get<res<csymtab_entry>>(_info);
}

const yama::type_info& yama::internal::ctype::_typeinf() const {
    YAMA_ASSERT(!_csymtab_entry_not_typeinf());
    return *std::get<safeptr<const yama::type_info>>(_info);
}

yama::internal::cmodule::cmodule(ctypesys& s, translation_unit& tu, const internal::import_path& where)
    : _s(s),
    _info(safeptr(*(_dummy_t*)&tu)),
    _where(where) {}

yama::internal::cmodule::cmodule(ctypesys& s, const res<module_info>& x, const internal::import_path& where)
    : _s(s),
    _info(x),
    _where(where) {}

std::optional<yama::internal::ctype> yama::internal::cmodule::type(const str& unqualified_name) {
    if (_tu_not_modinf()) {
        const auto& our_csymtab = deref_assert(_tu().syms.get(_tu().root().id));
        if (const auto entry = our_csymtab.fetch(unqualified_name); entry && entry->is_type()) {
            return ctype(*_s, res(entry), ip());
        }
    }
    else {
        if (_modinf()->contains(unqualified_name)) {
            return ctype(*_s, _modinf()->type(unqualified_name), ip());
        }
    }
    return std::nullopt;
}

yama::internal::env yama::internal::cmodule::_e() const {
    return _s->cs->dd->installs.parcel_env(ip().head()).value();
}

bool yama::internal::cmodule::_tu_not_modinf() const noexcept {
    YAMA_ASSERT(std::holds_alternative<safeptr<_dummy_t>>(_info) != std::holds_alternative<res<module_info>>(_info));
    return std::holds_alternative<safeptr<_dummy_t>>(_info);
}

const yama::internal::translation_unit& yama::internal::cmodule::_tu() const {
    YAMA_ASSERT(_tu_not_modinf());
    return *(translation_unit*)std::get<safeptr<_dummy_t>>(_info).get();
}

const yama::res<yama::module_info>& yama::internal::cmodule::_modinf() const {
    YAMA_ASSERT(!_tu_not_modinf());
    return std::get<res<module_info>>(_info);
}

yama::internal::ctypesys::ctypesys(compiler& cs)
    : cs(cs) {}

std::shared_ptr<yama::internal::cmodule> yama::internal::ctypesys::fetch_module(const import_path& x) const {
    const auto it = _modules.find(x);
    return
        it != _modules.end()
        ? it->second.base()
        : nullptr;
}

std::shared_ptr<yama::internal::cmodule> yama::internal::ctypesys::import(const import_path& x) {
    if (const auto found = fetch_module(x)) {
        return found;
    }
    std::shared_ptr<cmodule> result{};
    if (auto imported = cs->dd->importer.import_for_import_dir(x)) {
        if (imported->holds_module()) {
            result = register_module(x, imported->get_module());
        }
        else if (imported->holds_source()) {
            auto new_unit = yama::make_res<translation_unit>(*cs, std::move(imported->get_source()), x);
            cs->push_new_unit(new_unit); // <- register new translation unit
            result = register_module(*new_unit);
        }
        else YAMA_DEADEND;
    }
    return result;
}

std::optional<yama::internal::ctype> yama::internal::ctypesys::load(const fullname& x) {
    const auto m = fetch_module(x.ip());
    return
        m
        ? m->type(x.uqn().str())
        : std::nullopt;
}

std::shared_ptr<yama::internal::cmodule> yama::internal::ctypesys::register_module(const import_path& where, res<module_info> x) {
    if (_modules.contains(where)) return nullptr;
    const auto new_cmodule = yama::make_res<cmodule>(*this, x, where);
    _modules.insert({ where, new_cmodule });
    return new_cmodule;
}

std::shared_ptr<yama::internal::cmodule> yama::internal::ctypesys::register_module(translation_unit& x) {
    if (_modules.contains(x.src_path)) return nullptr;
    const auto new_cmodule = yama::make_res<cmodule>(*this, x, x.src_path);
    _modules.insert({ x.src_path, new_cmodule });
    return new_cmodule;
}

void yama::internal::ctypesys::cleanup() {
    _modules.clear();
}

yama::internal::ctypesys_local::ctypesys_local(translation_unit& tu)
    : tu(tu) {}

std::shared_ptr<yama::internal::cmodule> yama::internal::ctypesys_local::fetch_module(const import_path& x) const {
    return tu->cs->types.fetch_module(x);
}

std::shared_ptr<yama::internal::cmodule> yama::internal::ctypesys_local::import(const import_path & x) {
    return tu->cs->types.import(x);
}

std::optional<yama::internal::ctype> yama::internal::ctypesys_local::load(const fullname& x) {
    return tu->cs->types.load(x);
}

std::optional<yama::internal::ctype> yama::internal::ctypesys_local::load(const str& unqualified_name, bool& ambiguous) {
    auto try_load = [&](const import_path& ip) -> std::optional<ctype> {
        return tu->cs->types.load(qualified_name(ip, unqualified_name));
        };
    // guarantee ambiguous always gets set
    ambiguous = false;
    // search w/ local import path (shadowing import set matches)
    if (const auto result = try_load(tu->src_path)) {
        return *result;
    }
    // search import set for match (w/ fail if ambiguous)
    std::optional<ctype> result{};
    size_t matches = 0;
    for (const auto& I : _import_set) {
        if (const auto found = try_load(I)) {
            result = *found;
            matches++;
        }
    }
    if (matches > 1) ambiguous = true; // multiple import set matches, ambiguous!
    return result && matches == 1 ? std::make_optional(*result) : std::nullopt;
}

std::optional<yama::internal::ctype> yama::internal::ctypesys_local::load(const str& unqualified_name) {
    bool ambiguous{};
    return load(unqualified_name, ambiguous);
}

std::optional<yama::internal::ctype> yama::internal::ctypesys_local::load(const str& qualifier, const str& unqualified_name, bool& bad_qualifier) {
    // guarantee bad_qualifier always gets set
    bad_qualifier = false;
    // query qualifier symbol
    std::optional<ctype> result{};
    if (const auto qualifier_sym = tu->syms.lookup(tu->root(), qualifier, 0, lookup_proc::qualifier)) {
        const auto& symbol = qualifier_sym->as<import_csym>();
        // attempt load itself
        result = tu->cs->types.load(qualified_name(symbol.path, unqualified_name));
    }
    else bad_qualifier = true;
    return result;
}

std::optional<yama::internal::ctype> yama::internal::ctypesys_local::load(const str& qualifier, const str& unqualified_name) {
    bool bad_qualifier{};
    return load(qualifier, unqualified_name, bad_qualifier);
}

std::optional<yama::internal::ctype> yama::internal::ctypesys_local::load(const ast_TypeSpec& x, bool& ambiguous, bool& bad_qualifier) {
    // guarantee ambiguous and bad_qualifier always gets set
    ambiguous = false;
    bad_qualifier = false;
    if (const auto p = deref_assert(x.expr).as<ast_PrimaryExpr>()) {
        return load(*p, ambiguous, bad_qualifier);
    }
    else return std::nullopt;
}

std::optional<yama::internal::ctype> yama::internal::ctypesys_local::load(const ast_TypeSpec& x) {
    bool ambiguous{}, bad_qualifier{};
    return load(x, ambiguous, bad_qualifier);
}

std::optional<yama::internal::ctype> yama::internal::ctypesys_local::load(const ast_PrimaryExpr& x, bool& ambiguous, bool& bad_qualifier) {
    // guarantee ambiguous and bad_qualifier always gets set
    ambiguous = false;
    bad_qualifier = false;
    // attempt load based on if qualified
    const auto name = x.name.value().str(tu->src);
    return
        x.qualifier
        ? load(x.qualifier->str(tu->src), name, bad_qualifier)
        : load(name, ambiguous);
}

std::optional<yama::internal::ctype> yama::internal::ctypesys_local::load(const ast_PrimaryExpr& x) {
    bool ambiguous{}, bad_qualifier{};
    return load(x, ambiguous, bad_qualifier);
}

std::shared_ptr<yama::internal::cmodule> yama::internal::ctypesys_local::register_module(translation_unit& x) {
    return tu->cs->types.register_module(x);
}

void yama::internal::ctypesys_local::bind_import(const import_path& where) {
    // IMPORTANT: keep tu->src_path out of _import_set
    if (where != tu->src_path) _import_set.insert(where);
}

yama::internal::ctype yama::internal::ctypesys_local::default_none(const std::optional<ctype>& x) {
    return x.value_or(none_type());
}

yama::internal::ctype yama::internal::ctypesys_local::none_type() {
    return _get_builtin_cache().none;
}

yama::internal::ctype yama::internal::ctypesys_local::int_type() {
    return _get_builtin_cache().int0;
}

yama::internal::ctype yama::internal::ctypesys_local::uint_type() {
    return _get_builtin_cache().uint;
}

yama::internal::ctype yama::internal::ctypesys_local::float_type() {
    return _get_builtin_cache().float0;
}

yama::internal::ctype yama::internal::ctypesys_local::bool_type() {
    return _get_builtin_cache().bool0;
}

yama::internal::ctype yama::internal::ctypesys_local::char_type() {
    return _get_builtin_cache().char0;
}

yama::internal::ctype yama::internal::ctypesys_local::type_type() {
    return _get_builtin_cache().type;
}

const yama::internal::ctypesys_local::_builtin_cache& yama::internal::ctypesys_local::_get_builtin_cache() {
    if (!_builtin_cache_v) {
        auto load_builtin = [&](const str& x) -> ctype {
            return load(tu->cs->sp.fln(tu->e(), x).value()).value();
            };
        _builtin_cache_v = _builtin_cache{
            .none   = load_builtin("yama:None"_str),
            .int0   = load_builtin("yama:Int"_str),
            .uint   = load_builtin("yama:UInt"_str),
            .float0 = load_builtin("yama:Float"_str),
            .bool0  = load_builtin("yama:Bool"_str),
            .char0  = load_builtin("yama:Char"_str),
            .type   = load_builtin("yama:Type"_str),
        };
    }
    return _builtin_cache_v.value();
}
