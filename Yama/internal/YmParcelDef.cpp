

#include "YmParcelDef.h"

#include "../internal/general.h"
#include "../yama++/general.h"


bool YmParcelDef::verify() const {
    return info->verify();
}

bool YmParcelDef::addStruct(
    const std::string& name,
    _ym::KindEx k) {
    return info->registerType(info->mkNonMember(_ym::mustBe<YmKind_Struct>(k), name, false));
}

bool YmParcelDef::addProtocol(
    const std::string& name) {
    return info->registerType(info->mkNonMember(_ym::KindEx::Protocol, name, false));
}

bool YmParcelDef::addFn(
    const std::string& name,
    const std::string& returnTypeSymbol,
    _ym::CallBhvrCallbackInfo callBehaviour) {
    auto t = info->mkNonMember(_ym::KindEx::Fn, name, false);
    return
        t &&
        t->setupCall(callBehaviour, returnTypeSymbol, -1, false) &&
        info->registerType(std::move(t));
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
    return _addReadOnlyProperty(
        ownerName,
        name,
        std::move(typeSymbol),
        _ym::CallBhvrCallbackInfo::mk(_ym::storedPropertyGetCallBhvr),
        _ym::KindEx::StoredPropertyGet);
}

bool YmParcelDef::addStoredProperty(
    const std::string& ownerName,
    const std::string& name,
    std::string typeSymbol) {
    return _addProperty(
        ownerName,
        name,
        std::move(typeSymbol),
        _ym::CallBhvrCallbackInfo::mk(_ym::storedPropertyGetCallBhvr),
        _ym::CallBhvrCallbackInfo::mk(_ym::storedPropertySetCallBhvr),
        _ym::KindEx::StoredPropertyGet,
        _ym::KindEx::StoredPropertySet);
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
    if (auto var = info->mkNonMember(_ym::mustBe<YmKind_Var>(getK), name, false);
        var &&
        var->setupCall(getBehaviour, typeSymbol, -1, false) &&
        var->setupVar(isStoredVarGet) &&
        info->registerType(std::move(var))) {
        if (isStoredVarGet) {
            auto init = info->mkNonMember(_ym::KindEx::Fn, std::format("{}$init", name), true);
            ymAssert((bool)init);
            init->setupCall(initBehaviour, typeSymbol, -1, false);
            info->registerType(std::move(init), true);
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
    if (auto var = info->mkNonMember(_ym::mustBe<YmKind_Var>(getK), name, false);
        var &&
        var->setupCall(getBehaviour, typeSymbol, -1, true) &&
        var->setupVar(isStoredVarGet) &&
        info->registerType(std::move(var))) {
        auto assigner = info->mkNonMember(_ym::mustBe<YmKind_VarAssigner>(setK), std::format("{}$assigner", name), true);
        ymAssert((bool)assigner);
        assigner->setupCall(setBehaviour, "yama:None", -1, false);
        (void)assigner->addParam("x", typeSymbol, true).value();
        info->registerType(std::move(assigner), true);
        if (isStoredVarGet) {
            auto init = info->mkNonMember(_ym::KindEx::Fn, std::format("{}$init", name), true);
            ymAssert((bool)init);
            init->setupCall(initBehaviour, typeSymbol, -1, false);
            info->registerType(std::move(init), true);
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
    auto t = info->mkMember(_ym::mustBe<YmKind_Method>(k), ownerName, name, false);
    return
        t &&
        t->setupCall(callBehaviour, returnTypeSymbol, -1, false) &&
        info->registerType(std::move(t));
}

bool YmParcelDef::_addReadOnlyProperty(
    const std::string& ownerName,
    const std::string& name,
    std::string typeSymbol,
    _ym::CallBhvrCallbackInfo getBehaviour,
    _ym::KindEx getK) {
    bool isStoredProperty = getK == _ym::KindEx::StoredPropertyGet;
    if (auto t = info->mkMember(_ym::mustBe<YmKind_Property>(getK), ownerName, name, false)) {
        auto& owner = ym::deref(t->owner());
        auto slot = isStoredProperty ? owner.nextSlot() : -1;
        if (t->setupCall(getBehaviour, typeSymbol, slot, false) &&
            t->addParam("self", "$Self", true) &&
            info->registerType(std::move(t))) {
            return true;
        }
        if (isStoredProperty) {
            owner.unwindSlots(); // If fails.
        }
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
    bool isStoredProperty = getK == _ym::KindEx::StoredPropertyGet;
    if (auto property = info->mkMember(_ym::mustBe<YmKind_Property>(getK), ownerName, name, false)) {
        auto& owner = ym::deref(property->owner());
        auto slot = isStoredProperty ? owner.nextSlot() : -1;
        if (!(
            property->setupCall(getBehaviour, typeSymbol, slot, true) &&
            property->addParam("self", "$Self", true) &&
            info->registerType(std::move(property)))) {
            if (isStoredProperty) {
                owner.unwindSlots(); // If fails.
            }
            return false;
        }
        auto assigner = info->mkMember(_ym::mustBe<YmKind_PropertyAssigner>(setK), ownerName,
            std::format("{}$assigner", name), true);
        ymAssert((bool)assigner);
        assigner->setupCall(setBehaviour, "yama:None", -1, false);
        (void)assigner->addParam("self", "$Self", true).value();
        (void)assigner->addParam("x", typeSymbol, true).value();
        info->registerType(std::move(assigner), true);
        return true;
    }
    return false;
}

