

#pragma once


#ifdef _YM_FORBID_INCLUDE_IN_YAMA_DOT_H
#error Not allowed to expose this header file to header file yama.h!
#endif


#include <memory>
#include <mutex>
#include <unordered_map>

#include "../yama/yama.h"
#include "../yama++/Safe.h"
#include "ParcelData.h"


struct YmDm final {
public:
    YmDm() = default;


    // TODO: Maybe add string interning later.

    YmBool bindParcelDef(const std::string& path, ym::Safe<YmParcelDef> parceldef);
    std::shared_ptr<_ym::ParcelData> import(const std::string& path);
    std::shared_ptr<_ym::ParcelData> import(YmPID pid);


private:
    std::mutex _mtx;
    std::unordered_map<std::string, std::shared_ptr<_ym::ParcelData>> _bindings;
    std::unordered_map<std::string, std::shared_ptr<_ym::ParcelData>> _importsByPath;
    std::unordered_map<YmPID, std::shared_ptr<_ym::ParcelData>> _importsByPID;


    std::shared_ptr<_ym::ParcelData> _fetch(const std::string& path) const noexcept;
    std::shared_ptr<_ym::ParcelData> _fetch(YmPID pid) const noexcept;
    void _download(const std::string& path);
};

