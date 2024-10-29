

#pragma once


#include "type_instance.h"
#include "res_db.h"

#include "../core/api_component.h"
#include "../core/callsig.h"
#include "../core/type_info.h"
#include "../core/type.h"


#define _DUMP_LOG 0


namespace yama::dm {


    // NOTE: the Allocator type is for the allocator to use when instantiating
    //       *new* type_instance memory

    template<typename Allocator>
    class type_instantiator final : public api_component {
    public:

        // type_info_db is the database of type_info
        
        // type_db is the database of instantiated types, both to
        // be pulled, and pushed

        // type_batch_db is a database used to store types which are
        // in the middle of instantiation, before being transferred
        // to type_db upon success, or just reset on failure

        inline type_instantiator(
            const res_db<res<type_info>>& type_info_db,
            res_db<res<type_instance<Allocator>>>& type_db,
            res_db<res<type_instance<Allocator>>>& type_batch_db,
            Allocator al,
            std::shared_ptr<debug> dbg = nullptr);


        // instantiate attempts to instantiate the type under fullname,
        // and any other types which must also be instantiated in order
        // to satisfy direct/indirect dependency requirements

        // instantiate returns the number of types instantiated, which
        // will be 0 upon failure

        // instantiate will, if successful, will populate the type_db 
        // specified in the instantiator's ctor w/ the new types

        // instantiate will fail if any other types it attempts to 
        // instantiate fail for some reason, discarding all of them 
        // upon failure

        // instantiate will fail if a type under fullname has already
        // been instantiated

        inline size_t instantiate(const str& fullname);


    private:

        const res_db<res<type_info>>* _type_info_db = nullptr;
        res_db<res<type_instance<Allocator>>>* _type_db = nullptr;
        res_db<res<type_instance<Allocator>>>* _type_batch_db = nullptr;
        Allocator _al;


        inline auto& get_type_info_db() const noexcept { return deref_assert(_type_info_db); }
        inline auto& get_type_db() const noexcept { return deref_assert(_type_db); }
        inline auto& get_type_batch_db() const noexcept { return deref_assert(_type_batch_db); }

        inline std::optional<res<type_instance<Allocator>>> _fetch(const str& type_fullname) const noexcept;
        inline std::optional<res<type_instance<Allocator>>> _fetch_with_batch(const str& type_fullname) const noexcept;

        inline void _instantiate(const str& type_fullname);
        inline bool _check_already_instantiated(const str& type_fullname) const;

        inline bool _add_type(const str& type_fullname);
        inline void _transfer_batch_into_instances();
        inline void _dump_batch() noexcept;

        inline std::optional<res<type_info>> _pull_type_info(const str& type_fullname);
        inline res<type_instance<Allocator>> _create_instance(const str& type_fullname, res<type_info> info);
        inline void _add_to_batch(res<type_instance<Allocator>> instance);

        // herein, 'resolving' refers to the act of linking new types in the batch, w/
        // new types being added recursively as needed

        // resolving does not check the validity of links (ie. whether the type linked
        // against a given type constant symbol is reasonable to do so w/)

        inline bool _resolve_consts(type_instance<Allocator>& instance, res<type_info> info);
        inline bool _resolve_const(type_instance<Allocator>& instance, res<type_info> info, const_t index);
        template<const_type C>
        inline bool _resolve_scalar_const(type_instance<Allocator>& instance, res<type_info> info, const_t index);
        template<const_type C>
        inline bool _resolve_type_const(type_instance<Allocator>& instance, res<type_info> info, const_t index);

        // herein, 'checking' refers to the act of checking to see whether or not the
        // type constant links established during resolving are reasonable

        // resolving is performed first, and fully, to ensure that there's no ambiguity
        // as to whether checking will have access to any given piece of linkage info
        // (ie. it will never encounter partially linked type instances)

