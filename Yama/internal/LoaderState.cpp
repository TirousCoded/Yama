

#include "LoaderState.h"

#include "general.h"
#include "SpecSolver.h"


#define _DUMP_LOG 0

#if _DUMP_LOG
#include "../yama++/print.h"
#endif


_ym::LoaderState::LoaderState(ym::Safe<Area> staging, ym::Safe<PathBindings> binds) :
    _staging(staging),
    _binds(binds) {
}

YmParcel* _ym::LoaderState::import(const std::string& normalizedPath) {
    assertNormalNonCallSig(normalizedPath);
    ymAssert(!_staging->parcels.fetch(normalizedPath));
#if _DUMP_LOG
    ym::println("LoaderState: Importing \"{}\".", normalizedPath);
#endif
    _beginLoadOrImport(normalizedPath, false);
    eval(normalizedPath);
    // TODO: parcel() here crashes if above eval call fails, as the term
    //       stack won't be setup correctly, and it doesn't fail quietly.
    auto result = good() ? parcel() : nullptr;
    popAll();
    _endLoadOrImport();
    return result;
}

YmItem* _ym::LoaderState::load(const std::string& normalizedFullname) {
    assertNormalNonCallSig(normalizedFullname);
    ymAssert(!_staging->items.fetch(normalizedFullname));
#if _DUMP_LOG
    ym::println("LoaderState: Loading \"{}\".", normalizedFullname);
#endif
    _beginLoadOrImport(normalizedFullname, true);
    eval(normalizedFullname);
    // TODO: concrete() here crashes if above eval call fails, as the term
    //       stack won't be setup correctly, and it doesn't fail quietly.
    auto result0 = good() ? concrete() : nullptr;
    popAll();
    _processLateResolveQueue();
    _checkConstraintTypeLegality();
    _enforceConstraints();
    _checkRefConstCallSigConformance();
    // TODO: 'good()' check here in case late resolve and/or constraint checks fail.
    auto result1 = good() ? result0 : nullptr;
    _endLoadOrImport();
    return result1;
}

_ym::LoaderState::Mode _ym::LoaderState::mode() const noexcept {
    return _mode;
}

const _ym::LoaderState::Env* _ym::LoaderState::env() const noexcept {
    return _env ? &*_env : nullptr;
}

const std::string& _ym::LoaderState::errPrefix() const noexcept {
    return _errPrefix;
}

bool _ym::LoaderState::good() const noexcept {
    return !_failureFlag;
}

void _ym::LoaderState::fail() noexcept {
    _failureFlag = true;
}

void _ym::LoaderState::clearFlag() noexcept {
    _failureFlag = false;
}

void _ym::LoaderState::assertHeight(size_t min) const noexcept {
    ymAssert(height() >= min);
}

size_t _ym::LoaderState::height() const noexcept {
    return _terms.size();
}

const _ym::LoaderState::Term* _ym::LoaderState::top() const noexcept {
    return height() >= 1 ? &_terms.back() : nullptr;
}

std::span<const _ym::LoaderState::Term> _ym::LoaderState::topN(size_t n) const noexcept {
    return std::span(_terms).subspan(_terms.size() - std::min(n, height()));
}

ym::Safe<const _ym::LoaderState::Term> _ym::LoaderState::expect() const noexcept {
    return ym::Safe(top());
}

std::span<const _ym::LoaderState::Term> _ym::LoaderState::expectN(size_t n) const noexcept {
    ymAssert(topN(n).size() == n);
    return topN(n);
}

void _ym::LoaderState::push(Term x) {
#if _DUMP_LOG
    size_t old = height();
#endif
    _terms.push_back(std::move(x));
#if _DUMP_LOG
    _printTermStk(old);
#endif
}

void _ym::LoaderState::pop(size_t n) {
#if _DUMP_LOG
    size_t old = height();
#endif
    _terms.resize(_terms.size() - std::min(n, height()));
#if _DUMP_LOG
    if (n >= 1) {
        _printTermStk(old);
    }
#endif
}

void _ym::LoaderState::popAll() {
    pop(height());
}

void _ym::LoaderState::transact(size_t n, Term x) {
    pop(n);
    push(std::move(x));
}

