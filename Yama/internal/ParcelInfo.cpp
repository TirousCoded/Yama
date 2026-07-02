

#include "ParcelInfo.h"

#include <taul/unicode.h>

#include "general.h"
#include "SpecSolver.h"
#include "../yama++/general.h"

#include "YmObj.h" // <- Needed for YmCtx impl.


std::optional<_ym::Spec> _ym::normalizeRefSym(const std::string& symbol, std::string_view msg, SpecSolver solver) {
    if (auto result = Spec::type(symbol, solver)) {
        return result;
    }
    _ym::Global::raiseErr(
        YmErrCode_IllegalSpecifier,
        "{}; reference symbol \"{}\" is illegal!",
        (std::string)msg,
        symbol);
    return std::nullopt;
}

bool _ym::checkHasCallSig(const TypeInfo& type, std::string_view msg) {
    bool result = ymKind_HasCallSig(type.kind());
    if (!result) {
        Global::raiseErr(
            YmErrCode_CallSigNotFound,
            "{}; type {} has no call signature!",
            (std::string)msg,
            type.localName());
    }
    return result;
}

bool _ym::checkNonMember(const TypeInfo& type, std::string_view msg) {
    bool result = !ymKind_IsMember(type.kind());
    if (!result) {
        Global::raiseErr(
            YmErrCode_MemberType,
            "{}; type {} is a member type!",
            (std::string)msg,
            type.localName());
    }
    return result;
}

bool _ym::checkNonProtocolMember(const TypeInfo& type, std::string_view msg) {
    bool result = !type.isMethodReq();
    if (!result) {
        Global::raiseErr(
            YmErrCode_ProtocolMemberType,
            "{}; type {} is a protocol member type!",
            (std::string)msg,
            type.localName());
    }
    return result;
}

void _ym::methodReqCallBhvr(YmCtx* ctx, YmType* type, void* user) {
    YM_DEADEND;
}

void _ym::storedPropertyGetCallBhvr(YmCtx* ctx, YmType* type, void* user) {
    ctx->put(YM_PUSH, ctx->arg(0), YM_BORROW);
    ctx->getProperty(type, YM_PUSH);
    ctx->ret(ctx->pull());
}

void _ym::storedPropertySetCallBhvr(YmCtx* ctx, YmType* type, void* user) {
    // TODO: This code semi-duplicates code in YmCtx::setProperty.
    auto& subject = ym::deref(ctx->arg(0));
    auto& value = ym::deref(ctx->arg(1));
    auto& target = subject.slot(type->info->storedPropertySlot().value()).ref;
    ctx->release(ym::deref(target));
    target = &value;
    ctx->secure(value);
    ctx->ret(ctx->newNone());
}

void _ym::storedVarGetCallBhvr(YmCtx* ctx, YmType* type, void* user) {
    ctx->getVar(type, YM_PUSH);
    ctx->ret(ctx->pull());
}

void _ym::storedVarSetCallBhvr(YmCtx* ctx, YmType* type, void* user) {
    ctx->put(YM_PUSH, ctx->arg(0), YM_BORROW);
    ctx->setVar(type->var());
    ctx->ret(ctx->newNone());
}

_ym::TypeInfo::TypeInfo(ParcelInfo& parcel, KindEx k, const std::string& localName) :
    _parcel(&parcel),
    _k(k),
    _localName(localName) {
    _initMembership();
    _initTypeParams();
    _initMembers();
    _initVarAssigner();
}

_ym::ParcelInfo& _ym::TypeInfo::parcel() const noexcept {
    return ym::deref(_parcel);
}

_ym::KindEx _ym::TypeInfo::kindEx() const noexcept {
    return _k;
}

YmKind _ym::TypeInfo::kind() const noexcept {
    return kindOf(kindEx());
}

const std::string& _ym::TypeInfo::localName() const noexcept {
    return _localName;
}

bool _ym::TypeInfo::isRegular() const noexcept {
    return _ym::isRegular(kindEx());
}

bool _ym::TypeInfo::isIrregular() const noexcept {
    return _ym::isIrregular(kindEx());
}

bool _ym::TypeInfo::isPrimitive() const noexcept {
    return _ym::isPrimitive(kindEx());
}

bool _ym::TypeInfo::isGetter() const noexcept {
    return _ym::isGetter(kindEx());
}

bool _ym::TypeInfo::isSetter() const noexcept {
    return _ym::isSetter(kindEx());
}

bool _ym::TypeInfo::isVarLike() const noexcept {
    return _ym::isVarLike(kindEx());
}

bool _ym::TypeInfo::isProtocolReq() const noexcept {
    return _ym::isProtocolReq(kindEx());
}

bool _ym::TypeInfo::hasCallSig() const noexcept {
    return ymKind_HasCallSig(kind());
}

