

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

bool _ym::checkCallable(const TypeInfo& type, std::string_view msg) {
    bool result = ymKind_IsCallable(type.kind);
    if (!result) {
        Global::raiseErr(
            YmErrCode_NonCallableType,
            "{}; type {} (index {}) is non-callable!",
            (std::string)msg,
            type.localName,
            type.index);
    }
    return result;
}

bool _ym::checkNonMember(const TypeInfo& type, std::string_view msg) {
    bool result = !ymKind_IsMember(type.kind);
    if (!result) {
        Global::raiseErr(
            YmErrCode_MemberType,
            "{}; type {} (index {}) is a member type!",
            (std::string)msg,
            type.localName,
            type.index);
    }
    return result;
}

bool _ym::checkNonProtocolMember(const TypeInfo& type, std::string_view msg) {
    bool result = !type.isMethodReq();
    if (!result) {
        Global::raiseErr(
            YmErrCode_ProtocolMemberType,
            "{}; type {} (index {}) is a protocol member type!",
            (std::string)msg,
            type.localName,
            type.index);
    }
    return result;
}

void _ym::methodReqCallBhvr(YmCtx* ctx, void* user) {
    YM_DEADEND;
}

void _ym::storedPropertyGetCallBhvr(YmCtx* ctx, void* user) {
    // TODO: This code semi-duplicates code in YmCtx::getProperty.
    auto slotInd = (uintptr_t)user;
    auto& subject = ym::deref(ctx->arg(0));
    ctx->ret(subject.slot(slotInd).ref, YM_BORROW);
}

void _ym::storedPropertySetCallBhvr(YmCtx* ctx, void* user) {
    // TODO: This code semi-duplicates code in YmCtx::setProperty.
    auto slotInd = (uintptr_t)user;
    auto& subject = ym::deref(ctx->arg(0));
    auto& value = ym::deref(ctx->arg(1));
    auto& target = subject.slot(slotInd).ref;
    ctx->release(ym::deref(target));
    target = &value;
    ctx->secure(value);
    ctx->ret(ctx->newNone());
}

bool _ym::TypeInfo::returnTypeIsSelf() const noexcept {
    return
        returnType
        ? consts[*returnType].as<RefInfo>().sym == "Self"
        : false;
}

bool _ym::TypeInfo::paramTypeIsSelf(YmParamIndex index) const noexcept {
    return
        size_t(index) < params.size()
        ? consts[params[index].type].as<RefInfo>().sym == "Self"
        : false;
}

bool _ym::TypeInfo::isOwner() const noexcept {
    return !memberName();
}

std::optional<std::string_view> _ym::TypeInfo::ownerName() const noexcept {
    const auto [owner, member] = split_s<YmChar>(localName, "::");
    return
        !member.empty() // If empty, then no owner/member division.
        ? std::make_optional(owner)
        : std::nullopt;
}

std::optional<std::string_view> _ym::TypeInfo::memberName() const noexcept {
    const auto [owner, member] = split_s<YmChar>(localName, "::");
    return
        !member.empty() // If empty, then no owner/member division.
        ? std::make_optional(member)
        : std::nullopt;
}

const _ym::TypeParamInfo* _ym::TypeInfo::queryTypeParam(YmTypeParamIndex index) const noexcept {
    return
        size_t(index) < typeParams.size()
        ? typeParams[size_t(index)].get()
        : nullptr;
}

const _ym::TypeParamInfo* _ym::TypeInfo::queryTypeParam(const std::string& name) const noexcept {
    if (auto it = typeParamNameMap.find(name); it != typeParamNameMap.end()) {
        return it->second;
    }
    return nullptr;
}

const _ym::ParamInfo* _ym::TypeInfo::queryParam(YmParamIndex index) const noexcept {
    return
        size_t(index) < params.size()
        ? &params[size_t(index)]
        : nullptr;
}

const _ym::ParamInfo* _ym::TypeInfo::queryParam(const std::string& localName) const noexcept {
    // NOTE: Due to strict cap of YM_MAX_PARAMS it should be fine to do an O(n) search.
    for (const auto& param : params) {
        if (param.name == localName) return &param;
    }
    return nullptr;
}

