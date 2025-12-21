

#include "YmParcelDef.h"

#include "../internal/general.h"


bool YmParcelDef::verify() const {
    return info->verify();
}

std::optional<YmItemIndex> YmParcelDef::addStruct(
    const std::string& name) {
    return info->addItem(
        name,
        YmKind_Struct);
}

std::optional<YmItemIndex> YmParcelDef::addProtocol(
    const std::string& name) {
    return info->addItem(
        name,
        YmKind_Protocol);
}

std::optional<YmItemIndex> YmParcelDef::addFn(
    const std::string& name,
    std::string returnTypeSymbol,
    _ym::CallBhvrCallbackInfo callBehaviour) {
    return info->addItem(
        name,
        YmKind_Fn,
        std::make_optional(std::move(returnTypeSymbol)),
        callBehaviour);
}

std::optional<YmItemIndex> YmParcelDef::addMethod(
    YmItemIndex owner,
    const std::string& name,
    std::string returnTypeSymbol,
    _ym::CallBhvrCallbackInfo callBehaviour) {
    return info->addItem(
        owner,
        name,
        YmKind_Method,
        std::make_optional(std::move(returnTypeSymbol)),
        callBehaviour);
}

std::optional<YmItemIndex> YmParcelDef::addMethodReq(
    YmItemIndex owner,
    const std::string& name,
    std::string returnTypeSymbol) {
    return addMethod(
        owner,
        name,
        std::move(returnTypeSymbol),
        _ym::CallBhvrCallbackInfo::mk(_ym::methodReqCallBhvr, (void*)std::uintptr_t(-1)));
}

std::optional<YmParamIndex> YmParcelDef::addParam(
    YmItemIndex item,
    std::string name,
    std::string paramTypeSymbol) {
    return info->addParam(
        item,
        std::move(name),
        std::move(paramTypeSymbol));
}

std::optional<YmRef> YmParcelDef::addRef(
    YmItemIndex item,
    std::string symbol) {
    return info->addRef(
        item,
        std::move(symbol));
}