bool _ym::TypeInfo::hasUserDefinedCallSig() const noexcept {
    return ymKind_HasUserDefinedCallSig(kind());
}

bool _ym::TypeInfo::isOwner() const noexcept {
    return ymKind_IsOwner(kind());
}

bool _ym::TypeInfo::isMember() const noexcept {
    return ymKind_IsMember(kind());
}

bool _ym::TypeInfo::canHaveMembers() const noexcept {
    return ymKind_CanHaveMembers(kind());
}

bool _ym::TypeInfo::canHaveTypeParams() const noexcept {
    return ymKind_CanHaveTypeParams(kind());
}

bool _ym::TypeInfo::hasDefaultValue() const noexcept {
    if (isPrimitive()) {
        return true;
    }
    else if (isRegularStruct()) {
        return slots == 0;
    }
    else return false;
}

bool _ym::TypeInfo::isStruct() const noexcept {
    return kind() == YmKind_Struct;
}

bool _ym::TypeInfo::isProtocol() const noexcept {
    return kind() == YmKind_Protocol;
}

bool _ym::TypeInfo::isFn() const noexcept {
    return kind() == YmKind_Fn;
}

bool _ym::TypeInfo::isVar() const noexcept {
    return kind() == YmKind_Var;
}

bool _ym::TypeInfo::isVarAssigner() const noexcept {
    return kind() == YmKind_VarAssigner;
}

bool _ym::TypeInfo::isMethod() const noexcept {
    return kind() == YmKind_Method;
}

bool _ym::TypeInfo::isProperty() const noexcept {
    return kind() == YmKind_Property;
}

bool _ym::TypeInfo::isPropertyAssigner() const noexcept {
    return kind() == YmKind_PropertyAssigner;
}

bool _ym::TypeInfo::isRegularStruct() const noexcept {
    return kindEx() == KindEx::Struct;
}

bool _ym::TypeInfo::isRegularProtocol() const noexcept {
    return kindEx() == KindEx::Protocol;
}

bool _ym::TypeInfo::isRegularFn() const noexcept {
    return kindEx() == KindEx::Fn;
}

bool _ym::TypeInfo::isRegularVar() const noexcept {
    return kindEx() == KindEx::Var;
}

bool _ym::TypeInfo::isRegularVarAssigner() const noexcept {
    return kindEx() == KindEx::VarAssigner;
}

bool _ym::TypeInfo::isRegularMethod() const noexcept {
    return kindEx() == KindEx::Method;
}

bool _ym::TypeInfo::isRegularProperty() const noexcept {
    return kindEx() == KindEx::Property;
}

bool _ym::TypeInfo::isRegularPropertyAssigner() const noexcept {
    return kindEx() == KindEx::PropertyAssigner;
}

bool _ym::TypeInfo::isNone() const noexcept {
    return kindEx() == KindEx::None;
}

bool _ym::TypeInfo::isInt() const noexcept {
    return kindEx() == KindEx::Int;
}

bool _ym::TypeInfo::isUInt() const noexcept {
    return kindEx() == KindEx::UInt;
}

bool _ym::TypeInfo::isFloat() const noexcept {
    return kindEx() == KindEx::Float;
}

bool _ym::TypeInfo::isBool() const noexcept {
    return kindEx() == KindEx::Bool;
}

bool _ym::TypeInfo::isRune() const noexcept {
    return kindEx() == KindEx::Rune;
}

bool _ym::TypeInfo::isType() const noexcept {
    return kindEx() == KindEx::Type;
}

bool _ym::TypeInfo::isMethodReq() const noexcept {
    return kindEx() == KindEx::MethodReq;
}

bool _ym::TypeInfo::isStoredVarGet() const noexcept {
    return kindEx() == KindEx::StoredVarGet;
}

bool _ym::TypeInfo::isStoredVarSet() const noexcept {
    return kindEx() == KindEx::StoredVarSet;
}

bool _ym::TypeInfo::isStoredPropertyGet() const noexcept {
    return kindEx() == KindEx::StoredPropertyGet;
}

bool _ym::TypeInfo::isStoredPropertySet() const noexcept {
    return kindEx() == KindEx::StoredPropertySet;
}

std::optional<_ym::ConstIndex> _ym::TypeInfo::varConst() const noexcept {
    return
        _varAssigner
        ? std::make_optional(_varAssigner->varConst)
        : std::nullopt;
}

_ym::TypeInfo* _ym::TypeInfo::owner() const noexcept {
    return
        _membership
        ? _membership->owner.get()
        : nullptr;
}

std::optional<_ym::ConstIndex> _ym::TypeInfo::ownerConst() const noexcept {
    return
        _membership
        ? std::make_optional(_membership->ownerConst)
        : std::nullopt;
}

