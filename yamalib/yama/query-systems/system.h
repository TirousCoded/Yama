

#pragma once


#include "../core/api_component.h"

#include "key_traits.h"
#include "untyped_provider.h"
#include "provider.h"


namespace yama::qs {


    // yama::qs::system is the abstract base class of query system impls,
    // which are the primary frontend class of Yama's query system library

    // query system impls encapsulate a frontend interface of methods w/
    // which to interact w/ the query system, performing queries, alongside
    // other related operations

    // query system impls under-the-hood are implemented using query providers,
    // which each encapsulate the behaviour of a particular query provider type

    // query system impls may have a notion of upstream/downstream systems

    template<typename QTypes>
    class system : public api_component {
    public:

        inline system(std::shared_ptr<yama::debug> dbg = nullptr);


        // number returns the number of cached results in the provider for QType

        // number returns 0 if no provider could be found

        template<QTypes QType>
        inline size_t number() const noexcept;

        // this number overload deduces the QType to use from Key

        template<typename Key>
        inline size_t number() const noexcept;

        // exists returns if cached result data exists under keys ks

        // exists only returns true if all specified keys have corresponding
        // cached result data

        // exists will deduce what providers to use from the types of ks,
        // allowing for a heterogeneous mixture of key types.

        // exists returns false if no providers could be found for >=1 keys

        // exists returns true if sizeof...(ks) == 0, as in that scenario
        // there's no scenario where the keys in question can't be found

        template<typename... Keys>
        inline bool exists(const Keys&... ks) const noexcept;
        
        // query queries the result under key k, returning the result, if any

        // query will perform computation, and potentially caching, of new result
        // data if no result was already cached

        // query will deduce what provider to use from the type of k

        // query returns std::nullopt if computation fails

        // query returns std::nullopt if no provider could be found

        template<typename Key>
        inline auto query(const Key& k);

        // fetch queries the result under key k, returning the result, if any

        // fetch, unlike query, will not perform computation/caching of new
        // result data if a cached result couldn't be found

        // fetch will deduce what provider to use from the type of k

        // fetch returns std::nullopt if no cached result data was found

        // fetch returns std::nullopt if no provider could be found

        template<typename Key>
        inline auto fetch(const Key& k);

        // discard suggests to the query system to discard any cached result
        // data under the keys ks, if any

        // discard will deduce what providers to use from the types of ks,
        // allowing for a heterogeneous mixture of key types.

        // discard will fail quietly for the keys for which no provider
        // could be found

        // discard calls are just suggestions, and the impl is free to decide
        // what data should/shouldn't be deleted, and whether or not calls
        // should be propagated upstream

        template<typename... Keys>
        inline void discard(const Keys&... ks);

        // discard_all discards all cached results from all providers

        // discard_all calls are just suggestions, and the impl is free to 
        // decide what data should/shouldn't be deleted, and whether or not 
        // calls should be propagated upstream

        inline void discard_all();

        // these discard_all overloads discard the cached data of only a
        // single provider, specified by QType

        // these discard_all overloads fail quietly if no provider could 
        // be found

        template<QTypes QType>
        inline void discard_all();

        // this discard_all overload deduces the QType to use from Key

        template<typename Key>
        inline void discard_all();


    protected:

        // the main job of a query system impl is to define a get_provider override
        // which maps a given qtype to a specific query provider, if any

        // it's VARY IMPORTANT, that the query provider, or lack thereof, which a
        // given qtype argument maps to does not change during the lifetime of the
        // query system, they must remain constant

        virtual untyped_provider<QTypes>* get_provider(QTypes qtype) const noexcept = 0;

        // this defines the query system's behaviour in regards to the discard_all
        // overload w/out any parameters, which discards all contents from all
        // providers (w/out defining whether this includes upstream providers)

        virtual void do_discard_all() = 0;


    private:

        // returns nullptr if no provider was found, or if it couldn't be downcast
        // to a provider<QTypes, QType>* safely

        template<QTypes QType>
        inline provider<QTypes, QType>* _get_provider_as() const noexcept;

        template<typename Key>
        inline bool _exists_check(const Key& k) const noexcept;
        inline bool _exists() const noexcept; // <- here to stop recursion
        template<typename Key, typename... Keys>
        inline bool _exists(const Key& k, const Keys&... ks) const noexcept;

