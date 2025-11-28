

#include "YmParcel.h"


YmParcel::YmParcel(std::string path, std::shared_ptr<_ym::ParcelInfo> info) :
    pid(_acquirePID()),
    path(std::move(path)),
    info(std::move(info)) {}

YmWord YmParcel::items() const noexcept {
    return info->items();
}

const _ym::ItemInfo* YmParcel::item(const std::string& localName) const noexcept {
    return info->item(localName);
}

const _ym::ItemInfo* YmParcel::item(YmLID lid) const noexcept {
    return info->item(lid);
}

std::atomic<YmPID> YmParcel::_nextPID = 0;

YmPID YmParcel::_acquirePID() noexcept {
    // TODO: This'll break if _nextPID ever overflows.
    return _nextPID.fetch_add(1);
}

