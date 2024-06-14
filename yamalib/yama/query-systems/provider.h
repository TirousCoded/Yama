

#pragma once


#include <optional>

#include "untyped_provider.h"


namespace yama::qs {


    // this is the non-type erased type through which providers are interacted w/

    // this is the base class of provider impls, however, it should not be used
    // directly to derive provider impls, instead letting yama::qa::provider_impl
    // handle that responsibility

    template<typename QTypes, QTypes QType>
    class provider : public untyped_provider<QTypes> {
    public:

        using key_t = key<QTypes, QType>;
        using result_t = result<QTypes, QType>;


        inline bool is_qtype(QTypes x) const noexcept override final {
            return x == QType;
        }


        // IMPORTANT:
        //      update yama::provider_impl whenever we ammend the below

        // IMPORTANT:
        //      the reason std::nullopt is used instead of std::shared_ptr is to not
        //      force query providers to have to heap allocate data in scenarios where
        //      heap allocating is vary suboptimal to have to do

        // IMPORTANT:
        //      the exists method is useful for scenarios where we want to check whether
        //      cached result data exists, but we don't want to actually copy the data

        // these methods define the interface of the provider

        // number returns the number of results the provider has cached

        // exists returns if the provider has cached result data under k

        // query queries a result under k, returning it, if any, and performs 
        // computation and (maybe) caching of new result data

        // fetch queries a result under k, returning it, if any, but does not 
        // perform computation/caching of new result data

        // discard discards the cached result under k, if any, failing quietly 
        // if there is no such result data

        // discard is simply a suggestion to the impl, and the impl is free to
        // decide whether cached result data should actually be discarded

        // discard_all discards all cached results

        // discard_all is simply a suggestion to the impl, and the impl is free
        // to decide whether cached result data should actually be discarded

        virtual size_t number() const noexcept = 0;
        virtual bool exists(const key_t& k) const noexcept = 0;
        virtual std::optional<result_t> query(const key_t& k) = 0;
        virtual std::optional<result_t> fetch(const key_t& k) = 0;
        virtual void discard(const key_t& k) = 0;
        virtual void discard_all() = 0;
    };
}

