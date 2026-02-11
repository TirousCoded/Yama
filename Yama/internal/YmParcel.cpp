

#include "YmParcel.h"


YmParcel::YmParcel(std::string path, std::shared_ptr<_ym::ParcelInfo> info) :
    path(std::move(path)),
    info(std::move(info)) {
}

size_t YmParcel::items() const noexcept {
    return info->items();
}

const _ym::ItemInfo* YmParcel::item(const std::string& localName) const noexcept {
    return info->item(localName);
}

void YmParcel::resolveRedirects(_ym::Redirects& state) {
    redirects = state.compute(path);
}

