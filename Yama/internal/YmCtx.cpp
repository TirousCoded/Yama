

#include "YmCtx.h"

#include "general.h"
#include "YmParcel.h"
#include "YmItem.h"


YmParcel* YmCtx::fetchParcel(const std::string& path) const noexcept {
    if (const auto it = _importsByPath.find(path); it != _importsByPath.end()) {
        return ym::Safe(it->second.get());
    }
    return nullptr;
}

YmParcel* YmCtx::fetchParcel(YmPID pid) const noexcept {
    if (const auto it = _importsByPID.find(pid); it != _importsByPID.end()) {
        return ym::Safe(it->second.get());
    }
    return nullptr;
}

YmItem* YmCtx::fetchItem(const std::string& fullname) const noexcept {
    if (const auto it = _loadsByFullname.find(fullname); it != _loadsByFullname.end()) {
        return ym::Safe(it->second.get());
    }
    return nullptr;
}

YmItem* YmCtx::fetchItem(YmGID gid) const noexcept {
    if (const auto it = _loadsByGID.find(gid); it != _loadsByGID.end()) {
        return ym::Safe(it->second.get());
    }
    return nullptr;
}

YmParcel* YmCtx::import(const std::string& path) {
    if (!_ym::Global::pathIsLegal(path)) {
        _ym::Global::raiseErr(
            YmErrCode_IllegalPath,
            "Import failed; path \"{}\" is illegal!",
            path);
        return nullptr; // Fail
    }
    if (const auto result = fetchParcel(path)) {
        return result;
    }
    _download(path);
    return fetchParcel(path);
}

YmParcel* YmCtx::import(YmPID pid) {
    if (const auto result = fetchParcel(pid)) {
        return result;
    }
    _download(pid);
    return fetchParcel(pid);
}

YmItem* YmCtx::load(const std::string& fullname) {
    if (!_ym::Global::fullnameIsLegal(fullname)) {
        _ym::Global::raiseErr(
            YmErrCode_IllegalFullname,
            "Load failed; fullname \"{}\" is illegal!",
            fullname);
        return nullptr; // Fail
    }
    if (const auto result = fetchItem(fullname)) {
        return result;
    }
    _resolve(fullname);
    return fetchItem(fullname);
}

YmItem* YmCtx::load(YmGID gid) {
    throw std::runtime_error(""); // TODO
}

void YmCtx::pIterStart(ym::Safe<YmCtx> ctx) noexcept {
    _iter = ctx->_importsByPID.begin();
    _pastTheEndIter = ctx->_importsByPID.end();
    _updateIterCurr();
}

void YmCtx::pIterStartFrom(ym::Safe<YmCtx> ctx, ym::Safe<YmParcel> parcel) noexcept {
    _iter = ctx->_importsByPID.find(parcel->pid());
    _pastTheEndIter = ctx->_importsByPID.end();
    _updateIterCurr();
}

void YmCtx::pIterAdvance(size_t n) noexcept {
    for (size_t i = 0; i < n; i++) {
        if (pIterDone()) return;
        std::advance(_iter, 1);
        _updateIterCurr();
    }
}

YmParcel* YmCtx::pIterGet() noexcept {
    return _iterCurr;
}

bool YmCtx::pIterDone() noexcept {
    return _iter == _pastTheEndIter;
}

thread_local decltype(YmCtx::_importsByPID)::const_iterator YmCtx::_iter = decltype(YmCtx::_iter){};
thread_local decltype(YmCtx::_importsByPID)::const_iterator YmCtx::_pastTheEndIter = decltype(YmCtx::_pastTheEndIter){};
thread_local YmParcel* YmCtx::_iterCurr = nullptr;

void YmCtx::_download(const std::string& path) {
    if (auto data = domain->import(path)) {
        auto res = std::make_shared<YmParcel>(data);
        _importsByPID.try_emplace(res->pid(), res);
        _importsByPath.try_emplace(path, std::move(res));
    }
    else {
        _ym::Global::raiseErr(
            YmErrCode_ParcelNotFound,
            "Import failed; no parcel found at path \"{}\"!",
            path);
    }
}

void YmCtx::_download(YmPID pid) {
    if (auto data = domain->import(pid)) {
        auto res = std::make_shared<YmParcel>(data);
        const std::string& path = res->path();
        _importsByPID.try_emplace(res->pid(), res);
        _importsByPath.try_emplace(path, res);
    }
    else {
        _ym::Global::raiseErr(
            YmErrCode_ParcelNotFound,
            "Import failed; no parcel found with PID {}!",
            pid);
    }
}

void YmCtx::_resolve(const std::string& fullname) {
    ymAssert(_ym::Global::fullnameIsLegal(fullname));
    const auto [path, localName] = _ym::split_s<YmChar>(fullname, ":");
    ymAssert(!localName.empty());
    if (const auto parcel = import((std::string)path)) {
        const auto& data = *ym::Safe(parcel)->data;
        if (const auto item = data.item((std::string)localName)) {
            auto res = std::make_shared<YmItem>(ym::Safe(this), ym::Safe(parcel), ym::Safe(item));
            res->resolveConsts();
            _loadsByGID.try_emplace(res->gid, res);
            _loadsByFullname.try_emplace(fullname, res);
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

void YmCtx::_updateIterCurr() noexcept {
    _iterCurr =
        !pIterDone()
        ? _iter->second.get()
        : nullptr;
}

