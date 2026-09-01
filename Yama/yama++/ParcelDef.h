

#pragma once


#include <vector>

#include "Handle.h"


namespace ym {


    // TODO: All of the below haven't been unit tested.

    struct CallBhvrCallbackFn final {
        YmCallBhvrCallbackFn fn;
        void* data;


        inline CallBhvrCallbackFn(
            std::convertible_to<YmCallBhvrCallbackFn> auto const& fn,
            void* data = nullptr) noexcept :
            fn(YmCallBhvrCallbackFn(fn)),
            data(data) {
            assertSafe(this->fn);
        }
    };

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
            CallBhvrCallbackFn callBehaviour) noexcept {
            if (ymParcelDef_AddFn(
                get(),
                name.c_str(),
                returnTypeSymbol.c_str(),
                callBehaviour.fn,
                callBehaviour.data)) {
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
        inline bool addReadOnlyStoredVar(
            const std::string& name,
            const std::string& typeSymbol,
            CallBhvrCallbackFn initBehaviour) noexcept {
            return ymParcelDef_AddReadOnlyStoredVar(
                get(),
                name.c_str(),
                typeSymbol.c_str(),
                initBehaviour.fn,
                initBehaviour.data);
        }
        inline bool addStoredVar(
            const std::string& name,
            const std::string& typeSymbol,
            CallBhvrCallbackFn initBehaviour) noexcept {
            return ymParcelDef_AddStoredVar(
                get(),
                name.c_str(),
                typeSymbol.c_str(),
                initBehaviour.fn,
                initBehaviour.data);
        }
        inline bool addReadOnlyComputedVar(
            const std::string& name,
            const std::string& typeSymbol,
            CallBhvrCallbackFn getBehaviour) noexcept {
            return ymParcelDef_AddReadOnlyComputedVar(
                get(),
                name.c_str(),
                typeSymbol.c_str(),
                getBehaviour.fn,
                getBehaviour.data);
        }
        inline bool addComputedVar(
            const std::string& name,
            const std::string& typeSymbol,
            CallBhvrCallbackFn getBehaviour,
            CallBhvrCallbackFn setBehaviour) noexcept {
            return ymParcelDef_AddComputedVar(
                get(),
                name.c_str(),
                typeSymbol.c_str(),
                getBehaviour.fn,
                getBehaviour.data,
                setBehaviour.fn,
                setBehaviour.data);
        }
        inline bool addMethod(
            const std::string& ownerName,
            const std::string& name,
            const std::string& returnTypeSymbol,
            const std::vector<std::pair<std::string, std::string>>& paramNameAndTypeSymbols,
            const std::vector<std::string>& refTypeSymbols,
            CallBhvrCallbackFn callBehaviour) noexcept {
            if (ymParcelDef_AddMethod(
                get(),
                ownerName.c_str(),
                name.c_str(),
                returnTypeSymbol.c_str(),
                callBehaviour.fn,
                callBehaviour.data)) {
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
        inline bool addReadOnlyStoredProperty(
            const std::string& ownerName,
            const std::string& name,
            const std::string& typeSymbol) noexcept {
            return ymParcelDef_AddReadOnlyStoredProperty(
                get(),
                ownerName.c_str(),
                name.c_str(),
                typeSymbol.c_str());
        }
        inline bool addStoredProperty(
            const std::string& ownerName,
            const std::string& name,
            const std::string& typeSymbol) noexcept {
            return ymParcelDef_AddStoredProperty(
                get(),
                ownerName.c_str(),
                name.c_str(),
                typeSymbol.c_str());
        }
        inline bool addReadOnlyComputedProperty(
            const std::string& ownerName,
            const std::string& name,
            const std::string& typeSymbol,
            CallBhvrCallbackFn getBehaviour) noexcept {
            return ymParcelDef_AddReadOnlyComputedProperty(
                get(),
                ownerName.c_str(),
                name.c_str(),
                typeSymbol.c_str(),
                getBehaviour.fn,
                getBehaviour.data);
        }
        inline bool addComputedProperty(
            const std::string& ownerName,
            const std::string& name,
            const std::string& typeSymbol,
            CallBhvrCallbackFn getBehaviour,
            CallBhvrCallbackFn setBehaviour) noexcept {
            return ymParcelDef_AddComputedProperty(
                get(),
                ownerName.c_str(),
                name.c_str(),
                typeSymbol.c_str(),
                getBehaviour.fn,
                getBehaviour.data,
                setBehaviour.fn,
                setBehaviour.data);
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

