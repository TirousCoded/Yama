

#include "LoadManager.h"


#define _DUMP_LOG 0

#if _DUMP_LOG
#include "../yama++/print.h"
#endif


_ym::LoadManager::LoadManager(Area& staging, PathBindings& binds, Redirects& redirects) :
    staging(staging),
    _termStk(staging, binds, redirects,
        [this](YmParcel& p, const ItemInfo& info, std::vector<ym::Safe<YmItem>> itemArgs) -> ym::Safe<YmItem> {
            return _genNonMemberItemData(p, info, std::move(itemArgs));
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

YmItem* _ym::LoadManager::load(const Spec& fullname) {
    fullname.assertItem().assertNoCallSuff();
    ymAssert(!staging->items.fetch(fullname));
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

_ym::LoadManager::_GenItemDataResult _ym::LoadManager::_genItemData(
    ym::Safe<YmParcel> parcel,
    ym::Safe<const _ym::ItemInfo> info,
    YmItem* owner,
    std::vector<ym::Safe<YmItem>> itemArgs) {
    ymAssert(!owner || itemArgs.empty());
    ymAssert(bool(owner) != info->isOwner());
    // Generate our new item data.
    auto newItem =
        !owner
        ? std::make_shared<YmItem>(parcel, info, std::move(itemArgs))
        : std::make_shared<YmItem>(parcel, info, *owner);
    // Lookup if an item w/ same fullname is already loaded, aborting upload if
    // found, quietly returning it instead.
    if (auto existing = staging->items.fetch(newItem->fullname())) {
        return _GenItemDataResult{
            .item = *existing,
            .original = false,
        };
    }
#if _DUMP_LOG
    ym::println("LoadManager: Generating {} item data.", newItem->fullname());
#endif
    // Push before resolving consts to ensure that the loading resource is available
    // for lookup for resolving the consts of other items.
    if (!staging->items.push(newItem)) {
        YM_DEADEND;
    }
    return _GenItemDataResult{
        .item = *newItem,
        .original = true,
    };
}

ym::Safe<YmItem> _ym::LoadManager::_genNonMemberItemData(
    ym::Safe<YmParcel> parcel,
    ym::Safe<const _ym::ItemInfo> info,
    std::vector<ym::Safe<YmItem>> itemArgs) {
    ymAssert(info->isOwner());
    auto newItem = _genItemData(parcel, info, nullptr, std::move(itemArgs));
    if (newItem.original) {
        _scheduleLateResolve(*newItem.item);
        _genItemDataForMembers(parcel, info, newItem.item);
        _earlyResolveItem(*newItem.item, newItem.item);
    }
    return newItem.item;
}

void _ym::LoadManager::_genItemDataForMembers(
    ym::Safe<YmParcel> parcel,
    ym::Safe<const _ym::ItemInfo> info,
    ym::Safe<YmItem> self) {
    ymAssert(info->isOwner());
    for (const auto& [name, constInd] : info->membersByName) {
        _genMemberItemData(parcel, ym::Safe(_lookupMemberInfo(parcel, *info, name)), self);
    }
}

void _ym::LoadManager::_genMemberItemData(
    ym::Safe<YmParcel> parcel,
    ym::Safe<const _ym::ItemInfo> info,
    ym::Safe<YmItem> self) {
    ymAssert(!info->isOwner());
    if (auto newItem = _genItemData(parcel, info, self); newItem.original) {
        _scheduleLateResolve(*newItem.item);
        _earlyResolveItem(*newItem.item, self);
    }
}

const _ym::ItemInfo* _ym::LoadManager::_lookupMemberInfo(
    ym::Safe<YmParcel> parcel,
    const _ym::ItemInfo& ownerInfo,
    const std::string& memberName) const {
    return parcel->item(_localNameOfMember(ownerInfo, memberName));
}

std::string _ym::LoadManager::_localNameOfMember(
    const _ym::ItemInfo& ownerInfo,
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

void _ym::LoadManager::_earlyResolveItem(YmItem& x, ym::Safe<YmItem> self) {
#if _DUMP_LOG
    ym::println("LoadManager: Early resolving {} consts.", x.fullname());
#endif
    _earlyResolveConsts(x, self);
}

void _ym::LoadManager::_earlyResolveConsts(YmItem& x, ym::Safe<YmItem> self) {
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
                x.putRefConst(i, ym::Safe(staging->items.fetch(Spec::itemFast(memberName)).get()));
            }
        }
    }
}

void _ym::LoadManager::_scheduleLateResolve(YmItem& x) {
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
        _lateResolveItem(*_lateResolveQueue.front());
        _lateResolveQueue.pop();
    }
}

