

#pragma once


#include "../core/api_component.h"

#include "provider_traits.h"
#include "key_traits.h"
#include "untyped_provider.h"
#include "provider.h"


namespace yama::qs {


    // IMPORTANT:
    //      when writing a new query system impl, write unit tests for the following:
    //          1) basic behaviour of methods is as expected
    //          2) ensure that coherence is maintained if the impl has a notion of 
    //             an upstream system


    /*
        This write-up describes Yama's query system's underlying theory.

        -- Basics --

            The query system is the pull-based system by which Yama loads resources.

            The query system operates via the defining of 'systems', which things like
            contexts, domains, compilers, etc. all are. System resources are 'queried', 
            specifying the resource to load via a 'key'.

            Systems load in resources by computing them, and then caching their resources 
            for future queries. Systems encapsulate two kinds of information: 'primary' 
            and 'secondary'. This information, via immutability and referential transparency, 
            defines a 'resource set' of resources which not only may be loaded, but which all 
            conceptually 'exist' within the system at all times, due to the ones not in cache 
            still algebraically existing due to them being recomputable.

            These semantics allow for system impls, and the end-users thereof, to selectively
            decide what resources should/shouldn't be loaded at any given time, w/out having
            to worry about those resources becoming unavailable.

            Systems may be 'composed' w/ one another, creating 'system compositions', or just
            'compositions'. System compositions define sets of systems arranged into directed
            acyclic graphs of systems having upstream/downstream relationships. Compositions
            are governed by a concept known as 'coherence', which must be maintained at all
            times across the composition.

        -- Providers --

            Under the hood, for each unique key type, their is a 'provider' type, which 
            defines the unit responsible for loading via that particular key.
            
            Systems encapsulate sets of providers, this in part defining the system impl.
            
            Providers load resources via referentially transparent functions. These functions
            may be parameterized only by the larger system's primary/secondary information.

            Systems define three sets of providers: a 'provider set', a 'local provider set',
            and an 'upstream provider set'.

            The provider set is the union of the local provider set and the upstream provider
            set, and defines the providers which define the system's frontend behaviour, as
            well as the providers from which local providers thereof derive their secondary 
            information from.

            The local provider set is the set of providers which are directly a part of the
            impl of the system in question, rather than being gotten from upstream.

            The upstream provider set is the set of providers which are gotten from an upstream
            query system. When the local provider set does not provide a particular type of
            provider, one is gotten from upstream to fill its role.
            
            The above provider set semantics are augmented by 'transparent providers'.

            Transparent providers are providers which do not perform any computation.
            
            Transparent providers are used for a number of different things:
                1) being 'null providers' which do nothing;
                2) caching upstream query results locally, but not performing computation
                   for queries which fail upstream; or
                3) other tasks like the collecting of diagnostic information.

            Transparent providers augment provider set semantics in that their transparency
            results in them being specially excluded from the provider set, the local
            provider set, and the upstream provider set, meaning that transparent providers
            are effectively not acknowledged by the conceptual framework of the query system.

        -- Primary Information --

            Primary information is information added directly to the system from the
            external environment.

            Primary information is 'ammendable but immutable', meaning new information
            can be added, but cannot be removed, except if the entire system is reset.

            The adding of new primary information may add new resources to a system's
            resource set, but may not remove or change existing resources.

            Primary information defines an extendable, but otherwise constant source of 
            information for query providers.

        -- Secondary Information --
            
            Secondary information is the information encapsulated by providers, which is 
            a function ONLY of other primary/secondary information within the system.

            Secondary information is the cached result data of providers, which due to the
            referentially transparent nature of secondary information, and the effective
            constancy of primary information it may rely on (directly or indirectly) means
            that secondary information may be liberally deleted w/out compromising the
            availability of resources within a system, just requiring them to be recomputed.

        -- Coherence --

            Compositions of systems are at all times required to maintain 'coherence', 
            which is the primary way of establishing the validity of the composition.

            Coherence requires that for a given system in the composition, that its resource
            set is at all times a superset of the resource set of the system upstream of it.

            Coherence also requires that each unique key which queries successfully be
            uniquely mapped such that what resource a key refers to is never ambiguous.

            These rules help ensure that the logic of how the composition works is not
            needlessly hard to reason about, and in such a fashion that the various systems
            of the composition play nicely w/ one another.

            An example of how non-coherence could cause issues is if a downstream key/resource
            mapping *shadowed* an upstream one, and a scenario arose where some other upstream
            resource which wasn't shadowed had a dependence on the one that was, meaning that
            the shadowed resource was exposed downstream, w/ this creating ambiguity and 
            general complexity that could cause the composition's behaviour to be hard to
            reason about, and potentially leading to hard to diagnose bugs.

        -- Provider Nonintersection --

            The principle of 'provider nonintersection' is a principle for the design of Yama 
            query systems that can be exploited to make it easier to design system compositions 
            which properly maintain coherence.

            The principle is that given two systems, one downstream of the other, that the
            set-theoretic intersection of the local provider sets of the two systems should
            be the empty set. In other words, the two should have no local providers in
            common, w/ regards to provider key types.

            Ensuring provider nonintersection ensures that adding new primary information to the 
            upstream system cannot result in the upstream system's resource set intersecting the 
            resource set of the system downstream to it.

            Ensuring this resource set nonintersection is the key to maintaining coherence,
            as the way coherence is broken is when a upstream/downstream system pair have 
            resource set elements in common, w/ regards to their query keys.

        -- Committing --

            Committing is a technique which allows for the primary information of secondary
            information to be selectively deleted from a query system composition w/out losing
            the secondary information in question, and while maintaining coherence, and more
            broadly not compromising the invariants of the query system.

            Committing involves extracting secondary information from a downstream system, then
            resetting said system, and then adding this extracted information as new primary
            information to an upstream system, such that the previously downstream resources
            are moved upstream, and are decoupled from the information used to compute them.

            A good example of when this technique is useful is how Yama performs compilation.
            
            In Yama, 'domains' and their 'compilers' are query systems, forming a composition
            where the compiler is downstream to the domain. When compilation occurs, the compiler
            will pull type information from upstream, and then mix it w/ type information 
            computed from compiling source code, which is compiler primary information, to
            generate new type information.

            After compilation, we can 'commit' the compiled type information, moving it upstream
            into the domain, while deleting now unneeded information about the source code used
            to compile said type information, and whatever unneeded secondary information the 
            compiler might be carrying.
    */


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
        // what data should/shouldn't be deleted

