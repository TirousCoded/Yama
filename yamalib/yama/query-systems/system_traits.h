

#pragma once


#include <type_traits>
#include <concepts>


namespace yama::qs {


    // TODO: system_traits is here for *completeness*, but doesn't have much going for it at the moment...


    // trait type conformance concept

    template<typename T, typename QTypeEnum>
    concept system_traits_type =
        requires
    {
        // - an alias of the QTypeEnum template argument
        // - this defines the enum of different provider types for this query system
        // - query systems are defined/differentiated by their QTypeEnum enum type
        typename T::qtype_enum;
        requires std::is_same_v<typename T::qtype_enum, QTypeEnum>;
    };


    template<typename QTypeEnum>
    struct system_traits {};


    // quality-of-life helpers

    // NOTE: in Visual Studio, gotta static_assert the underlying concept to get concept error info

    template<typename QTypeEnum>
    constexpr bool system_traits_conforms = system_traits_type<system_traits<QTypeEnum>, QTypeEnum>;
}

