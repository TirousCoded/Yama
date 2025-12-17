

#pragma once


#include <optional>

#include "Handle.h"
#include "Parcel.h"
#include "Item.h"


namespace ym {


    // TODO: All of the below haven't been unit tested.

    // A RAII handle wrapping a YmCtx.
    class Context final : public Handle<YmCtx> {
    public:
        // Initializes a handle which takes RAII ownership of a newly initialized resource.
        // Only one handle should have ownership of a resource at a time.
        inline Context(std::convertible_to<Safe<YmDm>> auto const& dm) :
            Context(Safe(ymCtx_Create(Safe<YmDm>(dm)))) {
        }

        // Initializes a handle which takes RAII ownership of x.
        // Only one handle should have ownership of a resource at a time.
        inline explicit Context(Safe<YmCtx> x) noexcept :
            Handle(x) {
        }


        inline Safe<YmDm> domain() const noexcept {
            return Safe(ymCtx_Dm(*this));
        }

        // path is expected to be null-terminated.
        inline std::optional<Parcel> import(
            std::convertible_to<std::string_view> auto const& path) noexcept {
            if (auto result = ymCtx_Import(*this, std::string_view(path).data())) {
                return Parcel(Safe(result));
            }
            return std::nullopt;
        }

        // fullname is expected to be null-terminated.
        inline std::optional<Item> load(
            std::convertible_to<std::string_view> auto const& fullname) noexcept {
            if (auto result = ymCtx_Load(*this, std::string_view(fullname).data())) {
                return Item(Safe(result));
            }
            return std::nullopt;
        }

        inline void naturalize(std::convertible_to<Safe<YmParcel>> auto const& x) noexcept {
            ymCtx_NaturalizeParcel(*this, Safe<YmParcel>(x));
        }
        inline void naturalize(std::convertible_to<Safe<YmItem>> auto const& x) noexcept {
            ymCtx_NaturalizeItem(*this, Safe<YmItem>(x));
        }
    };
}

