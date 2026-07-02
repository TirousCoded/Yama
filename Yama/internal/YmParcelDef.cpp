

#include "YmParcelDef.h"

#include "../internal/general.h"


bool YmParcelDef::verify() const {
    return info->verify();
}

bool YmParcelDef::addStruct(
    const std::string& name,
    _ym::KindEx k) {
    return info->addType(_ym::mustBe<YmKind_Struct>(k), name);
}

bool YmParcelDef::addProtocol(
    const std::string& name) {
    return info->addType(_ym::KindEx::Protocol, name);
}

bool YmParcelDef::addFn(
    const std::string& name,
    std::string returnTypeSymbol,
    _ym::CallBhvrCallbackInfo callBehaviour) {
    return info->addType(_ym::KindEx::Fn, name, callBehaviour, std::move(returnTypeSymbol));
}

bool YmParcelDef::addReadOnlyStoredVar(
    const std::string& name,
    std::string typeSymbol,
    _ym::CallBhvrCallbackInfo initBehaviour) {
    return _addReadOnlyVar(
        name, std::move(typeSymbol),
        initBehaviour,
        _ym::CallBhvrCallbackInfo::mk(_ym::storedVarGetCallBhvr),
        _ym::KindEx::StoredVarGet);
}

bool YmParcelDef::addStoredVar(
    const std::string& name,
    std::string typeSymbol,
    _ym::CallBhvrCallbackInfo initBehaviour) {
    return _addVar(
        name, std::move(typeSymbol),
        initBehaviour,
        _ym::CallBhvrCallbackInfo::mk(_ym::storedVarGetCallBhvr),
        _ym::CallBhvrCallbackInfo::mk(_ym::storedVarSetCallBhvr),
        _ym::KindEx::StoredVarGet,
        _ym::KindEx::StoredVarSet);
}

bool YmParcelDef::addReadOnlyComputedVar(
    const std::string& name,
    std::string typeSymbol,
    _ym::CallBhvrCallbackInfo getBehaviour) {
    return _addReadOnlyVar(
        name, std::move(typeSymbol),
        _ym::CallBhvrCallbackInfo{},
        getBehaviour,
        _ym::KindEx::Var);
}

bool YmParcelDef::addComputedVar(
    const std::string& name,
    std::string typeSymbol,
    _ym::CallBhvrCallbackInfo getBehaviour,
    _ym::CallBhvrCallbackInfo setBehaviour) {
    return _addVar(
        name, std::move(typeSymbol),
        _ym::CallBhvrCallbackInfo{},
        getBehaviour,
        setBehaviour,
        _ym::KindEx::Var,
        _ym::KindEx::VarAssigner);
}

bool YmParcelDef::addMethod(
    const std::string& ownerName,
    const std::string& name,
    std::string returnTypeSymbol,
    _ym::CallBhvrCallbackInfo callBehaviour) {
    return _addMethod(
        ownerName,
        name,
        std::move(returnTypeSymbol),
        std::move(callBehaviour),
        _ym::KindEx::Method);
}

bool YmParcelDef::addMethodReq(
    const std::string& ownerName,
    const std::string& name,
    std::string returnTypeSymbol) {
    auto index = uintptr_t(-1);
    if (auto ownerType = info->type(ownerName)) {
        index = ownerType->members();
    }
    return _addMethod(
        ownerName,
        name,
        std::move(returnTypeSymbol),
        _ym::CallBhvrCallbackInfo::mk(
            _ym::methodReqCallBhvr,
            // Give the method its member index.
            (void*)index),
        _ym::KindEx::MethodReq);
}

bool YmParcelDef::addReadOnlyStoredProperty(
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
        _ym::CallBhvrCallbackInfo::mk(_ym::storedPropertyGetCallBhvr, (void*)slot),
        _ym::KindEx::StoredPropertyGet)) {
        return result;
    }
    if (ownerType) {
        ownerType->unwindSlots(); // If fails.
    }
    return false;
}

bool YmParcelDef::addStoredProperty(
    const std::string& ownerName,
    const std::string& name,
    std::string typeSymbol) {
    // TODO: Refactor to remove need for below comment.
    // NOTE: info->type(~) failing CANNOT prevent addProperty from being
    //       called, as otherwise we won't get proper error msgs.
    auto ownerType = info->type(ownerName);
    uint16_t slot = ownerType ? ownerType->nextSlot() : 0;
    if (_addProperty(
        ownerName,
        name,
        std::move(typeSymbol),
        _ym::CallBhvrCallbackInfo::mk(_ym::storedPropertyGetCallBhvr, (void*)slot),
        _ym::CallBhvrCallbackInfo::mk(_ym::storedPropertySetCallBhvr, (void*)slot),
        _ym::KindEx::StoredPropertyGet,
        _ym::KindEx::StoredPropertySet)) {
        return true;
    }
    if (ownerType) {
        ownerType->unwindSlots(); // If fails.
    }
    return false;
}