void _ym::LoadManager::_lateResolveItem(YmItem& x) {
#if _DUMP_LOG
    ym::println("LoadManager: Late resolving {} consts.", x.fullname());
#endif
    _lateResolveConsts(x);
}

void _ym::LoadManager::_lateResolveConsts(YmItem& x) {
    auto& consts = x.info->consts;
    for (ConstIndex i = 0; i < consts.size(); i++) {
        if (!_isEarlyResolveConst(consts[i])) {
            _lateResolveRefConst(x, i);
        }
    }
}

void _ym::LoadManager::_lateResolveRefConst(YmItem& x, ConstIndex index) {
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

YmItem* _ym::LoadManager::_initialLoad(const Spec& fullname) {
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
    for (auto& item : staging->items) {
#if _DUMP_LOG
        ym::println("LoadManager: Checking {} item params.", item.fullname());
#endif
        for (YmItemParamIndex i = 0; i < item.info->itemParamCount(); i++) {
            auto& itemParamConstraint = ym::deref(item.itemParamConstraint(i));
            if (itemParamConstraint.kind() != YmKind_Protocol) {
                _err(
                    YmErrCode_NonProtocolItem,
                    "{} item parameter #{} ({}) constraint type {} is not a protocol!",
                    item.fullname(),
                    i + 1,
                    item.info->itemParams[i]->name,
                    itemParamConstraint.fullname());
            }
            static const auto immediateItemParamRefConstraintRefSymPattern = std::regex("^(?!\\$Self)([^\\[:/]+)$");
            const auto& itemParamConstraintRefSym = item.info->consts[item.info->itemParams[i]->constraint].as<RefInfo>().sym;
            if (std::regex_match(itemParamConstraintRefSym.string(), immediateItemParamRefConstraintRefSymPattern)) {
                _err(
                    YmErrCode_IllegalConstraint,
                    "{} item parameter #{} ({}) constraint type symbol {} cannot use item parameter as a constraint type (as the constraining protocol's interface would be indeterminate!)",
                    item.fullname(),
                    i + 1,
                    item.info->itemParams[i]->name,
                    itemParamConstraintRefSym);
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
    for (auto& item : staging->items) {
#if _DUMP_LOG
        ym::println("LoadManager: Checking {} item args.", item.fullname());
#endif
        for (YmItemParamIndex i = 0; i < item.info->itemParamCount(); i++) {
            auto& itemArg = ym::deref(item.itemParam(i));
            auto& itemParamConstraint = ym::deref(item.itemParamConstraint(i));
            if (!itemArg.conforms(itemParamConstraint)) {
                _err(
                    YmErrCode_ItemArgsError,
                    "{} item argument #{} ({}={}) doesn't conform to constraint {}!",
                    item.fullname(),
                    i + 1,
                    item.info->itemParams[i]->name,
                    itemArg.fullname(),
                    itemParamConstraint.fullname());
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
    for (auto& item : staging->items) {
#if _DUMP_LOG
        ym::println("LoadManager: Checking {} ref consts.", item.fullname());
#endif
        auto& consts = item.info->consts;
        for (size_t i = 0; i < consts.size(); i++) {
            if (auto constInfo = consts[i].tryAs<RefInfo>()) {
                // TODO: This being recomputed here, even though it was already computed when
                //       handling late resolved consts, is wasteful, so try and find a way to
                //       refactor out this recompute.
                auto& originalRefSym = constInfo->sym;
                // Filter the ref sym such that redirections are handled.
                auto refSymAfterRedirects = originalRefSym.transformed(&item.parcel->redirects.value());
#if _DUMP_LOG
                ym::println("LoadManager: Redirecting {} -> {}.", originalRefSym, refSymAfterRedirects);
#endif
                if (auto callsuff = refSymAfterRedirects.callsuff()) {
#if _DUMP_LOG
                    ym::println("LoadManager:     {}", refSymAfterRedirects);
#endif
                    auto ref = item.constAsRef(i);
                    if (ref && !ref->checkCallSuff(callsuff)) {
                        // TODO: Improve this error!
                        _err(
                            YmErrCode_ItemNotFound,
                            "{} does not conform to call suffix {}!",
                            ref->fullname(),
                            std::string(*callsuff));
                    }
                }
            }
        }
    }
}

