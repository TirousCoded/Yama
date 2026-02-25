

#pragma once


#include <vector>

#include "Handle.h"


namespace ym {


    // TODO: All of the below haven't been unit tested.

    // A RAII handle wrapping a YmParcelDef.
    class ParcelDef final : public Handle<YmParcelDef> {
    public:
        inline ParcelDef() :
            ParcelDef(Safe(ymParcelDef_Create())) {
        }
        // Does not increment the ref count of resource.
        inline explicit ParcelDef(Safe<YmParcelDef> resource) noexcept :
            Handle(resource) {
        }


        inline std::optional<YmTypeIndex> addStruct(
            std::convertible_to<std::string_view> auto const& name) noexcept {
            if (auto result = ymParcelDef_AddStruct(
                *this,
                std::string_view(name).data())) {
                return result;
            }
            return std::nullopt;
        }
        inline std::optional<YmTypeIndex> addProtocol(
            std::convertible_to<std::string_view> auto const& name) noexcept {
            if (auto result = ymParcelDef_AddProtocol(
                *this,
                std::string_view(name).data())) {
                return result;
            }
            return std::nullopt;
        }
        inline std::optional<YmTypeIndex> addFn(
            std::convertible_to<std::string_view> auto const& name,
            std::convertible_to<std::string_view> auto const& returnTypeSymbol,
            const std::vector<std::pair<std::string, std::string>>& paramNameAndTypeSymbols,
            YmCallBhvrCallbackFn callBehaviour,
            void* callBehaviourData = nullptr) noexcept {
            if (auto result = ymParcelDef_AddFn(
                *this,
                std::string_view(name).data(),
                std::string_view(returnTypeSymbol).data(),
                callBehaviour,
                callBehaviourData)) {
                for (const auto& [name, typeSymbol] : paramNameAndTypeSymbols) {
                    addParam(result, name, typeSymbol);
                }
                return result;
            }
            return std::nullopt;
        }
        inline std::optional<YmTypeIndex> addMethod(
            YmTypeIndex owner,
            std::convertible_to<std::string_view> auto const& name,
            std::convertible_to<std::string_view> auto const& returnTypeSymbol,
            const std::vector<std::pair<std::string, std::string>>& paramNameAndTypeSymbols,
            YmCallBhvrCallbackFn callBehaviour,
            void* callBehaviourData = nullptr) noexcept {
            if (auto result = ymParcelDef_AddMethod(
                *this,
                owner,
                std::string_view(name).data(),
                std::string_view(returnTypeSymbol).data(),
                callBehaviour,
                callBehaviourData)) {
                for (const auto& [name, typeSymbol] : paramNameAndTypeSymbols) {
                    addParam(result, name, typeSymbol);
                }
                return result;
            }
            return std::nullopt;
        }
        inline std::optional<YmParamIndex> addParam(
            YmTypeIndex type,
            std::convertible_to<std::string_view> auto const& name,
            std::convertible_to<std::string_view> auto const& paramTypeSymbol) noexcept {
            if (auto result = ymParcelDef_AddParam(
                *this,
                type,
                std::string_view(name).data(),
                std::string_view(paramTypeSymbol).data())) {
                return result;
            }
            return std::nullopt;
        }
        inline std::optional<YmRef> addRef(
            YmTypeIndex type,
            std::convertible_to<std::string_view> auto const& symbol) noexcept {
            if (auto result = ymParcelDef_AddRef(
                *this,
                type,
                std::string_view(symbol).data())) {
                return result;
            }
            return std::nullopt;
        }
    };
}

