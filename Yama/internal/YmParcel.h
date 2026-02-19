

#pragma once


#include <atomic>
#include <memory>
#include <string>

#include "../yama/yama.h"
#include "../yama++/general.h"
#include "../yama++/Safe.h"
#include "ParcelInfo.h"
#include "Redirects.h"


struct YmParcel final : public std::enable_shared_from_this<YmParcel> {
public:
    using Name = _ym::Spec;


    const Name path;
    const std::shared_ptr<_ym::ParcelInfo> info;
    std::optional<_ym::RedirectSet> redirects;


    YmParcel(_ym::Spec path, std::shared_ptr<_ym::ParcelInfo> info);


    size_t items() const noexcept;
    const _ym::ItemInfo* item(const std::string& localName) const noexcept;

    inline const Name& getName() const noexcept { return path; }


    // NOTE: This overwrites existing redirects value, which is important as we NEED to recompute
    //       in circumstances where an import calls this on a binding, but then fails, and then a
    //       later import of the binding succeeds, resulting in the first result value becoming
    //       stale.

    void resolveRedirects(_ym::Redirects& state);
};

