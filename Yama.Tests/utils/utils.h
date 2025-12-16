

#pragma once


#include <string>
#include <vector>
#include <yama++/resources.h>

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
    inline const std::vector<std::string> illegalFullnames{
        // Illegal Path Components
        "",
        "/",
        "//",
        "/def",
        "abc/",
        "abc//def",
        "abc/def", // Legal path, illegal fullname.

        ":",
        "/:",
        "//:",
        "/def:",
        "abc/:",
        "abc//def:",
        "abc/def:",

        ":xyz",
        "/:xyz",
        "//:xyz",
        "/def:xyz",
        "abc/:xyz",
        "abc//def:xyz",

        // Illegal *Head* Components
        "abc:xyz:",
        "abc:xy:z",
        "abc:xyz/",
        "abc:xy/z",
    };
}


#define SETUP_ALL(ctx) \
SETUP_ERRCOUNTER; \
SETUP_DM; \
SETUP_CTX(ctx)

#define SETUP_DM \
YmDm* dm = ymDm_Create(); \
ASSERT_TRUE(dm); \
auto dm_ = ym::bindScoped(ym::Safe(dm))

#define SETUP_CTX(name) \
YmCtx* name = ymCtx_Create(dm); \
ASSERT_TRUE(name); \
auto name ## _ = ym::bindScoped(ym::Safe(name))

#define SETUP_CALLSIGDEF(name) \
YmCallSigDef* name = ymCallSigDef_Create(); \
ASSERT_TRUE(name); \
auto name ## _ = ym::bindScoped(ym::Safe(name))

#define SETUP_PARCELDEF(name) \
YmParcelDef* name = ymParcelDef_Create(); \
ASSERT_TRUE(name); \
auto name ## _ = ym::bindScoped(ym::Safe(name))

#define BIND_AND_IMPORT(ctx, name, def, path_cstr) \
EXPECT_EQ(ymDm_BindParcelDef(dm, path_cstr, def), YM_TRUE); \
auto name = ymCtx_Import(ctx, path_cstr); \
ASSERT_TRUE(name)

