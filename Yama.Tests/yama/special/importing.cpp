

#include <gtest/gtest.h>
#include <taul/strings.h>
#include <yama/yama.h>

#include "../../utils/utils.h"


TEST(Contexts, Fail_IllegalSpecifier) {
    for (const auto& path : illegalPaths) {
        SETUP_ERRCOUNTER;
        SETUP_DM;
        SETUP_CTX(ctx);
        SETUP_PARCELDEF(p);
        EXPECT_EQ(ymCtx_Import(ctx, path.c_str()), nullptr)
            << "path == \"" << path << "\"";
        EXPECT_EQ(err[YmErrCode_IllegalSpecifier], 1);
    }
}

TEST(Importing, Fail_ParcelNotFound) {
    SETUP_ERRCOUNTER;
    SETUP_DM;
    SETUP_CTX(ctx);
    EXPECT_EQ(ymCtx_Import(ctx, "missing"), nullptr);
    EXPECT_EQ(err[YmErrCode_ParcelNotFound], 1);
}

TEST(Importing, Fail_ParcelNotFound_HereNotAvailableForDirectImports) {
    SETUP_ERRCOUNTER;
    SETUP_DM;
    SETUP_CTX(ctx);
    EXPECT_EQ(ymCtx_Import(ctx, "%here"), nullptr);
    EXPECT_EQ(err[YmErrCode_ParcelNotFound], 1);
}

