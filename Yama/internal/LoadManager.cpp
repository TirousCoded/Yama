

#include "LoadManager.h"


#define _DUMP_LOG 0

#if _DUMP_LOG
#include "../yama++/print.h"
#endif


_ym::LoadManager::LoadManager(Area& staging, PathBindings& binds, Redirects& redirects) :
    staging(staging),
    _termStk(staging, binds, redirects,
        [this](YmParcel& p, const TypeInfo& info, std::vector<ym::Safe<YmType>> typeArgs) -> ym::Safe<YmType> {
            return _genNonMemberTypeData(p, info, std::move(typeArgs));
        }) {
}

YmParcel* _ym::LoadManager::import(const Spec& path) {
    path.assertPath();
    ymAssert(!staging->parcels.fetch(path));
#if _DUMP_LOG
    ym::println("LoadManager: Importing \"{}\".", path);
#endif
    _beginImportOrLoad();
    auto result = _initialImport(path);
    _endImportOrLoad();
    return result;
}

YmType* _ym::LoadManager::load(const Spec& fullname) {
    fullname.assertType().assertNoCallSuff();
    ymAssert(!staging->types.fetch(fullname));
#if _DUMP_LOG
    ym::println("LoadManager: Loading \"{}\".", fullname);
#endif
    _beginImportOrLoad();
    auto result = _initialLoad(fullname);
    _processLateResolveQueue();
    _checkConstraintTypeLegality();
    _enforceConstraints();
    _checkRefConstCallSigConformance();
    // If late resolve or something else failed.
    if (!_good()) {
        result = nullptr;
    }
    _endImportOrLoad();
    return result;
}

void _ym::LoadManager::_beginImportOrLoad() {
    _clearFlag();
}

void _ym::LoadManager::_endImportOrLoad() {
    _flushLateResolveQueue();
}

bool _ym::LoadManager::_good() const noexcept {
    return !_failFlag;
}

void _ym::LoadManager::_fail() noexcept {
    _failFlag = true;
}

void _ym::LoadManager::_clearFlag() noexcept {
    _failFlag = false;
}

_ym::LoadManager::_GenTypeDataResult _ym::LoadManager::_genTypeData(
    ym::Safe<YmParcel> parcel,
    ym::Safe<const _ym::TypeInfo> info,
    YmType* owner,
    std::vector<ym::Safe<YmType>> typeArgs) {
    ymAssert(!owner || typeArgs.empty());
    ymAssert(bool(owner) != info->isOwner());
    // Generate our new type data.
    auto newType =
        !owner
        ? std::make_shared<YmType>(parcel, info, std::move(typeArgs))
        : std::make_shared<YmType>(parcel, info, *owner);
    // Lookup if an type w/ same fullname is already loaded, aborting upload if
    // found, quietly returning it instead.
    if (auto existing = staging->types.fetch(newType->fullname())) {
        return _GenTypeDataResult{
            .type = *existing,
            .original = false,
        };
    }
#if _DUMP_LOG
    ym::println("LoadManager: Generating {} type data.", newType->fullname());
#endif
    // Push before resolving consts to ensure that the loading resource is available
    // for lookup for resolving the consts of other types.
    if (!staging->types.push(newType)) {
        YM_DEADEND;
    }
    return _GenTypeDataResult{
        .type = *newType,
        .original = true,
    };
}

ym::Safe<YmType> _ym::LoadManager::_genNonMemberTypeData(
    ym::Safe<YmParcel> parcel,
    ym::Safe<const _ym::TypeInfo> info,
    std::vector<ym::Safe<YmType>> typeArgs) {
    ymAssert(info->isOwner());
    auto newType = _genTypeData(parcel, info, nullptr, std::move(typeArgs));
    if (newType.original) {
        _scheduleLateResolve(*newType.type);
        _genTypeDataForMembers(parcel, info, newType.type);
        _earlyResolveType(*newType.type, newType.type);
    }
    return newType.type;
}

void _ym::LoadManager::_genTypeDataForMembers(
    ym::Safe<YmParcel> parcel,
    ym::Safe<const _ym::TypeInfo> info,
    ym::Safe<YmType> self) {
    ymAssert(info->isOwner());
    for (const auto& [name, constInd] : info->membersByName) {
        _genMemberTypeData(parcel, ym::Safe(_lookupMemberInfo(parcel, *info, name)), self);
    }
}

void _ym::LoadManager::_genMemberTypeData(
    ym::Safe<YmParcel> parcel,
    ym::Safe<const _ym::TypeInfo> info,
    ym::Safe<YmType> self) {
    ymAssert(!info->isOwner());
    if (auto newType = _genTypeData(parcel, info, self); newType.original) {
        _scheduleLateResolve(*newType.type);
        _earlyResolveType(*newType.type, self);
    }
}

