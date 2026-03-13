

#pragma once


#include <optional>

#include "Handle.h"
#include "Parcel.h"
#include "Type.h"
#include "Object.h"


namespace ym {


    // TODO: All of the below haven't been unit tested.

    // A RAII handle wrapping a YmCtx.
    class Context final : public Handle<YmCtx> {
    public:
        inline Context(std::convertible_to<Safe<YmDm>> auto const& dm) :
            Context(Safe(ymCtx_Create(Safe<YmDm>(dm)))) {
        }
        // Does not increment the ref count of resource.
        inline explicit Context(Safe<YmCtx> resource) noexcept :
            Handle(resource) {
        }


        inline Safe<YmDm> domain() const noexcept {
            return Safe(ymCtx_Dm(get()));
        }

        // path is expected to be null-terminated.
        inline std::optional<Parcel> import(
            std::convertible_to<std::string_view> auto const& path) noexcept {
            if (auto result = ymCtx_Import(get(), std::string_view(path).data())) {
                return Parcel(Safe(result));
            }
            return std::nullopt;
        }
        // fullname is expected to be null-terminated.
        inline std::optional<Type> load(
            std::convertible_to<std::string_view> auto const& fullname) noexcept {
            if (auto result = ymCtx_Load(get(), std::string_view(fullname).data())) {
                return Type(Safe(result));
            }
            return std::nullopt;
        }

        inline Type ldNone() noexcept { return Type(Safe(ymCtx_LdNone(get()))); }
        inline Type ldInt() noexcept { return Type(Safe(ymCtx_LdInt(get()))); }
        inline Type ldUInt() noexcept { return Type(Safe(ymCtx_LdUInt(get()))); }
        inline Type ldFloat() noexcept { return Type(Safe(ymCtx_LdFloat(get()))); }
        inline Type ldBool() noexcept { return Type(Safe(ymCtx_LdBool(get()))); }
        inline Type ldRune() noexcept { return Type(Safe(ymCtx_LdRune(get()))); }

        inline void naturalize(std::convertible_to<Safe<YmParcel>> auto const& x) noexcept {
            ymCtx_NaturalizeParcel(*this, Safe<YmParcel>(x));
        }
        inline void naturalize(std::convertible_to<Safe<YmType>> auto const& x) noexcept {
            ymCtx_NaturalizeType(*this, Safe<YmType>(x));
        }

        inline Object newNone() noexcept { return Object(Safe(ymCtx_NewNone(get()))); }
        inline Object newInt(YmInt v) noexcept { return Object(Safe(ymCtx_NewInt(get(), v))); }
        inline Object newUInt(YmUInt v) noexcept { return Object(Safe(ymCtx_NewUInt(get(), v))); }
        inline Object newFloat(YmFloat v) noexcept { return Object(Safe(ymCtx_NewFloat(get(), v))); }
        inline Object newBool(YmBool v) noexcept { return Object(Safe(ymCtx_NewBool(get(), v))); }
        inline Object newRune(YmRune v) noexcept { return Object(Safe(ymCtx_NewRune(get(), v))); }
    };
}

