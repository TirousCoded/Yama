

#pragma once


#include <atomic>
#include <memory>
#include <string>

#include "../yama/yama.h"
#include "../yama++/general.h"
#include "../yama++/Safe.h"
#include "ParcelInfo.h"


struct YmParcel final {
public:
    using Name = std::string;


    const Name path;
    const std::shared_ptr<_ym::ParcelInfo> info;


    YmParcel(std::string path, std::shared_ptr<_ym::ParcelInfo> info);


    YmWord items() const noexcept;
    const _ym::ItemInfo* item(const std::string& localName) const noexcept;

    inline const Name& getName() const noexcept { return path; }
};

