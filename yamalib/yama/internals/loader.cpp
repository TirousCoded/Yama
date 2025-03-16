

#include "loader.h"

#include "../core/type.h"
#include "../core/const_table.h"
#include "../core/callsig.h"

#include "type_instance.h"


yama::internal::loader::loader(
    std::shared_ptr<debug> dbg,
    res_state& upstream,
    install_manager& install_manager,
    importer& importer)
    : api_component(dbg),
    _install_manager(&install_manager),
    _importer(&importer),
    _state() {
    _state.set_upstream(upstream);
}

std::optional<yama::type> yama::internal::loader::load(const str& fullname) {
    const auto resolved_fullname = _resolve_fullname(fullname);
    if (!resolved_fullname) return std::nullopt;
    const auto success = _load(*resolved_fullname);
    _state.commit_or_discard(success);
    return
        success
        ? std::make_optional(type(deref_assert(_state.types.pull(*resolved_fullname))))
        : std::nullopt;
}

yama::internal::install_manager& yama::internal::loader::_get_install_manager() const noexcept {
    return deref_assert(_install_manager);
}

yama::internal::importer& yama::internal::loader::_get_importer() const noexcept {
    return deref_assert(_importer);
}

std::optional<yama::fullname> yama::internal::loader::_resolve_fullname(const str& fullname) {
    bool head_was_bad{};
    const auto result = yama::fullname::parse(_get_install_manager().domain_env(), fullname, head_was_bad);
    if (!result) {
        YAMA_RAISE(dbg(), dsignal::load_type_not_found);
        if (!head_was_bad) {
            YAMA_LOG(
                dbg(), load_error_c,
                "error: load {} failed; syntax error!",
                fullname);
        }
        else {
            YAMA_LOG(
                dbg(), load_error_c,
                "error: load {} failed; head specifies parcel not in env!",
                fullname);
        }
    }
    return result;
}

std::optional<yama::type> yama::internal::loader::_pull_type(const fullname& fullname) const noexcept {
    const auto found = _state.types.pull(fullname);
    return
        found
        ? std::make_optional(type(*found))
        : std::nullopt;
}

bool yama::internal::loader::_load(const fullname& fullname) {
    if (_check_already_loaded(fullname)) return true; // <- succeeds
    if (!_add_type(fullname)) return false;
    if (!_check()) return false;
    return true;
}

bool yama::internal::loader::_check_already_loaded(const fullname& fullname) const {
    if (!_state.types.exists(fullname)) return false;
    // this can only arise due to improper use of internal class loader
    YAMA_LOG(dbg(), load_error_c, "error: type {} already loaded!", _fmt_fullname(fullname));
    return true;
}

bool yama::internal::loader::_add_type(const fullname& fullname) {
    if (_pull_type(fullname)) return true; // exit if type has already been added
    YAMA_LOG(dbg(), load_c, "loading {}...", _fmt_fullname(fullname));
    const auto info = _acquire_type_info(fullname);
    if (!info) return false;
    return _create_and_link_instance(fullname, res(info));
}

std::shared_ptr<yama::type_info> yama::internal::loader::_acquire_type_info(const fullname& fullname) {
    const env& e = _get_install_manager().domain_env();
    const auto our_path = fullname.import_path().str(e);
    const auto our_module = _get_importer().import(e, our_path, _state);
    if (!our_module) {
        YAMA_RAISE(dbg(), dsignal::load_type_not_found);
        YAMA_LOG(
            dbg(), load_error_c,
            "error: no type info exists for type {}; {} not found!",
            _fmt_fullname(fullname),
            our_path);
        return nullptr;
    }
    if (!our_module->info().contains(fullname.unqualified_name())) {
        YAMA_RAISE(dbg(), dsignal::load_type_not_found);
        YAMA_LOG(
            dbg(), load_error_c,
            "error: no type info exists for type {}; {} does not have {}!",
            _fmt_fullname(fullname),
            our_path,
            fullname.unqualified_name());
        std::cerr << std::format("{}\n", our_module->info());
        return nullptr;
    }
    // TODO: having to clone this is gross, so maybe change type_instance to instead use raw ptr
    //       to type_info instead of a res
    return std::make_shared<type_info>(our_module->info()[fullname.unqualified_name()]);
}

bool yama::internal::loader::_create_and_link_instance(const fullname& fullname, res<type_info> info) {
    const env e = _get_install_manager().parcel_env(fullname.head()).value();
    const auto new_instance = _create_instance(fullname, info);
    // during linking, it's possible for other types to be loaded recursively, and these
    // new types may need to link against new_instance, and so to this end, we add our
    // new_instance to state before linking
    _add_instance_to_state(fullname, new_instance);
    return _resolve_consts(e, *new_instance, info); // link
}

yama::res<yama::internal::type_instance> yama::internal::loader::_create_instance(const fullname& fullname, res<type_info> info) {
    return make_res<type_instance>(_str_fullname(fullname), info);
}

void yama::internal::loader::_add_instance_to_state(const fullname& fullname, res<type_instance> instance) {
    const bool result = _state.types.push(fullname, instance);
    YAMA_ASSERT(result);
}

bool yama::internal::loader::_resolve_consts(const env& e, type_instance& instance, res<type_info> info) {
    const auto n = type(instance).consts().size();
    for (const_t i = 0; i < n; i++) {
        if (!_resolve_const(e, instance, info, i)) return false;
    }
    return true;
}

