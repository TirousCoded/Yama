

#include "YmParcelDef.h"

#include "../internal/general.h"
#include "../yama++/general.h"


bool YmParcelDef::addStruct(
    const std::string& name,
    _ym::KindEx k,
    _ym::ConstTableInfo initial) {
    return info->addStruct(name, k, std::move(initial));
}

bool YmParcelDef::addProtocol(
    const std::string& name,
    _ym::ConstTableInfo initial) {
    return info->addProtocol(name, std::move(initial));
}

bool YmParcelDef::addFn(
    const std::string& name,
    const std::string& returnTypeSymbol,
    _ym::CallBhvrCallbackInfo callBehaviour,
    _ym::ConstTableInfo initial) {
    return info->addFn(name, returnTypeSymbol, callBehaviour, std::move(initial));
}

bool YmParcelDef::addReadOnlyStoredVar(
    const std::string& name,
    const std::string& typeSymbol,
    _ym::CallBhvrCallbackInfo initBehaviour,
    _ym::ConstTableInfo initial) {
    return info->addReadOnlyStoredVar(name, typeSymbol, initBehaviour, std::move(initial));
}

bool YmParcelDef::addStoredVar(
    const std::string& name,
    const std::string& typeSymbol,
    _ym::CallBhvrCallbackInfo initBehaviour,
    _ym::ConstTableInfo initial) {
    return info->addStoredVar(name, typeSymbol, initBehaviour, std::move(initial));
}

bool YmParcelDef::addReadOnlyComputedVar(
    const std::string& name,
    const std::string& typeSymbol,
    _ym::CallBhvrCallbackInfo getBehaviour,
    _ym::ConstTableInfo initial) {
    return info->addReadOnlyComputedVar(name, typeSymbol, getBehaviour, std::move(initial));
}

bool YmParcelDef::addComputedVar(
    const std::string& name,
    const std::string& typeSymbol,
    _ym::CallBhvrCallbackInfo getBehaviour,
    _ym::CallBhvrCallbackInfo setBehaviour,
    _ym::ConstTableInfo initial) {
    return info->addComputedVar(name, typeSymbol, getBehaviour, setBehaviour, std::move(initial));
}

bool YmParcelDef::addMethod(
    const std::string& ownerName,
    const std::string& name,
    const std::string& returnTypeSymbol,
    _ym::CallBhvrCallbackInfo callBehaviour,
    _ym::ConstTableInfo initial) {
    return info->addMethod(ownerName, name, returnTypeSymbol, callBehaviour, std::move(initial));
}

bool YmParcelDef::addMethodReq(
    const std::string& ownerName,
    const std::string& name,
    const std::string& returnTypeSymbol,
    _ym::ConstTableInfo initial) {
    return info->addMethodReq(ownerName, name, returnTypeSymbol, std::move(initial));
}

bool YmParcelDef::addReadOnlyStoredProperty(
    const std::string& ownerName,
    const std::string& name,
    const std::string& typeSymbol,
    _ym::ConstTableInfo initial) {
    return info->addReadOnlyStoredProperty(ownerName, name, typeSymbol, std::move(initial));
}

bool YmParcelDef::addStoredProperty(
    const std::string& ownerName,
    const std::string& name,
    const std::string& typeSymbol,
    _ym::ConstTableInfo initial) {
    return info->addStoredProperty(ownerName, name, typeSymbol, std::move(initial));
}

bool YmParcelDef::addReadOnlyComputedProperty(
    const std::string& ownerName,
    const std::string& name,
    const std::string& typeSymbol,
    _ym::CallBhvrCallbackInfo getBehaviour,
    _ym::ConstTableInfo initial) {
    return info->addReadOnlyComputedProperty(ownerName, name, typeSymbol, getBehaviour, std::move(initial));
}

bool YmParcelDef::addComputedProperty(
    const std::string& ownerName,
    const std::string& name,
    const std::string& typeSymbol,
    _ym::CallBhvrCallbackInfo getBehaviour,
    _ym::CallBhvrCallbackInfo setBehaviour,
    _ym::ConstTableInfo initial) {
    return info->addComputedProperty(ownerName, name, typeSymbol, getBehaviour, setBehaviour, std::move(initial));
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

bool YmParcelDef::bindBCode(
    const std::string& localName,
    _ym::BCode code,
    _ym::BCodeDbgSyms syms) {
    return info->bindBCode(localName, std::move(code), std::move(syms));
}

