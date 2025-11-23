

#include "ParcelData.h"


YmWord _ym::ParcelData::items() const noexcept {
    return info->items();
}

const _ym::ItemInfo* _ym::ParcelData::item(const std::string& localName) const noexcept {
    return info->item(localName);
}

const _ym::ItemInfo* _ym::ParcelData::item(YmLID lid) const noexcept {
    return info->item(lid);
}

std::atomic<YmPID> _ym::ParcelData::_nextPID = 0;

YmPID _ym::ParcelData::_acquirePID() noexcept {
    // TODO: This'll break if _nextPID ever overflows.
    return _nextPID.fetch_add(1);
}