const _ym::TypeInfo* _ym::LoadManager::_lookupMemberInfo(
    ym::Safe<YmParcel> parcel,
    const _ym::TypeInfo& ownerInfo,
    const std::string& memberName) const {
    return parcel->type(_localNameOfMember(ownerInfo, memberName));
}

std::string _ym::LoadManager::_localNameOfMember(
    const _ym::TypeInfo& ownerInfo,
    const std::string& memberName) {
    ymAssert(ownerInfo.isOwner());
    return std::format("{}::{}", ownerInfo.localName, memberName);
}

bool _ym::LoadManager::_isEarlyResolveConst(const ConstInfo& constInfo) const {
    static const auto earlyResolvedRefSymPattern = std::regex("\\$Self(::[^\\[\\]:/]+)?$");
    return
        isVal(constInfo)
        ? true
        : std::regex_match(constInfo.as<RefInfo>().sym.string(), earlyResolvedRefSymPattern);
}

void _ym::LoadManager::_earlyResolveType(YmType& x, ym::Safe<YmType> self) {
#if _DUMP_LOG
    ym::println("LoadManager: Early resolving {} consts.", x.fullname());
#endif
    _earlyResolveConsts(x, self);
}

void _ym::LoadManager::_earlyResolveConsts(YmType& x, ym::Safe<YmType> self) {
    auto& consts = x.info->consts;
    for (ConstIndex i = 0; i < consts.size(); i++) {
        if (_isEarlyResolveConst(consts[i])) {
#if _DUMP_LOG
            ym::println("LoadManager: Resolving {} (const #{}).", fmt(consts[i]), i + 1);
#endif
            if (consts.isVal(i)) {
                x.putValConst(i);
            }
            else if (consts[i].as<RefInfo>().sym == "$Self") {
                x.putRefConst(i, self);
            }
            else {
                // NOTE: Important we use 'self->fullname()' here.
                auto memberName = std::format("{}::{}", self->fullname(), consts[i].as<RefInfo>().sym.string().substr(strlen("$Self::")));
                x.putRefConst(i, ym::Safe(staging->types.fetch(Spec::typeFast(memberName)).get()));
            }
        }
    }
}

void _ym::LoadManager::_scheduleLateResolve(YmType& x) {
#if _DUMP_LOG
    ym::println("LoadManager: Scheduling {} late resolution.", x.fullname());
#endif
    _lateResolveQueue.push(x);
}

void _ym::LoadManager::_processLateResolveQueue() {
    if (!_good()) {
        return;
    }
#if _DUMP_LOG
    ym::println("LoadManager: Processing late resolve queue.");
#endif
    // Stop processing once queue empties, or an error arises.
    while (!_lateResolveQueue.empty() && _good()) {
        _lateResolveType(*_lateResolveQueue.front());
        _lateResolveQueue.pop();
    }
}

void _ym::LoadManager::_lateResolveType(YmType& x) {
#if _DUMP_LOG
    ym::println("LoadManager: Late resolving {} consts.", x.fullname());
#endif
    _lateResolveConsts(x);
}

void _ym::LoadManager::_lateResolveConsts(YmType& x) {
    auto& consts = x.info->consts;
    for (ConstIndex i = 0; i < consts.size(); i++) {
        if (!_isEarlyResolveConst(consts[i])) {
            _lateResolveRefConst(x, i);
        }
    }
}

void _ym::LoadManager::_lateResolveRefConst(YmType& x, ConstIndex index) {
    auto& constInfo = x.info->consts[index];
#if _DUMP_LOG
    ym::println("LoadManager: Resolving {} (const #{}).", fmt(constInfo), index + 1);
#endif
    // Filter the ref sym such that redirections are handled.
    auto refSymAfterRedirects = constInfo.as<RefInfo>().sym.transformed(&x.parcel->redirects.value());
#if _DUMP_LOG
    ym::println("LoadManager: Redirecting {} -> {}.", constInfo.as<RefInfo>().sym, refSymAfterRedirects);
#endif
    _termStk.beginSession(
        // TODO: I'm really not 100% what we should provide end-user for diagnostic w/ regards to things
        //       like whether we should provide redirected ref sym, or original unredirected symbol?
        std::format("{} dependency {}", x.fullname(), refSymAfterRedirects),
        x,
        x.path(),
        *x.self());
    // We seperate out the callsuff from the symbol (if present), as that
    // stuff will be checked later.
    _termStk.fullname(refSymAfterRedirects.removeCallSuff());
    auto result = _termStk.expectConcrete();
    if (!result) {
        _fail();
    }
    x.putRefConst(index, result);
    _termStk.endSession();
}

void _ym::LoadManager::_flushLateResolveQueue() noexcept {
    while (!_lateResolveQueue.empty()) {
        _lateResolveQueue.pop();
    }
}

