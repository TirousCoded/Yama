

#include "ctypesys.h"

#include "../core/asserts.h"
#include "../core/general.h"

#include "compiler.h"
#include "csymtab.h"
#include "ctype_resolver.h"


using namespace yama::string_literals;


yama::internal::ctype::ctype(ctypesys& s, res<csymtab_entry> x, const import_path& where, const env& e)
    : _s(&s),
    _info(std::move(x)),
    _e(e),
    _where(where) {
    YAMA_ASSERT(_csymtab_entry()->is_type());
}

yama::internal::ctype::ctype(ctypesys& s, const type_info& x, const import_path& where, const env& e)
    : _s(&s),
    _info(&x),
    _e(e),
    _where(where) {}

yama::internal::fullname yama::internal::ctype::fullname() const {
    const str our_uqn =
        _csymtab_entry_not_typeinf()
        ? _csymtab_entry()->name
        : _typeinf().unqualified_name;
    return internal::fullname(qualified_name(_where, our_uqn));
}

yama::kind yama::internal::ctype::kind() const noexcept {
    if (_csymtab_entry_not_typeinf()) {
        yama::kind result{};
        static_assert(kinds == 2);
        if (_csymtab_entry()->is<prim_csym>())      result = yama::kind::primitive;
        else if (_csymtab_entry()->is<fn_csym>())   result = yama::kind::function;
        return result;
    }
    else {
        return _typeinf().kind();
    }
}

size_t yama::internal::ctype::param_count() const noexcept {
    if (_csymtab_entry_not_typeinf()) {
        return
            _csymtab_entry()->is<fn_csym>()
            ? _csymtab_entry()->as<fn_csym>().params.size()
            : 0;
    }
    else {
        return deref_assert(_typeinf().callsig()).params.size();
    }
}

std::optional<yama::internal::ctype> yama::internal::ctype::param_type(size_t param_index) const {
    if (_csymtab_entry_not_typeinf()) {
        if (!_csymtab_entry()->is<fn_csym>()) return std::nullopt;
        const auto& our_csym = _csymtab_entry()->as<fn_csym>();
        if (param_index >= our_csym.params.size()) return std::nullopt; // out-of-bounds
        return _sys().ctype_resolver()[our_csym.params[param_index].type];
    }
    else {
        const auto& our_params = deref_assert(_typeinf().callsig()).params;
        if (param_index >= our_params.size()) return std::nullopt; // out-of-bounds
        const auto& our_const = our_params[param_index];
        const str our_qn = _typeinf().consts.qualified_name(our_const).value();
        return _sys().load(_sys().sp().pull_f(e(), our_qn).value()); // <- can parse qn as fullname
    }
}

std::optional<yama::internal::ctype> yama::internal::ctype::return_type() const {
    if (_csymtab_entry_not_typeinf()) {
        return
            _csymtab_entry()->is<fn_csym>()
            ? _sys().ctype_resolver()[_csymtab_entry()->as<fn_csym>().return_type]
            : std::nullopt;
    }
    else {
        const auto& our_const = deref_assert(_typeinf().callsig()).ret;
        const str our_qn = _typeinf().consts.qualified_name(our_const).value();
        return _sys().load(_sys().sp().pull_f(e(), our_qn).value()); // <- can parse qn as fullname
    }
}

std::string yama::internal::ctype::fmt(const env& e) const {
    return fullname().fmt(e);
}

bool yama::internal::ctype::_csymtab_entry_not_typeinf() const noexcept {
    YAMA_ASSERT(std::holds_alternative<res<csymtab_entry>>(_info) != std::holds_alternative<const type_info*>(_info));
    return std::holds_alternative<res<csymtab_entry>>(_info);
}

const yama::res<yama::internal::csymtab_entry>& yama::internal::ctype::_csymtab_entry() const {
    YAMA_ASSERT(_csymtab_entry_not_typeinf());
    return std::get<res<csymtab_entry>>(_info);
}

const yama::type_info& yama::internal::ctype::_typeinf() const {
    YAMA_ASSERT(!_csymtab_entry_not_typeinf());
    return yama::deref_assert(std::get<const yama::type_info*>(_info));
}

yama::internal::cmodule::cmodule(ctypesys& s, const res<csymtab>& root_csymtab, const internal::import_path& where, const env& e)
    : _s(&s),
    _info(root_csymtab),
    _e(e),
    _where(where) {}

yama::internal::cmodule::cmodule(ctypesys& s, const res<module_info>& x, const internal::import_path& where, const env& e)
    : _s(&s),
    _info(x),
    _e(e),
    _where(where) {}

