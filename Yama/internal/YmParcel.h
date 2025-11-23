

#pragma once


#include <memory>

#include "../yama/yama.h"
#include "../yama++/general.h"
#include "../yama++/Safe.h"
#include "ParcelData.h"


struct YmParcel final {
public:
    std::shared_ptr<_ym::ParcelData> data;


    inline YmParcel(std::shared_ptr<_ym::ParcelData> data) :
        data(data) {
    }

    
    inline YmPID pid() const noexcept { return ym::deref(data).pid; }
    inline const std::string& path() const noexcept { return ym::deref(data).path; }

    inline YmWord items() const noexcept { return ym::deref(data).items(); }
    inline const _ym::ItemInfo* item(const std::string& localName) const noexcept { return ym::deref(data).item(localName); }
    inline const _ym::ItemInfo* item(YmLID lid) const noexcept { return ym::deref(data).item(lid); }
};

