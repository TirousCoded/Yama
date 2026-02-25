

#include "YmParcel.h"


YmParcel::YmParcel(_ym::Spec path, std::shared_ptr<_ym::ParcelInfo> info) :
    path(std::move(path)),
    info(std::move(info)) {
}

size_t YmParcel::types() const noexcept {
    return info->types();
}

const _ym::TypeInfo* YmParcel::type(const std::string& localName) const noexcept {
    return info->type(localName);
}

void YmParcel::resolveRedirects(_ym::Redirects& state) {
    redirects = state.compute(path);
}

