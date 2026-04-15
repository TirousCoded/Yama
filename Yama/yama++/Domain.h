

#pragma once


#include <functional>

#include "Handle.h"


namespace ym {


    // TODO: All of the below haven't been unit tested.

    // A RAII handle wrapping a YmDm.
    class Domain final : public Handle<YmDm> {
    public:
        inline Domain() :
            Domain(Safe(ymDm_Create()), false) {
        }
        // Increments resource's ref count if secure == true.
        inline Domain(Safe<YmDm> resource, bool secure) noexcept :
            Handle(resource, secure) {
        }


        // path is expected to be null-terminated.
        inline bool bind(
            std::convertible_to<std::string_view> auto const& path,
            std::convertible_to<Safe<YmParcelDef>> auto const& parceldef) noexcept {
            return ymDm_BindParcelDef(
                get(),
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
                get(),
                std::string_view(subject).data(),
                std::string_view(before).data(),
                std::string_view(after).data()) == YM_TRUE;
        }

        // TODO: Figure out how to optimize forEachParcel.

        inline size_t forEachParcel(
            YmForEachParcelCallbackFn x,
            void* user) noexcept {
            ymAssert(x != nullptr);
            return ymDm_ForEachParcel(get(), x, user);
        }
        inline size_t forEachParcel(
            const std::function<void(YmDm*, void*, YmParcel*, size_t, size_t)>& x) noexcept {
            return forEachParcel([](
                YmDm* dm,
                void* user,
                YmParcel* parcel,
                size_t increment,
                size_t total) {
                    ymAssert(user != nullptr);
                    (*(std::function<void(YmDm*, void*, YmParcel*, size_t, size_t)>*)user)(dm, nullptr, parcel, increment, total);
                },
                (void*)&x);
        }
    };
}

