

#include <gtest/gtest.h>
#include <taul/strings.h>
#include <yama/yama.h>

#include "../utils/utils.h"


TEST(Domains, CreateAndDestroy) {
    SETUP_ERRCOUNTER;
    SETUP_DM; // Macro will do the create/destroy.
}

TEST(Domains, BindParcelDef) {
    SETUP_ERRCOUNTER;
    SETUP_DM;
    SETUP_CTX(ctx);
    SETUP_PARCELDEF(p);
    std::string path = taul::utf8_s(u8"ab魂💩cd"); // Ensure can handle UTF-8.
    ASSERT_FALSE(ymCtx_Import(ctx, path.c_str())); // Not available yet.
    ymDm_BindParcelDef(dm, path.c_str(), p);
    ASSERT_TRUE(ymCtx_Import(ctx, path.c_str())); // Available
}

TEST(Domains, BindParcelDef_IllegalPath) {
    for (const auto& path : illegalPaths) {
        SETUP_ERRCOUNTER;
        SETUP_DM;
        SETUP_PARCELDEF(p);
        EXPECT_EQ(ymDm_BindParcelDef(dm, path.c_str(), p), YM_FALSE)
            << "path == \"" << path << "\"";
        EXPECT_EQ(err[YmErrCode_IllegalPath], 1);
    }
}