void _ym::LoaderState::eval(const std::string& pathOrFullname) {
#if _DUMP_LOG
    ym::println("LoaderState: {} {}", __func__, pathOrFullname);
#endif
    _Interp(*this)(pathOrFullname);
}

void _ym::LoaderState::normalizedPath(const std::string& normalizedPath) {
#if _DUMP_LOG
    ym::println("LoaderState: {} {}", __func__, normalizedPath);
#endif
    assertNormalNonCallSig(normalizedPath);
    push(Term(normalizedPath));
}

void _ym::LoaderState::normalizedFullname(const std::string& normalizedFullname) {
#if _DUMP_LOG
    ym::println("LoaderState: {} {}", __func__, normalizedFullname);
#endif
    assertNormalNonCallSig(normalizedFullname);
    if (auto result = _staging->items.fetch(normalizedFullname)) {
        push(Term(ym::Safe(result.get())));
    }
    else {
        eval(normalizedFullname);
        // TODO: Figure out way to remove this 'good' check.
        if (good()) item();
    }
}

YmParcel* _ym::LoaderState::parcel() {
#if _DUMP_LOG
    ym::println("LoaderState: {}", __func__);
#endif
    auto path = expect();
    if (!path->isPath()) {
        err(
            YmErrCode_IllegalSpecifier,
            "{}; \"{}\" is not an import path!",
            errPrefix(),
            path->fmt());
        return nullptr;
    }
    else {
        return _import(path->path().value());
    }
}

bool _ym::LoaderState::item() {
#if _DUMP_LOG
    ym::println("LoaderState: {}", __func__);
#endif
    auto item = expect();
    if (item->isPath()) {
        err(
            // TODO: Not 100% sure what error this should be.
            YmErrCode_InternalError,
            "{}; \"{}\" is not an item!",
            errPrefix(),
            item->fmt());
        return false;
    }
    return true;
}

YmItem* _ym::LoaderState::concrete() {
#if _DUMP_LOG
    ym::println("LoaderState: {}", __func__);
#endif
    if (!item()) {
        return nullptr;
    }
    auto item = expect();
    if (item->isGeneric()) {
        err(
            YmErrCode_GenericItem,
            "{}; \"{}\" is a generic item!",
            errPrefix(),
            item->fmt());
        return nullptr;
    }
    return item->concrete();
}

bool _ym::LoaderState::generic() {
#if _DUMP_LOG
    ym::println("LoaderState: {}", __func__);
#endif
    if (!item()) {
        return false;
    }
    auto item = expect();
    if (item->isConcrete()) {
        err(
            YmErrCode_ConcreteItem,
            "{}; \"{}\" is a concrete item!",
            errPrefix(),
            item->fmt());
        return false;
    }
    return true;
}

void _ym::LoaderState::here() {
#if _DUMP_LOG
    ym::println("LoaderState: {}", __func__);
#endif
    if (!env()) {
        err(
            YmErrCode_ParcelNotFound,
            "{}; %here illegal!",
            errPrefix());
    }
    else {
        push(Term(env()->here));
    }
}

void _ym::LoaderState::root(const std::string& id) {
#if _DUMP_LOG
    ym::println("LoaderState: {} {}", __func__, id);
#endif
    push(Term(id));
}

void _ym::LoaderState::submod(const std::string& id) {
#if _DUMP_LOG
    ym::println("LoaderState: {} {}", __func__, id);
#endif
    auto mod = expect();
    if (!mod->isPath()) {
        err(
            YmErrCode_IllegalSpecifier,
            "{}; \"{}\" is not an import path!",
            errPrefix(),
            id,
            mod->fmt());
    }
    else {
        transact(1, Term(std::format("{}/{}", mod->fmt(), id)));
    }
}

void _ym::LoaderState::self() {
#if _DUMP_LOG
    ym::println("LoaderState: {}", __func__);
#endif
    if (!env()) {
        err(
            YmErrCode_ItemNotFound,
            "{}; $Self illegal!",
            errPrefix());
    }
    else {
        push(Term(env()->self));
    }
}