const std::string& _ym::TypeInfo::ownerName() const noexcept {
    return
        _membership
        ? _membership->ownerName
        : localName();
}

const std::string& _ym::TypeInfo::memberName() const noexcept {
    static const std::string empty = "";
    return
        _membership
        ? _membership->memberName
        : empty;
}

YmTypeParams _ym::TypeInfo::typeParams() const noexcept {
    return
        _typeParams
        ? (YmMembers)_typeParams->count()
        : 0;
}

bool _ym::TypeInfo::isParameterized() const noexcept {
    return typeParams() >= 1;
}

const _ym::TypeInfo::TypeParam* _ym::TypeInfo::typeParam(ConstIndex index) const noexcept {
    return
        _typeParams
        ? _typeParams->byIndex(index)
        : nullptr;
}

const _ym::TypeInfo::TypeParam* _ym::TypeInfo::typeParam(const std::string& name) const noexcept {
    return
        _typeParams
        ? _typeParams->byName(name)
        : nullptr;
}

YmMembers _ym::TypeInfo::members() const noexcept {
    return
        _members
        ? (YmMembers)_members->count()
        : 0;
}

const _ym::TypeInfo::Member* _ym::TypeInfo::member(ConstIndex index) const noexcept {
    return
        _members
        ? _members->byIndex(index)
        : nullptr;
}

const _ym::TypeInfo::Member* _ym::TypeInfo::member(const std::string& name) const noexcept {
    return
        _members
        ? _members->byName(name)
        : nullptr;
}

std::optional<_ym::ConstIndex> _ym::TypeInfo::returnTypeConst() const noexcept {
    return
        _call
        ? std::make_optional(_call->returnTypeConst)
        : std::nullopt;
}

YmParams _ym::TypeInfo::params() const noexcept {
    return
        _call
        ? _call->count()
        : 0;
}

YmParams _ym::TypeInfo::positionalParams() const noexcept {
    return
        _call
        ? _call->positionalCount()
        : 0;
}

YmParams _ym::TypeInfo::namedParams() const noexcept {
    return
        _call
        ? _call->namedCount()
        : 0;
}

const _ym::TypeInfo::Param* _ym::TypeInfo::param(YmParamIndex index) const noexcept {
    return
        _call
        ? _call->param(index)
        : nullptr;
}

const _ym::TypeInfo::Param* _ym::TypeInfo::param(const std::string& name) const noexcept {
    return
        _call
        ? _call->param(name)
        : nullptr;
}

const _ym::CallBhvrCallbackInfo* _ym::TypeInfo::callBehaviour() const noexcept {
    return
        _call
        ? &_call->callBehaviour
        : nullptr;
}

std::optional<_ym::ConstIndex> _ym::TypeInfo::assignerConst() const noexcept {
    return
        _call
        ? _call->assignerConst
        : std::nullopt;
}

std::optional<_ym::ConstIndex> _ym::TypeInfo::initializerConst() const noexcept {
    return
        _var
        ? _var->initializerConst
        : std::nullopt;
}

std::optional<YmUInt16> _ym::TypeInfo::storedPropertySlot() const noexcept {
    // TODO: Figure out an alternative to below, this is SUPER HACKY!!!
    return
        (callBehaviour() && (isStoredPropertyGet() || isStoredPropertySet()))
        ? std::make_optional((YmUInt16)(std::uintptr_t)callBehaviour()->user)
        : std::nullopt;
}

uint16_t _ym::TypeInfo::nextSlot() noexcept {
    ymAssert(slots < decltype(slots)(-1));
    slots++;
    return slots - 1;
}

void _ym::TypeInfo::unwindSlots(uint16_t n) noexcept {
    slots -= (n <= slots) ? n : slots;
}

std::optional<YmTypeParamIndex> _ym::TypeInfo::addTypeParam(std::string name, std::string constraintTypeSymbol) {
    bool badTypeParamRef = false;
    SpecSolver solver{};
    // TODO: Does this lambda fn object heap alloc w/ each addTypeParam call?
    solver.typeParamCallback = [this, &name, &badTypeParamRef](taul::str id, bool rootOfEntireTree) {
        const auto typeParamName = (std::string)id.substr(1);
        if (rootOfEntireTree) {
            Global::raiseErr(
                YmErrCode_IllegalConstraint,
                "Cannot add type parameter; cannot use type parameter {} as a constraint type!",
                typeParamName);
            badTypeParamRef = true;
        }
        if (!typeParam(typeParamName) && typeParamName != name) {
            Global::raiseErr(
                YmErrCode_IllegalSpecifier,
                "Cannot add type parameter; type parameter {} not found!",
                typeParamName);
            badTypeParamRef = true;
        }
        };
    auto normalizedConstraintTypeSym = normalizeRefSym(constraintTypeSymbol, "Cannot add type parameter", solver);
    if (!normalizedConstraintTypeSym) {
        return std::nullopt;
    }
    if (badTypeParamRef) {
        return std::nullopt;
    }
    if (!checkNonMember(*this, "Cannot add type parameter")) {
        return std::nullopt;
    }
    if (_typeParams && _typeParams->add(name, consts.pullRef(std::move(normalizedConstraintTypeSym.value())).value())) {
        return typeParams() - 1;
    }
    return std::nullopt;
}

