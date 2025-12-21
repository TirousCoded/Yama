

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
    if (!parceldef->verify()) {
        return false;
    }
    std::scoped_lock lk(_sessionLock);
    _setBinding(path, std::make_shared<YmParcel>(path, parceldef->info));
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

std::shared_ptr<YmItem> _ym::DmLoader::fetchItem(const std::string& fullname) const noexcept {
    std::shared_lock lk(_accessLock);
    return _pubItems.fetch(fullname);
}

std::shared_ptr<YmParcel> _ym::DmLoader::import(const std::string& path) {
    if (const auto result = fetchParcel(path)) {
        ymAssert(_ym::Global::pathIsLegal(path));
        return result;
    }
    std::scoped_lock lk(_sessionLock);
    _beginSession();
    auto result = _import(path);
    return
        _endSession()
        ? result
        : nullptr;
}

std::shared_ptr<YmItem> _ym::DmLoader::load(const std::string& fullname) {
    if (const auto result = fetchItem(fullname)) {
        ymAssert(_ym::Global::fullnameIsLegal(fullname));
        return result;
    }
    std::scoped_lock lk(_sessionLock);
    _beginSession();
    auto result = _load(fullname);
    return
        _endSession()
        ? result
        : nullptr;
}

void _ym::DmLoader::_setBinding(const std::string& path, std::shared_ptr<YmParcel> x) {
    ymAssert(x != nullptr);
    _bindings[path] = std::move(x); // Overwrite existing, if any.
}

std::shared_ptr<YmParcel> _ym::DmLoader::_queryBinding(const std::string& path) {
    if (const auto it = _bindings.find(path); it != _bindings.end()) {
        return it->second;
    }
    return nullptr;
}

void _ym::DmLoader::_beginSession() {
    _success = true;
}

bool _ym::DmLoader::_endSession() {
    // It's presumed that _sessionLock is held by method calling this one.
    std::scoped_lock lk(_accessLock);
    _privParcels.commitOrDiscard(_success);
    _privItems.commitOrDiscard(_success);
    return _success;
}

std::shared_ptr<YmParcel> _ym::DmLoader::_import(const std::string& path, std::optional<std::string> indirectForLoadOf) {
    auto resolved = _resolveHere(path, indirectForLoadOf);
    if (auto existing = _privParcels.fetch(resolved)) {
        ymAssert(_ym::Global::pathIsLegal(path));
        return existing;
    }
    if (!_ym::Global::pathIsLegal(path)) {
        _success = false;
        if (indirectForLoadOf) {
            _ym::Global::raiseErr(
                YmErrCode_IllegalPath,
                "For {}; dependency import failed; path \"{}\" is illegal!",
                indirectForLoadOf.value(),
                path);
        }
        else {
            _ym::Global::raiseErr(
                YmErrCode_IllegalPath,
                "Import failed; path \"{}\" is illegal!",
                path);
        }
        return nullptr;
    }
    if (!_privParcels.push(_queryBinding(resolved))) {
        _success = false;
        if (indirectForLoadOf) {
            _ym::Global::raiseErr(
                YmErrCode_ParcelNotFound,
                "For {}; dependency import failed; no parcel found at path \"{}\"!",
                indirectForLoadOf.value(),
                path);
        }
        else {
            _ym::Global::raiseErr(
                YmErrCode_ParcelNotFound,
                "Import failed; no parcel found at path \"{}\"!",
                path);
        }
    }
    return _privParcels.fetch(resolved);
}

std::shared_ptr<YmItem> _ym::DmLoader::_load(const std::string& refSymOrFullname, std::optional<std::string> indirectForLoadOf) {
    auto resolved = _resolveRefSym(refSymOrFullname, indirectForLoadOf);
    if (auto existing = _privItems.fetch(resolved)) {
        return existing;
    }
    if (indirectForLoadOf) {
        if (!_ym::Global::refSymIsLegal(refSymOrFullname)) {
            _success = false;
            _ym::Global::raiseErr(
                YmErrCode_IllegalRefSym,
                "For {}; dependency load failed; reference symbol \"{}\" is illegal!",
                indirectForLoadOf.value(),
                refSymOrFullname);
            return nullptr;
        }
    }
    else {
        if (!_ym::Global::fullnameIsLegal(refSymOrFullname)) {
            _success = false;
            _ym::Global::raiseErr(
                YmErrCode_IllegalFullname,
                "Load failed; fullname \"{}\" is illegal!",
                refSymOrFullname);
            return nullptr;
        }
    }
    const auto [path, localName] = _ym::split_s<YmChar>(resolved, ":");
    ymAssert(!localName.empty());
    if (const auto parcel = _import((std::string)path, indirectForLoadOf)) {
        if (const auto item = parcel->item((std::string)localName)) {
            auto res = std::make_shared<YmItem>(ym::Safe(*parcel), ym::Safe(item));
            // Push before resolving consts to ensure that the loading resource is available
            // for lookup for resolving the consts of other items.
            const bool success = _privItems.push(res);
            ymAssert(success);
            _resolveConsts(*res);
        }
        else {
            _success = false;
            if (indirectForLoadOf) {
                _ym::Global::raiseErr(
                    YmErrCode_ItemNotFound,
                    "For {}; dependency load failed; {} contains no item \"{}\"!",
                    indirectForLoadOf.value(),
                    (std::string)path,
                    (std::string)localName);
            }
            else {
                _ym::Global::raiseErr(
                    YmErrCode_ItemNotFound,
                    "Load failed; {} contains no item \"{}\"!",
                    (std::string)path,
                    (std::string)localName);
            }
        }
    }
    return _privItems.fetch(resolved);
}

std::string _ym::DmLoader::_resolveHere(const std::string& pathOrFullname, const std::optional<std::string>& indirectForLoadOf) const {
    if (!indirectForLoadOf) {
        return pathOrFullname;
    }
    if (std::string_view(pathOrFullname).substr(0, refSymHere.length()) != refSymHere) {
        return pathOrFullname;
    }
    auto result = pathOrFullname;
    result.replace(0, refSymHere.length(), split_s<YmChar>(*indirectForLoadOf, ":").first);
    return result;
}

std::string _ym::DmLoader::_resolveRefSym(const std::string& refSymOrFullname, const std::optional<std::string>& indirectForLoadOf) const {
    return
        refSymOrFullname == "Self" && indirectForLoadOf
        ? (std::string)split_s<YmChar>(*indirectForLoadOf, "::").first
        : _resolveHere(refSymOrFullname, indirectForLoadOf);
}

void _ym::DmLoader::_resolveConsts(YmItem& x) {
    for (size_t i = 0; i < x.info->consts.size(); i++) {
        auto& sym = x.info->consts[i];
        if (isVal(sym))         x.putValConst(i);
        else if (isRef(sym))    x.putRefConst(i, _load(sym.as<_ym::RefInfo>().sym, x.fullname).get());
        else                    YM_DEADEND;
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

std::shared_ptr<YmItem> _ym::CtxLoader::fetchItem(const std::string& fullname) const noexcept {
    return _items.fetch(fullname);
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

std::shared_ptr<YmItem> _ym::CtxLoader::load(const std::string& fullname) {
    if (auto result = fetchItem(fullname)) {
        return result;
    }
    return
        _items.push(upstream()->load(fullname))
        ? fetchItem(fullname)
        : nullptr;
}

const _ym::Area<YmParcel>& _ym::CtxLoader::parcels() const {
    return _parcels;
}

const _ym::Area<YmItem>& _ym::CtxLoader::items() const {
    return _items;
}

