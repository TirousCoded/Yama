

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
    using ID = YmPID;
    using Name = std::string;


    const ID pid;
    const Name path;
    const std::shared_ptr<_ym::ParcelInfo> info;


    YmParcel(std::string path, std::shared_ptr<_ym::ParcelInfo> info);


    YmWord items() const noexcept;
    const _ym::ItemInfo* item(const std::string& localName) const noexcept;
    const _ym::ItemInfo* item(YmLID lid) const noexcept;

    inline const ID& getID() const noexcept { return pid; }
    inline const Name& getName() const noexcept { return path; }


private:
    //


    static std::atomic<YmPID> _nextPID;


    static YmPID _acquirePID() noexcept;
};

