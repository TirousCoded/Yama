

#include "Loader.h"

#include "general.h"
#include "YmDm.h"
#include "YmParcelDef.h"


_ym::DmLoader::DmLoader() :
    SynchronizedLoader() {
    _privParcels.setUpstream(&_pubParcels);
    _privItems.setUpstream(&_pubItems);
}

bool _ym::DmLoader::bindParcelDef(const std::string& path, ym::Safe<YmParcelDef> parceldef) {
    if (!_ym::Global::pathIsLegal(path)) {
        _ym::Global::raiseErr(
            YmErrCode_IllegalPath,
            "Cannot bind parcel def.; path \"{}\" is illegal!",
            path);
        return false;
    }
    std::scoped_lock lk(_sessionLock);
    auto success = _bindings.push(std::make_shared<YmParcel>(path, parceldef->info));
    ymAssert(success);
    return true;
}

void _ym::DmLoader::reset() noexcept {
    std::scoped_lock lk(_accessLock, _sessionLock);
    _privParcels.discard(true);
    _privItems.discard(true);
}

std::shared_ptr<YmParcel> _ym::DmLoader::fetchParcel(const std::string& path) const noexcept {
    std::shared_lock lk(_accessLock);
    return _pubParcels.fetch(path);
}

std::shared_ptr<YmParcel> _ym::DmLoader::fetchParcel(YmPID pid) const noexcept {
    std::shared_lock lk(_accessLock);
    return _pubParcels.fetch(pid);
}

std::shared_ptr<YmItem> _ym::DmLoader::fetchItem(const std::string& fullname) const noexcept {
    std::shared_lock lk(_accessLock);
    return _pubItems.fetch(fullname);
}

std::shared_ptr<YmItem> _ym::DmLoader::fetchItem(YmGID gid) const noexcept {
    std::shared_lock lk(_accessLock);
    return _pubItems.fetch(gid);
}

std::shared_ptr<YmParcel> _ym::DmLoader::import(const std::string& path) {
    if (const auto result = fetchParcel(path)) {
        ymAssert(_ym::Global::pathIsLegal(path));
        return result;
    }
    std::scoped_lock lk(_sessionLock);
    _beginSession();
    auto result = _import(path);
    _endSession();
    return result;
}

std::shared_ptr<YmParcel> _ym::DmLoader::import(YmPID pid) {
    if (const auto result = fetchParcel(pid)) {
        return result;
    }
    std::scoped_lock lk(_sessionLock);
    _beginSession();
    auto result = _import(pid);
    _endSession();
    return result;
}

std::shared_ptr<YmItem> _ym::DmLoader::load(const std::string& fullname) {
    if (const auto result = fetchItem(fullname)) {
        ymAssert(_ym::Global::fullnameIsLegal(fullname));
        return result;
    }
    std::scoped_lock lk(_sessionLock);
    _beginSession();
    auto result = _load(fullname);
    _endSession();
    return result;
}

std::shared_ptr<YmItem> _ym::DmLoader::load(YmGID gid) {
    if (const auto result = fetchItem(gid)) {
        return result;
    }
    std::scoped_lock lk(_sessionLock);
    _beginSession();
    auto result = _load(gid);
    _endSession();
    return result;
}

void _ym::DmLoader::_beginSession() {
    _success = true;
}

void _ym::DmLoader::_endSession() {
    // It's presumed that _sessionLock is held by method calling this one.
    std::scoped_lock lk(_accessLock);
    _privParcels.commitOrDiscard(_success);
    _privItems.commitOrDiscard(_success);
}

std::shared_ptr<YmParcel> _ym::DmLoader::_import(const std::string& path) {
    if (auto existing = _privParcels.fetch(path)) {
        ymAssert(_ym::Global::pathIsLegal(path));
        return existing;
    }
    if (!_ym::Global::pathIsLegal(path)) {
        _success = false;
        _ym::Global::raiseErr(
            YmErrCode_IllegalPath,
            "Import failed; path \"{}\" is illegal!",
            path);
        return nullptr;
    }
    if (!_privParcels.push(_bindings.fetch(path))) {
        _success = false;
        _ym::Global::raiseErr(
            YmErrCode_ParcelNotFound,
            "Import failed; no parcel found at path \"{}\"!",
            path);
    }
    return _privParcels.fetch(path);
}

std::shared_ptr<YmParcel> _ym::DmLoader::_import(YmPID pid) {
    if (auto existing = _privParcels.fetch(pid)) {
        return existing;
    }
    if (!_privParcels.push(_bindings.fetch(pid))) {
        _success = false;
        _ym::Global::raiseErr(
            YmErrCode_ParcelNotFound,
            "Import failed; no parcel found with PID {}!",
            pid);
    }
    return _privParcels.fetch(pid);
}