std::optional<YmParamIndex> _ym::TypeInfo::addParam(std::string name, std::string paramTypeSymbol, bool skipHasCallSigCheck) {
    auto normalizedParamTypeSym = normalizeRefSym(paramTypeSymbol, "Cannot add parameter");
    if (!normalizedParamTypeSym) {
        return std::nullopt;
    }
    if (!skipHasCallSigCheck && !checkHasCallSig(*this, "Cannot add parameter")) {
        return std::nullopt;
    }
    if (_call && _call->addParam(name, consts.pullRef(std::move(normalizedParamTypeSym.value())).value())) {
        return params() - 1;
    }
    return std::nullopt;
}

void _ym::TypeInfo::beginNamedParams() {
    if (!checkHasCallSig(*this, "Cannot begin named params")) {
        return;
    }
    if (!checkNonProtocolMember(*this, "Cannot begin named params")) {
        return;
    }
    if (_call) {
        _call->beginNamedParams();
    }
}

std::optional<YmRef> _ym::TypeInfo::addRef(std::string symbol) {
    auto normalizedSymbol = normalizeRefSym(symbol, "Cannot add reference");
    if (!normalizedSymbol) {
        return std::nullopt;
    }
    if (auto result = consts.pullRef(std::move(normalizedSymbol.value()), size_t(YmRef(-1)))) {
        refs.push_back(result.value());
        return YmRef(refs.size() - 1);
    }
    _ym::Global::raiseErr(
        YmErrCode_InternalError,
        "Cannot add reference; internal failure!");
    return std::nullopt;
}

void _ym::TypeInfo::registerMember(const std::string& name) {
    if (_members) {
        _members->registerMember(*this, name);
    }
}

void _ym::TypeInfo::registerMembershipWithOwner() {
    if (isMember()) {
        owner()->registerMember(memberName());
    }
}

void _ym::TypeInfo::setupCall(
    CallBhvrCallbackInfo callBehaviour,
    std::optional<ConstIndex> assignerConst,
    ConstIndex returnTypeConst) {
    if (!_call) {
        _call = std::unique_ptr<_Call>(new _Call{
            .callBehaviour = std::move(callBehaviour),
            .assignerConst = assignerConst,
            .returnTypeConst = returnTypeConst,
            });
    }
}

void _ym::TypeInfo::setupVar(std::optional<ConstIndex> initializerConst) {
    if (!_var) {
        _var = std::unique_ptr<_Var>(new _Var{
            .initializerConst = initializerConst,
            });
    }
}

std::string _ym::TypeInfo::fullnameForRef() const {
    return std::format("%here:{}", localName());
}

void _ym::TypeInfo::_initMembership() {
    if (isMember()) {
        auto ownerName = _extractOwnerName(localName());
        _membership = std::unique_ptr<_Membership>(new _Membership{
            .ownerName = ownerName, // Can't avoid copy here.
            .memberName = _extractMemberName(localName()),
            .owner = ym::Safe(parcel().type(ownerName)),
            // $Self here nicely accounts for things like generics.
            .ownerConst = consts.pullRef(Spec::typeFast("$Self")).value(),
            });
    }
}

void _ym::TypeInfo::_initTypeParams() {
    // TODO: Could we defer initializing _typeParams upon first member register?
    if (isOwner()) {
        _typeParams = std::unique_ptr<_TypeParams>(new _TypeParams{});
    }
}

void _ym::TypeInfo::_initMembers() {
    // TODO: Could we defer initializing _members upon first member register?
    if (canHaveMembers()) {
        _members = std::unique_ptr<_Members>(new _Members{});
    }
}

void _ym::TypeInfo::_initVarAssigner() {
    if (isVarAssigner()) {
        _varAssigner = std::unique_ptr<_VarAssigner>(new _VarAssigner{
            .varConst = consts.pullRef(Spec::typeFast(parcel().type((std::string)split_s<char>(localName(), "$assigner", true).first)->fullnameForRef())).value(),
            });
    }
}

std::string _ym::TypeInfo::_extractOwnerName(const std::string& localName) noexcept {
    return (std::string)split_s<YmChar>(localName, "::").first;
}

std::string _ym::TypeInfo::_extractMemberName(const std::string& localName) noexcept {
    return (std::string)split_s<YmChar>(localName, "::").second;
}

