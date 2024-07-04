

#pragma once


#include <string>
#include <string_view>
#include <concepts>

#include "../core/general.h"


namespace yama::qs {


    // query providers are categorized by key type, and in order 
    // differentiate them at runtime, qtype_t values are used to give 
    // each key type a corresponding unique runtime value

    // qtype_t uses std::string_view for this, as this helps avoid the 
    // coupling that comes w/ using an enum, as that would centralize 
    // things, and force usage of things like templates

    // these values should always be initialized from string literals,
    // to ensure that their data is in static readonly memory

    using qtype_t = std::string_view;

    // use this for formatting, as std::string_view needs to be
    // converted to std::string anyway, as otherwise it won't
    // format correctly

    inline std::string fmt_qtype(qtype_t qtype) { return std::string(qtype); }


    // IMPORTANT:
    //      key type names should ALWAYS end w/ '_k'
    //
    //      provider qtype values should ALWAYS be the string name
    //      of the key type, but w/out the '_k' part

    template<typename T>
    concept key_type = 
        hashable_type<T> &&
        std::equality_comparable<T> &&
        requires
    {
        // - qtype is a static constexpr field defining the qtype of the key type
        // - qtype should be defined using a simple string literal
        // - again, qtype MUST be constexpr
        { T::qtype } noexcept -> std::convertible_to<qtype_t>;

        // - result defines the result which the key type corresponds to
        // - different key types may share the same result
        typename T::result;
    };


    // quality-of-life helpers

    // these helps enforce that T::qtype above MUST be constexpr

    template<key_type Key>
    constexpr qtype_t qtype_of() noexcept { return Key::qtype; }
    template<key_type Key>
    constexpr qtype_t qtype_of(const Key&) noexcept { return qtype_of<Key>(); }

    template<key_type Key>
    using result = typename Key::result;
}

