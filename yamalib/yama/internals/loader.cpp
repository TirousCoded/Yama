

#include "loader.h"

#include "../core/type.h"
#include "../core/const_table.h"
#include "../core/callsig.h"

#include "type_instance.h"
#include "specifiers.h"
#include "domain_data.h"


yama::internal::loader::loader(
    std::shared_ptr<debug> dbg,
    domain_data& dd,
    res_state& upstream)
    : api_component(dbg),
    _dd(dd),
    state() {
    state.set_upstream(upstream);
}

std::optional<yama::type> yama::internal::loader::load(const str& fullname) {
    const auto resolved_fullname = _resolve_fullname(fullname);
    if (!resolved_fullname) return std::nullopt;
    _last_was_success = _load(*resolved_fullname);
    return _last_was_success ? _pull_type(*resolved_fullname) : std::nullopt;
}

void yama::internal::loader::commit_or_discard() {
    state.commit_or_discard(_last_was_success);
    _unlock(); // can't forget
}

void yama::internal::loader::commit_or_discard(std::shared_mutex& protects_upstream) {
    state.commit_or_discard(_last_was_success, protects_upstream);
    _unlock(); // can't forget
}

void yama::internal::loader::_lock() {
    _dd->new_data_mtx.lock();
    YAMA_ASSERT(!_holds_lock); // top-level loader use shouldn't be recursive
    _holds_lock = true;
}

void yama::internal::loader::_unlock() {
    if (!_holds_lock) return;
    _holds_lock = false;
    _dd->new_data_mtx.unlock();
}

std::optional<yama::internal::fullname> yama::internal::loader::_resolve_fullname(const str& fullname) {
    bool head_was_bad{};
    // TODO: be careful replacing fullname::parse w/ a cache, as the usage of this in loader::load
    //       will, when called in domain, only be protected by inclusive lock meant for READ ONLY STUFF
    const auto result = internal::fullname::parse(_dd->installs.domain_env(), fullname, head_was_bad);
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
    const auto found = state.types.pull(fullname);
    return
        found
        ? std::make_optional(create_type(*found))
        : std::nullopt;
}

bool yama::internal::loader::_load(const fullname& fullname) {
    if (_check_already_loaded(fullname)) return true; // <- succeeds
    _lock(); // <- lock for new data gen
    if (!_add_type(fullname)) return false;
    if (!_check()) return false;
    return true;
}

bool yama::internal::loader::_check_already_loaded(const fullname& fullname) const {
    return state.types.exists(fullname);
}

bool yama::internal::loader::_add_type(const fullname& fullname) {
    if (_pull_type(fullname)) return true; // exit if type has already been added
    YAMA_LOG(dbg(), load_c, "loading {}...", fullname);
    const auto info = _acquire_type_info(fullname);
    if (!info) return false;
    return _create_and_link_instance(fullname, res(info));
}

std::shared_ptr<yama::type_info> yama::internal::loader::_acquire_type_info(const fullname& fullname) {
    const env& e = _dd->installs.domain_env();
    const auto our_path = fullname.import_path().str(e);
    const auto our_module = _dd->importer.import(e, our_path, state);
    _dd->importer.commit_or_discard(); // can't forget (also, we don't need to lock for this one)
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
        return nullptr;
    }
    // TODO: having to clone this is gross, so maybe change type_instance to instead use raw ptr
    //       to type_info instead of a res
    return std::make_shared<type_info>(our_module->info()[fullname.unqualified_name()]);
}

bool yama::internal::loader::_create_and_link_instance(const fullname& fullname, res<type_info> info) {
    const env e = _dd->installs.parcel_env(fullname.head()).value();
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
    const bool result = state.types.push(fullname, instance);
    YAMA_ASSERT(result);
}

bool yama::internal::loader::_resolve_consts(const env& e, type_instance& instance, res<type_info> info) {
    const auto n = create_type(instance).consts().size();
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
    for (const auto& [key, value] : state.types) {
        const env e = _dd->installs.parcel_env(key.head()).value();
        auto& instance = *value;
        const auto& info = get_type_mem(instance)->info;
        if (!_check_consts(e, instance, info)) return false;
    }
    return true;
}

bool yama::internal::loader::_check_consts(const env& e, type_instance& instance, res<type_info> info) {
    const auto n = create_type(instance).consts().size();
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
    const auto  t               = create_type(instance);
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
    const auto t                = create_type(instance);
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
    return fullname.str(_dd->installs.domain_env());
}

std::string yama::internal::loader::_fmt_fullname(const fullname& fullname) const {
    return fullname.fmt(_dd->installs.domain_env());
}

