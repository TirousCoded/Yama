

#include "Loader.h"

#include "general.h"
#include "YmDm.h"
#include "YmParcelDef.h"
#include "SpecSolver.h"

#include "../yama++/resources.h"


_ym::DmLoader::DmLoader() :
    SynchronizedLoader(),
    _ldrState(_staging, _binds) {
    _staging.setUpstream(&_commits);
    _bindYamaParcel();
}

bool _ym::DmLoader::bindParcelDef(const std::string& path, ym::Safe<YmParcelDef> parceldef, bool bindIsForYamaParcel) {
    if (auto normalizedPath = SpecSolver{}(path, SpecSolver::MustBe::Path)) {
        if (!bindIsForYamaParcel && *normalizedPath == "yama") {
            _ym::Global::raiseErr(
                YmErrCode_PathBindError,
                "Cannot bind parcel def.; would overwrite \"yama\" parcel!");
            return false;
        }
        if (!parceldef->verify()) {
            return false;
        }
        std::scoped_lock lk(_updateLock);
        _binds.set(*normalizedPath, std::make_shared<YmParcel>(*normalizedPath, parceldef->info));
        return true;
    }
    else {
        _ym::Global::raiseErr(
            YmErrCode_IllegalSpecifier,
            "Cannot bind parcel def.; path \"{}\" is illegal!",
            path);
        return false;
    }
}

void _ym::DmLoader::reset() noexcept {
    std::scoped_lock lk(_accessLock, _updateLock);
    // TODO: Should also reset _commits/_binds? (If so, call _bindYamaParcel.)
    _staging.discard(true);
}

std::shared_ptr<YmParcel> _ym::DmLoader::fetchParcel(const std::string& normalizedPath) const noexcept {
    assertNormal(normalizedPath);
    std::shared_lock lk(_accessLock);
    return _commits.parcels.fetch(normalizedPath);
}

std::shared_ptr<YmItem> _ym::DmLoader::fetchItem(const std::string& normalizedFullname, bool* failedDueToCallSigNonConform) const noexcept {
    assertNormal(normalizedFullname);
    if (failedDueToCallSigNonConform) {
        *failedDueToCallSigNonConform = false;
    }
    // TODO: DmLoader and CtxLoader share callsuff checking code for fetchItem.
    //       We can make a non-virtual base class 'fetchItem' method which handles this, and then
    //       this can call a virtual 'doFetchItem' method.
    //       Also, try to update _ym::DmLoader::load to generalize it's code w/ above.
    //       Also _ym::LoaderState::_checkRefConstCallSigConformance too.
    const auto [fullname, callsuff] = seperateCallSuff(normalizedFullname);
    std::shared_lock lk(_accessLock);
    auto result = _commits.items.fetch(std::string(fullname));
    lk.unlock(); // Don't need it anymore.
    if (result && callsuff && result->callsuff() != callsuff) {
        if (failedDueToCallSigNonConform) {
            *failedDueToCallSigNonConform = true;
        }
        // TODO: Improve this error!
        _ym::Global::raiseErr(
            YmErrCode_ItemNotFound,
            "{} does not conform to call suffix \"{}\"!",
            result->fullname(),
            std::string(*callsuff));
        return nullptr;
    }
    return result;
}

std::shared_ptr<YmParcel> _ym::DmLoader::import(const std::string& normalizedPath) {
    assertNormalNonCallSig(normalizedPath);
    if (const auto result = fetchParcel(normalizedPath)) {
        return result;
    }
    std::scoped_lock lk(_updateLock);
    auto result = _ldrState.import(normalizedPath);
    _staging.commitOrDiscard(result, _accessLock);
    return
        result
        ? result->shared_from_this()
        : nullptr;
}

std::shared_ptr<YmItem> _ym::DmLoader::load(const std::string& normalizedFullname) {
    assertNormal(normalizedFullname);
    bool failedDueToCallSigNonConform{};
    if (const auto result = fetchItem(normalizedFullname, &failedDueToCallSigNonConform); result || failedDueToCallSigNonConform) {
        return result;
    }
    auto [fullname, callsuff] = seperateCallSuff(normalizedFullname);
    std::scoped_lock lk(_updateLock);
    auto result = _ldrState.load(std::string(fullname));
    if (result && callsuff && result->callsuff() != callsuff) {
        // TODO: Improve this error!
        _ym::Global::raiseErr(
            YmErrCode_ItemNotFound,
            "{} does not conform to call suffix \"{}\"!",
            result->fullname(),
            std::string(*callsuff));
        _staging.discard(); // Can't forget!
        return nullptr;
    }
    _staging.commitOrDiscard(result, _accessLock);
    return
        result
        ? result->shared_from_this()
        : nullptr;
}

void _ym::DmLoader::_bindYamaParcel() {
    auto p = ym::makeScoped<YmParcelDef>();
    p->addStruct("None");
    p->addStruct("Int");
    p->addStruct("UInt");
    p->addStruct("Float");
    p->addStruct("Bool");
    p->addStruct("Rune");
    p->addProtocol("Any");
    if (!bindParcelDef("yama", *p, true)) YM_DEADEND;
}

_ym::CtxLoader::CtxLoader(const std::shared_ptr<Loader>& upstream) :
    UnsynchronizedLoader(),
    _upstream(upstream) {
}

std::shared_ptr<_ym::Loader> _ym::CtxLoader::upstream() const {
    auto result = _upstream.lock();
    ymAssert(result != nullptr);
    return result;
}

void _ym::CtxLoader::reset() noexcept {
    _commits.discard(true);
}

std::shared_ptr<YmParcel> _ym::CtxLoader::fetchParcel(const std::string& normalizedPath) const noexcept {
    assertNormal(normalizedPath);
    return _commits.parcels.fetch(normalizedPath);
}

std::shared_ptr<YmItem> _ym::CtxLoader::fetchItem(const std::string& normalizedFullname, bool* failedDueToCallSigNonConform) const noexcept {
    assertNormal(normalizedFullname);
    if (failedDueToCallSigNonConform) {
        *failedDueToCallSigNonConform = false;
    }
    const auto [fullname, callsuff] = seperateCallSuff(normalizedFullname);
    auto result = _commits.items.fetch(std::string(fullname));
    if (result && callsuff && result->callsuff() != callsuff) {
        if (failedDueToCallSigNonConform) {
            *failedDueToCallSigNonConform = true;
        }
        // TODO: Improve this error!
        _ym::Global::raiseErr(
            YmErrCode_ItemNotFound,
            "{} does not conform to call suffix \"{}\"!",
            result->fullname(),
            std::string(*callsuff));
        return nullptr;
    }
    return result;
}

std::shared_ptr<YmParcel> _ym::CtxLoader::import(const std::string& normalizedPath) {
    assertNormal(normalizedPath);
    if (auto result = fetchParcel(normalizedPath)) {
        return result;
    }
    auto parcel = upstream()->import(normalizedPath);
    return
        _commits.parcels.push(parcel)
        ? parcel
        : nullptr;
}

std::shared_ptr<YmItem> _ym::CtxLoader::load(const std::string& normalizedFullname) {
    assertNormal(normalizedFullname);
    bool failedDueToCallSigNonConform{};
    if (auto result = fetchItem(normalizedFullname, &failedDueToCallSigNonConform); result || failedDueToCallSigNonConform) {
        return result;
    }
    auto item = upstream()->load(normalizedFullname);
    return
        _commits.items.push(item)
        ? item
        : nullptr;
}

const _ym::Area& _ym::CtxLoader::commits() const {
    return _commits;
}

