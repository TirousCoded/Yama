

#pragma once


#include "res.h"
#include "kind.h"
#include "qs.h"

#include "../query-systems/provider_traits.h"
#include "../query-systems/key_traits.h"


namespace yama {


    // query key

    struct type_data_k final {
        str fullname;   // the fullname of the type
    };
}


template<>
struct yama::qs::key_traits<yama::qtype, yama::type_data_k> final {
    using qtypes = yama::qtype;
    using key = yama::type_data_k;
    static constexpr auto qtype = yama::type_data_qt;
};


namespace yama {


    /*
        -- reference (symbol) tables --

            the behaviour of a type (especially a function-like type) is governed
            in part by said type's 'reference table', which provides an array of
            references to other types which the type may access in order to operate

            reference tables make it so that within their associated types, the
            types the table references can be identified by an index value

            these reference tables are generated from 'reference symbol tables',
            which get linked into reference tables during type instantation

            each reference symbol is a decorated fullname string which is used to
            query a corresponding type, according to the semantics of the system
            performing the instantiation
    */


    // type_info is a base struct used to derive *aggregate initializable*
    // structs which encapsulate pre-instantiation information about a type

    // these are meant to be really clean and nice to use, so as to be put in
    // the Yama API frontend for end-users thereof to use to define types

    struct type_info {
        str                 fullname;   // the fullname of the type
        std::vector<str>    refsyms;    // the reference symbol vector
    };


    // each derivative of type_info must define a static 'kind' method which
    // returns the kind of the type_info

    // this static method ensures that the kind of a type can be deduced at
    // compile-time from the type_info derivative's type, rather than having
    // to use a virtual method (which would prevent the quality-of-life that
    // comes w/ simple aggregate initialization), or having to have the end-user
    // specify the kind in a field (which could result in the system breaking
    // if the end-user makes a mistake + forcing them to write this reduces
    // how nice and clean the frontend API is)

    template<typename T>
    concept type_info_derivative =
        std::is_base_of_v<type_info, T> &&
        std::is_aggregate_v<T> &&
        requires
    {
        { T::kind() } noexcept -> std::same_as<kind>;
    };


    // kind_of helps force the above 'kind' static method to be constexpr,
    // as I couldn't find a way to make the concept require this

    template<type_info_derivative T>
    constexpr kind kind_of() noexcept {
        return T::kind();
    }

    template<type_info_derivative T>
    constexpr kind kind_of(const T&) noexcept {
        return kind_of<T>();
    }


    // type_data wraps type_info in a type-erased object which encapsulates 
    // a handle to it alongside storing runtime information about its kind of type,
    // acting as a *carrier* for this information

    class type_data final {
    public:

        // ctor for init via injecting the info for it

        template<type_info_derivative T>
        inline type_data(T info) noexcept;

        type_data() = delete;
        type_data(const type_data&) = default;
        type_data(type_data&& other) noexcept;

        ~type_data() noexcept = default;

        type_data& operator=(const type_data&) = default;
        type_data& operator=(type_data&& other) noexcept;


        // fullname returns the fullname of the type encapsulated

        str fullname() const noexcept;

        // refsyms returns the reference symbol table of the type encapsulated

        std::span<const str> refsyms() const noexcept;


        // kind returns the kind of type encapsulated

        kind kind() const noexcept;

        // info returns a read-only view of the info as T

        // behaviour is undefined if T is not the correct info type

        template<type_info_derivative T>
        inline const T& info() const noexcept;


    private:

        res<type_info> _info;
        yama::kind _kind;
    };


    template<type_info_derivative T>
    inline type_data::type_data(T info) noexcept 
        : _info(make_res<T>(std::move(info))), 
        _kind(kind_of<T>()) {}

    template<type_info_derivative T>
    inline const T& type_data::info() const noexcept {
        YAMA_ASSERT(this->kind() == kind_of<T>());
        return *(const T*)_info.get();
    }
}


template<>
struct yama::qs::provider_traits<yama::qtype, yama::type_data_qt> final {
    using qtypes = yama::qtype;
    static constexpr auto qtype = yama::type_data_qt;
    using key = yama::type_data_k;
    using result = yama::type_data;
};