        inline bool _check_batch();
        inline bool _check_consts(type_instance<Allocator>& instance, res<type_info> info);
        inline bool _check_const(type_instance<Allocator>& instance, res<type_info> info, const_t index);
        template<const_type C>
        inline bool _check_scalar_const(type_instance<Allocator>& instance, res<type_info> info, const_t index);
        template<const_type C>
        inline bool _check_type_const(type_instance<Allocator>& instance, res<type_info> info, const_t index);
        inline bool _check_no_kind_mismatch(type_instance<Allocator>& instance, res<type_info> info, const_t index, const type& resolved);
        inline bool _check_no_callsig_mismatch(type_instance<Allocator>& instance, res<type_info> info, const_t index, const type& resolved);
    };


    template<typename Allocator>
    inline type_instantiator<Allocator>::type_instantiator(
        const res_db<res<type_info>>& type_info_db,
        res_db<res<type_instance<Allocator>>>& type_db, 
        res_db<res<type_instance<Allocator>>>& type_batch_db, 
        Allocator al, 
        std::shared_ptr<debug> dbg) 
        : api_component(dbg), 
        _type_info_db(&type_info_db), 
        _type_db(&type_db), 
        _type_batch_db(&type_batch_db), 
        _al(std::move(al)) {}

    template<typename Allocator>
    inline size_t type_instantiator<Allocator>::instantiate(const str& fullname) {
        const size_t old_size = get_type_db().size();
        _instantiate(fullname);
        const size_t new_size = get_type_db().size();
        return new_size - old_size;
    }

    template<typename Allocator>
    inline std::optional<res<type_instance<Allocator>>> type_instantiator<Allocator>::_fetch(const str& type_fullname) const noexcept {
        return get_type_db().pull(type_fullname);
    }

    template<typename Allocator>
    inline std::optional<res<type_instance<Allocator>>> type_instantiator<Allocator>::_fetch_with_batch(const str& type_fullname) const noexcept {
        // attempt to fetch from _type_db
        if (const auto first_attempt = _fetch(type_fullname)) {
            return first_attempt;
        }
        // if that didn't work, attempt to fetch from _type_batch_db
        const auto second_attempt = get_type_batch_db().pull(type_fullname);
        return second_attempt;
    }

    template<typename Allocator>
    inline void type_instantiator<Allocator>::_instantiate(const str& type_fullname) {
        if (_check_already_instantiated(type_fullname)) {
            return;
        }
        if (!_add_type(type_fullname)) {
            _dump_batch();
            return;
        }
        if (!_check_batch()) {
            _dump_batch();
            return;
        }
        _transfer_batch_into_instances();
    }

    template<typename Allocator>
    inline bool type_instantiator<Allocator>::_check_already_instantiated(const str& type_fullname) const {
        const bool result = get_type_db().exists(type_fullname);
        if (result) {
            YAMA_RAISE(dbg(), dsignal::instantiate_type_already_instantiated);
            YAMA_LOG(dbg(), instantiate_c, "error: type {} already instantiated!", type_fullname);
        }
        return result;
    }

    template<typename Allocator>
    inline bool type_instantiator<Allocator>::_add_type(const str& type_fullname) {
        // if this has already been added to _type_db or _type_batch_db, return
        // successful up-front
        if (_fetch_with_batch(type_fullname)) {
            return true;
        }
        // only log if we're actually instantiating a type not already
        YAMA_LOG(dbg(), instantiate_c, "instantiating {}...", type_fullname);
        // query res<type_info> to link
        const auto info = _pull_type_info(type_fullname);
        if (!info) {
            return false;
        }
        // create our new type instance
        const auto new_instance = _create_instance(type_fullname, *info);
        // add new_instance to the batch so it can be linked against by others,
        // w/ us doing this up-front, since mutually recursive _add_type calls
        // arising from _resolve_consts may need this info
        _add_to_batch(new_instance);
        // link the type instance, recursively adding/linking dependencies,
        // (as well as resolving other constants, too)
        if (!_resolve_consts(*new_instance, *info)) {
            return false;
        }
        // if we survived to here, then we've succeeded
        return true;
    }

    template<typename Allocator>
    inline void type_instantiator<Allocator>::_transfer_batch_into_instances() {
        get_type_batch_db().transfer(get_type_db());
        _dump_batch();
    }

    template<typename Allocator>
    inline void type_instantiator<Allocator>::_dump_batch() noexcept {
        get_type_batch_db().reset();
    }

