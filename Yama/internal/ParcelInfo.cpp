

#include "ParcelInfo.h"

#include <taul/unicode.h>

#include "../yama++/general.h"
#include "general.h"
#include "SpecSolver.h"

#include "BCodeExec.h"
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

void _ym::bcodeExecCallBhvr(YmCtx* ctx, YmType* type, void* user) {
    if (ctx && type) {
        BCodeExec(*ctx, *type->info->bcode())();
    }
    else YM_DEADEND;
}

void _ym::storedPropertyGetCallBhvr(YmCtx* ctx, YmType* type, void* user) {
    ctx->put(YM_PUSH, ctx->arg(0));
    ctx->getProperty(type, YM_PUSH);
    ctx->retObj(ctx->pull(false));
}

void _ym::storedPropertySetCallBhvr(YmCtx* ctx, YmType* type, void* user) {
    ctx->put(YM_PUSH, ctx->arg(0));
    ctx->put(YM_PUSH, ctx->arg(1));
    ctx->setProperty(type->assignee());
    ctx->retObj(ctx->newNone(false));
}

void _ym::storedVarGetCallBhvr(YmCtx* ctx, YmType* type, void* user) {
    ctx->getVar(type, YM_PUSH);
    ctx->retObj(ctx->pull(false));
}

void _ym::storedVarSetCallBhvr(YmCtx* ctx, YmType* type, void* user) {
    ctx->put(YM_PUSH, ctx->arg(0));
    ctx->setVar(type->assignee());
    ctx->retObj(ctx->newNone(false));
}

