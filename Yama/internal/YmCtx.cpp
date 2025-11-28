

#include "YmCtx.h"

#include "general.h"
#include "YmParcel.h"
#include "YmItem.h"


YmCtx::YmCtx(ym::Safe<YmDm> domain) :
    domain(domain),
    loader(std::make_shared<_ym::CtxLoader>(domain->loader)) {}

std::shared_ptr<YmParcel> YmCtx::import(const std::string& path) {
    return loader->import(path);
}

std::shared_ptr<YmParcel> YmCtx::import(YmPID pid) {
    return loader->import(pid);
}

std::shared_ptr<YmItem> YmCtx::load(const std::string& fullname) {
    return loader->load(fullname);
}

std::shared_ptr<YmItem> YmCtx::load(YmGID gid) {
    return loader->load(gid);
}

void YmCtx::pIterStart(ym::Safe<YmCtx> ctx) noexcept {
    _pIt = ctx->loader->parcels().begin();
    _pEnd = ctx->loader->parcels().end();
}

void YmCtx::pIterStartFrom(ym::Safe<YmCtx> ctx, ym::Safe<YmParcel> parcel) noexcept {
    _pIt = ctx->loader->parcels().find(parcel->pid);
    _pEnd = ctx->loader->parcels().end();
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

thread_local _ym::Area<YmParcel>::Iterator YmCtx::_pIt = {};
thread_local _ym::Area<YmParcel>::Iterator YmCtx::_pEnd = {};

