

#pragma once


#include <type_traits>
#include <concepts>


namespace yama::qs {


    // trait type conformance concept

    template<typename T, typename QTypeEnum, QTypeEnum QType>
    concept provider_traits_type =
        requires
    {
        // - an alias of the QTypeEnum template argument
        // - this defines which query system the provider traits belong too
        // - query systems are defined/differentiated by their QTypeEnum enum type
        typename T::qtype_enum;
        requires std::is_same_v<typename T::qtype_enum, QTypeEnum>;

        // - a constant equal to the QType template argument
        // - this defines the type of provider
        // - this MUST be constexpr (can't check that w/ concept though)
        { T::qtype } noexcept -> std::convertible_to<typename T::qtype_enum>;
        
        // - the type of key associated w/ this type of provider
        // - each key type must be associated w/ no more than one provider type,
        //   as the provider type must be able to be deduced from the key type
        typename T::key;
        
        // - the type of result associated w/ this type of provider
        typename T::result;
    };


    template<typename QTypeEnum, QTypeEnum QType>
    struct provider_traits {};


    // quality-of-life helpers

    // NOTE: in Visual Studio, gotta static_assert the underlying concept to get concept error info

    template<typename QTypeEnum, QTypeEnum QType>
    constexpr bool provider_traits_conforms = provider_traits_type<provider_traits<QTypeEnum, QType>, QTypeEnum, QType>;

    template<typename QTypeEnum, QTypeEnum QType>
    using key = provider_traits<QTypeEnum, QType>::key;

    template<typename QTypeEnum, QTypeEnum QType>
    using result = provider_traits<QTypeEnum, QType>::result;
}