bool _ym::TypeInfo::isMethodReq() const noexcept {
    return
        kind == YmKind_Method &&
        callBehaviour &&
        callBehaviour->fn == methodReqCallBhvr;
}

bool _ym::TypeInfo::isStoredPropertyGet() const noexcept {
    return
        kind == YmKind_Property &&
        callBehaviour &&
        callBehaviour->fn == storedPropertyGetCallBhvr;
}

bool _ym::TypeInfo::isStoredPropertySet() const noexcept {
    return
        kind == YmKind_PropertyAssigner &&
        callBehaviour &&
        callBehaviour->fn == storedPropertySetCallBhvr;
}

std::optional<YmUInt16> _ym::TypeInfo::storedPropertyIndex() const noexcept {
    return
        (callBehaviour && (isStoredPropertyGet() || isStoredPropertySet()))
        ? std::make_optional((YmUInt16)(std::uintptr_t)callBehaviour->user)
        : std::nullopt;
}

YmParams _ym::TypeInfo::paramCount() const noexcept {
    return (YmParams)params.size();
}

YmParams _ym::TypeInfo::positionalParamCount() const noexcept {
    return positionalParams;
}

YmParams _ym::TypeInfo::namedParamCount() const noexcept {
    return paramCount() - positionalParamCount();
}

bool _ym::TypeInfo::isPositionalParam(YmParamIndex index) const noexcept {
    return index < positionalParamCount();
}

bool _ym::TypeInfo::isNamedParam(YmParamIndex index) const noexcept {
    return !isPositionalParam(index) && index < paramCount();
}

YmMembers _ym::TypeInfo::memberCount() const noexcept {
    return (YmMembers)membersByIndex.size();
}

YmTypeParams _ym::TypeInfo::typeParamCount() const noexcept {
    return (YmTypeParams)typeParams.size();
}

bool _ym::TypeInfo::isParameterized() const noexcept {
    return typeParamCount() >= 1;
}

