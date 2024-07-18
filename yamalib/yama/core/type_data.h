

#pragma once


#include "res.h"
#include "kind.h"
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
        std::is_base_of_v<type_info, T> &&
        std::is_aggregate_v<T> &&
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

    // type_data wraps type_info in a type-erased object which encapsulates 
    // a handle to it alongside storing runtime information about its kind of type,
    // acting as a *carrier* for this information

    class type_data final {
    public:

        // ctor for init via injecting the info for it

        template<type_info_derived_type T>
        inline type_data(T info) noexcept;

        type_data() = delete;
        type_data(const type_data&) = default;
        type_data(type_data&&) noexcept = default;

        ~type_data() noexcept = default;

        type_data& operator=(const type_data&) = default;
        type_data& operator=(type_data&&) noexcept = default;


        // fullname returns the fullname of the type encapsulated

        str fullname() const noexcept;

        // linksyms returns the link symbol table of the type encapsulated

        std::span<const linksym> linksyms() const noexcept;

        // NOTE: making this return an lvalue to avoid costly callsig_info copies

        // callsig returns the call signature of the type encapsulated, if any

        const std::optional<callsig_info>& callsig() const noexcept;


        // kind returns the kind of type encapsulated

        kind kind() const noexcept;

        // info returns a read-only view of the info as T

        // behaviour is undefined if T is not the correct info type

        template<type_info_derived_type T>
        inline const T& info() const noexcept;


    private:

        res<type_info> _info;
        yama::kind _kind;
    };


    template<type_info_derived_type T>
    inline type_data::type_data(T info) noexcept 
        : _info(make_res<T>(std::move(info))), 
        _kind(kind_of<T>()) {}

    template<type_info_derived_type T>
    inline const T& type_data::info() const noexcept {
        YAMA_ASSERT(this->kind() == kind_of<T>());
        return *(const T*)_info.get();
    }
}

