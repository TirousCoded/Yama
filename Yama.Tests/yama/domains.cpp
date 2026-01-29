

#include <gtest/gtest.h>
#include <taul/strings.h>
#include <yama/yama.h>

#include "../utils/utils.h"


TEST(Domains, CreateAndDestroy) {
    SETUP_ERRCOUNTER;
    SETUP_DM; // Macro will do the create/destroy.
}

TEST(Domains, BindParcelDef) {
    SETUP_ALL(ctx);
    SETUP_PARCELDEF(p);
    std::string path = taul::utf8_s(u8"ab魂💩cd"); // Ensure can handle UTF-8.
    ASSERT_FALSE(ymCtx_Import(ctx, path.c_str())); // Not available yet.
    EXPECT_EQ(ymDm_BindParcelDef(dm, path.c_str(), p), YM_TRUE);
    ASSERT_TRUE(ymCtx_Import(ctx, path.c_str())); // Available
}

TEST(Domains, BindParcelDef_Normalizes) {
    SETUP_ALL(ctx);
    SETUP_PARCELDEF(p_def);
    ymParcelDef_AddStruct(p_def, "A");
    ASSERT_TRUE(ymDm_BindParcelDef(dm, "  \r\r  \n  p   / \r\n   q    /  r   \n\n   \n  ", p_def));
    auto p = import(ctx, "p/q/r"); // Check imports correctly.
    EXPECT_STREQ(ymParcel_Path(p), "p/q/r");
    load(ctx, "p/q/r:A"); // Check works w/ load.
}

TEST(Domains, BindParcelDef_Overwriting) {
    SETUP_ALL(ctx);
    SETUP_PARCELDEF(p1_def);
    SETUP_PARCELDEF(p2_def);
    ymParcelDef_AddStruct(p2_def, "A"); // Used to differentiate.
    ASSERT_TRUE(ymDm_BindParcelDef(dm, "p", p1_def)); // Initial
    ASSERT_TRUE(ymDm_BindParcelDef(dm, "p", p2_def)); // Overwrite
    load(ctx, "p:A"); // Check it's actually the correct one.
}

TEST(Domains, BindParcelDef_PathBindError_CannotOverwriteYamaParcel) {
    SETUP_ALL(ctx);
    SETUP_PARCELDEF(p_def);
    ASSERT_FALSE(ymDm_BindParcelDef(dm, "yama", p_def)); // Overwrite
    EXPECT_EQ(err[YmErrCode_PathBindError], 1);
    load(ctx, "yama:Int"); // Check it's actually the correct one.
}

TEST(Domains, BindParcelDef_IllegalSpecifier) {
    for (const auto& path : illegalPaths) {
        SETUP_ERRCOUNTER;
        SETUP_DM;
        SETUP_PARCELDEF(p);
        EXPECT_EQ(ymDm_BindParcelDef(dm, path.c_str(), p), YM_FALSE)
            << "path == \"" << path << "\"";
        EXPECT_EQ(err[YmErrCode_IllegalSpecifier], 1);
    }
}