void _ym::LoaderState::itemParam(const std::string& id) {
#if _DUMP_LOG
    ym::println("LoaderState: {} {}", __func__, id);
#endif
    if (!env()) {
        err(
            YmErrCode_ItemNotFound,
            "{}; ${} illegal!",
            errPrefix(),
            id);
    }
    else if (auto arg = env()->self->itemParam(id)) {
        push(Term(ym::Safe(arg)));
    }
    else {
        err(
            YmErrCode_ItemNotFound,
            "{}; ${} illegal!",
            errPrefix(),
            id);
    }
}

void _ym::LoaderState::itemInParcel(const std::string& id) {
#if _DUMP_LOG
    ym::println("LoaderState: {} {}", __func__, id);
#endif
    if (auto p = parcel()) {
        if (auto info = p->item(id)) {
            transact(
                1,
                !info->isParameterized()
                ? Term(_genNonMemberItemData(*p, *info))    // Concrete (ie. and w/out item params.)
                : Term(p->path, *info));                    // Generic
        }
        else {
            err(
                YmErrCode_ItemNotFound,
                "{}; {} contains no item \"{}\"!",
                errPrefix(),
                p->path,
                id);
        }
    }
}

void _ym::LoaderState::member(const std::string& id) {
#if _DUMP_LOG
    ym::println("LoaderState: {} {}", __func__, id);
#endif
    auto owner = expect();
    if (owner->isPath()) {
        err(
            // TODO: Not 100% sure what error this should be.
            YmErrCode_InternalError,
            "{}; \"{}\" is not an item!",
            errPrefix(),
            owner->fmt());
    }
    else if (owner->isGeneric()) {
        // TODO: I'm not 100% about this error case, nor do I think we have
        //       this covered in our unit tests.
        //
        //       Also, beginArgs/endArgs will need to be considered when we
        //       look into this w/ regards to revising our code.
        err(
            YmErrCode_GenericItem,
            "{}; \"{}\" is not a concrete item!",
            errPrefix(),
            owner->fmt());
    }
    else if (!owner->hasMembers()) {
        err(
            YmErrCode_ItemCannotHaveMembers,
            "{}; \"{}\" has no members!",
            errPrefix(),
            owner->fmt());
    }
    else if (!owner->hasMember(id)) {
        err(
            YmErrCode_ItemNotFound,
            "{}; \"{}\" has no member \"{}\"!",
            errPrefix(),
            owner->fmt(),
            id);
    }
    else {
        transact(1, Term(ym::Safe(ym::deref(owner->concrete()).member(id))));
    }
}

void _ym::LoaderState::beginArgs() {
#if _DUMP_LOG
    ym::println("LoaderState: {}", __func__);
#endif
    auto item = expect();
    if (item->isPath()) {
        err(
            // TODO: Not 100% sure what error this should be.
            YmErrCode_InternalError,
            "{}; \"{}\" is not an item!",
            errPrefix(),
            item->fmt());
    }
    else if (item->isConcrete()) {
        err(
            YmErrCode_ConcreteItem,
            "{}; \"{}\" is not a generic item!",
            errPrefix(),
            item->fmt());
    }
    else {
        _terms.back().awaitingArgs = true;
    }
}

