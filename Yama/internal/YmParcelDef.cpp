

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
    YmTypeIndex owner,
    const std::string& name,
    std::string returnTypeSymbol,
    _ym::CallBhvrCallbackInfo callBehaviour) {
    return info->addType(
        owner,
        name,
        YmKind_Method,
        std::make_optional(std::move(returnTypeSymbol)),
        callBehaviour);
}

std::optional<YmTypeIndex> YmParcelDef::addMethodReq(
    YmTypeIndex owner,
    const std::string& name,
    std::string returnTypeSymbol) {
    return addMethod(
        owner,
        name,
        std::move(returnTypeSymbol),
        _ym::CallBhvrCallbackInfo::mk(_ym::methodReqCallBhvr, (void*)std::uintptr_t(-1)));
}

std::optional<YmTypeParamIndex> YmParcelDef::addTypeParam(
    YmTypeIndex type,
    std::string name,
    std::string constraintTypeSymbol) {
    return info->addTypeParam(
        type,
        std::move(name),
        std::move(constraintTypeSymbol));
}

std::optional<YmParamIndex> YmParcelDef::addParam(
    YmTypeIndex type,
    std::string name,
    std::string paramTypeSymbol) {
    return info->addParam(
        type,
        std::move(name),
        std::move(paramTypeSymbol));
}

std::optional<YmRef> YmParcelDef::addRef(
    YmTypeIndex type,
    std::string symbol) {
    return info->addRef(
        type,
        std::move(symbol));
}