YmTypeParams _ym::TypeInfo::_TypeParams::count() const noexcept {
    return (YmTypeParams)typeParamsByIndex.size();
}

const _ym::TypeInfo::TypeParam* _ym::TypeInfo::_TypeParams::byIndex(size_t index) const noexcept {
    return
        index < count()
        ? typeParamsByIndex[index].get()
        : nullptr;
}

const _ym::TypeInfo::TypeParam* _ym::TypeInfo::_TypeParams::byName(const std::string& name) const noexcept {
    auto it = typeParamsByName.find(name);
    return
        it != typeParamsByName.end()
        ? it->second.get()
        : nullptr;
}

bool _ym::TypeInfo::_TypeParams::add(const std::string& name, ConstIndex constraintConst) {
    if (byName(name)) {
        // TODO: Should we raise error?
        return false;
    }
    if (count() >= size_t(YM_MAX_TYPE_PARAMS)) {
        Global::raiseErr(
            YmErrCode_LimitReached,
            "Cannot add type parameter; would exceed {} limit!",
            YM_MAX_TYPE_PARAMS);
        return false;
    }
    typeParamsByIndex.push_back(std::unique_ptr<TypeParam>(new TypeParam{
        .index = count(),
        .name = name,
        .constraintConst = constraintConst,
        }));
    typeParamsByName.try_emplace(name, ym::Safe(typeParamsByIndex.back().get()));
    return true;
}

YmMembers _ym::TypeInfo::_Members::count() const noexcept {
    return (YmMembers)membersByIndex.size();
}

const _ym::TypeInfo::Member* _ym::TypeInfo::_Members::byIndex(size_t index) const noexcept {
    return
        index < count()
        ? membersByIndex[index].get()
        : nullptr;
}

const _ym::TypeInfo::Member* _ym::TypeInfo::_Members::byName(const std::string& name) const noexcept {
    auto it = membersByName.find(name);
    return
        it != membersByName.end()
        ? it->second.get()
        : nullptr;
}

void _ym::TypeInfo::_Members::registerMember(TypeInfo& owner, const std::string& name) {
    if (byName(name)) {
        return;
    }
    membersByIndex.push_back(std::unique_ptr<Member>(new Member{
        .index = count(),
        .name = name,
        .type = ym::deref(owner.parcel().type(std::format("{}::{}", owner.localName(), name))),
        // Pull ref constant of *this for our owner to be setup w/.
        // Using $Self::[MEMBER] here nicely accounts for things like generics.
        .typeConst = owner.consts.pullRef(Spec::typeFast(std::format("$Self::{}", name))).value(),
        }));
    membersByName.try_emplace(name, ym::Safe(membersByIndex.back().get()));
}

YmParams _ym::TypeInfo::_Call::count() const noexcept {
    return (YmParams)params.size();
}

YmParams _ym::TypeInfo::_Call::positionalCount() const noexcept {
    return positionalParamsN;
}

YmParams _ym::TypeInfo::_Call::namedCount() const noexcept {
    return count() - positionalCount();
}

const _ym::TypeInfo::Param* _ym::TypeInfo::_Call::param(YmParamIndex index) const noexcept {
    return
        index < count()
        ? &params[index]
        : nullptr;
}

const _ym::TypeInfo::Param* _ym::TypeInfo::_Call::param(const std::string& name) const noexcept {
    // NOTE: Due to strict cap of YM_MAX_PARAMS it should be fine to do an O(n) search.
    for (YmParamIndex i = 0; i < count(); i++) {
        if (auto p = param(i); p->name == name) {
            return p;
        }
    }
    return nullptr;
}

bool _ym::TypeInfo::_Call::addParam(const std::string& name, ConstIndex typeConst) {
    if (param(name)) {
        Global::raiseErr(
            YmErrCode_NameConflict,
            "Cannot add parameter; name \"{}\" already taken!",
            name);
        return false;
    }
    if (!definingNamed) {
        if (positionalCount() >= YM_MAX_POSITIONAL_PARAMS) {
            Global::raiseErr(
                YmErrCode_LimitReached,
                "Cannot add parameter; positional params would exceed {} limit!",
                YM_MAX_POSITIONAL_PARAMS);
            return false;
        }
    }
    else {
        if (namedCount() >= YM_MAX_NAMED_PARAMS) {
            Global::raiseErr(
                YmErrCode_LimitReached,
                "Cannot add parameters; named params would exceed {} limit!",
                YM_MAX_NAMED_PARAMS);
            return false;
        }
    }
    params.push_back(Param{
        .category = definingNamed ? YmParamCategory_Named : YmParamCategory_Positional,
        .index = count(),
        .name = name,
        .typeConst = typeConst,
        });
    if (!definingNamed) {
        positionalParamsN++;
    }
    return true;
}

void _ym::TypeInfo::_Call::beginNamedParams() noexcept {
    definingNamed = true;
}

