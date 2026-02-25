

#pragma once


#ifdef _YM_FORBID_INCLUDE_IN_YAMA_DOT_H
#error Not allowed to expose this header file to header file yama.h!
#endif


#include <memory>

#include "../yama/yama.h"
#include "../yama++/Safe.h"
#include "Loader.h"
#include "RefCounter.h"


struct YmDm final {
public:
    // refs is not managed internally by this class.
    _ym::AtomicRefCounter<YmRefCount> refs;

    const std::shared_ptr<_ym::DmLoader> loader;


    YmDm();


    // TODO: Maybe add string interning later.

    bool bindParcelDef(const std::string& path, ym::Safe<YmParcelDef> parceldef);
    bool addRedirect(const std::string& subject, const std::string& before, const std::string& after);
};