std::optional<yama::internal::ctype> yama::internal::cmodule::type(const str& unqualified_name) {
    if (_csymtab_not_modinf()) {
        if (const auto entry = _csymtab()->fetch(unqualified_name); entry && entry->is_type()) {
            return ctype(_sys(), res(entry), import_path(), e());
        }
    }
    else {
        if (_modinf()->contains(unqualified_name)) {
            return ctype(_sys(), _modinf()->type(unqualified_name), import_path(), e());
        }
    }
    return std::nullopt;
}

bool yama::internal::cmodule::_csymtab_not_modinf() const noexcept {
    YAMA_ASSERT(std::holds_alternative<res<csymtab>>(_info) != std::holds_alternative<res<module_info>>(_info));
    return std::holds_alternative<res<csymtab>>(_info);
}

const yama::res<yama::internal::csymtab>& yama::internal::cmodule::_csymtab() const {
    YAMA_ASSERT(_csymtab_not_modinf());
    return std::get<res<csymtab>>(_info);
}

const yama::res<yama::module_info>& yama::internal::cmodule::_modinf() const {
    YAMA_ASSERT(!_csymtab_not_modinf());
    return std::get<res<module_info>>(_info);
}

yama::internal::ctypesys::ctypesys(
    specifier_provider& sp,
    res<compiler_services> services,
    internal::ctype_resolver& ctype_resolver)
    : _sp(&sp),
    _services(std::move(services)),
    _ctype_resolver(&ctype_resolver) {}

yama::internal::ctype_resolver& yama::internal::ctypesys::ctype_resolver() const noexcept {
    return deref_assert(_ctype_resolver);
}

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
    if (const auto imported = services()->import(x)) {
        if (imported->result.holds_module()) {
            auto new_cmodule = yama::make_res<cmodule>(*this, imported->result.get_module(), x, imported->e);
            _modules.insert({ x, new_cmodule });
            return new_cmodule;
        }
        else if (imported->result.holds_source()) {
            // TODO: impl multi-source support
        }
        else YAMA_DEADEND;
    }
    return nullptr;
}

std::optional<yama::internal::ctype> yama::internal::ctypesys::load(const fullname& x) {
    const auto m = fetch_module(x.import_path());
    return
        m
        ? m->type(x.unqualified_name())
        : std::nullopt;
}

bool yama::internal::ctypesys::register_module(const import_path& where, res<csymtab> x, const env& e) {
    if (_modules.contains(where)) return false;
    const auto new_cmodule = yama::make_res<cmodule>(*this, x, where, e);
    _modules.insert({ where, new_cmodule });
    return true;
}

yama::internal::ctypesys_local::ctypesys_local(ctypesys& upstream, const taul::source_code& src, const import_path& local_import_path)
    : _upstream(&upstream),
    _src(&src),
    _local_ip(local_import_path) {}

std::optional<yama::internal::ctype> yama::internal::ctypesys_local::load(const str& unqualified_name, bool& ambiguous) {
    auto try_load = [&](const import_path& ip) -> std::optional<ctype> {
        return upstream().load(qualified_name(ip, unqualified_name));
        };
    // guarantee ambiguous always gets set
    ambiguous = false;
    // search w/ local import path (shadowing import set matches)
    if (const auto result = try_load(_local_ip)) {
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

std::optional<yama::internal::ctype> yama::internal::ctypesys_local::load(const ast_TypeSpec& x, bool& ambiguous) {
    // guarantee ambiguous always gets set
    ambiguous = false;
    // TODO: later we'll need to account for import alias qualifiers
    return load(x.type.str(yama::deref_assert(_src)), ambiguous);
}

std::optional<yama::internal::ctype> yama::internal::ctypesys_local::load(const ast_TypeSpec& x) {
    bool ambiguous{};
    return load(x, ambiguous);
}

bool yama::internal::ctypesys_local::register_module(const import_path& where, res<csymtab> x, const env& e) {
    return upstream().register_module(where, std::move(x), e);
}

void yama::internal::ctypesys_local::add_import(const import_path& where) {
    // IMPORTANT: keep _local_ip out of _import_set
    if (where != _local_ip) _import_set.insert(where);
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

const yama::internal::ctypesys_local::_builtin_cache& yama::internal::ctypesys_local::_get_builtin_cache() {
    if (!_builtin_cache_v) {
        auto load_builtin = [&](const str& x) -> ctype {
            return load(sp().pull_f(services()->env(), x).value()).value();
            };
        _builtin_cache_v = _builtin_cache{
            .none   = load_builtin("yama:None"_str),
            .int0   = load_builtin("yama:Int"_str),
            .uint   = load_builtin("yama:UInt"_str),
            .float0 = load_builtin("yama:Float"_str),
            .bool0  = load_builtin("yama:Bool"_str),
            .char0  = load_builtin("yama:Char"_str),
        };
    }
    return _builtin_cache_v.value();
}
