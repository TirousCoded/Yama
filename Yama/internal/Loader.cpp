

#include "Loader.h"

#include "general.h"
#include "YmDm.h"
#include "YmParcelDef.h"
#include "SpecSolver.h"

#include "../yama++/resources.h"


_ym::DmLoader::DmLoader() :
    SynchronizedLoader(),
    _ldr(_staging, _binds, _redirects) {
    _staging.setUpstream(&_commits);
    _bindYamaParcel();
}

bool _ym::DmLoader::bindParcelDef(const std::string& path, ym::Safe<YmParcelDef> parceldef, bool bindIsForYamaParcel) {
    if (auto p = Spec::path(path)) {
        if (!bindIsForYamaParcel && *p == "yama") {
            _ym::Global::raiseErr(
                YmErrCode_PathBindError,
                "Cannot bind parcel def.; would overwrite \"yama\" parcel!");
            return false;
        }
        if (!parceldef->verify()) {
            return false;
        }
        std::scoped_lock lk(_updateLock);
        _binds.set(*p, std::make_shared<YmParcel>(*p, parceldef->info));
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

bool _ym::DmLoader::addRedirect(const std::string& subject, const std::string& before, const std::string& after) {
    auto pSubject = Spec::path(subject);
    auto pBefore = Spec::path(before);
    auto pAfter = Spec::path(after);
    if (!pSubject) {
        Global::raiseErr(
            YmErrCode_IllegalSpecifier,
            "Cannot add redirect; subject path \"{}\" is illegal!",
            subject);
        return false;
    }
    if (!pBefore) {
        Global::raiseErr(
            YmErrCode_IllegalSpecifier,
            "Cannot add redirect; 'before' path \"{}\" is illegal!",
            before);
        return false;
    }
    if (!pAfter) {
        Global::raiseErr(
            YmErrCode_IllegalSpecifier,
            "Cannot add redirect; 'after' path \"{}\" is illegal!",
            after);
        return false;
    }
    std::scoped_lock lk(_updateLock);
    _redirects.add(*pSubject, *pBefore, *pAfter);
    return true;
}

void _ym::DmLoader::reset() noexcept {
    std::scoped_lock lk(_accessLock, _updateLock);
    // TODO: Should also reset _commits/_binds/_redirects? (If so, remember to call _bindYamaParcel.)
    _staging.discard(true);
}

std::shared_ptr<YmParcel> _ym::DmLoader::fetchParcel(const Spec& path) const noexcept {
    std::shared_lock lk(_accessLock);
    return _commits.parcels.fetch(path);
}

std::shared_ptr<YmItem> _ym::DmLoader::fetchItem(const Spec& fullname, bool* failedDueToCallSigNonConform) const noexcept {
    if (failedDueToCallSigNonConform) {
        *failedDueToCallSigNonConform = false;
    }
    // TODO: DmLoader and CtxLoader share callsuff checking code for fetchItem.
    //       We can make a non-virtual base class 'fetchItem' method which handles this, and then
    //       this can call a virtual 'doFetchItem' method.
    //       Also, try to update _ym::DmLoader::load to generalize it's code w/ above.
    //       Also _ym::LoaderManager::_checkRefConstCallSigConformance too.
    std::shared_lock lk(_accessLock);
    auto result = _commits.items.fetch(fullname.removeCallSuff());
    lk.unlock(); // Don't need it anymore.
    if (result && !result->checkCallSuff(fullname.callsuff())) {
        if (failedDueToCallSigNonConform) {
            *failedDueToCallSigNonConform = true;
        }
        // TODO: Improve this error!
        _ym::Global::raiseErr(
            YmErrCode_ItemNotFound,
            "{} does not conform to call suffix \"{}\"!",
            result->fullname(),
            std::string(*fullname.callsuff()));
        return nullptr;
    }
    return result;
}

std::shared_ptr<YmParcel> _ym::DmLoader::import(const Spec& path) {
    if (const auto result = fetchParcel(path)) {
        return result;
    }
    std::scoped_lock lk(_updateLock);
    auto result = _ldr.import(path);
    _staging.commitOrDiscard(result, _accessLock);
    return
        result
        ? result->shared_from_this()
        : nullptr;
}

std::shared_ptr<YmItem> _ym::DmLoader::load(const Spec& fullname) {
    bool failedDueToCallSigNonConform{};
    if (const auto result = fetchItem(fullname, &failedDueToCallSigNonConform); result || failedDueToCallSigNonConform) {
        return result;
    }
    std::scoped_lock lk(_updateLock);
    auto result = _ldr.load(fullname.removeCallSuff());
    if (result && !result->checkCallSuff(fullname.callsuff())) {
        // TODO: Improve this error!
        _ym::Global::raiseErr(
            YmErrCode_ItemNotFound,
            "{} does not conform to call suffix \"{}\"!",
            result->fullname(),
            std::string(*fullname.callsuff()));
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

std::shared_ptr<YmParcel> _ym::CtxLoader::fetchParcel(const Spec& path) const noexcept {
    return _commits.parcels.fetch(path);
}

std::shared_ptr<YmItem> _ym::CtxLoader::fetchItem(const Spec& fullname, bool* failedDueToCallSigNonConform) const noexcept {
    if (failedDueToCallSigNonConform) {
        *failedDueToCallSigNonConform = false;
    }
    auto result = _commits.items.fetch(fullname.removeCallSuff());
    if (result && !result->checkCallSuff(fullname.callsuff())) {
        if (failedDueToCallSigNonConform) {
            *failedDueToCallSigNonConform = true;
        }
        // TODO: Improve this error!
        _ym::Global::raiseErr(
            YmErrCode_ItemNotFound,
            "{} does not conform to call suffix \"{}\"!",
            result->fullname(),
            std::string(*fullname.callsuff()));
        return nullptr;
    }
    return result;
}

std::shared_ptr<YmParcel> _ym::CtxLoader::import(const Spec& path) {
    if (auto result = fetchParcel(path)) {
        return result;
    }
    auto parcel = upstream()->import(path);
    return
        _commits.parcels.push(parcel)
        ? parcel
        : nullptr;
}

std::shared_ptr<YmItem> _ym::CtxLoader::load(const Spec& fullname) {
    bool failedDueToCallSigNonConform{};
    if (auto result = fetchItem(fullname, &failedDueToCallSigNonConform); result || failedDueToCallSigNonConform) {
        return result;
    }
    auto item = upstream()->load(fullname);
    return
        _commits.items.push(item)
        ? item
        : nullptr;
}

const _ym::Area& _ym::CtxLoader::commits() const {
    return _commits;
}

