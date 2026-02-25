

#pragma once


#include "Handle.h"


namespace ym {


    // TODO: All of the below haven't been unit tested.

    // A RAII handle wrapping a YmDm.
    class Domain final : public Handle<YmDm> {
    public:
        inline Domain() :
            Domain(Safe(ymDm_Create())) {
        }
        // Does not increment the ref count of resource.
        inline explicit Domain(Safe<YmDm> resource) noexcept :
            Handle(resource) {
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
        // subject is expected to be null-terminated.
        // before is expected to be null-terminated.
        // after is expected to be null-terminated.
        inline bool addRedirect(
            std::convertible_to<std::string_view> auto const& subject,
            std::convertible_to<std::string_view> auto const& before,
            std::convertible_to<std::string_view> auto const& after) noexcept {
            return ymDm_AddRedirect(
                *this,
                std::string_view(subject).data(),
                std::string_view(before).data(),
                std::string_view(after).data()) == YM_TRUE;
        }
    };
}

