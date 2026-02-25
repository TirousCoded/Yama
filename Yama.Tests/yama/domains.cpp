

#include <gtest/gtest.h>
#include <taul/strings.h>
#include <yama/yama.h>

#include "../utils/utils.h"


TEST(Domains, CreateAndDestroy) {
    SETUP_ERRCOUNTER;
    SETUP_DM; // Macro will do the create/destroy.
}

TEST(Domains, RefCounting) {
    SETUP_ERRCOUNTER;
    auto dm = ymDm_Create();
    ASSERT_TRUE(dm);
    EXPECT_EQ(ymDm_RefCount(dm), 1); // Initial
    EXPECT_EQ(ymDm_Secure(dm), 1); // 1 -> 2
    EXPECT_EQ(ymDm_RefCount(dm), 2);
    EXPECT_EQ(ymDm_Secure(dm), 2); // 2 -> 3
    EXPECT_EQ(ymDm_RefCount(dm), 3);
    EXPECT_EQ(ymDm_Release(dm), 3); // 3 -> 2
    EXPECT_EQ(ymDm_RefCount(dm), 2);
    EXPECT_EQ(ymDm_Release(dm), 2); // 2 -> 1
    EXPECT_EQ(ymDm_RefCount(dm), 1);
    EXPECT_EQ(ymDm_Release(dm), 1); // Destroys
}

TEST(Domains, BindParcelDef) {
    SETUP_ALL(ctx);
    SETUP_PARCELDEF(p_def);
    std::string path = taul::utf8_s(u8"ab魂💩cd"); // Ensure can handle UTF-8.
    ASSERT_FALSE(ymCtx_Import(ctx, path.c_str())); // Not available yet.
    EXPECT_EQ(ymDm_BindParcelDef(dm, path.c_str(), p_def), YM_TRUE);
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
        SETUP_PARCELDEF(p_def);
        EXPECT_EQ(ymDm_BindParcelDef(dm, path.c_str(), p_def), YM_FALSE)
            << "path == \"" << path << "\"";
        EXPECT_EQ(err[YmErrCode_IllegalSpecifier], 1);
    }
}

// TODO: What about testing ymDm_AddRedirect UTF-8 support?

TEST(Domains, AddRedirect) {
    SETUP_ALL(ctx);
    SETUP_PARCELDEF(p_def);
    ymParcelDef_AddFn(p_def, "f", "alt:Int", ymInertCallBhvrFn, nullptr);
    ymDm_BindParcelDef(dm, "p", p_def);
    ASSERT_EQ(ymDm_AddRedirect(dm, "p", "alt", "yama"), YM_TRUE);
    YmType* Int = load(ctx, "yama:Int");
    YmType* p_f = load(ctx, "p:f");
    EXPECT_EQ(ymType_ReturnType(p_f), Int);
}

TEST(Domains, AddRedirect_Normalizes) {
    SETUP_ALL(ctx);
    SETUP_PARCELDEF(p_a_def);
    ymParcelDef_AddFn(p_a_def, "f", "alt/a:A", ymInertCallBhvrFn, nullptr);
    SETUP_PARCELDEF(q_a_def);
    ymParcelDef_AddStruct(q_a_def, "A");
    ymDm_BindParcelDef(dm, "p/a", p_a_def);
    ymDm_BindParcelDef(dm, "q/a", q_a_def);
    ASSERT_EQ(ymDm_AddRedirect(dm, "  p  \n\r  /  a \r\r\r   ", " \n\t alt   / \na   ", "  q  \n\t\n  /\r\r  a \t\t\t  "), YM_TRUE);
    YmType* Int = load(ctx, "yama:Int");
    YmType* p_a_f = load(ctx, "p/a:f");
    YmType* q_a_A = load(ctx, "q/a:A");
    EXPECT_EQ(ymType_ReturnType(p_a_f), q_a_A);
}

TEST(Domains, AddRedirect_Overwriting) {
    SETUP_ALL(ctx);
    SETUP_PARCELDEF(p_def);
    ymParcelDef_AddFn(p_def, "f", "alt:Int", ymInertCallBhvrFn, nullptr);
    ymDm_BindParcelDef(dm, "p", p_def);
    ASSERT_EQ(ymDm_AddRedirect(dm, "p", "alt", "missing"), YM_TRUE); // Overwrite this.
    ASSERT_EQ(ymDm_AddRedirect(dm, "p", "alt", "yama"), YM_TRUE);
    YmType* Int = load(ctx, "yama:Int");
    YmType* p_f = load(ctx, "p:f");
    EXPECT_EQ(ymType_ReturnType(p_f), Int);
}

TEST(Domains, AddRedirect_Overwriting_ParcelRedirectsAreLockedUponFirstImport) {
    SETUP_ALL(ctx);
    SETUP_PARCELDEF(p_def);
    ymParcelDef_AddFn(p_def, "f", "alt:Int", ymInertCallBhvrFn, nullptr);
    ymParcelDef_AddFn(p_def, "g", "alt:Float", ymInertCallBhvrFn, nullptr);
    ymDm_BindParcelDef(dm, "p", p_def);
    ASSERT_EQ(ymDm_AddRedirect(dm, "p", "alt", "yama"), YM_TRUE);
    YmType* p_f = load(ctx, "p:f");
    ASSERT_EQ(ymDm_AddRedirect(dm, "p", "alt", "missing"), YM_TRUE); // Should be ignored.
    YmType* p_g = load(ctx, "p:g");
    YmType* Int = load(ctx, "yama:Int");
    YmType* Float = load(ctx, "yama:Float");
    EXPECT_EQ(ymType_ReturnType(p_f), Int);
    EXPECT_EQ(ymType_ReturnType(p_g), Float);
}

TEST(Domains, AddRedirect_IllegalSpecifier) {
    for (const auto& path : illegalPaths) {
        SETUP_ERRCOUNTER;
        SETUP_DM;
        EXPECT_EQ(ymDm_AddRedirect(dm, path.c_str(), "abc", "abc"), YM_FALSE) << "path == \"" << path << "\"";
        EXPECT_EQ(ymDm_AddRedirect(dm, "abc", path.c_str(), "abc"), YM_FALSE) << "path == \"" << path << "\"";
        EXPECT_EQ(ymDm_AddRedirect(dm, "abc", "abc", path.c_str()), YM_FALSE) << "path == \"" << path << "\"";
        EXPECT_EQ(err[YmErrCode_IllegalSpecifier], 3);
    }
}

