

#pragma once


#include <optional>
#include <vector>

#include "kind.h"
#include "ptype.h"
#include "call_fn.h"
#include "linksym.h"
#include "callsig_info.h"


namespace yama {


    // type_info is a base struct used to derive *aggregate initializable*
    // structs which encapsulate pre-instantiation information about a type

    // these are meant to be really clean and nice to use, so as to be put in
    // the Yama API frontend for end-users thereof to use to define types

    struct type_info {
        str                             fullname;   // the fullname of the type
        std::optional<callsig_info>     callsig;    // the call signature of the type, if any
        std::vector<linksym>            linksyms;   // the link symbol vector
    };


    template<typename T>
    concept type_info_derived_type =
        std::is_base_of_v<type_info, T>&&
        std::is_aggregate_v<T>&&
        requires
    {
        // each derivative of type_info must define a static 'kind' method which
        // returns the kind of the type_info

        // this static method ensures that the kind of a type can be deduced at
        // compile-time from the type_info derivative's type, rather than having
        // to use a virtual method (which would prevent the quality-of-life that
        // comes w/ simple aggregate initialization), or having to have the end-user
        // specify the kind in a field (which could result in the system breaking
        // if the end-user makes a mistake + forcing them to write this reduces
        // how nice and clean the frontend API is)
        { T::kind() } noexcept -> std::convertible_to<kind>;
    };


    // kind_of helps force the above methods to be constexpr, as I couldn't find
    // a way to make the concept require this

    template<type_info_derived_type T>
    constexpr kind kind_of() noexcept {
        return T::kind();
    }
    template<type_info_derived_type T>
    constexpr kind kind_of(const T&) noexcept {
        return kind_of<T>();
    }


    // below defines the impls of type_info

    static_assert(kinds == 2);

    // IMPORTANT:
    //      as we update the below, be sure to update type_data-tests.cpp also
    //      (specifically the tests at the vary bottom under the static_assert)


    struct primitive_info final : public type_info {
        ptype ptype;   // the type of primitive value objects of this type encapsulate


        static constexpr auto kind() noexcept { return kind::primitive; }
    };

    static_assert(type_info_derived_type<primitive_info>);


    struct function_info final : public type_info {
        call_fn cf; // the call_fn encapsulating function call behaviour
        size_t objs; // the number of objects in the call frame of this function


        static constexpr auto kind() noexcept { return kind::function; }
    };

    static_assert(type_info_derived_type<function_info>);
}

