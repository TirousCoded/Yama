

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

size_t _ym::DmLoader::forEachParcel(YmForEachParcelCallbackFn callback, void* user, YmDm* dm) {
    ymAssert(callback != nullptr);
    ym::assertSafe(dm);
    std::shared_lock lk(_accessLock);
    size_t i = 0;
    for (auto& parcel : _commits.parcels) {
        callback(dm, user, &parcel, i, _commits.parcels.count());
        i++;
    }
    return _commits.parcels.count();
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

std::shared_ptr<YmType> _ym::DmLoader::fetchType(const Spec& fullname, bool* failedDueToCallSigNonConform) const noexcept {
    if (failedDueToCallSigNonConform) {
        *failedDueToCallSigNonConform = false;
    }
    // TODO: DmLoader and CtxLoader share callsuff checking code for fetchType.
    //       We can make a non-virtual base class 'fetchType' method which handles this, and then
    //       this can call a virtual 'doFetchType' method.
    //       Also, try to update _ym::DmLoader::load to generalize it's code w/ above.
    //       Also _ym::LoaderManager::_checkRefConstCallSigConformance too.
    std::shared_lock lk(_accessLock);
    auto result = _commits.types.fetch(fullname.removeCallSuff());
    lk.unlock(); // Don't need it anymore.
    if (result && !result->checkCallSuff(fullname.callsuff())) {
        if (failedDueToCallSigNonConform) {
            *failedDueToCallSigNonConform = true;
        }
        // TODO: Improve this error!
        _ym::Global::raiseErr(
            YmErrCode_TypeNotFound,
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

std::shared_ptr<YmType> _ym::DmLoader::load(const Spec& fullname) {
    bool failedDueToCallSigNonConform{};
    if (const auto result = fetchType(fullname, &failedDueToCallSigNonConform); result || failedDueToCallSigNonConform) {
        return result;
    }
    std::scoped_lock lk(_updateLock);
    auto result = _ldr.load(fullname.removeCallSuff());
    if (result && !result->checkCallSuff(fullname.callsuff())) {
        // TODO: Improve this error!
        _ym::Global::raiseErr(
            YmErrCode_TypeNotFound,
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
    _preloadBuiltins();
}

std::shared_ptr<_ym::Loader> _ym::CtxLoader::upstream() const {
    auto result = _upstream.lock();
    ymAssert(result != nullptr);
    return result;
}

YmType& _ym::CtxLoader::ldNone() const noexcept {
    return *_builtins.value().none;
}

YmType& _ym::CtxLoader::ldInt() const noexcept {
    return *_builtins.value().int0;
}

YmType& _ym::CtxLoader::ldUInt() const noexcept {
    return *_builtins.value().uint;
}

YmType& _ym::CtxLoader::ldFloat() const noexcept {
    return *_builtins.value().float0;
}

YmType& _ym::CtxLoader::ldBool() const noexcept {
    return *_builtins.value().bool0;
}

YmType& _ym::CtxLoader::ldRune() const noexcept {
    return *_builtins.value().rune;
}

void _ym::CtxLoader::reset() noexcept {
    _commits.discard(true);
}

std::shared_ptr<YmParcel> _ym::CtxLoader::fetchParcel(const Spec& path) const noexcept {
    return _commits.parcels.fetch(path);
}

std::shared_ptr<YmType> _ym::CtxLoader::fetchType(const Spec& fullname, bool* failedDueToCallSigNonConform) const noexcept {
    if (failedDueToCallSigNonConform) {
        *failedDueToCallSigNonConform = false;
    }
    auto result = _commits.types.fetch(fullname.removeCallSuff());
    if (result && !result->checkCallSuff(fullname.callsuff())) {
        if (failedDueToCallSigNonConform) {
            *failedDueToCallSigNonConform = true;
        }
        // TODO: Improve this error!
        _ym::Global::raiseErr(
            YmErrCode_TypeNotFound,
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

std::shared_ptr<YmType> _ym::CtxLoader::load(const Spec& fullname) {
    bool failedDueToCallSigNonConform{};
    if (auto result = fetchType(fullname, &failedDueToCallSigNonConform); result || failedDueToCallSigNonConform) {
        return result;
    }
    auto type = upstream()->load(fullname);
    return
        _commits.types.push(type)
        ? type
        : nullptr;
}

const _ym::Area& _ym::CtxLoader::commits() const {
    return _commits;
}

void _ym::CtxLoader::_preloadBuiltins() {
    _builtins = _Builtins{
        .none = ym::deref(load(Spec::typeFast("yama:None"))),
        .int0 = ym::deref(load(Spec::typeFast("yama:Int"))),
        .uint = ym::deref(load(Spec::typeFast("yama:UInt"))),
        .float0 = ym::deref(load(Spec::typeFast("yama:Float"))),
        .bool0 = ym::deref(load(Spec::typeFast("yama:Bool"))),
        .rune = ym::deref(load(Spec::typeFast("yama:Rune"))),
    };
}