        template<typename Key>
        inline void _discard_data(const Key& k);
        inline void _discard(); // <- here to stop recursion
        template<typename Key, typename... Keys>
        inline void _discard(const Key& k, const Keys&... ks);
    };

    template<typename QTypes>
    inline system<QTypes>::system(std::shared_ptr<yama::debug> dbg) 
        : api_component(dbg) {}

    template<typename QTypes>
    template<QTypes QType>
    inline size_t system<QTypes>::number() const noexcept {
        const auto prov = _get_provider_as<QType>();
        return
            prov
            ? prov->number()
            : 0;
    }
    
    template<typename QTypes>
    template<typename Key>
    inline size_t system<QTypes>::number() const noexcept {
        return number<key_traits<QTypes, Key>::qtype>();
    }

    template<typename QTypes>
    template<typename... Keys>
    inline bool system<QTypes>::exists(const Keys&... ks) const noexcept {
        return _exists(ks...); // <- recursive
    }

    template<typename QTypes>
    template<typename Key>
    inline auto system<QTypes>::query(const Key& k) {
        constexpr auto qtype = key_traits<QTypes, Key>::qtype;
        using return_t = std::optional<result<QTypes, qtype>>;
        return_t result = std::nullopt;
        if (const auto prov = _get_provider_as<qtype>()) {
            result = prov->query(k);
        }
        return result;
    }

    template<typename QTypes>
    template<typename Key>
    inline auto system<QTypes>::fetch(const Key& k) {
        constexpr auto qtype = key_traits<QTypes, Key>::qtype;
        using return_t = std::optional<result<QTypes, qtype>>;
        return_t result = std::nullopt;
        if (const auto prov = _get_provider_as<qtype>()) {
            result = prov->fetch(k);
        }
        return result;
    }

    template<typename QTypes>
    template<typename ...Keys>
    inline void system<QTypes>::discard(const Keys&... ks) {
        _discard(ks...); // <- recursive
    }

    template<typename QTypes>
    inline void system<QTypes>::discard_all() {
        do_discard_all();
    }

    template<typename QTypes>
    template<QTypes QType>
    inline void system<QTypes>::discard_all() {
        if (const auto prov = _get_provider_as<QType>()) {
            prov->discard_all();
        }
    }

    template<typename QTypes>
    template<typename Key>
    inline void system<QTypes>::discard_all() {
        discard_all<key_traits<QTypes, Key>::qtype>();
    }

    template<typename QTypes>
    template<QTypes QType>
    inline provider<QTypes, QType>* system<QTypes>::_get_provider_as() const noexcept {
        const auto ptr = get_provider(QType);
        return
            (ptr && ptr->is_qtype(QType))
            ? (provider<QTypes, QType>*)ptr
            : nullptr;
    }

    template<typename QTypes>
    template<typename Key>
    inline bool system<QTypes>::_exists_check(const Key& k) const noexcept {
        constexpr auto qtype = key_traits<QTypes, Key>::qtype;
        if (const auto prov = _get_provider_as<qtype>()) {
            return prov->exists(k);
        }
        return false;
    }

    template<typename QTypes>
    inline bool system<QTypes>::_exists() const noexcept {
        return true;
    }

    template<typename QTypes>
    template<typename Key, typename ...Keys>
    inline bool system<QTypes>::_exists(const Key& k, const Keys&... ks) const noexcept {
        return 
            _exists_check(k) && // <- short-circuit to skip rest of eval if check fails
            _exists(ks...); // <- recurse
    }

    template<typename QTypes>
    template<typename Key>
    inline void system<QTypes>::_discard_data(const Key& k) {
        if (const auto prov = _get_provider_as<key_traits<QTypes, Key>::qtype>()) {
            prov->discard(k);
        }
    }

    template<typename QTypes>
    inline void system<QTypes>::_discard() {
        // do nothing
    }

    template<typename QTypes>
    template<typename Key, typename ...Keys>
    inline void system<QTypes>::_discard(const Key& k, const Keys&... ks) {
        _discard_data(k);
        _discard(ks...); // <- recurse
    }
}