        template<typename... Keys>
        inline void discard(const Keys&... ks);

        // discard_all discards all cached results from all local providers

        // discard_all is impl dependent w/ regards to whether or not it
        // also discards from all upstream providers in the system's provider
        // set, and whether the discard_all call should propagate upstream
        // more broadly

        // discard_all calls are just suggestions, and the impl is free to 
        // decide what data should/shouldn't be deleted

        inline void discard_all();

        // these discard_all overloads discard the cached data of only a
        // single provider, specified by QType

        // these discard_all overloads are impl dependent w/ regards to whether
        // they result in the call being propagated upstream

        // these discard_all overloads fail quietly if no provider could 
        // be found

        template<QTypes QType>
        inline void discard_all();

        // this discard_all overload deduces the QType to use from Key

        template<typename Key>
        inline void discard_all();

        // reset discards all cached results from all providers, in addition
        // to completely clearing all primary information from the system

        // reset calls do not propagate upstream

        // reset calls are impl dependent w/ regards to whether they result
        // in the discarding of upstream secondary information

        inline void reset();


    protected:

        // the main job of a query system impl is to define a get_provider override
        // which maps a given qtype to a specific query provider, if any

        // it's VARY IMPORTANT, that the query provider, or lack thereof, which a
        // given qtype argument maps to does not change during the lifetime of the
        // query system, they must remain constant

        virtual untyped_provider<QTypes>* get_provider(QTypes qtype) const noexcept = 0;

        // this defines the query system's behaviour in regards to the discard_all
        // overload w/out any parameters, which discards all secondary information
        // from all local providers

        virtual void do_discard_all() = 0;

        // this defines the query system's behaviour in regards to the reset method

        virtual void do_reset() = 0;


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
    inline void system<QTypes>::reset() {
        do_reset();
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