void _ym::LoaderState::endArgs() {
#if _DUMP_LOG
    ym::println("LoaderState: {}", __func__);
#endif
    auto inputs = expectN(_countInputsToEndArgs().value()); // Fails if no corresponding beginArgs call.
    auto& generic = inputs[0];
    auto args = inputs.subspan(1);
    auto fmtConstructed = [&]() -> std::string {
        std::string argPack{};
        bool first = true;
        for (const auto& arg : args) {
            if (!first) {
                argPack += ", ";
            }
            argPack += arg.fmt();
            first = false;
        }
        return std::format("{}[{}]", generic.fmt(), argPack);
        };
    if (generic.isPath()) {
        err(
            // TODO: Not 100% sure what error this should be.
            YmErrCode_InternalError,
            "{}; \"{}\" is not an item!",
            errPrefix(),
            generic.fmt());
        return;
    }
    if (generic.isConcrete()) {
        err(
            YmErrCode_ConcreteItem,
            "{}; \"{}\" is not a generic item!",
            errPrefix(),
            generic.fmt());
        return;
    }
    if (args.size() != generic.itemParamCount()) {
        err(
            YmErrCode_ItemArgsError,
            "{}; \"{}\" has {} item argument(s), but expects {}!",
            errPrefix(),
            fmtConstructed(),
            args.size(),
            generic.itemParamCount());
        return;
    }
    bool badArgs = false;
    for (size_t i = 0; i < args.size(); i++) {
        auto& itemParamName = ym::deref(generic.info()).itemParams[i]->name;
        auto& arg = args[i];
        if (arg.isPath()) {
            badArgs = true;
            err(
                // TODO: Not 100% sure what error this should be.
                YmErrCode_InternalError,
                "{}; \"{}\" item argument #{} ({}=\"{}\") is not an item!",
                errPrefix(),
                fmtConstructed(),
                i + 1,
                itemParamName,
                arg.fmt());
            continue;
        }
        if (arg.isGeneric()) {
            badArgs = true;
            err(
                YmErrCode_GenericItem,
                "{}; \"{}\" item argument #{} ({}=\"{}\") is not a concrete item!",
                errPrefix(),
                fmtConstructed(),
                i + 1,
                itemParamName,
                arg.fmt());
            continue;
        }
    }
    if (badArgs) {
        return;
    }
    std::vector<ym::Safe<YmItem>> itemArgs{};
    for (const auto& arg : args) {
        itemArgs.push_back(ym::Safe(arg.item()));
    }
    transact(
        inputs.size(),
        Term(_genNonMemberItemData(
            ym::Safe(_import(generic.path().value())),
            ym::deref(generic.info()),
            std::move(itemArgs))));
}

void _ym::LoaderState::_beginLoadOrImport(const std::string& normalizedSpecifier, bool loadNotImport) {
    assertNormal(normalizedSpecifier);
    ymAssert(_lateResolveQueue.empty());
    clearFlag();
    if (loadNotImport)  _setupForFullname(normalizedSpecifier);
    else                _setupForImportPath(normalizedSpecifier);
}

void _ym::LoaderState::_endLoadOrImport() {
    while (!_lateResolveQueue.empty()) {
        _lateResolveQueue.pop();
    }
}

void _ym::LoaderState::_setupForSpecifier(Mode mode, std::optional<Env> env, std::string errPrefix) {
    _terms.clear();
    _mode = mode;
    _env = std::move(env);
    _errPrefix = std::move(errPrefix);
}

void _ym::LoaderState::_setupForImportPath(const std::string& normalizedPath) {
    assertNormal(normalizedPath);
    _setupForSpecifier(Mode::Path, std::nullopt, std::format("{} import failed", normalizedPath));
}

void _ym::LoaderState::_setupForFullname(const std::string& normalizedFullname) {
    assertNormal(normalizedFullname);
    _setupForSpecifier(Mode::Fullname, std::nullopt, std::format("{} load failed", normalizedFullname));
}

void _ym::LoaderState::_setupForRefSym(const std::string& normalizedRefSym, Env env) {
    assertNormal(normalizedRefSym);
    auto errPrefix = std::format("Dependency (of {}) {} load failed", env.self->fullname(), normalizedRefSym);
    _setupForSpecifier(Mode::RefSym, std::move(env), errPrefix);
}

_ym::LoaderState::_GenItemDataResult _ym::LoaderState::_genItemData(
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
    if (auto existing = _staging->items.fetch(newItem->fullname())) {
        return _GenItemDataResult{
            .item = *existing,
            .original = false,
        };
    }
#if _DUMP_LOG
    ym::println("LoaderState: Generating {} item data.", newItem->fullname());
#endif
    // Push before resolving consts to ensure that the loading resource is available
    // for lookup for resolving the consts of other items.
    if (!_staging->items.push(newItem)) {
        YM_DEADEND;
    }
    return _GenItemDataResult{
        .item = *newItem,
        .original = true,
    };
}

