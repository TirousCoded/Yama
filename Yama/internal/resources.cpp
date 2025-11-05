

#include "resources.h"

#include "general.h"


YmBool YmDm::bindParcelDef(const std::string& path, ym::Safe<YmParcelDef> parceldef) {
    if (!_ym::Global::pathIsLegal(path)) {
        _ym::Global::raiseErr(
            YmErrCode_IllegalPath,
            "Cannot bind parcel def.; path \"{}\" is illegal!",
            path);
        return YM_FALSE;
    }
    std::scoped_lock lock(_mtx);
    _bindings.try_emplace(path, std::move(_ym::ParcelData::create(path)));
    return YM_TRUE;
}

std::optional<_ym::Res<_ym::ParcelData>> YmDm::import(const std::string& path) {
    ymAssert(_ym::Global::pathIsLegal(path));
    std::scoped_lock lock(_mtx);
    if (const auto it = _bindings.find(path); it != _bindings.end()) {
        return it->second;
    }
    return std::nullopt;
}

std::optional<_ym::Res<YmParcel>> YmCtx::fetch(const std::string& path) const noexcept {
    if (const auto it = _imports.find(path); it != _imports.end()) {
        return it->second;
    }
    return std::nullopt;
}

std::optional<_ym::Res<YmParcel>> YmCtx::import(const std::string& path) {
    if (!_ym::Global::pathIsLegal(path)) {
        _ym::Global::raiseErr(
            YmErrCode_IllegalPath,
            "Import failed; path \"{}\" is illegal!",
            path);
        return std::nullopt; // Fail
    }
    if (const auto result = fetch(path)) {
        return result;
    }
    _download(path);
    return fetch(path);
}

void YmCtx::_download(const std::string& path) {
    if (auto data = domain->import(path)) {
        _imports.try_emplace(path, std::move(YmParcel::create(std::move(*data))));
    }
}

