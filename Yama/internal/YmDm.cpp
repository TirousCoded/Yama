

#include "YmDm.h"

#include "general.h"
#include "YmParcelDef.h"


YmBool YmDm::bindParcelDef(const std::string& path, ym::Safe<YmParcelDef> parceldef) {
    if (!_ym::Global::pathIsLegal(path)) {
        _ym::Global::raiseErr(
            YmErrCode_IllegalPath,
            "Cannot bind parcel def.; path \"{}\" is illegal!",
            path);
        return YM_FALSE;
    }
    std::scoped_lock lock(_mtx);
    _bindings.try_emplace(path, std::make_shared<_ym::ParcelData>(path, parceldef->info));
    return YM_TRUE;
}

std::shared_ptr<_ym::ParcelData> YmDm::import(const std::string& path) {
    ymAssert(_ym::Global::pathIsLegal(path));
    std::scoped_lock lock(_mtx);
    if (const auto result = _fetch(path)) {
        return result;
    }
    _download(path);
    return _fetch(path);
}

std::shared_ptr<_ym::ParcelData> YmDm::import(YmPID pid) {
    std::scoped_lock lock(_mtx);
    return _fetch(pid);
}

std::shared_ptr<_ym::ParcelData> YmDm::_fetch(const std::string& path) const noexcept {
    if (const auto it = _importsByPath.find(path); it != _importsByPath.end()) {
        return it->second;
    }
    return nullptr;
}

std::shared_ptr<_ym::ParcelData> YmDm::_fetch(YmPID pid) const noexcept {
    if (const auto it = _importsByPID.find(pid); it != _importsByPID.end()) {
        return it->second;
    }
    return nullptr;
}

void YmDm::_download(const std::string& path) {
    ymAssert(_ym::Global::pathIsLegal(path));
    if (const auto it = _bindings.find(path); it != _bindings.end()) {
        _importsByPath.try_emplace(it->first, it->second);
        _importsByPID.try_emplace(it->second->pid, it->second);
    }
}

