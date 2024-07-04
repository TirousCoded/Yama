

#pragma once


#include "general.h"


namespace yama::qs {


    // this is an opaque base class type all provider types within a query system
    // may be aliased as such that query systems layers may interact w/ providers
    // w/out having to worry about what type they are

    // this base class is to extend yama::provider, and is not to be inherited
    // by any other class

    class untyped_provider {
    public:
        
        virtual ~untyped_provider() noexcept = default;


        // is_qtype returns if the provider's qtype is x

        // this is meant to be overrided only by yama::provider

        virtual bool is_qtype(qtype_t x) const noexcept = 0;

        template<key_type Key>
        inline bool is_qtype() const noexcept {
            return is_qtype(qtype_of<Key>());
        }
    };
}