    template<typename Allocator>
    inline std::optional<res<type_info>> type_instantiator<Allocator>::_pull_type_info(const str& type_fullname) {
        const auto result = get_type_info_db().pull(type_fullname);
        if (!result) {
            YAMA_RAISE(dbg(), dsignal::instantiate_type_not_found);
            YAMA_LOG(
                dbg(), instantiate_c,
                "error: no type info exists for type {}!",
                type_fullname);
        }
        return result;
    }

    template<typename Allocator>
    inline res<type_instance<Allocator>> type_instantiator<Allocator>::_create_instance(const str& type_fullname, res<type_info> info) {
        return make_res<type_instance<Allocator>>(_al, type_fullname, info);
    }

    template<typename Allocator>
    inline void type_instantiator<Allocator>::_add_to_batch(res<type_instance<Allocator>> instance) {
        get_type_batch_db().push(instance);
    }

    template<typename Allocator>
    inline bool type_instantiator<Allocator>::_resolve_consts(type_instance<Allocator>& instance, res<type_info> info) {
        const auto t = type(instance);
        for (const_t i = 0; i < t.consts().size(); i++) {
            if (!_resolve_const(instance, info, i)) {
                return false;
            }
        }
        return true;
    }

    template<typename Allocator>
    inline bool type_instantiator<Allocator>::_resolve_const(type_instance<Allocator>& instance, res<type_info> info, const_t index) {
        static_assert(const_types == 7);
        switch (info->consts.const_type(index).value()) {
        case int_const:             return _resolve_scalar_const<int_const>(instance, info, index);             break;
        case uint_const:            return _resolve_scalar_const<uint_const>(instance, info, index);            break;
        case float_const:           return _resolve_scalar_const<float_const>(instance, info, index);           break;
        case bool_const:            return _resolve_scalar_const<bool_const>(instance, info, index);            break;
        case char_const:            return _resolve_scalar_const<char_const>(instance, info, index);            break;
        case primitive_type_const:  return _resolve_type_const<primitive_type_const>(instance, info, index);    break;
        case function_type_const:   return _resolve_type_const<function_type_const>(instance, info, index);     break;
        default:                    YAMA_DEADEND;                                                               break;
        }
        YAMA_DEADEND;
        return bool();
    }

    template<typename Allocator>
    template<const_type C>
    inline bool type_instantiator<Allocator>::_resolve_scalar_const(type_instance<Allocator>& instance, res<type_info> info, const_t index) {
        if (const auto x = info->consts.get<C>(index)) {
            instance.put<C>(index, x->v);
        }
        else YAMA_DEADEND;
        return true;
    }

    template<typename Allocator>
    template<const_type C>
    inline bool type_instantiator<Allocator>::_resolve_type_const(type_instance<Allocator>& instance, res<type_info> info, const_t index) {
        // TODO: when we add *incomplete type cloning*-using stuff, this
        //       method may need to be revised
        YAMA_ASSERT(type(instance).consts().is_stub(index));
        const str fullname = info->consts.fullname(index).value();
        if (!_add_type(fullname)) {
            return false;
        }
        const auto& resolved = type(*_fetch_with_batch(fullname).value());
#if _DUMP_LOG
        std::cerr << std::format(">> {} type const. {} => {}\n", instance.fullname(), index, resolved);
#endif
        instance.put<C>(index, resolved);
        return true;
    }

    template<typename Allocator>
    inline bool type_instantiator<Allocator>::_check_batch() {
        for (const auto& I : get_type_batch_db()) {
            auto& instance = *I.second;
            const auto& info = internal::get_type_mem(instance)->info;
            if (!_check_consts(instance, info)) {
                return false;
            }
        }
        return true;
    }

    template<typename Allocator>
    inline bool type_instantiator<Allocator>::_check_consts(type_instance<Allocator>& instance, res<type_info> info) {
        const auto t = type(instance);
        for (const_t i = 0; i < t.consts().size(); i++) {
            if (!_check_const(instance, info, i)) {
                return false;
            }
        }
        return true;
    }

