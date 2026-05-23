

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
        std::nullopt,
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
        std::nullopt,
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

std::optional<YmTypeIndex> YmParcelDef::addReadOnlyStoredProperty(
    const std::string& ownerName,
    const std::string& name,
    std::string typeSymbol) {
    // TODO: Refactor to remove need for below comment.
    // NOTE: info->type(~) failing CANNOT prevent addReadOnlyProperty from being
    //       called, as otherwise we won't get proper error msgs.
    auto ownerType = info->type(ownerName);
    uint16_t slot = ownerType ? ownerType->nextSlot() : 0;
    if (auto result = _addReadOnlyProperty(
        ownerName,
        name,
        std::move(typeSymbol),
        _ym::CallBhvrCallbackInfo::mk(
            _ym::storedPropertyGetCallBhvr,
            // Pass slot index.
            (void*)slot))) {
        return result;
    }
    if (ownerType) {
        ownerType->unwindSlots(); // If fails.
    }
    return std::nullopt;
}

std::optional<YmTypeIndex> YmParcelDef::addStoredProperty(
    const std::string& ownerName,
    const std::string& name,
    std::string typeSymbol) {
    // TODO: Refactor to remove need for below comment.
    // NOTE: info->type(~) failing CANNOT prevent addProperty from being
    //       called, as otherwise we won't get proper error msgs.
    auto ownerType = info->type(ownerName);
    uint16_t slot = ownerType ? ownerType->nextSlot() : 0;
    if (auto result = _addProperty(
        ownerName,
        name,
        std::move(typeSymbol),
        _ym::CallBhvrCallbackInfo::mk(
            _ym::storedPropertyGetCallBhvr,
            // Pass slot index.
            (void*)slot),
        _ym::CallBhvrCallbackInfo::mk(
            _ym::storedPropertySetCallBhvr,
            // Pass slot index.
            (void*)slot))) {
        return result;
    }
    if (ownerType) {
        ownerType->unwindSlots(); // If fails.
    }
    return std::nullopt;
}

std::optional<YmTypeIndex> YmParcelDef::addReadOnlyComputedProperty(
    const std::string& ownerName,
    const std::string& name,
    std::string typeSymbol,
    _ym::CallBhvrCallbackInfo getCallBehaviour) {
    return _addReadOnlyProperty(
        ownerName,
        name,
        std::move(typeSymbol),
        getCallBehaviour);
}

std::optional<YmTypeIndex> YmParcelDef::addComputedProperty(
    const std::string& ownerName,
    const std::string& name,
    std::string typeSymbol,
    _ym::CallBhvrCallbackInfo getCallBehaviour,
    _ym::CallBhvrCallbackInfo setCallBehaviour) {
    return _addProperty(
        ownerName,
        name,
        std::move(typeSymbol),
        getCallBehaviour,
        setCallBehaviour);
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

std::optional<YmTypeIndex> YmParcelDef::_addReadOnlyProperty(
    const std::string& ownerName,
    const std::string& name,
    std::string typeSymbol,
    _ym::CallBhvrCallbackInfo getCallBehaviour) {
    if (auto result = info->addType(
        ownerName,
        name,
        YmKind_Property,
        typeSymbol,
        std::nullopt,
        getCallBehaviour)) {
        (void)info->addParam(std::format("{}::{}", ownerName, name), "self", "$Self", true).value();
        return result;
    }
    return std::nullopt;
}

std::optional<YmTypeIndex> YmParcelDef::_addProperty(
    const std::string& ownerName,
    const std::string& name,
    std::string typeSymbol,
    _ym::CallBhvrCallbackInfo getCallBehaviour,
    _ym::CallBhvrCallbackInfo setCallBehaviour) {
    if (auto result = info->addType(
        ownerName,
        name,
        YmKind_Property,
        typeSymbol,
        std::format("$Self::{}$assigner", name),
        getCallBehaviour)) {
        (void)info->addParam(std::format("{}::{}", ownerName, name), "self", "$Self", true).value();
        auto assignerName = std::format("{}$assigner", name);
        (void)info->addType(
            ownerName,
            assignerName,
            YmKind_PropertyAssigner,
            "yama:None",
            std::nullopt,
            setCallBehaviour,
            true)
            .value();
        auto assignerLocalName = std::format("{}::{}", ownerName, assignerName);
        (void)info->addParam(assignerLocalName, "self", "$Self", true).value();
        (void)info->addParam(assignerLocalName, "x", typeSymbol, true).value();
        return result;
    }
    return std::nullopt;
}

