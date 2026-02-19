

#include "YmCtx.h"

#include "general.h"
#include "YmParcel.h"
#include "YmItem.h"
#include "SpecSolver.h"


YmCtx::YmCtx(ym::Safe<YmDm> domain) :
    domain(domain),
    loader(std::make_shared<_ym::CtxLoader>(domain->loader)) {}

std::shared_ptr<YmParcel> YmCtx::import(const std::string& path) {
    if (auto s = _ym::Spec::path(path)) {
        return loader->import(*s);
    }
    else {
        _ym::Global::raiseErr(
            YmErrCode_IllegalSpecifier,
            "Import failed; \"{}\" syntax error!",
            path);
        return nullptr;
    }
}

std::shared_ptr<YmItem> YmCtx::load(const std::string& fullname) {
    if (auto s = _ym::Spec::item(fullname)) {
        return loader->load(*s);
    }
    else {
        _ym::Global::raiseErr(
            YmErrCode_IllegalSpecifier,
            "Load failed; \"{}\" syntax error!",
            fullname);
        return nullptr;
    }
}

void YmCtx::pIterStart(ym::Safe<YmCtx> ctx) noexcept {
    _pIt = ctx->loader->commits().parcels.begin();
    _pEnd = ctx->loader->commits().parcels.end();
}

void YmCtx::pIterStartFrom(ym::Safe<YmCtx> ctx, ym::Safe<YmParcel> parcel) noexcept {
    _pIt = ctx->loader->commits().parcels.find(parcel->getName());
    _pEnd = ctx->loader->commits().parcels.end();
}

void YmCtx::pIterAdvance(size_t n) noexcept {
    for (size_t i = 0; i < n; i++) {
        if (pIterDone()) return;
        std::advance(_pIt, 1);
    }
}

YmParcel* YmCtx::pIterGet() noexcept {
    return
        pIterDone()
        ? nullptr
        : &*_pIt;
}

bool YmCtx::pIterDone() noexcept {
    return _pIt == _pEnd;
}

thread_local _ym::Section<YmParcel>::Iterator YmCtx::_pIt = {};
thread_local _ym::Section<YmParcel>::Iterator YmCtx::_pEnd = {};

