

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

std::shared_ptr<YmItem> _ym::DmLoader::fetchItem(const std::string& normalizedFullname) const noexcept {
    assertNormal(normalizedFullname);
    std::shared_lock lk(_accessLock);
    return _commits.items.fetch(normalizedFullname);
}

std::shared_ptr<YmParcel> _ym::DmLoader::import(const std::string& normalizedPath) {
    assertNormal(normalizedPath);
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
    if (const auto result = fetchItem(normalizedFullname)) {
        return result;
    }
    std::scoped_lock lk(_updateLock);
    auto result = _ldrState.load(normalizedFullname);
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

std::shared_ptr<YmItem> _ym::CtxLoader::fetchItem(const std::string& normalizedFullname) const noexcept {
    assertNormal(normalizedFullname);
    return _commits.items.fetch(normalizedFullname);
}

std::shared_ptr<YmParcel> _ym::CtxLoader::import(const std::string& normalizedPath) {
    assertNormal(normalizedPath);
    if (auto result = fetchParcel(normalizedPath)) {
        return result;
    }
    return
        _commits.parcels.push(upstream()->import(normalizedPath))
        ? fetchParcel(normalizedPath)
        : nullptr;
}

std::shared_ptr<YmItem> _ym::CtxLoader::load(const std::string& normalizedFullname) {
    assertNormal(normalizedFullname);
    if (auto result = fetchItem(normalizedFullname)) {
        return result;
    }
    return
        _commits.items.push(upstream()->load(normalizedFullname))
        ? fetchItem(normalizedFullname)
        : nullptr;
}

const _ym::Area& _ym::CtxLoader::commits() const {
    return _commits;
}