_ym::TypeInfo::TypeInfo(
    ParcelInfo& parcel,
    KindEx k,
    const std::string& localName,
    ConstTableInfo initial) :
    consts(std::move(initial)),
    _parcel(&parcel),
    _k(k),
    _localName(localName) {
    _initMembership();
    _initTypeParams();
    _initMembers();
    _initAssigner();
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

bool _ym::TypeInfo::isRefCarrier() const noexcept {
    return _ym::isRefCarrier(kindEx(), slots());
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
        return slots() == 0;
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

std::optional<_ym::ConstIndex> _ym::TypeInfo::assigneeConst() const noexcept {
    return
        _assigner
        ? std::make_optional(_assigner->assigneeConst)
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

_ym::Slots _ym::TypeInfo::slots() const noexcept {
    return slotsOf(kindEx()).value_or(_slots);
}

bool _ym::TypeInfo::checkIsRefSlot(Slots index) const noexcept {
    bool result = false;
    forEachRefSlotIndex([&result, &index](Slots i) {
        if (i == index) result = true;
        });
    return result;
}

std::optional<_ym::Slots> _ym::TypeInfo::storedPropertySlot() const noexcept {
    if (_call) {
        return _call->slot;
    }
    return std::nullopt;
}

const _ym::BCode* _ym::TypeInfo::bcode() const noexcept {
    return
        _bcode
        ? &_bcode->code
        : nullptr;
}

const _ym::BCodeDbgSyms* _ym::TypeInfo::bsyms() const noexcept {
    return
        _bcode
        ? &_bcode->syms
        : nullptr;
}

_ym::Slots _ym::TypeInfo::nextSlot() noexcept {
    ymAssert(!slotsOf(kindEx()).has_value());
    ymAssert(slots() < decltype(slots())(-1));
    _slots++;
    return _slots - 1;
}

void _ym::TypeInfo::unwindSlots(Slots n) noexcept {
    ymAssert(!slotsOf(kindEx()).has_value());
    _slots -= (n <= _slots) ? n : _slots;
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

bool _ym::TypeInfo::setupCall(
    CallBhvrCallbackInfo callBehaviour,
    const std::string& returnTypeSymbol,
    Slots slot,
    bool hasAssigner) {
    ymAssert(hasCallSig());
    if (auto returnType = _checkedRef(returnTypeSymbol)) {
        _initCall(
            callBehaviour,
            hasAssigner
            ? _uncheckedRefOpt(
                isOwner()
                ? std::format("%here:{}$assigner", localName())
                : std::format("$Self::{}$assigner", memberName()))
            : std::nullopt,
            *returnType,
            slot);
        return true;
    }
    return false;
}

bool _ym::TypeInfo::setupVar(
    bool hasInitializer) {
    if (!_var) {
        _var = std::unique_ptr<_Var>(new _Var{
            .initializerConst =
                hasInitializer
                ? _uncheckedRefOpt(
                    isOwner()
                    ? std::format("%here:{}$init", localName())
                    : std::format("$Self::{}$init", memberName()))
                : std::nullopt,
            });
    }
    return true;
}

bool _ym::TypeInfo::setupBCode(
    BCode code,
    BCodeDbgSyms syms) {
    if (!_bcode) {
        _bcode = std::unique_ptr<_BCode>(new _BCode{
            .code = std::move(code),
            .syms = std::move(syms),
            });
    }
    return true;
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

void _ym::TypeInfo::_initMembership() {
    if (isMember()) {
        auto ownerName = _extractOwnerName(localName());
        _membership = std::unique_ptr<_Membership>(new _Membership{
            .ownerName = ownerName, // Can't avoid copy here.
            .memberName = _extractMemberName(localName()),
            .owner = ym::Safe(parcel().type(ownerName)),
            // $Self here nicely accounts for things like generics.
            .ownerConst = _uncheckedRef("$Self"),
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

void _ym::TypeInfo::_initAssigner() {
    if (isSetter()) {
        _assigner = std::unique_ptr<_Assigner>(new _Assigner{
            .assigneeConst =
                isVarAssigner()
                ? _uncheckedRefFmt("%here:{}", _extractAssigneeLocalName(localName()))
                : _uncheckedRefFmt("$Self::{}", _extractMemberName(_extractAssigneeLocalName(localName()))),
            });
    }
}

void _ym::TypeInfo::_initCall(
    CallBhvrCallbackInfo callBehaviour,
    std::optional<ConstIndex> assignerConst,
    ConstIndex returnTypeConst,
    Slots slot) {
    if (!_call) {
        _call = std::unique_ptr<_Call>(new _Call{
            .callBehaviour = callBehaviour,
            .assignerConst = assignerConst,
            .returnTypeConst = returnTypeConst,
            .slot = slot,
            });
    }
}

std::optional<size_t> _ym::TypeInfo::_checkedRef(const std::string& symbol) {
    return consts.pullRef(normalizeRefSym(symbol, "Cannot add type; invalid type symbol"));
}

size_t _ym::TypeInfo::_uncheckedRef(std::string normalizedSymbol) {
    return consts.pullRef(Spec::typeFast(std::move(normalizedSymbol))).value();
}

std::optional<size_t> _ym::TypeInfo::_uncheckedRefOpt(std::string normalizedSymbol) {
    return _uncheckedRef(std::move(normalizedSymbol));
}

std::string _ym::TypeInfo::_extractOwnerName(const std::string& localName) noexcept {
    return (std::string)split_s<YmChar>(localName, "::").first;
}

std::string _ym::TypeInfo::_extractMemberName(const std::string& localName) noexcept {
    return (std::string)split_s<YmChar>(localName, "::").second;
}

std::string _ym::TypeInfo::_extractAssigneeLocalName(const std::string& localName) noexcept {
    return (std::string)split_s<YmChar>(localName, "$assigner").first;
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
        .typeConst = owner.consts.pullRef(std::format("$Self::{}", name)).value(),
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

size_t _ym::ParcelInfo::types() const noexcept {
    return _types.size();
}

_ym::TypeInfo* _ym::ParcelInfo::type(const std::string& localName) noexcept {
    const auto it = _types.find(localName);
    return
        it != _types.end()
        ? it->second.get()
        : nullptr;
}

const _ym::TypeInfo* _ym::ParcelInfo::type(const std::string& localName) const noexcept {
    const auto it = _types.find(localName);
    return
        it != _types.end()
        ? it->second.get()
        : nullptr;
}

bool _ym::ParcelInfo::addStruct(
    const std::string& name,
    KindEx k,
    ConstTableInfo initial) {
    _newNonMember(mustBe<YmKind_Struct>(k), name, false, std::move(initial));
    return _submit();
}

bool _ym::ParcelInfo::addProtocol(
    const std::string& name,
    ConstTableInfo initial) {
    _newNonMember(KindEx::Protocol, name, false, std::move(initial));
    return _submit();
}

bool _ym::ParcelInfo::addFn(
    const std::string& name,
    const std::string& returnTypeSymbol,
    _ym::CallBhvrCallbackInfo callBehaviour,
    ConstTableInfo initial) {
    _newNonMember(KindEx::Fn, name, false, std::move(initial));
    _setupCall(callBehaviour, returnTypeSymbol, -1, false);
    return _submit();
}

bool _ym::ParcelInfo::addReadOnlyStoredVar(
    const std::string& name,
    const std::string& typeSymbol,
    _ym::CallBhvrCallbackInfo initBehaviour,
    ConstTableInfo initial) {
    _newNonMember(KindEx::StoredVarGet, name, false, std::move(initial));
    _setupCall(CallBhvrCallbackInfo::mk(storedVarGetCallBhvr), typeSymbol, -1, false);
    _setupVar(true);
    if (_submit()) {
        _newNonMember(KindEx::Fn, std::format("{}$init", name), true);
        _setupCall(initBehaviour, typeSymbol, -1, false);
        _mustSucceed();
        _submit();
        return true;
    }
    return false;
}

bool _ym::ParcelInfo::addStoredVar(
    const std::string& name,
    const std::string& typeSymbol,
    _ym::CallBhvrCallbackInfo initBehaviour,
    ConstTableInfo initial) {
    _newNonMember(KindEx::StoredVarGet, name, false, std::move(initial));
    _setupCall(CallBhvrCallbackInfo::mk(storedVarGetCallBhvr), typeSymbol, -1, true);
    _setupVar(true);
    if (_submit()) {
        _newNonMember(KindEx::StoredVarSet, std::format("{}$assigner", name), true);
        _setupCall(CallBhvrCallbackInfo::mk(storedVarSetCallBhvr), "yama:None", -1, false);
        _mustSucceed();
        if (auto assigner = _submit()) {
            (void)assigner->addParam("x", typeSymbol, true).value();
        }

        _newNonMember(KindEx::Fn, std::format("{}$init", name), true);
        _setupCall(initBehaviour, typeSymbol, -1, false);
        _mustSucceed();
        _submit();

        return true;
    }
    return false;
}

bool _ym::ParcelInfo::addReadOnlyComputedVar(
    const std::string& name,
    const std::string& typeSymbol,
    _ym::CallBhvrCallbackInfo getBehaviour,
    ConstTableInfo initial) {
    _newNonMember(KindEx::Var, name, false, std::move(initial));
    _setupCall(getBehaviour, typeSymbol, -1, false);
    _setupVar(false);
    return _submit();
}

bool _ym::ParcelInfo::addComputedVar(
    const std::string& name,
    const std::string& typeSymbol,
    _ym::CallBhvrCallbackInfo getBehaviour,
    _ym::CallBhvrCallbackInfo setBehaviour,
    ConstTableInfo initial) {
    _newNonMember(KindEx::Var, name, false, std::move(initial));
    _setupCall(getBehaviour, typeSymbol, -1, true);
    _setupVar(false);
    if (_submit()) {
        _newNonMember(KindEx::VarAssigner, std::format("{}$assigner", name), true);
        _setupCall(setBehaviour, "yama:None", -1, false);
        _mustSucceed();
        if (auto assigner = _submit()) {
            (void)assigner->addParam("x", typeSymbol, true).value();
        }
        return true;
    }
    return false;
}

bool _ym::ParcelInfo::addMethod(
    const std::string& ownerName,
    const std::string& name,
    const std::string& returnTypeSymbol,
    _ym::CallBhvrCallbackInfo callBehaviour,
    ConstTableInfo initial) {
    _newMember(KindEx::Method, ownerName, name, false, std::move(initial));
    _setupCall(callBehaviour, returnTypeSymbol, -1, false);
    return _submit();
}

bool _ym::ParcelInfo::addMethodReq(
    const std::string& ownerName,
    const std::string& name,
    const std::string& returnTypeSymbol,
    ConstTableInfo initial) {
    _newMember(KindEx::MethodReq, ownerName, name, false, std::move(initial));
    _setupCall(CallBhvrCallbackInfo::mk(methodReqCallBhvr, (void*)_getNextMemberIndex(ownerName)), returnTypeSymbol, -1, false);
    return _submit();
}

bool _ym::ParcelInfo::addReadOnlyStoredProperty(
    const std::string& ownerName,
    const std::string& name,
    const std::string& typeSymbol,
    ConstTableInfo initial) {
    _newMember(KindEx::StoredPropertyGet, ownerName, name, false, std::move(initial));
    if (_curr) {
        auto& owner = ym::deref(_curr->owner());
        auto slot = owner.nextSlot();
        _setupCall(
            CallBhvrCallbackInfo::mk(storedPropertyGetCallBhvr),
            typeSymbol,
            slot,
            false);
        if (auto t = _submit()) {
            (void)t->addParam("self", "$Self", true).value();
            return true;
        }
        else owner.unwindSlots();
    }
    return false;
}

bool _ym::ParcelInfo::addStoredProperty(
    const std::string& ownerName,
    const std::string& name,
    const std::string& typeSymbol,
    ConstTableInfo initial) {
    _newMember(KindEx::StoredPropertyGet, ownerName, name, false, std::move(initial));
    if (_curr) {
        auto& owner = ym::deref(_curr->owner());
        auto slot = owner.nextSlot();
        _setupCall(
            CallBhvrCallbackInfo::mk(storedPropertyGetCallBhvr),
            typeSymbol,
            slot,
            true);
        if (auto t = _submit()) {
            (void)t->addParam("self", "$Self", true).value();

            _newMember(KindEx::StoredPropertySet, ownerName, std::format("{}$assigner", name), true);
            _setupCall(
                CallBhvrCallbackInfo::mk(storedPropertySetCallBhvr),
                "yama:None",
                -1,
                false);
            _mustSucceed();
            if (auto assigner = _submit()) {
                (void)assigner->addParam("self", "$Self", true).value();
                (void)assigner->addParam("x", typeSymbol, true).value();
            }

            return true;
        }
        else owner.unwindSlots();
    }
    return false;
}

bool _ym::ParcelInfo::addReadOnlyComputedProperty(
    const std::string& ownerName,
    const std::string& name,
    const std::string& typeSymbol,
    _ym::CallBhvrCallbackInfo getBehaviour,
    ConstTableInfo initial) {
    _newMember(KindEx::Property, ownerName, name, false, std::move(initial));
    _setupCall(getBehaviour, typeSymbol, -1, false);
    if (auto t = _submit()) {
        (void)t->addParam("self", "$Self", true).value();
        return true;
    }
    return false;
}

bool _ym::ParcelInfo::addComputedProperty(
    const std::string& ownerName,
    const std::string& name,
    const std::string& typeSymbol,
    _ym::CallBhvrCallbackInfo getBehaviour,
    _ym::CallBhvrCallbackInfo setBehaviour,
    ConstTableInfo initial) {
    _newMember(KindEx::Property, ownerName, name, false, std::move(initial));
    _setupCall(getBehaviour, typeSymbol, -1, true);
    if (auto t = _submit()) {
        (void)t->addParam("self", "$Self", true).value();

        _newMember(KindEx::PropertyAssigner, ownerName, std::format("{}$assigner", name), true);
        _setupCall(setBehaviour, "yama:None", -1, false);
        _mustSucceed();
        if (auto assigner = _submit()) {
            (void)assigner->addParam("self", "$Self", true).value();
            (void)assigner->addParam("x", typeSymbol, true).value();
        }

        return true;
    }
    return false;
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

bool _ym::ParcelInfo::bindBCode(
    const std::string& localName,
    BCode code,
    BCodeDbgSyms syms) {
    if (auto info = type(localName)) {
        return info->setupBCode(std::move(code), std::move(syms));
    }
    return false;
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
    std::string_view msg,
    bool skipLocalNameLegalityCheck) {
    auto err = [&]() {
        Global::raiseErr(
            YmErrCode_IllegalName,
            "{}; name \"{}\" is illegal!",
            (std::string)msg,
            name);
        };
    if (skipLocalNameLegalityCheck) {
        return true;
    }
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

std::unique_ptr<_ym::TypeInfo> _ym::ParcelInfo::_mkNonMember(
    KindEx k,
    const std::string& localName,
    bool skipLocalNameLegalityCheck,
    ConstTableInfo initial) {
    if (!_checkNameLegality(localName, "Cannot add type", skipLocalNameLegalityCheck)) {
        return nullptr;
    }
    if (type(localName)) {
        Global::raiseErr(
            YmErrCode_NameConflict,
            "Cannot add type; name \"{}\" already taken!",
            localName);
        return nullptr;
    }
    return std::make_unique<TypeInfo>(*this, k, localName, std::move(initial));
}

std::unique_ptr<_ym::TypeInfo> _ym::ParcelInfo::_mkMember(
    KindEx k,
    const std::string& ownerName,
    const std::string& memberName,
    bool skipLocalNameLegalityCheck,
    ConstTableInfo initial) {
    // TODO: This _checkNameLegality's error msgs will only detail the memberName, rather
    //       than the whole local name, which is somewhat suboptimal.
    if (!_checkNameLegality(memberName, "Cannot add type", skipLocalNameLegalityCheck)) {
        return nullptr;
    }
    auto ownerTypePtr = type(ownerName);
    if (!ownerTypePtr) {
        Global::raiseErr(
            YmErrCode_TypeNotFound,
            "Cannot add type; owner {} not found!",
            ownerName);
        return nullptr;
    }
    auto& ownerType = ym::deref(ownerTypePtr);
    if (!_checkNoMemberLevelNameConflict(ownerType, memberName, "Cannot add type")) {
        return nullptr;
    }
    if (!ownerType.canHaveMembers()) {
        Global::raiseErr(
            YmErrCode_TypeCannotHaveMembers,
            "Cannot add type; owner {} is a {} which cannot have members!",
            ownerType.localName(),
            ymKind_Fmt(ownerType.kind()));
        return nullptr;
    }
    if (kindOf(k) == YmKind_Method) {
        if (!isProtocolReq(k) && ownerType.isProtocol()) {
            Global::raiseErr(
                YmErrCode_ProtocolType,
                // NOTE: Doesn't refer to KindEx notion of 'regular'.
                "Cannot add regular method to {} type {}!",
                ymKind_Fmt(ownerType.kind()),
                ownerType.localName());
            return nullptr;
        }
        else if (isProtocolReq(k) && !ownerType.isProtocol()) {
            Global::raiseErr(
                YmErrCode_NonProtocolType,
                "Cannot add method req. to {} type {}!",
                ymKind_Fmt(ownerType.kind()),
                ownerType.localName());
            return nullptr;
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
            return nullptr;
        }
        if (k == KindEx::StoredPropertyGet && ownerType.slots() >= YM_MAX_STORED_PROPERTIES) {
            Global::raiseErr(
                YmErrCode_LimitReached,
                "Cannot add stored property to {} type {}; would exceed {} limit!",
                ymKind_Fmt(ownerType.kind()),
                ownerType.localName(),
                YM_MAX_STORED_PROPERTIES);
            return nullptr;
        }
    }
    return std::make_unique<TypeInfo>(*this, k,
        std::format("{}::{}", ownerType.localName(), memberName),
        std::move(initial));
}

_ym::TypeInfo* _ym::ParcelInfo::_registerType(std::unique_ptr<TypeInfo> t) {
    if (!t) {
        return nullptr;
    }
    auto& result = *t;
    _types.try_emplace(result.localName(), std::move(t));
    result.registerMembershipWithOwner();
    return &result;
}

thread_local std::unique_ptr<_ym::TypeInfo> _ym::ParcelInfo::_curr = nullptr;

void _ym::ParcelInfo::_newNonMember(
    KindEx k,
    const std::string& localName,
    bool skipLocalNameLegalityCheck,
    ConstTableInfo initial) {
    _curr = _mkNonMember(k, localName, skipLocalNameLegalityCheck, std::move(initial));
}

void _ym::ParcelInfo::_newMember(
    KindEx k,
    const std::string& ownerName,
    const std::string& memberName,
    bool skipLocalNameLegalityCheck,
    ConstTableInfo initial) {
    _curr = _mkMember(k, ownerName, memberName, skipLocalNameLegalityCheck, std::move(initial));
}

void _ym::ParcelInfo::_setupCall(
    CallBhvrCallbackInfo callBehaviour,
    const std::string& returnTypeSymbol,
    Slots slot,
    bool hasAssigner) {
    if (_curr) {
        if (!_curr->setupCall(callBehaviour, returnTypeSymbol, slot, hasAssigner)) {
            _curr.reset();
        }
    }
}

void _ym::ParcelInfo::_setupVar(bool hasInitializer) {
    if (_curr) {
        if (!_curr->setupVar(hasInitializer)) {
            _curr.reset();
        }
    }
}

void _ym::ParcelInfo::_bindBCode(BCode code, BCodeDbgSyms syms) {
    if (_curr) {
        if (!_curr->setupBCode(std::move(code), std::move(syms))) {
            _curr.reset();
        }
    }
}

void _ym::ParcelInfo::_mustSucceed() {
    ymVerify(_curr != nullptr);
}

_ym::TypeInfo* _ym::ParcelInfo::_submit() {
    return _registerType(std::move(_curr));
}

uintptr_t _ym::ParcelInfo::_getNextMemberIndex(const std::string& ownerName) const noexcept {
    auto owner = type(ownerName);
    return owner ? owner->members() : -1;
}

