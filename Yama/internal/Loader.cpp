

#include "Loader.h"

#include "general.h"
#include "YmDm.h"
#include "YmParcelDef.h"
#include "SpecSolver.h"


_ym::DmLoader::DmLoader() :
    SynchronizedLoader(),
    _ldrState(_staging, _binds) {
    _staging.setUpstream(&_commits);
}

bool _ym::DmLoader::bindParcelDef(const std::string& path, ym::Safe<YmParcelDef> parceldef) {
    if (!_ym::Global::pathIsLegal(path)) {
        _ym::Global::raiseErr(
            YmErrCode_IllegalSpecifier,
            "Cannot bind parcel def.; path \"{}\" is illegal!",
            path);
        return false;
    }
    if (!parceldef->verify()) {
        return false;
    }
    std::scoped_lock lk(_updateLock);
    _binds.set(path, std::make_shared<YmParcel>(path, parceldef->info));
    return true;
}

void _ym::DmLoader::reset() noexcept {
    std::scoped_lock lk(_accessLock, _updateLock);
    // TODO: Should also reset _commits/_binds?
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

