

#pragma once


//


namespace yama::qs {


    // specializations require:
    //      using qtypes = ...;
    //          * an alias of the QTypes template argument
    //          * this defines which query system the provider traits belong too
    //          * query systems are defined/differentiated by their QTypes enum type
    //      static constexpr qtypes qtype = ...;
    //          * a constant equal to the QType template argument
    //          * this defines the type of provider
    //      using key = ...;
    //          * the type of key associated w/ this type of provider
    //          * each key type must be associated w/ no more than one provider type,
    //            as the provider type must be able to be deduced from the key type
    //      using result = ...;
    //          * the type of result associated w/ this type of provider

    template<typename QTypes, QTypes QType>
    struct provider_traits {};


    // quality-of-life helpers

    template<typename QTypes, QTypes QType>
    using key = provider_traits<QTypes, QType>::key;

    template<typename QTypes, QTypes QType>
    using result = provider_traits<QTypes, QType>::result;
}