bool _ym::ParcelInfo::verify() const {
    bool success = true;
    for (const auto& type : _types) {
        continue; // TODO: Add verif. checks when needed.
        success = false;
    }
    return success;
}

size_t _ym::ParcelInfo::types() const noexcept {
    return _types.size();
}

_ym::TypeInfo* _ym::ParcelInfo::type(const std::string& localName) noexcept {
    const auto it = _lookup.find(localName);
    return
        it != _lookup.end()
        ? _types[it->second].get()
        : nullptr;
}

const _ym::TypeInfo* _ym::ParcelInfo::type(const std::string& localName) const noexcept {
    const auto it = _lookup.find(localName);
    return
        it != _lookup.end()
        ? _types[it->second].get()
        : nullptr;
}

bool _ym::ParcelInfo::addType(
    KindEx k,
    const std::string& localName,
    bool skipLocalNameLegalityCheck) {
    return _registerType(_makeType(k, localName, skipLocalNameLegalityCheck));
}

bool _ym::ParcelInfo::addType(
    KindEx k,
    const std::string& ownerName,
    const std::string& memberName,
    bool skipLocalNameLegalityCheck) {
    return _registerType(_makeType(k, ownerName, memberName, skipLocalNameLegalityCheck));
}

bool _ym::ParcelInfo::addType(
    KindEx k,
    const std::string& localName,
    CallBhvrCallbackInfo callBehaviour,
    std::string returnTypeSymbol,
    std::optional<std::string> assignerSymbol,
    bool skipLocalNameLegalityCheck) {
    auto t = _makeType(k, localName, skipLocalNameLegalityCheck);
    return
        t &&
        _setupCall(*t, callBehaviour, std::move(returnTypeSymbol), std::move(assignerSymbol)) &&
        _registerType(std::move(*t));
}

bool _ym::ParcelInfo::addVarType(
    KindEx k,
    const std::string& localName,
    CallBhvrCallbackInfo callBehaviour,
    std::string returnTypeSymbol,
    std::optional<std::string> assignerSymbol,
    std::optional<std::string> initializerSymbol,
    bool skipLocalNameLegalityCheck) {
    auto t = _makeType(mustBe<YmKind_Var>(k), localName, skipLocalNameLegalityCheck);
    return
        t &&
        _setupCall(*t, callBehaviour, std::move(returnTypeSymbol), std::move(assignerSymbol)) &&
        _setupVar(*t, std::move(initializerSymbol)) &&
        _registerType(std::move(*t));
}

bool _ym::ParcelInfo::addType(
    KindEx k,
    const std::string& ownerName,
    const std::string& memberName,
    CallBhvrCallbackInfo callBehaviour,
    std::string returnTypeSymbol,
    std::optional<std::string> assignerSymbol,
    bool skipLocalNameLegalityCheck) {
    auto t = _makeType(k, ownerName, memberName, skipLocalNameLegalityCheck);
    return
        t &&
        _setupCall(*t, callBehaviour, std::move(returnTypeSymbol), std::move(assignerSymbol)) &&
        _registerType(std::move(*t));
}

std::optional<YmTypeParamIndex> _ym::ParcelInfo::addTypeParam(
    std::string typeName,
    std::string name,
    std::string constraintTypeSymbol) {
    if (auto info = _expectType(typeName, "Cannot add type parameter")) {
        if (!_checkNameLegality(name, "Cannot add type parameter")) {
            return std::nullopt;
        }
        if (!_checkNoMemberLevelNameConflict(*info, name, "Cannot add type parameter")) {
            return std::nullopt;
        }
        if (!_checkCanHaveTypeParams(*info, "Cannot add type parameter")) {
            return std::nullopt;
        }
        return info->addTypeParam(std::move(name), std::move(constraintTypeSymbol));
    }
    return std::nullopt;
}

std::optional<YmParamIndex> _ym::ParcelInfo::addParam(
    std::string typeName,
    std::string name,
    std::string paramTypeSymbol,
    bool skipCallSigChecks) {
    if (auto info = _expectType(typeName, "Cannot add parameter")) {
        if (!_checkNameLegality(name, "Cannot add parameter")) {
            return std::nullopt;
        }
        if (!skipCallSigChecks && !_checkHasCallSig(*info, "Cannot add parameter")) {
            return std::nullopt;
        }
        if (!skipCallSigChecks && !_checkHasUserDefinedCallSig(*info, "Cannot add parameter")) {
            return std::nullopt;
        }
        return info->addParam(std::move(name), std::move(paramTypeSymbol), skipCallSigChecks);
    }
    return std::nullopt;
}

void _ym::ParcelInfo::beginNamedParams(
    const std::string& typeName) {
    if (auto info = _expectType(typeName, "Cannot begin named params")) {
        if (!_checkIsntPropertyOrAssigner(*info, "Cannot begin named params")) {
            return;
        }
        info->beginNamedParams();
    }
}

