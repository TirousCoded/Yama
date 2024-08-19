

#pragma once


#include "type_instance.h"
#include "res_db.h"

#include "../core/api_component.h"
#include "../core/callsig.h"
#include "../core/type_info.h"
#include "../core/type.h"


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


        inline void _instantiate_begin();
        inline void _instantiate_end(size_t n);

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

        inline bool _resolve_consts(type_instance<Allocator>& instance, res<type_info> info);
        inline bool _resolve_const(type_instance<Allocator>& instance, res<type_info> info, const_t index);

        static_assert(const_types == 7);
        inline bool _resolve_int_const(type_instance<Allocator>& instance, res<type_info> info, const_t index);
        inline bool _resolve_uint_const(type_instance<Allocator>& instance, res<type_info> info, const_t index);
        inline bool _resolve_float_const(type_instance<Allocator>& instance, res<type_info> info, const_t index);
        inline bool _resolve_bool_const(type_instance<Allocator>& instance, res<type_info> info, const_t index);
        inline bool _resolve_char_const(type_instance<Allocator>& instance, res<type_info> info, const_t index);
        inline bool _resolve_primitive_type_const(type_instance<Allocator>& instance, res<type_info> info, const_t index);
        inline bool _resolve_function_type_const(type_instance<Allocator>& instance, res<type_info> info, const_t index);

        template<const_type C>
        inline bool _resolve_scalar_const(type_instance<Allocator>& instance, res<type_info> info, const_t index);
        template<const_type C>
        inline bool _resolve_type_const(type_instance<Allocator>& instance, res<type_info> info, const_t index);

        inline bool _check_for_kind_mismatch(type_instance<Allocator>& instance, res<type_info> info, const_t index, const type& resolved);
        inline bool _check_for_callsig_mismatch(type_instance<Allocator>& instance, res<type_info> info, const_t index, const type& resolved);
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
        _instantiate_begin();
        size_t old_size{}, new_size{};
        YAMA_DEREF_SAFE(_type_db) {
            old_size = _type_db->size();
            _instantiate(fullname);
            new_size = _type_db->size();
        }
        YAMA_ASSERT(new_size >= old_size);
        const size_t result = new_size - old_size;
        _instantiate_end(result);
        return result;
    }

    template<typename Allocator>
    inline void type_instantiator<Allocator>::_instantiate_begin() {
        YAMA_LOG(dbg(), type_instant_c, "type instant. batch...");
    }

    template<typename Allocator>
    inline void type_instantiator<Allocator>::_instantiate_end(size_t n) {
        if (n > 0) {
            YAMA_LOG(dbg(), type_instant_c, "type instant. success! ({} types)", n);
        }
        else {
            YAMA_LOG(dbg(), type_instant_c, "type instant. failure! ({} types)", n);
        }
    }

    template<typename Allocator>
    inline std::optional<res<type_instance<Allocator>>> type_instantiator<Allocator>::_fetch(const str& type_fullname) const noexcept {
        std::optional<res<type_instance<Allocator>>> result{};
        YAMA_DEREF_SAFE(_type_db) {
            result = _type_db->pull(type_fullname);
        }
        return result;
    }

    template<typename Allocator>
    inline std::optional<res<type_instance<Allocator>>> type_instantiator<Allocator>::_fetch_with_batch(const str& type_fullname) const noexcept {
        // attempt to fetch from _type_db
        if (const auto first_attempt = _fetch(type_fullname)) {
            return first_attempt;
        }
        // if that didn't work, attempt to fetch from _type_batch_db
        std::optional<res<type_instance<Allocator>>> second_attempt{};
        YAMA_DEREF_SAFE(_type_batch_db) {
            second_attempt = _type_batch_db->pull(type_fullname);
        }
        return second_attempt;
    }

    template<typename Allocator>
    inline void type_instantiator<Allocator>::_instantiate(const str& type_fullname) {
        if (_check_already_instantiated(type_fullname)) {
            return;
        }
        if (_add_type(type_fullname)) {
            _transfer_batch_into_instances();
        }
        _dump_batch();
    }

    template<typename Allocator>
    inline bool type_instantiator<Allocator>::_check_already_instantiated(const str& type_fullname) const {
        bool result = false;
        YAMA_DEREF_SAFE(_type_db) {
            result = _type_db->exists(type_fullname);
        }
        if (result) {
            YAMA_LOG(dbg(), type_instant_c, "error: type {} already instantiated!", type_fullname);
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
        YAMA_LOG(dbg(), type_instant_c, "instantiating {}...", type_fullname);
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
        YAMA_DEREF_SAFE(_type_db && _type_batch_db) {
            _type_batch_db->transfer(*_type_db);
        }
    }

    template<typename Allocator>
    inline void type_instantiator<Allocator>::_dump_batch() noexcept {
        YAMA_DEREF_SAFE(_type_batch_db) {
            _type_batch_db->reset();
        }
    }

    template<typename Allocator>
    inline std::optional<res<type_info>> type_instantiator<Allocator>::_pull_type_info(const str& type_fullname) {
        std::optional<res<type_info>> info{};
        YAMA_DEREF_SAFE(_type_info_db) {
            info = _type_info_db->pull(type_fullname);
        }
        return info;
    }

    template<typename Allocator>
    inline res<type_instance<Allocator>> type_instantiator<Allocator>::_create_instance(const str& type_fullname, res<type_info> info) {
        return make_res<type_instance<Allocator>>(_al, type_fullname, info);
    }

    template<typename Allocator>
    inline void type_instantiator<Allocator>::_add_to_batch(res<type_instance<Allocator>> instance) {
        YAMA_DEREF_SAFE(_type_batch_db) {
            _type_batch_db->push(instance);
        }
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
        bool result{};
        static_assert(const_types == 7);
        switch (info->consts.const_type(index).value()) {
        case int_const:             result = _resolve_int_const(instance, info, index);             break;
        case uint_const:            result = _resolve_uint_const(instance, info, index);            break;
        case float_const:           result = _resolve_float_const(instance, info, index);           break;
        case bool_const:            result = _resolve_bool_const(instance, info, index);            break;
        case char_const:            result = _resolve_char_const(instance, info, index);            break;
        case primitive_type_const:  result = _resolve_primitive_type_const(instance, info, index);  break;
        case function_type_const:   result = _resolve_function_type_const(instance, info, index);   break;
        default:                    YAMA_DEADEND;                                                   break;
        }
        return result;
    }

    template<typename Allocator>
    inline bool type_instantiator<Allocator>::_resolve_int_const(type_instance<Allocator>& instance, res<type_info> info, const_t index) {
        return _resolve_scalar_const<int_const>(instance, info, index);
    }

    template<typename Allocator>
    inline bool type_instantiator<Allocator>::_resolve_uint_const(type_instance<Allocator>& instance, res<type_info> info, const_t index) {
        return _resolve_scalar_const<uint_const>(instance, info, index);
    }

    template<typename Allocator>
    inline bool type_instantiator<Allocator>::_resolve_float_const(type_instance<Allocator>& instance, res<type_info> info, const_t index) {
        return _resolve_scalar_const<float_const>(instance, info, index);
    }

    template<typename Allocator>
    inline bool type_instantiator<Allocator>::_resolve_bool_const(type_instance<Allocator>& instance, res<type_info> info, const_t index) {
        return _resolve_scalar_const<bool_const>(instance, info, index);
    }

    template<typename Allocator>
    inline bool type_instantiator<Allocator>::_resolve_char_const(type_instance<Allocator>& instance, res<type_info> info, const_t index) {
        return _resolve_scalar_const<char_const>(instance, info, index);
    }

    template<typename Allocator>
    inline bool type_instantiator<Allocator>::_resolve_primitive_type_const(type_instance<Allocator>& instance, res<type_info> info, const_t index) {
        return _resolve_type_const<primitive_type_const>(instance, info, index);
    }

    template<typename Allocator>
    inline bool type_instantiator<Allocator>::_resolve_function_type_const(type_instance<Allocator>& instance, res<type_info> info, const_t index) {
        return _resolve_type_const<function_type_const>(instance, info, index);
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
        YAMA_ASSERT(_fetch_with_batch(fullname));
        const auto& resolved = type(**_fetch_with_batch(fullname));
        if (!_check_for_kind_mismatch(instance, info, index, resolved)) {
            return false;
        }
        if (!_check_for_callsig_mismatch(instance, info, index, resolved)) {
            return false;
        }
        instance.put<C>(index, resolved);
        return true;
    }

    template<typename Allocator>
    inline bool type_instantiator<Allocator>::_check_for_kind_mismatch(type_instance<Allocator>& instance, res<type_info> info, const_t index, const type& resolved) {
        const auto  t               = type(instance);
        const str   symbol_fullname = info->consts.fullname(index).value();
        const auto  symbol_kind     = info->consts.kind(index).value();
        const auto  resolved_kind   = resolved.kind();
        const bool  success         = symbol_kind == resolved_kind;
        if (!success) {
            YAMA_LOG(
                dbg(), type_instant_c,
                "error: {} type constant symbol {} (at constant index {}) has corresponding type {} matched against it, but the type constant symbol describes a {}, and the resolved type is a {}!",
                t.fullname(), symbol_fullname, index, resolved.fullname(),
                symbol_kind, resolved_kind);
        }
        return success;
    }

    template<typename Allocator>
    inline bool type_instantiator<Allocator>::_check_for_callsig_mismatch(type_instance<Allocator>& instance, res<type_info> info, const_t index, const type& resolved) {
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
        // TODO: if when trying to impl above we encounter issues involving us needing complete
        //       linkage info before being able to resolve this, maybe seperate 'linking' and 
        //       'verifying' (which would be where these checks are done), making the ladder such
        //       that it occurs AFTER all actual linking has occurred
        //
        //       to do this, in _instantiate, wrap the '_transfer_batch_into_instances()' call in
        //       a nested if-statement which calls a '_verify_batch' method, and then make _verify_batch
        //       scan the batch, then scan the type consts of the types therein, and perform these
        //       kind and callsig mismatch checks there instead
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
        bool result{};
        YAMA_DEREF_SAFE(symbol_callsig) {
            // get fmt string of resolved_callsig and compare it against symbol_callsig's string
            const auto& symbol_callsig_s    = symbol_callsig->fmt(info->consts);
            const auto  resolved_callsig_s  = resolved_callsig->fmt();
            result = symbol_callsig_s == resolved_callsig_s;
            if (!result) {
                YAMA_LOG(
                    dbg(), type_instant_c,
                    "error: {} type constant symbol {} (at constant index {}) has corresponding type {} matched against it, but the type constant symbol's callsig {} doesn't match the resolved type's callsig {}!",
                    t.fullname(), info->consts.fullname(index).value(), index, resolved.fullname(),
                    symbol_callsig_s, resolved_callsig_s);
            }
        }
        return result;
    }
}

