

#pragma once


#include <memory>

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

        // these methods define the interface of the provider

        // number returns the number of results the provider has cached

        // query queries a result under key, returning it, if any, and performs 
        // computation and (maybe) caching of new result data

        // fetch queries a result under key, returning it, if any, but does not 
        // perform computation/caching of new result data

        // discard discards the cached result under key, if any, failing quietly 
        // if there is no such result data

        // discard_all discards all cached results

        virtual size_t number() const noexcept = 0;
        virtual std::shared_ptr<result_t> query(const key_t& key) = 0;
        virtual std::shared_ptr<result_t> fetch(const key_t& key) = 0;
        virtual void discard(const key_t& key) = 0;
        virtual void discard_all() = 0;
    };
}