bool _ym::TypeInfo::hasDefaultValue() const noexcept {
    // TODO: This logic will work for both primitive types (None, Int, UInt, Type, etc.) and
    //       for structs w/out any stored properties.
    //       HOWEVER, this logic is also QUITE HACKY, and so we'd be best to try and replace
    //       it in the future when possible.
    return
        kind == YmKind_Struct &&
        slots == 0;
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
        if (!queryTypeParam(typeParamName) && typeParamName != name) {
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
    if (typeParams.size() >= size_t(YM_MAX_TYPE_PARAMS)) {
        Global::raiseErr(
            YmErrCode_LimitReached,
            "Cannot add type parameter; would exceed {} limit!",
            YM_MAX_TYPE_PARAMS);
        return std::nullopt;
    }
    typeParams.emplace_back(std::make_shared<TypeParamInfo>(TypeParamInfo{
        .index = YmTypeParamIndex(typeParams.size()),
        .name = name,
        .constraint = consts.pullRef(std::move(normalizedConstraintTypeSym.value())).value(),
        }));
    typeParamNameMap.try_emplace(std::move(name), ym::Safe(typeParams.back().get()));
    return typeParams.back()->index;
}

std::optional<YmParamIndex> _ym::TypeInfo::addParam(std::string name, std::string paramTypeSymbol, bool skipCallabilityCheck) {
    auto normalizedParamTypeSym = normalizeRefSym(paramTypeSymbol, "Cannot add parameter");
    if (!normalizedParamTypeSym) {
        return std::nullopt;
    }
    if (!skipCallabilityCheck && !checkCallable(*this, "Cannot add parameter")) {
        return std::nullopt;
    }
    if (queryParam(name)) {
        Global::raiseErr(
            YmErrCode_NameConflict,
            "Cannot add parameter; name \"{}\" already taken!",
            name);
        return std::nullopt;
    }
    if (!usesNamedParams) {
        if (positionalParamCount() >= YM_MAX_POSITIONAL_PARAMS) {
            Global::raiseErr(
                YmErrCode_LimitReached,
                "Cannot add parameter; positional params would exceed {} limit!",
                YM_MAX_POSITIONAL_PARAMS);
            return std::nullopt;
        }
    }
    else {
        if (namedParamCount() >= YM_MAX_NAMED_PARAMS) {
            Global::raiseErr(
                YmErrCode_LimitReached,
                "Cannot add parameters; named params would exceed {} limit!",
                YM_MAX_NAMED_PARAMS);
            return std::nullopt;
        }
    }
    params.push_back(ParamInfo{
        .index = YmParamIndex(params.size()),
        .name = std::move(name),
        .type = consts.pullRef(std::move(normalizedParamTypeSym.value())).value(),
        });
    if (!usesNamedParams) {
        positionalParams++;
    }
    return params.back().index;
}

void _ym::TypeInfo::beginNamedParams() {
    if (!checkCallable(*this, "Cannot begin named params")) {
        return;
    }
    if (!checkNonProtocolMember(*this, "Cannot begin named params")) {
        return;
    }
    usesNamedParams = true;
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

void _ym::TypeInfo::attemptSetupAsMember(ParcelInfo& parcel) {
    if (isOwner()) {
        return;
    }
    if (auto owner = parcel.type((std::string)ownerName().value())) {
        auto membName = (std::string)memberName().value();
        // Bind this->owner to a ref to our new owner.
        // Using $Self here nicely accounts for things like generics.
        this->owner = consts.pullRef(Spec::typeFast("$Self")).value();
        // Pull ref constant of *this for our owner to be setup w/.
        // Using $Self::[MEMBER] here nicely accounts for things like generics.
        auto ref = owner->consts.pullRef(Spec::typeFast(std::move(std::format("$Self::{}", membName)))).value();
        // Bind ref to by-index/name lookup in *owner.
        owner->membersByIndex.push_back(ref);
        owner->membersByName.try_emplace(membName, std::move(ref)); // Move ref.
    }
}

std::string _ym::TypeInfo::fullnameForRef() const {
    return std::format("%here:{}", (std::string)localName);
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
        ? &_types[it->second]
        : nullptr;
}

const _ym::TypeInfo* _ym::ParcelInfo::type(const std::string& localName) const noexcept {
    const auto it = _lookup.find(localName);
    return
        it != _lookup.end()
        ? &_types[it->second]
        : nullptr;
}

_ym::TypeInfo* _ym::ParcelInfo::type(YmTypeIndex index) noexcept {
    return
        index < YmTypeIndex(types())
        ? &_types[index]
        : nullptr;
}

const _ym::TypeInfo* _ym::ParcelInfo::type(YmTypeIndex index) const noexcept {
    return
        index < YmTypeIndex(types())
        ? &_types[index]
        : nullptr;
}

std::optional<YmTypeIndex> _ym::ParcelInfo::addType(
    std::string localName,
    YmKind kind,
    std::optional<std::string> returnTypeSymbol,
    std::optional<std::string> assignerTypeSymbol,
    std::optional<CallBhvrCallbackInfo> callBehaviour,
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
    TypeInfo newType{
        .index = (YmTypeIndex)types(),
        .localName = localName,
        .kind = kind,
    };
    if (returnTypeSymbol) {
        ymAssert(ymKind_IsCallable(newType.kind));
        if (auto normalizedReturnTypeSym = normalizeRefSym(*returnTypeSymbol,
            // TODO: Update this check later when we add parcel-level variables.
            kind == YmKind_Property
            // TODO: These error msgs are *clunky*, improve them.
            ? "Cannot add type; invalid var type symbol"
            : "Cannot add type; invalid return type symbol")) {
            newType.returnType = newType.consts.pullRef(std::move(*normalizedReturnTypeSym));
        }
        else return std::nullopt;
    }
    if (assignerTypeSymbol) {
        // Fail quietly if normalization fails, as that should mean that the issue
        // is the getter's fullname, from which the assigner was derived.
        if (auto normalizedAssignerTypeSym = Spec::type(*assignerTypeSymbol)) {
            newType.assigner = newType.consts.pullRef(std::move(*normalizedAssignerTypeSym));
        }
    }
    newType.callBehaviour = callBehaviour;
    // NOTE: Make sure error checks stay ABOVE mutations of *this.
    _types.push_back(std::move(newType));
    auto& result = _types.back();
    // NOTE: Move localName into _lookup to avoid heap alloc.
    _lookup.try_emplace(std::move(localName), result.index);
    result.attemptSetupAsMember(*this);
    return result.index;
}

std::optional<YmTypeIndex> _ym::ParcelInfo::addType(
    std::string ownerName,
    std::string memberName,
    YmKind kind,
    std::optional<std::string> returnTypeSymbol,
    std::optional<std::string> assignerTypeSymbol,
    std::optional<CallBhvrCallbackInfo> callBehaviour,
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
    if (!ymKind_HasMembers(ownerType.kind)) {
        Global::raiseErr(
            YmErrCode_TypeCannotHaveMembers,
            "Cannot add type; owner {} is a {} which cannot have members!",
            ownerType.localName,
            ymKind_Fmt(ownerType.kind));
        return std::nullopt;
    }
    bool ownerIsProtocol = ownerType.kind == YmKind_Protocol;
    if (kind == YmKind_Method) {
        ymAssert(callBehaviour.has_value());
        bool isMethodReq = callBehaviour.value().fn == methodReqCallBhvr;
        if (!isMethodReq && ownerIsProtocol) {
            Global::raiseErr(
                YmErrCode_ProtocolType,
                "Cannot add regular method to {} type {}!",
                ymKind_Fmt(ownerType.kind),
                ownerType.localName);
            return std::nullopt;
        }
        else if (isMethodReq && !ownerIsProtocol) {
            Global::raiseErr(
                YmErrCode_NonProtocolType,
                "Cannot add method req. to {} type {}!",
                ymKind_Fmt(ownerType.kind),
                ownerType.localName);
            return std::nullopt;
        }
    }
    if (kind == YmKind_Property) {
        if (ownerIsProtocol) {
            Global::raiseErr(
                YmErrCode_ProtocolType,
                "Cannot add regular property to {} type {}!",
                ymKind_Fmt(ownerType.kind),
                ownerType.localName);
            return std::nullopt;
        }
    }
    return addType(
        std::format("{}::{}", ownerType.localName, memberName),
        kind,
        std::move(returnTypeSymbol),
        std::move(assignerTypeSymbol),
        callBehaviour,
        true);
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
        return info->addTypeParam(std::move(name), std::move(constraintTypeSymbol));
    }
    return std::nullopt;
}

std::optional<YmParamIndex> _ym::ParcelInfo::addParam(
    std::string typeName,
    std::string name,
    std::string paramTypeSymbol,
    bool skipCallabilityAndOtherRelatedCheck) {
    if (auto info = _expectType(typeName, "Cannot add parameter")) {
        if (!_checkNameLegality(name, "Cannot add parameter")) {
            return std::nullopt;
        }
        if (!skipCallabilityAndOtherRelatedCheck && !_checkIsntPropertyOrAssigner(*info, "Cannot add parameter")) {
            return std::nullopt;
        }
        return info->addParam(std::move(name), std::move(paramTypeSymbol), skipCallabilityAndOtherRelatedCheck);
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
    else if (owner.queryTypeParam(name)) {
        Global::raiseErr(
            YmErrCode_NameConflict,
            "{}; name \"{}\" already taken!",
            (std::string)msg,
            name);
        return false;
    }
    else if (owner.membersByName.contains(name)) {
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
    if (t.kind == YmKind_Property) {
        Global::raiseErr(
            YmErrCode_PropertyType,
            "{}; {} is a property type!",
            (std::string)msg,
            t.localName);
        return false;
    }
    if (t.kind == YmKind_PropertyAssigner) {
        Global::raiseErr(
            YmErrCode_PropertyAssignerType,
            "{}; {} is a property assigner type!",
            (std::string)msg,
            t.localName);
        return false;
    }
    return true;
}

