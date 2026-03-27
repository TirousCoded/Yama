

#pragma once


#include <vector>

#include "Handle.h"


namespace ym {


    // TODO: All of the below haven't been unit tested.

    // A RAII handle wrapping a YmParcelDef.
    class ParcelDef final : public Handle<YmParcelDef> {
    public:
        inline ParcelDef() :
            ParcelDef(Safe(ymParcelDef_Create()), false) {
        }
        // Increments resource's ref count if secure == true.
        inline explicit ParcelDef(Safe<YmParcelDef> resource, bool secure) noexcept :
            Handle(resource, secure) {
        }


        inline std::optional<YmTypeIndex> addStruct(
            std::convertible_to<std::string_view> auto const& name) noexcept {
            if (auto result = ymParcelDef_AddStruct(
                get(),
                std::string_view(name).data());
                result != YM_NO_TYPE_INDEX) {
                return result;
            }
            return std::nullopt;
        }
        inline std::optional<YmTypeIndex> addProtocol(
            std::convertible_to<std::string_view> auto const& name) noexcept {
            if (auto result = ymParcelDef_AddProtocol(
                get(),
                std::string_view(name).data());
                result != YM_NO_TYPE_INDEX) {
                return result;
            }
            return std::nullopt;
        }
        inline std::optional<YmTypeIndex> addFn(
            std::convertible_to<std::string_view> auto const& name,
            std::convertible_to<std::string_view> auto const& returnTypeSymbol,
            const std::vector<std::pair<std::string, std::string>>& paramNameAndTypeSymbols,
            const std::vector<std::string>& refTypeSymbols,
            YmCallBhvrCallbackFn callBehaviour,
            void* callBehaviourData = nullptr) noexcept {
            if (auto result = ymParcelDef_AddFn(
                get(),
                std::string_view(name).data(),
                std::string_view(returnTypeSymbol).data(),
                callBehaviour,
                callBehaviourData);
                result != YM_NO_TYPE_INDEX) {
                for (const auto& [name, typeSymbol] : paramNameAndTypeSymbols) {
                    addParam(result, name, typeSymbol);
                }
                for (const auto& typeSymbol : refTypeSymbols) {
                    addRef(result, typeSymbol);
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
            const std::vector<std::string>& refTypeSymbols,
            YmCallBhvrCallbackFn callBehaviour,
            void* callBehaviourData = nullptr) noexcept {
            if (auto result = ymParcelDef_AddMethod(
                get(),
                owner,
                std::string_view(name).data(),
                std::string_view(returnTypeSymbol).data(),
                callBehaviour,
                callBehaviourData);
                result != YM_NO_TYPE_INDEX) {
                for (const auto& [name, typeSymbol] : paramNameAndTypeSymbols) {
                    addParam(result, name, typeSymbol);
                }
                for (const auto& typeSymbol : refTypeSymbols) {
                    addRef(result, typeSymbol);
                }
                return result;
            }
            return std::nullopt;
        }
        inline std::optional<YmTypeIndex> addMethodReq(
            YmTypeIndex owner,
            std::convertible_to<std::string_view> auto const& name,
            std::convertible_to<std::string_view> auto const& returnTypeSymbol,
            const std::vector<std::pair<std::string, std::string>>& paramNameAndTypeSymbols) noexcept {
            if (auto result = ymParcelDef_AddMethodReq(
                get(),
                owner,
                std::string_view(name).data(),
                std::string_view(returnTypeSymbol).data());
                result != YM_NO_TYPE_INDEX) {
                for (const auto& [name, typeSymbol] : paramNameAndTypeSymbols) {
                    addParam(result, name, typeSymbol);
                }
                return result;
            }
            return std::nullopt;
        }
        inline std::optional<YmTypeParamIndex> addTypeParam(
            YmTypeIndex type,
            std::convertible_to<std::string_view> auto const& name,
            std::convertible_to<std::string_view> auto const& constraintTypeSymbol) noexcept {
            if (auto result = ymParcelDef_AddTypeParam(
                get(),
                type,
                std::string_view(name).data(),
                std::string_view(constraintTypeSymbol).data());
                result != YM_NO_TYPE_PARAM_INDEX) {
                return result;
            }
            return std::nullopt;
        }
        inline std::optional<YmParamIndex> addParam(
            YmTypeIndex type,
            std::convertible_to<std::string_view> auto const& name,
            std::convertible_to<std::string_view> auto const& paramTypeSymbol) noexcept {
            if (auto result = ymParcelDef_AddParam(
                get(),
                type,
                std::string_view(name).data(),
                std::string_view(paramTypeSymbol).data());
                result != YM_NO_PARAM_INDEX) {
                return result;
            }
            return std::nullopt;
        }
        inline std::optional<YmRef> addRef(
            YmTypeIndex type,
            std::convertible_to<std::string_view> auto const& symbol) noexcept {
            if (auto result = ymParcelDef_AddRef(
                get(),
                type,
                std::string_view(symbol).data());
                result != YM_NO_REF) {
                return result;
            }
            return std::nullopt;
        }
    };
}

