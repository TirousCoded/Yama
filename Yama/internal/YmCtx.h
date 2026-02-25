

#pragma once


#ifdef _YM_FORBID_INCLUDE_IN_YAMA_DOT_H
#error Not allowed to expose this header file to header file yama.h!
#endif


#include <unordered_map>

#include "../yama/yama.h"
#include "../yama++/Safe.h"
#include "Loader.h"
#include "RefCounter.h"
#include "YmDm.h"


struct YmCtx final {
public:
    // refs is not managed internally by this class.
    _ym::AtomicRefCounter<YmRefCount> refs;

    const ym::Safe<YmDm> domain;
    const std::shared_ptr<_ym::CtxLoader> loader;


    YmCtx(ym::Safe<YmDm> domain);


    std::shared_ptr<YmParcel> import(const std::string& path);
    std::shared_ptr<YmType> load(const std::string& fullname);


    static void pIterStart(ym::Safe<YmCtx> ctx) noexcept;
    static void pIterStartFrom(ym::Safe<YmCtx> ctx, ym::Safe<YmParcel> parcel) noexcept;
    static void pIterAdvance(size_t n) noexcept;
    static YmParcel* pIterGet() noexcept;
    static bool pIterDone() noexcept;


private:
    //


    static thread_local _ym::Section<YmParcel>::Iterator _pIt, _pEnd;
};