ym::Safe<YmItem> _ym::LoaderState::_genNonMemberItemData(
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

void _ym::LoaderState::_genItemDataForMembers(
    ym::Safe<YmParcel> parcel,
    ym::Safe<const _ym::ItemInfo> info,
    ym::Safe<YmItem> self) {
    ymAssert(info->isOwner());
    for (const auto& [name, constInd] : info->membersByName) {
        _genMemberItemData(parcel, ym::Safe(_lookupMemberInfo(parcel, *info, name)), self);
    }
}

void _ym::LoaderState::_genMemberItemData(
    ym::Safe<YmParcel> parcel,
    ym::Safe<const _ym::ItemInfo> info,
    ym::Safe<YmItem> self) {
    ymAssert(!info->isOwner());
    if (auto newItem = _genItemData(parcel, info, self); newItem.original) {
        _scheduleLateResolve(*newItem.item);
        _earlyResolveItem(*newItem.item, self);
    }
}

const _ym::ItemInfo* _ym::LoaderState::_lookupMemberInfo(
    ym::Safe<YmParcel> parcel,
    const _ym::ItemInfo& ownerInfo,
    const std::string& memberName) const {
    return parcel->item(_localNameOfMember(ownerInfo, memberName));
}

std::string _ym::LoaderState::_localNameOfMember(
    const _ym::ItemInfo& ownerInfo,
    const std::string& memberName) {
    ymAssert(ownerInfo.isOwner());
    return std::format("{}::{}", ownerInfo.localName, memberName);
}

bool _ym::LoaderState::_isEarlyResolveConst(const ConstInfo& constInfo) const {
    static const auto earlyResolvedRefSymPattern = std::regex("\\$Self(::[^\\[\\]:/]+)?$");
    return
        isVal(constInfo)
        ? true
        : std::regex_match(constInfo.as<RefInfo>().sym, earlyResolvedRefSymPattern);
}

void _ym::LoaderState::_earlyResolveItem(YmItem& x, ym::Safe<YmItem> self) {
#if _DUMP_LOG
    ym::println("LoaderState: Early resolving {} consts.", x.fullname());
#endif
    _earlyResolveConsts(x, self);
}

void _ym::LoaderState::_earlyResolveConsts(YmItem& x, ym::Safe<YmItem> self) {
    auto& consts = x.info->consts;
    for (ConstIndex i = 0; i < consts.size(); i++) {
        if (_isEarlyResolveConst(consts[i])) {
#if _DUMP_LOG
            ym::println("LoaderState: Resolving {} (const #{}).", fmt(consts[i]), i + 1);
#endif
            if (consts.isVal(i)) {
                x.putValConst(i);
            }
            else if (consts[i].as<RefInfo>().sym == "$Self") {
                x.putRefConst(i, self);
            }
            else {
                // NOTE: Important we use 'self->fullname()' here.
                auto memberName = std::format("{}::{}", self->fullname(), consts[i].as<RefInfo>().sym.substr(strlen("$Self::")));
                x.putRefConst(i, ym::Safe(_staging->items.fetch(memberName).get()));
            }
        }
    }
}

void _ym::LoaderState::_scheduleLateResolve(YmItem& x) {
#if _DUMP_LOG
    ym::println("LoaderState: Scheduling {} late resolution.", x.fullname());
#endif
    _lateResolveQueue.push(x);
}

void _ym::LoaderState::_processLateResolveQueue() {
    if (!good()) {
        return;
    }
#if _DUMP_LOG
    ym::println("LoaderState: Processing late resolve queue.");
#endif
    // Stop processing once queue empties, or an error arises.
    while (!_lateResolveQueue.empty() && good()) {
        _lateResolveItem(*_lateResolveQueue.front());
        _lateResolveQueue.pop();
    }
}

void _ym::LoaderState::_lateResolveItem(YmItem& x) {
#if _DUMP_LOG
    ym::println("LoaderState: Late resolving {} consts.", x.fullname());
#endif
    _setupForRefSym(
        x.fullname(),
        Env{
            .item = x,
            .here = x.path(),
            .self = x.self(),
        });
    _lateResolveConsts(x);
}

void _ym::LoaderState::_lateResolveConsts(YmItem& x) {
    auto& consts = x.info->consts;
    for (ConstIndex i = 0; i < consts.size(); i++) {
        if (!_isEarlyResolveConst(consts[i])) {
            _lateResolveRefConst(x, i);
        }
    }
}

void _ym::LoaderState::_lateResolveRefConst(YmItem& x, ConstIndex index) {
    auto& constInfo = x.info->consts[index];
#if _DUMP_LOG
    ym::println("LoaderState: Resolving {} (const #{}).", fmt(constInfo), index + 1);
#endif
    // NOTE: We seperate out the callsuff from the symbol (if present), as that
    //       stuff will be checked later.
    auto [fullname, callsuff] = seperateCallSuff(constInfo.as<RefInfo>().sym);
    // TODO: normalizedFullname contains redundent checks covered by the call
    //       to concrete below.
    normalizedFullname(std::string(fullname));
    // TODO: concrete() crashes if above fails resulting in stack not being
    //       setup correctly, so gotta check w/ 'good' first.
    x.putRefConst(index, good() ? concrete() : nullptr);
    popAll();
}

void _ym::LoaderState::_checkConstraintTypeLegality() {
    if (!good()) {
        return;
    }
#if _DUMP_LOG
    ym::println("LoaderState: Checking constraint type legality.");
#endif
    for (auto& item : _staging->items) {
#if _DUMP_LOG
        ym::println("LoaderState: Checking {} item params.", item.fullname());
#endif
        for (YmItemParamIndex i = 0; i < item.info->itemParamCount(); i++) {
            auto& itemParamConstraint = ym::deref(item.itemParamConstraint(i));
            if (itemParamConstraint.kind() != YmKind_Protocol) {
                err(
                    YmErrCode_NonProtocolItem,
                    "For {}; item parameter #{} ({}) constraint type {} is not a protocol!",
                    item.fullname(),
                    i + 1,
                    item.info->itemParams[i]->name,
                    itemParamConstraint.fullname());
            }
            static const auto immediateItemParamRefConstraintRefSymPattern = std::regex("^(?!\\$Self)([^\\[:/]+)$");
            const auto& itemParamConstraintRefSym = item.info->consts[item.info->itemParams[i]->constraint].as<RefInfo>().sym;
            if (std::regex_match(itemParamConstraintRefSym, immediateItemParamRefConstraintRefSymPattern)) {
                err(
                    YmErrCode_IllegalConstraint,
                    "For {}; item parameter #{} ({}) constraint type symbol {} cannot use item parameter as a constraint type (as the constraining protocol's interface would be indeterminate!)",
                    item.fullname(),
                    i + 1,
                    item.info->itemParams[i]->name,
                    itemParamConstraintRefSym);
            }
        }
    }
}

void _ym::LoaderState::_enforceConstraints() {
    if (!good()) {
        return;
    }
#if _DUMP_LOG
    ym::println("LoaderState: Enforcing constraints.");
#endif
    for (auto& item : _staging->items) {
#if _DUMP_LOG
        ym::println("LoaderState: Checking {} item args.", item.fullname());
#endif
        for (YmItemParamIndex i = 0; i < item.info->itemParamCount(); i++) {
            auto& itemArg = ym::deref(item.itemParam(i));
            auto& itemParamConstraint = ym::deref(item.itemParamConstraint(i));
            if (!itemArg.conforms(itemParamConstraint)) {
                err(
                    YmErrCode_ItemArgsError,
                    "For {}; item argument #{} ({}={}) doesn't conform to constraint {}!",
                    item.fullname(),
                    i + 1,
                    item.info->itemParams[i]->name,
                    itemArg.fullname(),
                    itemParamConstraint.fullname());
            }
        }
    }
}

void _ym::LoaderState::_checkRefConstCallSigConformance() {
    if (!good()) {
        return;
    }
#if _DUMP_LOG
    ym::println("LoaderState: Checking ref. const callsig conformance.");
#endif
    for (auto& item : _staging->items) {
#if _DUMP_LOG
        ym::println("LoaderState: Checking {} ref consts.", item.fullname());
#endif
        auto& consts = item.info->consts;
        for (size_t i = 0; i < consts.size(); i++) {
            if (auto constInfo = consts[i].tryAs<RefInfo>()) {
                auto [fullname, callsuff] = seperateCallSuff(constInfo->sym);
                if (callsuff) {
#if _DUMP_LOG
                    ym::println("LoaderState:     {}", constInfo->sym);
#endif
                    auto ref = item.constAsRef(i);
                    if (ref && /*callsuff &&*/ ref->callsuff() != callsuff) {
                        // TODO: Improve this error!
                        err(
                            YmErrCode_ItemNotFound,
                            "{}; {} does not conform to call suffix \"{}\"!",
                            errPrefix(),
                            ref->fullname(),
                            std::string(*callsuff));
                    }
                }
            }
        }
    }
}

YmParcel* _ym::LoaderState::_import(const std::string& normalizedPath) {
    assertNormal(normalizedPath);
    if (auto existing = _staging->parcels.fetch(normalizedPath)) {
        return existing.get();
    }
    if (!_staging->parcels.push(_binds->get(normalizedPath))) {
        err(
            YmErrCode_ParcelNotFound,
            "{}; no parcel found at path \"{}\"!",
            errPrefix(),
            normalizedPath);
    }
    return _staging->parcels.fetch(normalizedPath).get();
}

std::optional<size_t> _ym::LoaderState::_countInputsToEndArgs() const noexcept {
    for (size_t i = 0; i < height(); i++) {
        if (_terms[_terms.size() - 1 - i].awaitingArgs) {
            return i + 1;
        }
    }
    return std::nullopt;
}

void _ym::LoaderState::_printTermStk(size_t oldHeight) {
#if _DUMP_LOG
    std::string output{};
    auto t = expectN(height());
    bool first = true;
    for (size_t i = 0; i < t.size(); i++) {
        if (!first) {
            output += " ";
        }
        output += t[i].fmt(true);
        if (_terms[_terms.size() - height() + i].awaitingArgs) {
            output += "/marked";
        }
        first = false;
    }
    ym::println("    ({} -> {}) {}", oldHeight, height(), output);
#endif
}

_ym::LoaderState::_Interp::_Interp(LoaderState& client) :
    _client(client) {
}

void _ym::LoaderState::_Interp::operator()(const std::string& pathOrFullname) {
    _pathOrFullnamePtr = &pathOrFullname;
    eval(SpecParser{}(taul::str(pathOrFullname)));
}

const std::string& _ym::LoaderState::_Interp::_pathOrFullname() const noexcept {
    return ym::deref(_pathOrFullnamePtr);
}

void _ym::LoaderState::_Interp::syntaxErr() {
    YM_DEADEND;
}

void _ym::LoaderState::_Interp::rootId(const taul::str& id) {
    if (_client->good()) {
        using namespace taul::string_literals;
        if (id == "%here"_str)                  _client->here();
        else if (id == "$Self"_str)             _client->self();
        // TODO: Remove the avoidable std::string heap alloc.
        else if (id.substr(0, 1) == "$"_str)    _client->itemParam(std::string(id.substr(1)));
        else                                    _client->root(std::string(id));
    }
}

void _ym::LoaderState::_Interp::slashId(const taul::str& id) {
    if (_client->good()) {
        // TODO: Remove the avoidable std::string heap alloc.
        _client->submod(std::string(id));
    }
}

void _ym::LoaderState::_Interp::colonId(const taul::str& id) {
    if (_client->good()) {
        // TODO: Remove the avoidable std::string heap alloc.
        _client->itemInParcel(std::string(id));
    }
}

void _ym::LoaderState::_Interp::dblColonId(const taul::str& id) {
    if (_client->good()) {
        // TODO: Remove the avoidable std::string heap alloc.
        _client->member(std::string(id));
    }
}

void _ym::LoaderState::_Interp::openItemArgs() {
    if (_client->good()) {
        _client->beginArgs();
    }
}

void _ym::LoaderState::_Interp::itemArgsArgDelimiter() {
    if (_client->good()) {
    }
}

void _ym::LoaderState::_Interp::closeItemArgs() {
    if (_client->good()) {
        _client->endArgs();
    }
}

void _ym::LoaderState::_Interp::openCallSuff() {
    YM_DEADEND;
}

void _ym::LoaderState::_Interp::callSuffParamDelimiter() {
    YM_DEADEND;
}

void _ym::LoaderState::_Interp::callSuffReturnType() {
    YM_DEADEND;
}

void _ym::LoaderState::_Interp::closeCallSuff() {
    YM_DEADEND;
}

