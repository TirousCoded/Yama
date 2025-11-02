

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
    _bindings.try_emplace(path, _ym::Res(YmParcel::create(path)));
    return YM_TRUE;
}

YmParcel* YmCtx::import(const std::string& path) {
    if (!_ym::Global::pathIsLegal(path)) {
        _ym::Global::raiseErr(
            YmErrCode_IllegalPath,
            "Import failed; path \"{}\" is illegal!",
            path);
        return nullptr; // Fail
    }
    // Try finding existing.
    if (const auto it = _imports.find(path); it != _imports.end()) {
        return _ym::cloneRef(it->second);
    }
    // Try finding bound parcel to import.
    if (const auto it = domain->_bindings.find(path); it != domain->_bindings.end()) {
        return _ym::cloneRef(it->second);
    }
    return nullptr; // Fail
}