std::shared_ptr<YmItem> _ym::DmLoader::_load(const std::string& fullname) {
    if (auto existing = _privItems.fetch(fullname)) {
        ymAssert(_ym::Global::fullnameIsLegal(fullname));
        return existing;
    }
    if (!_ym::Global::fullnameIsLegal(fullname)) {
        _success = false;
        _ym::Global::raiseErr(
            YmErrCode_IllegalFullname,
            "Load failed; fullname \"{}\" is illegal!",
            fullname);
        return nullptr;
    }
    const auto [path, localName] = _ym::split_s<YmChar>(fullname, ":");
    ymAssert(!localName.empty());
    if (const auto parcel = _import((std::string)path)) {
        if (const auto item = parcel->item((std::string)localName)) {
            auto res = std::make_shared<YmItem>(ym::Safe(*parcel), ym::Safe(item));
            // Push before resolving consts to ensure that the loading resource is available
            // for lookup for resolving the consts of other items.
            const bool success = _pubItems.push(res);
            ymAssert(success);
            _resolveConsts(*res);
        }
        else {
            _success = false;
            _ym::Global::raiseErr(
                YmErrCode_ItemNotFound,
                "Load failed; {} contains no item \"{}\"!",
                (std::string)path,
                (std::string)localName);
        }
    }
    return _privItems.fetch(fullname);
}

std::shared_ptr<YmItem> _ym::DmLoader::_load(YmGID gid) {
    if (auto existing = _privItems.fetch(gid)) {
        return existing;
    }
    YmPID pid = ymGID_PID(gid);
    YmLID lid = ymGID_LID(gid);
    if (const auto parcel = _import(pid)) {
        if (const auto item = parcel->item(lid)) {
            auto res = std::make_shared<YmItem>(ym::Safe(*parcel), ym::Safe(item));
            // Push before resolving consts to ensure that the loading resource is available
            // for lookup for resolving the consts of other items.
            const bool success = _pubItems.push(res);
            ymAssert(success);
            _resolveConsts(*res);
        }
        else {
            _success = false;
            _ym::Global::raiseErr(
                YmErrCode_ItemNotFound,
                "Load failed; {} contains no item with LID {}!",
                parcel->path,
                lid);
        }
    }
    return _privItems.fetch(gid);
}

void _ym::DmLoader::_resolveConsts(YmItem& x) {
    for (YmConst i = 0; i < YmConst(x.info->consts.size()); i++) {
        auto& info = x.info->consts[i];
        auto type = _ym::constTypeOf(info);
        if (_ym::isValConstType(type)) {
            x.putValConst(i);
        }
        else if (_ym::isRefConstType(type)) {
            x.putRefConst(i, _load(info.as<_ym::RefConstInfo>().sym).get());
        }
        else YM_DEADEND;
    }
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
    _parcels.discard(true);
    _items.discard(true);
}

std::shared_ptr<YmParcel> _ym::CtxLoader::fetchParcel(const std::string& path) const noexcept {
    return _parcels.fetch(path);
}

std::shared_ptr<YmParcel> _ym::CtxLoader::fetchParcel(YmPID pid) const noexcept {
    return _parcels.fetch(pid);
}

std::shared_ptr<YmItem> _ym::CtxLoader::fetchItem(const std::string& fullname) const noexcept {
    return _items.fetch(fullname);
}

std::shared_ptr<YmItem> _ym::CtxLoader::fetchItem(YmGID gid) const noexcept {
    return _items.fetch(gid);
}

std::shared_ptr<YmParcel> _ym::CtxLoader::import(const std::string& path) {
    if (auto result = fetchParcel(path)) {
        return result;
    }
    return
        _parcels.push(upstream()->import(path))
        ? fetchParcel(path)
        : nullptr;
}

std::shared_ptr<YmParcel> _ym::CtxLoader::import(YmPID pid) {
    if (auto result = fetchParcel(pid)) {
        return result;
    }
    return
        _parcels.push(upstream()->import(pid))
        ? fetchParcel(pid)
        : nullptr;
}

std::shared_ptr<YmItem> _ym::CtxLoader::load(const std::string& fullname) {
    if (auto result = fetchItem(fullname)) {
        return result;
    }
    return
        _items.push(upstream()->load(fullname))
        ? fetchItem(fullname)
        : nullptr;
}

std::shared_ptr<YmItem> _ym::CtxLoader::load(YmGID gid) {
    if (auto result = fetchItem(gid)) {
        return result;
    }
    return
        _items.push(upstream()->load(gid))
        ? fetchItem(gid)
        : nullptr;
}

const _ym::Area<YmParcel>& _ym::CtxLoader::parcels() const {
    return _parcels;
}

const _ym::Area<YmItem>& _ym::CtxLoader::items() const {
    return _items;
}