YmParcel* _ym::LoadManager::_initialImport(const Spec& path) {
    _termStk.beginSession(path);
    // TODO: Not 100% sure about this, as I'm thinking a more properly solution would be some kind of
    //       setup that calls _termStk.path.
    _termStk.specifier(path);
    auto result = _termStk.importParcel();
    _termStk.endSession();
    if (!result) {
        _fail();
    }
    return result;
}

YmType* _ym::LoadManager::_initialLoad(const Spec& fullname) {
    _termStk.beginSession(fullname);
    _termStk.fullname(fullname);
    auto result = _termStk.expectConcrete();
    _termStk.endSession();
    if (!result) {
        _fail();
    }
    return result;
}

void _ym::LoadManager::_checkConstraintTypeLegality() {
    if (!_good()) {
        return;
    }
#if _DUMP_LOG
    ym::println("LoadManager: Checking constraint type legality.");
#endif
    for (auto& type : staging->types) {
#if _DUMP_LOG
        ym::println("LoadManager: Checking {} type params.", type.fullname());
#endif
        for (YmTypeParamIndex i = 0; i < type.info->typeParamCount(); i++) {
            auto& typeParamConstraint = ym::deref(type.typeParamConstraint(i));
            if (typeParamConstraint.kind() != YmKind_Protocol) {
                _err(
                    YmErrCode_NonProtocolType,
                    "{} type parameter #{} ({}) constraint type {} is not a protocol!",
                    type.fullname(),
                    i + 1,
                    type.info->typeParams[i]->name,
                    typeParamConstraint.fullname());
            }
            static const auto immediateTypeParamRefConstraintRefSymPattern = std::regex("^(?!\\$Self)([^\\[:/]+)$");
            const auto& typeParamConstraintRefSym = type.info->consts[type.info->typeParams[i]->constraint].as<RefInfo>().sym;
            if (std::regex_match(typeParamConstraintRefSym.string(), immediateTypeParamRefConstraintRefSymPattern)) {
                _err(
                    YmErrCode_IllegalConstraint,
                    "{} type parameter #{} ({}) constraint type symbol {} cannot use type parameter as a constraint type (as the constraining protocol's interface would be indeterminate!)",
                    type.fullname(),
                    i + 1,
                    type.info->typeParams[i]->name,
                    typeParamConstraintRefSym);
            }
        }
    }
}

void _ym::LoadManager::_enforceConstraints() {
    if (!_good()) {
        return;
    }
#if _DUMP_LOG
    ym::println("LoadManager: Enforcing constraints.");
#endif
    for (auto& type : staging->types) {
#if _DUMP_LOG
        ym::println("LoadManager: Checking {} type args.", type.fullname());
#endif
        for (YmTypeParamIndex i = 0; i < type.info->typeParamCount(); i++) {
            auto& typeArg = ym::deref(type.typeParam(i));
            auto& typeParamConstraint = ym::deref(type.typeParamConstraint(i));
            if (!typeArg.conforms(typeParamConstraint)) {
                _err(
                    YmErrCode_TypeArgsError,
                    "{} type argument #{} ({}={}) doesn't conform to constraint {}!",
                    type.fullname(),
                    i + 1,
                    type.info->typeParams[i]->name,
                    typeArg.fullname(),
                    typeParamConstraint.fullname());
            }
        }
    }
}

void _ym::LoadManager::_checkRefConstCallSigConformance() {
    if (!_good()) {
        return;
    }
#if _DUMP_LOG
    ym::println("LoadManager: Checking ref. const callsig conformance.");
#endif
    for (auto& type : staging->types) {
#if _DUMP_LOG
        ym::println("LoadManager: Checking {} ref consts.", type.fullname());
#endif
        auto& consts = type.info->consts;
        for (size_t i = 0; i < consts.size(); i++) {
            if (auto constInfo = consts[i].tryAs<RefInfo>()) {
                // TODO: This being recomputed here, even though it was already computed when
                //       handling late resolved consts, is wasteful, so try and find a way to
                //       refactor out this recompute.
                auto& originalRefSym = constInfo->sym;
                // Filter the ref sym such that redirections are handled.
                auto refSymAfterRedirects = originalRefSym.transformed(&type.parcel->redirects.value());
#if _DUMP_LOG
                ym::println("LoadManager: Redirecting {} -> {}.", originalRefSym, refSymAfterRedirects);
#endif
                if (auto callsuff = refSymAfterRedirects.callsuff()) {
#if _DUMP_LOG
                    ym::println("LoadManager:     {}", refSymAfterRedirects);
#endif
                    auto ref = type.constAsRef(i);
                    if (ref && !ref->checkCallSuff(callsuff)) {
                        // TODO: Improve this error!
                        _err(
                            YmErrCode_TypeNotFound,
                            "{} does not conform to call suffix {}!",
                            ref->fullname(),
                            std::string(*callsuff));
                    }
                }
            }
        }
    }
}

