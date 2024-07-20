

#pragma once


#include "../core/api_component.h"
#include "../core/linksym.h"
#include "../core/type_data.h"
#include "../core/type.h"
#include "../domain-support/res_db.h"


namespace yama::dm {


    // NOTE: the Allocator type is for the allocator to use when instantiating
    //       *new* yama::type_instance memory

    template<typename Allocator>
    class type_instantiator final : public api_component {
    public:

        // type_data_db is the database of type_data
        
        // type_db is the database of instantiated types, both to
        // be pulled, and pushed

        // type_batch_db is a database used to store types which are
        // in the middle of instantiation, before being transferred
        // to type_db upon success, or just reset on failure

        inline type_instantiator(
            const res_db<type_data>& type_data_db,
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

        const res_db<type_data>* _type_data_db = nullptr;
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
        inline std::optional<type_data> _pull_type_data(const str& type_fullname);
        inline res<type_instance<Allocator>> _create_instance(const str& type_fullname, const type_data& data);
        inline void _add_to_batch(res<type_instance<Allocator>> instance);
        inline bool _resolve_links(type_instance<Allocator>& instance);
        inline bool _resolve_link(type_instance<Allocator>& instance, type witness, link_index index);
        inline bool _check_for_kind_mismatch(type_instance<Allocator>& instance, link_index index, const type& resolved);
        inline bool _check_for_callsig_mismatch(type_instance<Allocator>& instance, link_index index, const type& resolved);
        inline void _transfer_batch_into_instances();
        inline void _dump_batch() noexcept;
    };


    template<typename Allocator>
    inline type_instantiator<Allocator>::type_instantiator(
        const res_db<type_data>& type_data_db, 
        res_db<res<type_instance<Allocator>>>& type_db, 
        res_db<res<type_instance<Allocator>>>& type_batch_db, 
        Allocator al, 
        std::shared_ptr<debug> dbg) 
        : api_component(dbg), 
        _type_data_db(&type_data_db), 
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
        if (_check_already_instantiated(type_fullname)) return;
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
        YAMA_LOG(dbg(), type_instant_c, "instantiating {}...", type_fullname);
        // if this has already been added to _type_db or _type_batch_db, return
        // successful up-front
        if (_fetch_with_batch(type_fullname)) {
            return true;
        }
        // query type_data to link
        const auto data = _pull_type_data(type_fullname);
        if (!data) {
            return false;
        }
        // create our new type instance
        const auto new_instance = _create_instance(type_fullname, *data);
        // add new_instance to the batch so it can be linked against by others,
        // w/ us doing this up-front, since mutually recursive _add_type calls
        // arising from _resolve_links may need this info
        _add_to_batch(new_instance);
        // link the type instance, recursively adding/linking dependencies
        if (!_resolve_links(*new_instance)) {
            return false;
        }
        // if we survived to here, then we've succeeded
        return true;
    }

    template<typename Allocator>
    inline std::optional<type_data> type_instantiator<Allocator>::_pull_type_data(const str& type_fullname) {
        std::optional<type_data> data{};
        YAMA_DEREF_SAFE(_type_data_db) {
            data = _type_data_db->pull(type_fullname);
        }
        return data;
    }

    template<typename Allocator>
    inline res<type_instance<Allocator>> type_instantiator<Allocator>::_create_instance(const str& type_fullname, const type_data& data) {
        return make_res<type_instance<Allocator>>(_al, type_fullname, data);
    }

    template<typename Allocator>
    inline void type_instantiator<Allocator>::_add_to_batch(res<type_instance<Allocator>> instance) {
        YAMA_DEREF_SAFE(_type_batch_db) {
            _type_batch_db->push(instance);
        }
    }

    template<typename Allocator>
    inline bool type_instantiator<Allocator>::_resolve_links(type_instance<Allocator>& instance) {
        const auto witness = type(instance);
        for (link_index i = 0; i < witness.links().size(); i++) {
            if (!_resolve_link(instance, witness, i)) {
                return false;
            }
        }
        return true;
    }

    template<typename Allocator>
    inline bool type_instantiator<Allocator>::_resolve_link(type_instance<Allocator>& instance, type witness, link_index index) {
        // TODO: when we add *incomplete type cloning*-using stuff, this
        //       method may need to be revised
        YAMA_ASSERT(!witness.links()[index]);
        if (!_add_type(witness.linksyms()[index].fullname)) {
            return false;
        }
        YAMA_ASSERT(_fetch_with_batch(witness.linksyms()[index].fullname));
        const auto& resolved = type(**_fetch_with_batch(witness.linksyms()[index].fullname));
        if (!_check_for_kind_mismatch(instance, index, resolved)) {
            return false;
        }
        // NOTE: note how we skip the below check in the event of a kind mismatch,
        //       as w/out kind matching, callsig mismatch checking I feel is no
        //       longer all that meaningful
        if (!_check_for_callsig_mismatch(instance, index, resolved)) {
            return false;
        }
        instance.put_link(index, resolved);
        return true;
    }

    template<typename Allocator>
    inline bool type_instantiator<Allocator>::_check_for_kind_mismatch(type_instance<Allocator>& instance, link_index index, const type& resolved) {
        const auto t = type(instance);
        const auto& linksym_v = t.linksyms()[index];
        const auto linksym_kind = linksym_v.kind;
        const auto resolved_kind = resolved.kind();
        const bool success = linksym_kind == resolved_kind;
        if (!success) {
            YAMA_LOG(
                dbg(), type_instant_c,
                "error: {} link symbol {} (at link index {}) has corresponding type {} matched against it, but the link symbol describes a {}, and the resolved type is a {}!",
                t.fullname(), linksym_v.fullname, index, resolved.fullname(),
                linksym_kind, resolved_kind);
        }
        return success;
    }

    template<typename Allocator>
    inline bool type_instantiator<Allocator>::_check_for_callsig_mismatch(type_instance<Allocator>& instance, link_index index, const type& resolved) {
        const auto t = type(instance);
        const auto& linksym_v = t.linksyms()[index];
        // TODO: right now linksym callsig strings are identical to the fmt of the callsig of
        //       the instantiated type's they specify (due to Yama's current lack of qualifiers 
        //       or generics)
        //
        //       so, for right now, the *correct* thing to do is to compare the linksym's
        //       callsig string against the callsig of the resolved instantiated type, however,
        //       later on it won't be this simple and this code will need to be revised
        const auto& linksym_callsig = linksym_v.callsig;
        const auto resolved_callsig = resolved.callsig();
        // if either is std::nullopt, return whether they both are
        if (!(bool)linksym_callsig || !(bool)resolved_callsig) {
            // gotta convert both to bool, then compare, as they're not the same std::optional type
            return (bool)linksym_callsig == (bool)resolved_callsig;
        }
        // get fmt string of resolved_callsig and compare it against type_callsig's string
        const auto& linksym_callsig_s = linksym_callsig->fmt(t.linksyms());
        const auto resolved_callsig_s = resolved_callsig->fmt();
        const bool result = linksym_callsig_s == resolved_callsig_s;
        if (!result) {
            YAMA_LOG(
                dbg(), type_instant_c, 
                "error: {} link symbol {} (at link index {}) has corresponding type {} matched against it, but the link symbol's callsig {} does not match the resolved type's callsig {}!",
                t.fullname(), linksym_v.fullname, index, resolved.fullname(), 
                linksym_callsig_s, resolved_callsig_s);
        }
        return result;
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
}

