

#pragma once


#include "provider_traits.h"
#include "provider.h"
#include "system.h"


namespace yama::qs {


    // IMPORTANT:
    //      query provider impls are restricted in their view of the external environment
    //      to only having access to a 'primary' and 'secondary' information source
    //
    //      the primary information source is a pointer to a struct of some type from 
    //      which the impl may derive information from outside the query system
    //
    //      the primary source struct is expected to be used such that the invariant of
    //      the external information provided to the provider is 'ammendable but immutable'
    //      is not violated, w/ behaviour being undefined if it is
    //
    //      the secondary information source is an entire query system from which the
    //      query provider may then employ information which is a function of other
    //      query providers elsewhere
    //
    //      the secondary source query system must be the the same query system which
    //      the provider is a part of the impl of


    // when a provider has no primary information source, it's suppose to use this
    // yama::qs::no_source_t unit type as its primary_source type

    struct no_source_t final {};

    // then, when initializing a provider object, a lvalue-reference to this 
    // yama::qs::no_source object should be passed to the constructor

    // the use of 'inline' here guarantees that there'll be only a single instance
    // of this global variable across all translation units
    // (see https://www.youtube.com/watch?v=rQhBECyA6ew for details)

    inline no_source_t no_source = no_source_t{};


    // this is the helper used to actually define provider impls via
    // use of provider_impl paired w/ a policy object type

    // provider_impl should only be used to define provider impls, not
    // to otherwise interact w/ instances of them

    // the policy object type requires:
    //      using primary_source = ...;
    //          * defines the 'primary source' from which the provider derives information
    //          * should be the yama::qs::no_source_t unit type if no primary should is defined
    //      [NAME](primary_source& primary_src, yama::qs::system<...>& secondary_src)
    //          * this is a constructor
    //          * the impl is required to define a constructor which accepts a primary
    //            and secondary source object
    //          * this constructor gets wrapped in the constructor of the yama::provider_impl
    //            constructor w/ the same parameters
    //          * the secondary_src must be an lvalue of the query system object the provider
    //            is a part of
    //      size_t number() const noexcept;
    //      bool exists(const yama::qs::key<...>& k) const noexcept;
    //      std::optional<yama::qs::result<...>> query(const yama::qs::key<...>& key);
    //      std::optional<yama::qs::result<...>> fetch(const yama::qs::key<...>& key);
    //      void discard(const yama::qs::key<...>& key);
    //      void discard_all();
    //          * these define the behaviour of the provider's interface of methods
    //          * these are the same as those found in yama::provider's interface,
    //            and they are simply wrapped in overrides of those

    template<typename QTypes, QTypes QType, typename Policy>
    class provider_impl final : public provider<QTypes, QType> {
    public:

        using primary_source_t = Policy::primary_source;
        
        using system_t = system<QTypes>;

        using key_t = key<QTypes, QType>;
        using result_t = result<QTypes, QType>;


        // behaviour is undefined if secondary_src is not an lvalue of the 
        // query system object which this provider is part of

        inline provider_impl(primary_source_t& primary_src, system_t& secondary_src)
            : _policy(primary_src, secondary_src) {}


        inline size_t number() const noexcept override final {
            return _policy.number();
        }

        inline bool exists(const key_t& k) const noexcept override final {
            return _policy.exists(k);
        }

        inline std::optional<result_t> query(const key_t& k) override final {
            return _policy.query(k);
        }
        
        inline std::optional<result_t> fetch(const key_t& k) override final {
            return _policy.fetch(k);
        }
        
        inline void discard(const key_t& k) override final {
            _policy.discard(k);
        }
        
        inline void discard_all() override final {
            _policy.discard_all();
        }


    private:

        Policy _policy;
    };
}