std::optional<YmRef> _ym::ParcelInfo::addRef(
    std::string typeName,
    std::string symbol) {
    auto info = _expectType(typeName, "Cannot add reference");
    return
        info
        ? info->addRef(std::move(symbol))
        : std::nullopt;
}

_ym::TypeInfo* _ym::ParcelInfo::_expectType(
    const std::string& typeName,
    std::string_view msg) {
    if (_ym::TypeInfo* result = type(typeName)) {
        return result;
    }
    _ym::Global::raiseErr(
        YmErrCode_TypeNotFound,
        "{}; type {} not found!",
        (std::string)msg,
        typeName);
    return nullptr;
}

bool _ym::ParcelInfo::_checkNameLegality(
    const std::string& name,
    std::string_view msg) {
    auto err = [&]() {
        Global::raiseErr(
            YmErrCode_IllegalName,
            "{}; name \"{}\" is illegal!",
            (std::string)msg,
            name);
        };
    if (name.empty()) {
        err();
        return false;
    }
    bool first = true; // If we're at first char (ie. it cannot be a digit.)
    for (taul::decoder<char> d(taul::utf8, name); !d.done();) {
        if (auto dr = d.next()) {
            if (!taul::is_unicode(dr->cp)) {
                err();
                return false;
            }
            if (!taul::in_codepoint_range(dr->cp, U'a', U'z') &&
                !taul::in_codepoint_range(dr->cp, U'A', U'Z') &&
                !taul::in_codepoint_range(dr->cp, U'0', U'9') &&
                dr->cp != U'_' &&
                taul::is_ascii(dr->cp)) {
                err();
                return false;
            }
            if (first && taul::in_codepoint_range(dr->cp, U'0', U'9')) {
                err();
                return false;
            }
        }
        else {
            err();
            return false;
        }
        first = false;
    }
    return true;
}

bool _ym::ParcelInfo::_checkNoMemberLevelNameConflict(
    const TypeInfo& owner,
    const std::string& name,
    std::string_view msg) {
    if (name == "Self") {
        Global::raiseErr(
            YmErrCode_NameConflict,
            "{}; name \"Self\" is illegal!",
            (std::string)msg);
        return false;
    }
    else if (owner.typeParam(name)) {
        Global::raiseErr(
            YmErrCode_NameConflict,
            "{}; name \"{}\" already taken!",
            (std::string)msg,
            name);
        return false;
    }
    else if (owner.member(name)) {
        Global::raiseErr(
            YmErrCode_NameConflict,
            "{}; name \"{}\" already taken!",
            (std::string)msg,
            name);
        return false;
    }
    else return true;
}

bool _ym::ParcelInfo::_checkIsntPropertyOrAssigner(const TypeInfo& t, std::string_view msg) {
    if (t.isProperty()) {
        Global::raiseErr(
            YmErrCode_PropertyType,
            "{}; {} is a property type!",
            (std::string)msg,
            t.localName());
        return false;
    }
    if (t.isPropertyAssigner()) {
        Global::raiseErr(
            YmErrCode_PropertyAssignerType,
            "{}; {} is a property assigner type!",
            (std::string)msg,
            t.localName());
        return false;
    }
    return true;
}

bool _ym::ParcelInfo::_checkHasCallSig(const TypeInfo& t, std::string_view msg) {
    if (!t.hasCallSig()) {
        Global::raiseErr(
            YmErrCode_CallSigNotFound,
            "{}; {} call signature not found!",
            (std::string)msg,
            t.localName());
        return false;
    }
    return true;
}

bool _ym::ParcelInfo::_checkHasUserDefinedCallSig(const TypeInfo& t, std::string_view msg) {
    if (!t.hasUserDefinedCallSig()) {
        Global::raiseErr(
            YmErrCode_CallSigNotUserDefined,
            "{}; {} call signature not user-defined!",
            (std::string)msg,
            t.localName());
        return false;
    }
    return true;
}

bool _ym::ParcelInfo::_checkCanHaveTypeParams(const TypeInfo& t, std::string_view msg) {
    if (!t.canHaveTypeParams()) {
        Global::raiseErr(
            YmErrCode_TypeCannotHaveTypeParams,
            "{}; {} cannot have type parameters!",
            (std::string)msg,
            t.localName());
        return false;
    }
    return true;
}

std::optional<_ym::TypeInfo> _ym::ParcelInfo::_makeType(
    KindEx k,
    const std::string& localName,
    bool skipLocalNameLegalityCheck) {
    if (!skipLocalNameLegalityCheck && !_checkNameLegality(localName, "Cannot add type")) {
        return std::nullopt;
    }
    if (type(localName)) {
        Global::raiseErr(
            YmErrCode_NameConflict,
            "Cannot add type; name \"{}\" already taken!",
            localName);
        return std::nullopt;
    }
    return TypeInfo(*this, k, localName);
}

