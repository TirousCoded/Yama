

#pragma once


#include "../core/general.h"
#include "../core/type_data.h"

#include "ha_struct.h"


namespace yama {


    class type;
    template<typename Allocator>
    class type_instance;


    namespace internal {


        struct type_mem_header final {
            str fullname; // the fullname of this type
            type_data data; // the underlying type_data this type is based on
            size_t links; // the number of links in the link table

            // storing the kind here for fast RTTI access if we need it

            kind kind; // the kind of type this is

            // using this stubs counter we can test for completeness w/out having to
            // iterate over the link table at any point, as we'll incrementally
            // reduce stubs for each link put onto a stub

            size_t stubs; // the number of stubs outstanding


            inline size_t length() const noexcept { return links; }
        };


        // the ha_struct_anon_ref element type is for the link table

        // ha_struct_anon_ref has a nullptr value, so I decided not to wrap in std::optional

        // originally I tried using yama::type, but that caused too many problems...

        using type_mem = internal::ha_struct<type_mem_header, internal::ha_struct_anon_ref>;
    }
}

