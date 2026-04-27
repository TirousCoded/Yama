

#include "YmParcelDef.h"

#include "../internal/general.h"


bool YmParcelDef::verify() const {
    return info->verify();
}

std::optional<YmTypeIndex> YmParcelDef::addStruct(
    const std::string& name) {
    return info->addType(
        name,
        YmKind_Struct);
}

std::optional<YmTypeIndex> YmParcelDef::addProtocol(
    const std::string& name) {
    return info->addType(
        name,
        YmKind_Protocol);
}

std::optional<YmTypeIndex> YmParcelDef::addFn(
    const std::string& name,
    std::string returnTypeSymbol,
    _ym::CallBhvrCallbackInfo callBehaviour) {
    return info->addType(
        name,
        YmKind_Fn,
        std::make_optional(std::move(returnTypeSymbol)),
        callBehaviour);
}

std::optional<YmTypeIndex> YmParcelDef::addMethod(
    const std::string& ownerName,
    const std::string& name,
    std::string returnTypeSymbol,
    _ym::CallBhvrCallbackInfo callBehaviour) {
    return info->addType(
        ownerName,
        name,
        YmKind_Method,
        std::make_optional(std::move(returnTypeSymbol)),
        callBehaviour);
}

std::optional<YmTypeIndex> YmParcelDef::addMethodReq(
    const std::string& ownerName,
    const std::string& name,
    std::string returnTypeSymbol) {
    auto index = uintptr_t(-1);
    if (auto ownerType = info->type(ownerName)) {
        index = ownerType->memberCount();
    }
    return addMethod(
        ownerName,
        name,
        std::move(returnTypeSymbol),
        _ym::CallBhvrCallbackInfo::mk(
            _ym::methodReqCallBhvr,
            // Give the method its member index.
            (void*)index));
}

std::optional<YmTypeParamIndex> YmParcelDef::addTypeParam(
    std::string typeName,
    std::string name,
    std::string constraintTypeSymbol) {
    return info->addTypeParam(
        std::move(typeName),
        std::move(name),
        std::move(constraintTypeSymbol));
}

std::optional<YmParamIndex> YmParcelDef::addParam(
    std::string typeName,
    std::string name,
    std::string paramTypeSymbol) {
    return info->addParam(
        std::move(typeName),
        std::move(name),
        std::move(paramTypeSymbol));
}

void YmParcelDef::beginNamedParams(
    const std::string& typeName) {
    info->beginNamedParams(typeName);
}

std::optional<YmRef> YmParcelDef::addRef(
    std::string typeName,
    std::string symbol) {
    return info->addRef(
        std::move(typeName),
        std::move(symbol));
}

