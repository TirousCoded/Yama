

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

        "abc:xyz",
        "abc:xyz::m",
        "abc:xyz::m::m",
        "abc:xyz[abc:xyz, abc:xyz]",
        "abc:xyz[abc:xyz, abc:xyz]::m",
        "abc:xyz[abc:xyz, abc:xyz]::m::m",
        "abc:xyz::m[abc:xyz, abc:xyz]",
        "abc:xyz::m[abc:xyz, abc:xyz]::m",
        "abc:xyz::m[abc:xyz, abc:xyz]::m::m",
        "abc:xyz() -> p:A",
        "abc:xyz(p:A) -> p:A",
        "abc:xyz(p:A, p:A) -> p:A",
        "abc:xyz::m() -> p:A",
        "abc:xyz::m(p:A) -> p:A",
        "abc:xyz::m(p:A, p:A) -> p:A",
        "abc:xyz[abc:xyz, abc:xyz]() -> p:A",
        "abc:xyz[abc:xyz, abc:xyz](p:A) -> p:A",
        "abc:xyz[abc:xyz, abc:xyz](p:A, p:A) -> p:A",
        "abc:xyz[abc:xyz, abc:xyz]::m() -> p:A",
        "abc:xyz[abc:xyz, abc:xyz]::m(p:A) -> p:A",
        "abc:xyz[abc:xyz, abc:xyz]::m(p:A, p:A) -> p:A",
        "abc:xyz[abc:xyz() -> p:A]",
        "abc:xyz(abc:xyz() -> p:A) -> p:A",
        "abc:xyz() -> abc:xyz() -> p:A",
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
        ":ghi",
        ":ghi::m",
        "/:",
        "//:",
        "/def:",
        "abc/:",
        "abc//def:",
        "abc/def:",

        "::",
        "::m",
        "/::",
        "//::",
        "/def::",
        "abc/::",
        "abc//def::",
        "abc/def::",
        "abc/def:ghi::",

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
        "abc:xyz[abc:xyz:]",
        "abc:xyz[abc:xy:z]",
        "abc:xyz[abc:xyz/]",
        "abc:xyz[abc:xy/z]",
        "abc:xyz[abc:xyz:, abc:xyz]",
        "abc:xyz[abc:xy:z, abc:xyz]",
        "abc:xyz[abc:xyz/, abc:xyz]",
        "abc:xyz[abc:xy/z, abc:xyz]",
        "abc:xyz,",
        "abc:xyz[abc:xyz,]",
        "abc:xyz[abc:xyz, abc:xyz]/m",
        "abc:xyz[abc:xyz() -> p:A]",
        "abc:xyz(abc:xyz() -> p:A) -> p:A",
        "abc:xyz() -> abc:xyz() -> p:A",
    };


    // Throws std::runtime_error on import error in order to force unit test to crash.
    inline ym::Safe<YmParcel> import(YmCtx* ctx, const std::string& path) {
        auto result = ymCtx_Import(ctx, path.c_str());
        EXPECT_TRUE(result);
        if (!result) throw std::runtime_error(""); // Abort test.
        return ym::Safe(result);
    }
    // Throws std::runtime_error on load error in order to force unit test to crash.
    inline ym::Safe<YmType> load(YmCtx* ctx, const std::string& fullname) {
        auto result = ymCtx_Load(ctx, fullname.c_str());
        EXPECT_TRUE(result);
        if (!result) throw std::runtime_error(""); // Abort test.
        return ym::Safe(result);
    }
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

#define SETUP_PARCELDEF(name) \
YmParcelDef* name = ymParcelDef_Create(); \
ASSERT_TRUE(name); \
auto name ## _ = ym::bindScoped(ym::Safe(name))

#define BIND_AND_IMPORT(ctx, name, def, path_cstr) \
EXPECT_EQ(ymDm_BindParcelDef(dm, path_cstr, def), YM_TRUE); \
auto name = ymCtx_Import(ctx, path_cstr); \
ASSERT_TRUE(name)

