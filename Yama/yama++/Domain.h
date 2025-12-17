

#pragma once


#include "Handle.h"


namespace ym {


    // TODO: All of the below haven't been unit tested.

    // A RAII handle wrapping a YmDm.
    class Domain final : public Handle<YmDm> {
    public:
        // Initializes a handle which takes RAII ownership of a newly initialized resource.
        // Only one handle should have ownership of a resource at a time.
        inline Domain() :
            Domain(Safe(ymDm_Create())) {
        }

        // Initializes a handle which takes RAII ownership of x.
        // Only one handle should have ownership of a resource at a time.
        inline explicit Domain(Safe<YmDm> x) noexcept :
            Handle(x) {
        }


        // path is expected to be null-terminated.
        inline bool bind(
            std::convertible_to<std::string_view> auto const& path,
            std::convertible_to<Safe<YmParcelDef>> auto const& parceldef) noexcept {
            return ymDm_BindParcelDef(
                *this,
                std::string_view(path).data(),
                Safe<YmParcelDef>(parceldef)) == YM_TRUE;
        }
    };
}

