

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
        // Increments resource's ref count if secure == true.
        inline static std::optional<ParcelDef> maybe(YmParcelDef* resource, bool secure) noexcept {
            return
                resource
                ? std::make_optional(ParcelDef(*resource, secure))
                : std::nullopt;
        }


        inline bool addStruct(
            const std::string& name) noexcept {
            return ymParcelDef_AddStruct(
                get(),
                name.c_str());
        }
        inline bool addProtocol(
            const std::string& name) noexcept {
            return ymParcelDef_AddProtocol(
                get(),
                name.c_str());
        }
        inline bool addFn(
            const std::string& name,
            const std::string& returnTypeSymbol,
            const std::vector<std::pair<std::string, std::string>>& paramNameAndTypeSymbols,
            const std::vector<std::string>& refTypeSymbols,
            YmCallBhvrCallbackFn callBehaviour,
            void* callBehaviourData = nullptr) noexcept {
            if (ymParcelDef_AddFn(
                get(),
                name.c_str(),
                returnTypeSymbol.c_str(),
                callBehaviour,
                callBehaviourData)) {
                for (const auto& [paramName, typeSymbol] : paramNameAndTypeSymbols) {
                    addParam(name, paramName, typeSymbol);
                }
                for (const auto& typeSymbol : refTypeSymbols) {
                    addRef(name, typeSymbol);
                }
                return true;
            }
            return false;
        }
        inline bool addMethod(
            const std::string& ownerName,
            const std::string& name,
            const std::string& returnTypeSymbol,
            const std::vector<std::pair<std::string, std::string>>& paramNameAndTypeSymbols,
            const std::vector<std::string>& refTypeSymbols,
            YmCallBhvrCallbackFn callBehaviour,
            void* callBehaviourData = nullptr) noexcept {
            if (ymParcelDef_AddMethod(
                get(),
                ownerName.c_str(),
                name.c_str(),
                returnTypeSymbol.c_str(),
                callBehaviour,
                callBehaviourData)) {
                auto methodName = std::format("{}::{}", ownerName, name);
                for (const auto& [paramName, typeSymbol] : paramNameAndTypeSymbols) {
                    addParam(methodName, paramName, typeSymbol);
                }
                for (const auto& typeSymbol : refTypeSymbols) {
                    addRef(methodName, typeSymbol);
                }
                return true;
            }
            return false;
        }
        inline bool addMethodReq(
            const std::string& ownerName,
            const std::string& name,
            const std::string& returnTypeSymbol,
            const std::vector<std::pair<std::string, std::string>>& paramNameAndTypeSymbols) noexcept {
            if (ymParcelDef_AddMethodReq(
                get(),
                ownerName.c_str(),
                name.c_str(),
                returnTypeSymbol.c_str())) {
                auto methodName = std::format("{}::{}", ownerName, name);
                for (const auto& [paramName, typeSymbol] : paramNameAndTypeSymbols) {
                    addParam(methodName, paramName, typeSymbol);
                }
                return true;
            }
            return false;
        }
        inline std::optional<YmTypeParamIndex> addTypeParam(
            const std::string& typeName,
            const std::string& name,
            const std::string& constraintTypeSymbol) noexcept {
            if (auto result = ymParcelDef_AddTypeParam(
                get(),
                typeName.c_str(),
                name.c_str(),
                constraintTypeSymbol.c_str());
                result != YM_NO_TYPE_PARAM_INDEX) {
                return result;
            }
            return std::nullopt;
        }
        inline std::optional<YmParamIndex> addParam(
            const std::string& typeName,
            const std::string& name,
            const std::string& paramTypeSymbol) noexcept {
            if (auto result = ymParcelDef_AddParam(
                get(),
                typeName.c_str(),
                name.c_str(),
                paramTypeSymbol.c_str());
                result != YM_NO_PARAM_INDEX) {
                return result;
            }
            return std::nullopt;
        }
        inline std::optional<YmRef> addRef(
            const std::string& typeName,
            const std::string& symbol) noexcept {
            if (auto result = ymParcelDef_AddRef(
                get(),
                typeName.c_str(),
                symbol.c_str());
                result != YM_NO_REF) {
                return result;
            }
            return std::nullopt;
        }
    };
}

