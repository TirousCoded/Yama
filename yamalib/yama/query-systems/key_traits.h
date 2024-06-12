

#pragma once


//


namespace yama::qs {


    // specializations require:
    //      using qtypes = ...;
    //          * an alias of the QTypes template argument
    //          * this defines which query system the key traits belong too
    //          * query systems are defined/differentiated by their QTypes enum type
    //      using key = ...;
    //          * an alias of the Key template argument
    //      static constexpr qtypes qtype = ...;
    //          * a constant equal to the query provider type this key is associated w/
    //          * each key type must correspond to only a single provider type within a query system
    //          * the specialization of provider_traits for qtype must agree on the key/qtype correspondence

    template<typename QTypes, typename Key>
    struct key_traits {};


    // quality-of-life helpers

    template<typename QTypes, typename Key>
    constexpr auto qtype_of() noexcept {
        return key_traits<QTypes, Key>::qtype;
    }

    template<typename QTypes, typename Key>
    constexpr auto qtype_of(const Key&) noexcept {
        return qtype_of<QTypes, Key>();
    }
}

