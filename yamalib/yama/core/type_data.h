

#pragma once


#include <atomic>

#include "res.h"
#include "kind.h"
#include "linksym.h"
#include "callsig_info.h"
#include "type_info.h"


namespace yama {


    // IMPORTANT:
    //      this code is provided to allow for the end-user to define their own
    //      types, which they can then push to a yama::domain
    //
    //      this is part of the frontend, but being technical and niche, should
    //      avoid being liberally exposed on the interfaces of things like 
    //      yama::type, yama::callsig, etc.


    namespace dm {
        class static_verifier;
    }


    // type_data wraps type_info in a type-erased object which encapsulates 
    // a handle to it alongside storing runtime information about its kind of type,
    // acting as a *carrier* for this information

    class type_data final {
    public:

        friend class yama::dm::static_verifier;


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


        // verified returns if the type_data has been statically verified

        bool verified() const noexcept;


    private:

        // NOTE: _other_state exists so modifying verified is reflected across
        //       all type_data to the same underlying type_info
        
        // NOTE: keep _kind out of _other_state to minimize memory indirection,
        //       as we want yama::type to be able to access it liberally

        struct _other_state final {
            // NOTE: using an atomic_bool here as we do want to impl multithreading
            //       stuff WAY later on in this project, so I decided to make this
            //       atomic now so I need-not worry about forgetting to later

            std::atomic_bool verified = false;
        };


        res<type_info> _info;
        res<_other_state> _other;
        yama::kind _kind;
    };


    template<type_info_derived_type T>
    inline type_data::type_data(T info) noexcept 
        : _info(make_res<T>(std::move(info))),
        _other(make_res<_other_state>()),
        _kind(kind_of<T>()) {}

    template<type_info_derived_type T>
    inline const T& type_data::info() const noexcept {
        YAMA_ASSERT(this->kind() == kind_of<T>());
        return *(const T*)_info.get();
    }
}

