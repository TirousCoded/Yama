

#pragma once


#include <atomic>
#include <memory>
#include <string>

#include "../yama/yama.h"
#include "ParcelInfo.h"


namespace _ym {


    // Encapsulates parcel info bound to a path and PID.
    class ParcelData final {
    public:
        const YmPID pid;
        const std::string path;
        const std::shared_ptr<ParcelInfo> info;


        inline ParcelData(std::string path, std::shared_ptr<ParcelInfo> info) :
            pid(_acquirePID()),
            path(std::move(path)),
            info(std::move(info)) {}


        YmWord items() const noexcept;
        const ItemInfo* item(const std::string& localName) const noexcept;
        const ItemInfo* item(YmLID lid) const noexcept;


    private:
        static std::atomic<YmPID> _nextPID;


        static YmPID _acquirePID() noexcept;
    };
}

