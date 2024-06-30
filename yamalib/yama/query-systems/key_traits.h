

#pragma once


#include <type_traits>
#include <concepts>


namespace yama::qs {


    // trait type conformance concept

    template<typename T, typename QTypeEnum, typename Key>
    concept key_traits_type =
        requires
    {
        // - an alias of the QTypeEnum template argument
        // - this defines which query system the key traits belong too
        // - query systems are defined/differentiated by their QTypeEnum enum type
        typename T::qtype_enum;
        requires std::is_same_v<typename T::qtype_enum, QTypeEnum>;
        
        // - an alias of the Key template argument
        typename T::key;
        requires std::is_same_v<typename T::key, Key>;

        // - a constant equal to the query provider type this key is associated w/
        // - each key type must correspond to only a single provider type within a query system
        // - the specialization of provider_traits for qtype must agree on the key/qtype correspondence
        // - this MUST be constexpr (can't check that w/ concept though)
        { T::qtype } noexcept -> std::convertible_to<typename T::qtype_enum>;
    };


    template<typename QTypeEnum, typename Key>
    struct key_traits {};


    // quality-of-life helpers

    // NOTE: in Visual Studio, gotta static_assert the underlying concept to get concept error info

    template<typename QTypeEnum, typename Key>
    constexpr bool key_traits_conforms = key_traits_type<key_traits<QTypeEnum, Key>, QTypeEnum, Key>;

    // NOTE: these also help assert that *::qtype MUST be constexpr

    template<typename QTypeEnum, typename Key>
    constexpr auto qtype_of() noexcept {
        return key_traits<QTypeEnum, Key>::qtype;
    }

    template<typename QTypeEnum, typename Key>
    constexpr auto qtype_of(const Key&) noexcept {
        return qtype_of<QTypeEnum, Key>();
    }
}