    template<typename Allocator>
    inline bool type_instantiator<Allocator>::_check_const(type_instance<Allocator>& instance, res<type_info> info, const_t index) {
        static_assert(const_types == 7);
        switch (info->consts.const_type(index).value()) {
        case int_const:             return _check_scalar_const<int_const>(instance, info, index);           break;
        case uint_const:            return _check_scalar_const<uint_const>(instance, info, index);          break;
        case float_const:           return _check_scalar_const<float_const>(instance, info, index);         break;
        case bool_const:            return _check_scalar_const<bool_const>(instance, info, index);          break;
        case char_const:            return _check_scalar_const<char_const>(instance, info, index);          break;
        case primitive_type_const:  return _check_type_const<primitive_type_const>(instance, info, index);  break;
        case function_type_const:   return _check_type_const<function_type_const>(instance, info, index);   break;
        default:                    YAMA_DEADEND;                                                           break;
        }
        YAMA_DEADEND;
        return bool();
    }

    template<typename Allocator>
    template<const_type C>
    inline bool type_instantiator<Allocator>::_check_scalar_const(type_instance<Allocator>& instance, res<type_info> info, const_t index) {
        return true; // nothing to check
    }

    template<typename Allocator>
    template<const_type C>
    inline bool type_instantiator<Allocator>::_check_type_const(type_instance<Allocator>& instance, res<type_info> info, const_t index) {
        YAMA_ASSERT(!type(instance).consts().is_stub(index));
        const str fullname = info->consts.fullname(index).value();
        const auto& resolved = type(*_fetch_with_batch(fullname).value());
        return
            _check_no_kind_mismatch(instance, info, index, resolved) &&
            _check_no_callsig_mismatch(instance, info, index, resolved);
    }

    template<typename Allocator>
    inline bool type_instantiator<Allocator>::_check_no_kind_mismatch(type_instance<Allocator>& instance, res<type_info> info, const_t index, const type& resolved) {
        const auto  t               = type(instance);
        const str   symbol_fullname = info->consts.fullname(index).value();
        const auto  symbol_kind     = info->consts.kind(index).value();
        const auto  resolved_kind   = resolved.kind();
        const bool  success         = symbol_kind == resolved_kind;
        if (!success) {
            YAMA_RAISE(dbg(), dsignal::instantiate_kind_mismatch);
            YAMA_LOG(
                dbg(), instantiate_c,
                "error: {} type constant symbol {} (at constant index {}) has corresponding type {} matched against it, but the type constant symbol describes a {}, and the resolved type is a {}!",
                t.fullname(), symbol_fullname, index, resolved.fullname(),
                symbol_kind, resolved_kind);
        }
        return success;
    }

    template<typename Allocator>
    inline bool type_instantiator<Allocator>::_check_no_callsig_mismatch(type_instance<Allocator>& instance, res<type_info> info, const_t index, const type& resolved) {
        // TODO: right now type constant callsig strings are identical to the fmt of the callsig of
        //       the instantiated type's they specify (due to Yama's current lack of namespaces 
        //       or generics)
        //
        //       so, for right now, the *correct* thing to do is to compare the constant symbol's
        //       callsig string against the callsig of the resolved instantiated type, however,
        //       later on it won't be this simple and this code will need to be revised
        // TODO: in particular, we're gonna need a way to translate the callsig fmt string of
        //       resolved into the *language* of the constant symbol table of the constant symbol
        //       at index
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
        const auto& symbol_callsig_s = deref_assert(symbol_callsig).fmt(info->consts);
        const auto  resolved_callsig_s = deref_assert(resolved_callsig).fmt();
        const bool  result = symbol_callsig_s == resolved_callsig_s;
        if (!result) {
            YAMA_RAISE(dbg(), dsignal::instantiate_callsig_mismatch);
            YAMA_LOG(
                dbg(), instantiate_c,
                "error: {} type constant symbol {} (at constant index {}) has corresponding type {} matched against it, but the type constant symbol's callsig {} doesn't match the resolved type's callsig {}!",
                t.fullname(), info->consts.fullname(index).value(), index, resolved.fullname(),
                symbol_callsig_s, resolved_callsig_s);
        }
        return result;
    }
}

