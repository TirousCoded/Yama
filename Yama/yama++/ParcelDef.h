

#pragma once


#include "Handle.h"


namespace ym {


    // TODO: All of the below haven't been unit tested.

    // A RAII handle wrapping a YmParcelDef.
    class ParcelDef final : public Handle<YmParcelDef> {
    public:
        // Initializes a handle which takes RAII ownership of a newly initialized resource.
        // Only one handle should have ownership of a resource at a time.
        inline ParcelDef() :
            ParcelDef(Safe(ymParcelDef_Create())) {
        }

        // Initializes a handle which takes RAII ownership of x.
        // Only one handle should have ownership of a resource at a time.
        inline explicit ParcelDef(Safe<YmParcelDef> x) noexcept :
            Handle(x) {
        }


        inline std::optional<YmItemIndex> addStruct(
            std::convertible_to<std::string_view> auto const& name) noexcept {
            if (auto result = ymParcelDef_AddStruct(
                *this,
                std::string_view(name).data())) {
                return result;
            }
            return std::nullopt;
        }
        inline std::optional<YmItemIndex> addFn(
            std::convertible_to<std::string_view> auto const& name,
            std::convertible_to<std::string_view> auto const& returnTypeSymbol) noexcept {
            if (auto result = ymParcelDef_AddFn(
                *this,
                std::string_view(name).data(),
                std::string_view(returnTypeSymbol).data())) {
                return result;
            }
            return std::nullopt;
        }
        inline std::optional<YmItemIndex> addMethod(
            YmItemIndex owner,
            std::convertible_to<std::string_view> auto const& name,
            std::convertible_to<std::string_view> auto const& returnTypeSymbol) noexcept {
            if (auto result = ymParcelDef_AddMethod(
                *this,
                owner,
                std::string_view(name).data(),
                std::string_view(returnTypeSymbol).data())) {
                return result;
            }
            return std::nullopt;
        }
        inline std::optional<YmParamIndex> addParam(
            YmItemIndex item,
            std::convertible_to<std::string_view> auto const& name,
            std::convertible_to<std::string_view> auto const& paramTypeSymbol) noexcept {
            if (auto result = ymParcelDef_AddParam(
                *this,
                item,
                std::string_view(name).data(),
                std::string_view(paramTypeSymbol).data())) {
                return result;
            }
            return std::nullopt;
        }
        inline std::optional<YmRef> addRef(
            YmItemIndex item,
            std::convertible_to<std::string_view> auto const& symbol) noexcept {
            if (auto result = ymParcelDef_AddRef(
                *this,
                item,
                std::string_view(symbol).data())) {
                return result;
            }
            return std::nullopt;
        }
    };

    void foo() {
        ParcelDef aa{};
        auto panic_index = aa.addFn("panic", "yama:Never").value();
        aa.addParam(panic_index, "msg", "yama:Str");
    }
}