bool YmParcelDef::addReadOnlyComputedProperty(
    const std::string& ownerName,
    const std::string& name,
    std::string typeSymbol,
    _ym::CallBhvrCallbackInfo getBehaviour) {
    return _addReadOnlyProperty(
        ownerName,
        name,
        std::move(typeSymbol),
        getBehaviour,
        _ym::KindEx::Property);
}

bool YmParcelDef::addComputedProperty(
    const std::string& ownerName,
    const std::string& name,
    std::string typeSymbol,
    _ym::CallBhvrCallbackInfo getBehaviour,
    _ym::CallBhvrCallbackInfo setBehaviour) {
    return _addProperty(
        ownerName,
        name,
        std::move(typeSymbol),
        getBehaviour,
        setBehaviour,
        _ym::KindEx::Property,
        _ym::KindEx::PropertyAssigner);
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

bool YmParcelDef::_addReadOnlyVar(
    const std::string& name,
    std::string typeSymbol,
    _ym::CallBhvrCallbackInfo initBehaviour,
    _ym::CallBhvrCallbackInfo getBehaviour,
    _ym::KindEx getK) {
    auto isStoredVarGet = getK == _ym::KindEx::StoredVarGet;
    if (info->addVarType(
        _ym::mustBe<YmKind_Var>(getK),
        name,
        getBehaviour,
        typeSymbol,
        std::nullopt,
        isStoredVarGet ? std::make_optional(std::format("%here:{}$init", name)) : std::nullopt)) {
        if (isStoredVarGet) {
            (void)info->addType(
                _ym::KindEx::Fn,
                std::format("{}$init", name),
                initBehaviour,
                typeSymbol,
                std::nullopt,
                true);
        }
        return true;
    }
    return false;
}

bool YmParcelDef::_addVar(
    const std::string& name,
    std::string typeSymbol,
    _ym::CallBhvrCallbackInfo initBehaviour,
    _ym::CallBhvrCallbackInfo getBehaviour,
    _ym::CallBhvrCallbackInfo setBehaviour,
    _ym::KindEx getK,
    _ym::KindEx setK) {
    auto isStoredVarGet = getK == _ym::KindEx::StoredVarGet;
    if (info->addVarType(
        _ym::mustBe<YmKind_Var>(getK),
        name,
        getBehaviour,
        typeSymbol,
        std::format("%here:{}$assigner", name),
        isStoredVarGet ? std::make_optional(std::format("%here:{}$init", name)) : std::nullopt)) {
        auto assignerLocalName = std::format("{}$assigner", name);
        (void)info->addType(
            _ym::mustBe<YmKind_VarAssigner>(setK),
            assignerLocalName, setBehaviour, "yama:None", std::nullopt, true);
        (void)info->addParam(assignerLocalName, "x", typeSymbol, true).value();
        if (isStoredVarGet) {
            (void)info->addType(
                _ym::KindEx::Fn,
                std::format("{}$init", name),
                initBehaviour,
                typeSymbol,
                std::nullopt,
                true);
        }
        return true;
    }
    return false;
}

bool YmParcelDef::_addMethod(
    const std::string& ownerName,
    const std::string& name,
    std::string returnTypeSymbol,
    _ym::CallBhvrCallbackInfo callBehaviour,
    _ym::KindEx k) {
    return info->addType(_ym::mustBe<YmKind_Method>(k), ownerName, name, callBehaviour, std::move(returnTypeSymbol));
}

bool YmParcelDef::_addReadOnlyProperty(
    const std::string& ownerName,
    const std::string& name,
    std::string typeSymbol,
    _ym::CallBhvrCallbackInfo getBehaviour,
    _ym::KindEx getK) {
    if (info->addType(_ym::mustBe<YmKind_Property>(getK), ownerName, name, getBehaviour, std::move(typeSymbol))) {
        (void)info->addParam(std::format("{}::{}", ownerName, name), "self", "$Self", true).value();
        return true;
    }
    return false;
}

bool YmParcelDef::_addProperty(
    const std::string& ownerName,
    const std::string& name,
    std::string typeSymbol,
    _ym::CallBhvrCallbackInfo getBehaviour,
    _ym::CallBhvrCallbackInfo setBehaviour,
    _ym::KindEx getK,
    _ym::KindEx setK) {
    if (info->addType(
        _ym::mustBe<YmKind_Property>(getK),
        ownerName, name, getBehaviour, typeSymbol, std::format("$Self::{}$assigner", name))) {
        (void)info->addParam(std::format("{}::{}", ownerName, name), "self", "$Self", true).value();
        auto assignerName = std::format("{}$assigner", name);
        (void)info->addType(
            _ym::mustBe<YmKind_PropertyAssigner>(setK),
            ownerName, assignerName, setBehaviour, "yama:None", std::nullopt, true);
        auto assignerLocalName = std::format("{}::{}", ownerName, assignerName);
        (void)info->addParam(assignerLocalName, "self", "$Self", true).value();
        (void)info->addParam(assignerLocalName, "x", typeSymbol, true).value();
        return true;
    }
    return false;
}

