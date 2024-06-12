

#pragma once


//


namespace yama::qs {


    // this is an opaque base class type all provider types within a query system
    // may be aliased as such that query systems layers may interact w/ providers
    // w/out having to worry about what type they are

    // this base class is to extend yama::provider, and is not to be inherited
    // by any other class

    template<typename QTypes>
    class untyped_provider {
    public:
        
        virtual ~untyped_provider() noexcept = default;


        // is_qtype returns if the provider's query type is x

        // this is meant to be overrided only by yama::provider

        virtual bool is_qtype(QTypes x) const noexcept = 0;
    };
}

