

#pragma once


#include <optional>

#include "general.h"
#include "untyped_provider.h"


namespace yama::qs {


    // IMPORTANT:
    //      provider impls get their primary info from the external environment
    //      through the ctor and interface of the provider impl itself
    //
    //      provider impls are responsible for ensuring that they do not violate
    //      being 'ammendable but immutable' (though ammendability is optional)
    
    // IMPORTANT:
    //      provider impls get their secondary info via a query system which is
    //      injected into the provider
    //
    //      this query system should be the one the provider is itself a component
    //      of the impl of, to avoid problematic complexity
    //
    //      this query system should be injected via a mutable lvalue-reference
    //      injected into the ctor, and this should be the first parameter of the 
    //      ctor itself (for consistency)

    //
    //      providers are passed a query system injected into their ctor, w/ this
    //      system then being accessible to provider impl code via 'get_system'
    //
    //      provider impls are expected to forward an injected query system from
    //      their ctor to the provider ctor mentioned above
    //
    //      the query system returned by get_system is expected to be used by the
    //      provider impl to satisfy ALL of their secondary info needs
    //
    //      the query system returned by get_system is intended to be the query
    //      system inside which the provider impl resides as a subcomponent
    //
    //      if the above about the injected query system being the one inside 
    //      which the provider impl resides as a subcomponent is not satisfied,
    //      then any complexity which arises from this invariant's violation is
    //      undefined behaviour


    // this is the non-type erased provider type, through which provider
    // impls are derived


    template<key_type Key>
    class provider : public untyped_provider {
    public:

        using key_t = typename Key;
        using result_t = result<key_t>;


        inline bool is_qtype(qtype_t x) const noexcept override final {
            return x == qtype_of<key_t>();
        }


        // number returns the number of results the provider has cached

        virtual size_t number() const noexcept = 0;

        // exists returns if the provider has cached result data under k

        // exists is useful for scenarios where we want to check whether cached 
        // result data exists, but we don't want to actually copy the data

        virtual bool exists(const key_t& k) const noexcept = 0;

        // query queries a result under k, returning it, if any, and performs 
        // computation and (maybe) caching of new result data

        virtual std::optional<result_t> query(const key_t& k) = 0;

        // fetch queries a result under k, returning it, if any, but does not 
        // perform computation/caching of new result data

        virtual std::optional<result_t> fetch(const key_t& k) = 0;

        // discard discards the cached result under k, if any, failing quietly 
        // if there is no such result data

        // discard is simply a suggestion to the impl, and the impl is free to
        // decide whether cached result data should actually be discarded

        virtual void discard(const key_t& k) = 0;

        // discard_all discards all cached results

        // discard_all is simply a suggestion to the impl, and the impl is free
        // to decide whether cached result data should actually be discarded

        virtual void discard_all() = 0;
    };
}