std::optional<_ym::TypeInfo> _ym::ParcelInfo::_makeType(
    KindEx k,
    const std::string& ownerName,
    const std::string& memberName,
    bool skipLocalNameLegalityCheck) {
    // TODO: This _checkNameLegality's error msgs will only detail the memberName, rather
    //       than the whole local name, which is somewhat suboptimal.
    if (!skipLocalNameLegalityCheck && !_checkNameLegality(memberName, "Cannot add type")) {
        return std::nullopt;
    }
    auto ownerTypePtr = type(ownerName);
    if (!ownerTypePtr) {
        Global::raiseErr(
            YmErrCode_TypeNotFound,
            "Cannot add type; owner {} not found!",
            ownerName);
        return std::nullopt;
    }
    auto& ownerType = ym::deref(ownerTypePtr);
    if (!_checkNoMemberLevelNameConflict(ownerType, memberName, "Cannot add type")) {
        return std::nullopt;
    }
    if (!ownerType.canHaveMembers()) {
        Global::raiseErr(
            YmErrCode_TypeCannotHaveMembers,
            "Cannot add type; owner {} is a {} which cannot have members!",
            ownerType.localName(),
            ymKind_Fmt(ownerType.kind()));
        return std::nullopt;
    }
    if (kindOf(k) == YmKind_Method) {
        if (!isProtocolReq(k) && ownerType.isProtocol()) {
            Global::raiseErr(
                YmErrCode_ProtocolType,
                // NOTE: Doesn't refer to KindEx notion of 'regular'.
                "Cannot add regular method to {} type {}!",
                ymKind_Fmt(ownerType.kind()),
                ownerType.localName());
            return std::nullopt;
        }
        else if (isProtocolReq(k) && !ownerType.isProtocol()) {
            Global::raiseErr(
                YmErrCode_NonProtocolType,
                "Cannot add method req. to {} type {}!",
                ymKind_Fmt(ownerType.kind()),
                ownerType.localName());
            return std::nullopt;
        }
    }
    if (kindOf(k) == YmKind_Property) {
        if (ownerType.isProtocol()) {
            Global::raiseErr(
                YmErrCode_ProtocolType,
                // NOTE: Doesn't refer to KindEx notion of 'regular'.
                "Cannot add regular property to {} type {}!",
                ymKind_Fmt(ownerType.kind()),
                ownerType.localName());
            return std::nullopt;
        }
    }
    return _makeType(
        k,
        std::format("{}::{}", ownerType.localName(), memberName),
        true);
}

bool _ym::ParcelInfo::_setupCall(
    TypeInfo& t,
    CallBhvrCallbackInfo callBehaviour,
    std::string returnTypeSymbol,
    std::optional<std::string> assignerSymbol) {
    ymAssert(t.hasCallSig());
    auto normalizedReturnTypeSym = normalizeRefSym(returnTypeSymbol,
        t.isVarLike()
        // TODO: These error msgs are *clunky*, improve them.
        ? "Cannot add type; invalid var type symbol"
        : "Cannot add type; invalid return type symbol");
    if (!normalizedReturnTypeSym) {
        return false;
    }
    // Fail quietly if assigner normalization fails, as that should mean that
    // the issue is the getter's fullname, from which the assigner was derived.
    std::optional<ConstIndex> assignerConst{};
    if (assignerSymbol) {
        if (auto normalizedAssignerTypeSym = Spec::type(*assignerSymbol)) {
            assignerConst = t.consts.pullRef(std::move(*normalizedAssignerTypeSym));
        }
    }
    t.setupCall(
        callBehaviour,
        assignerConst,
        t.consts.pullRef(std::move(*normalizedReturnTypeSym)).value());
    return true;
}

bool _ym::ParcelInfo::_setupVar(TypeInfo& t, std::optional<std::string> initializerSymbol) {
    bool hasInit = initializerSymbol.has_value();
    t.setupVar(
        hasInit
        // If initializerSymbol.has_value(), then Spec::type shouldn't be able to fail.
        ? t.consts.pullRef(Spec::type(std::move(*initializerSymbol)).value())
        : std::nullopt);
    return true;
}

bool _ym::ParcelInfo::_registerType(TypeInfo t) {
    _types.push_back(std::make_unique<TypeInfo>(std::move(t)));
    auto& result = *_types.back();
    _lookup.try_emplace(result.localName(), _types.size() - 1);
    result.registerMembershipWithOwner();
    return true;
}

bool _ym::ParcelInfo::_registerType(std::optional<TypeInfo> t) {
    return
        t
        ? _registerType(std::move(*t))
        : false;
}

