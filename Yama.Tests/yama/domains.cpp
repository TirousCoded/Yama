

#include <gtest/gtest.h>
#include <taul/strings.h>
#include <yama/yama.h>

#include "../utils/utils.h"


TEST(Domains, CreateAndDestroy) {
    SETUP_ERRCOUNTER;
    SETUP_DM; // Macro will do the create/destroy.
}

TEST(Domains, RType) {
    SETUP_ERRCOUNTER;
    SETUP_DM;
    EXPECT_EQ(ymRType(dm), YmRType_Dm);
}

TEST(Domains, RC) {
    SETUP_ERRCOUNTER;
    SETUP_DM;
    ASSERT_EQ(ymRefCount(dm), 1); // Initial value.
    ymAddRef(dm);
    EXPECT_EQ(ymRefCount(dm), 2);
    ymAddRef(dm);
    EXPECT_EQ(ymRefCount(dm), 3);
    ymDrop(dm);
    EXPECT_EQ(ymRefCount(dm), 2);
    ymDrop(dm);
    EXPECT_EQ(ymRefCount(dm), 1);
    // ScopedDrop will drop final one, and the actual release of
    // the resource is unobservable.
}

TEST(Domains, BindParcelDef) {
    SETUP_ERRCOUNTER;
    SETUP_DM;
    SETUP_CTX;
    SETUP_PARCELDEF(p);
    std::string path = taul::utf8_s(u8"ab魂💩cd"); // Ensure can handle UTF-8.
    ASSERT_FALSE(ymCtx_Import(ctx, path.c_str())); // Not available yet.
    ymDm_BindParcelDef(dm, path.c_str(), p);
    ASSERT_TRUE(ymCtx_Import(ctx, path.c_str())); // Available
}

TEST(Domains, BindParcelDef_IllegalPaths) {
    for (const auto& path : illegalPaths) {
        SETUP_ERRCOUNTER;
        SETUP_DM;
        SETUP_PARCELDEF(p);
        EXPECT_EQ(ymDm_BindParcelDef(dm, path.c_str(), p), YM_FALSE)
            << "path == \"" << path << "\"";
        EXPECT_EQ(err[YmErrCode_IllegalPath], 1);
    }
}

