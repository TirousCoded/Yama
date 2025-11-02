

#pragma once


#include <string>
#include <vector>
#include <yama++/ScopedDrop.h>

#include "ErrCounter.h"


namespace {
    inline const std::vector<std::string> illegalPaths{
        "",
        "/",
        "//",
        "/def",
        "abc/",
        "abc//def",
        ":",
        "ab:c",
        "abc/d:ef",
        "abc/def:",
    };
}

#define SETUP_DM \
YmDm* dm = ymDm_Create(); \
ASSERT_TRUE(dm); \
ym::ScopedDrop dm_(dm)

#define SETUP_CTX \
YmCtx* ctx = ymCtx_Create(dm); \
ASSERT_TRUE(ctx); \
ym::ScopedDrop ctx_(ctx)

#define SETUP_PARCELDEF(name) \
YmParcelDef* name = ymParcelDef_Create(); \
ASSERT_TRUE(name); \
ym::ScopedDrop name ## _(name)

#define SETUP_PARCEL(name, parceldef, path) \
ymDm_BindParcelDef(dm, path.c_str(), parceldef); \
YmParcel* name = ymCtx_Import(ctx, path.c_str()); \
ASSERT_TRUE(name); \
ym::ScopedDrop name ## _(name)