bool yama::internal::loader::_resolve_const(const env& e, type_instance& instance, res<type_info> info, const_t index) {
    static_assert(const_types == 7);
    switch (info->consts.const_type(index).value()) {
    case int_const:             return _resolve_scalar_const<int_const>(instance, info, index);                 break;
    case uint_const:            return _resolve_scalar_const<uint_const>(instance, info, index);                break;
    case float_const:           return _resolve_scalar_const<float_const>(instance, info, index);               break;
    case bool_const:            return _resolve_scalar_const<bool_const>(instance, info, index);                break;
    case char_const:            return _resolve_scalar_const<char_const>(instance, info, index);                break;
    case primitive_type_const:  return _resolve_type_const<primitive_type_const>(e, instance, info, index);     break;
    case function_type_const:   return _resolve_type_const<function_type_const>(e, instance, info, index);      break;
    default:                    YAMA_DEADEND;                                                                   break;
    }
    YAMA_DEADEND;
    return bool();
}

bool yama::internal::loader::_check() {
    for (const auto& I : _state.types) {
        const env e = _get_install_manager().parcel_env(I.first.head()).value();
        auto& instance = *I.second;
        const auto& info = get_type_mem(instance)->info;
        if (!_check_consts(e, instance, info)) return false;
    }
    return true;
}

bool yama::internal::loader::_check_consts(const env& e, type_instance& instance, res<type_info> info) {
    const auto n = type(instance).consts().size();
    for (const_t i = 0; i < n; i++) {
        if (!_check_const(e, instance, info, i)) return false;
    }
    return true;
}

bool yama::internal::loader::_check_const(const env& e, type_instance& instance, res<type_info> info, const_t index) {
    static_assert(const_types == 7);
    switch (info->consts.const_type(index).value()) {
    case int_const:             return _check_scalar_const<int_const>(instance, info, index);               break;
    case uint_const:            return _check_scalar_const<uint_const>(instance, info, index);              break;
    case float_const:           return _check_scalar_const<float_const>(instance, info, index);             break;
    case bool_const:            return _check_scalar_const<bool_const>(instance, info, index);              break;
    case char_const:            return _check_scalar_const<char_const>(instance, info, index);              break;
    case primitive_type_const:  return _check_type_const<primitive_type_const>(e, instance, info, index);   break;
    case function_type_const:   return _check_type_const<function_type_const>(e, instance, info, index);    break;
    default:                    YAMA_DEADEND;                                                               break;
    }
    YAMA_DEADEND;
    return bool();
}

bool yama::internal::loader::_check_no_kind_mismatch(type_instance& instance, res<type_info> info, const_t index, const type& resolved) {
    const auto  t               = type(instance);
    const str   symbol_fullname = info->consts.qualified_name(index).value();
    const auto  symbol_kind     = info->consts.kind(index).value();
    const auto  resolved_kind   = resolved.kind();
    const bool  success         = symbol_kind == resolved_kind;
    if (!success) {
        YAMA_RAISE(dbg(), dsignal::load_kind_mismatch);
        YAMA_LOG(
            dbg(), load_error_c,
            "error: {} type constant symbol {} (at constant index {}) has corresponding type {} matched against it, but the type constant symbol describes a {}, and the resolved type is a {}!",
            t.fullname(), symbol_fullname, index, resolved.fullname(),
            symbol_kind, resolved_kind);
    }
    return success;
}

bool yama::internal::loader::_check_no_callsig_mismatch(type_instance& instance, res<type_info> info, const_t index, const type& resolved) {
    const auto t                = type(instance);
    const auto symbol_callsig   = info->consts.callsig(index);
    const auto resolved_callsig = resolved.callsig();
    // if symbol_callsig is found to not be of a type w/ a callsig, return
    // successful up-front, as that means we're not dealing w/ one anyway
    if (!symbol_callsig) {
        return true;
    }
    // otherwise, expect outside code to guarantee no kind mismatch
    YAMA_ASSERT(symbol_callsig && resolved_callsig);
    // get fmt string of resolved_callsig and compare it against symbol_callsig's string
    const auto& symbol_callsig_s    = deref_assert(symbol_callsig).fmt(t.consts()); // <- use consts of type t (ie. NOT type_info)
    const auto  resolved_callsig_s  = deref_assert(resolved_callsig).fmt();
    const bool  result              = symbol_callsig_s == resolved_callsig_s;
    if (!result) {
        YAMA_RAISE(dbg(), dsignal::load_callsig_mismatch);
        YAMA_LOG(
            dbg(), load_error_c,
            "error: {} type constant symbol {} (at constant index {}) has corresponding type {} matched against it, but the type constant symbol's callsig {} doesn't match the resolved type's callsig {}!",
            t.fullname(), info->consts.qualified_name(index).value(), index, resolved.fullname(),
            symbol_callsig_s, resolved_callsig_s);
    }
    return result;
}

yama::str yama::internal::loader::_str_fullname(const fullname& fullname) const {
    return fullname.str(_get_install_manager().domain_env());
}

std::string yama::internal::loader::_fmt_fullname(const fullname& fullname) const {
    return fullname.fmt(_get_install_manager().domain_env());
}

