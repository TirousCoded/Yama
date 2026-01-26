

#include <gtest/gtest.h>
#include <taul/strings.h>
#include <yama/yama.h>

#include "../utils/utils.h"


TEST(Contexts, CreateAndDestroy) {
    SETUP_ERRCOUNTER;
    SETUP_DM;
    SETUP_CTX(ctx); // Macro will do the create/destroy.
}

TEST(Contexts, Dm) {
    SETUP_ALL(ctx);
    EXPECT_EQ(ymCtx_Dm(ctx), dm);
}

// NOTE: See special/importing.cpp for unit tests covering importing semantics.

TEST(Contexts, Import) {
    SETUP_ALL(ctx);
    SETUP_PARCELDEF(p);
    std::string path = taul::utf8_s(u8"ab魂💩cd"); // Ensure can handle UTF-8.
    ymDm_BindParcelDef(dm, path.c_str(), p);
    auto a = ymCtx_Import(ctx, path.c_str());
    auto b = ymCtx_Import(ctx, path.c_str()); // Should yield same result.
    ASSERT_TRUE(a);
    ASSERT_TRUE(b);
    ASSERT_EQ(a, b);
}

TEST(Contexts, Import_Normalizes) {
    SETUP_ALL(ctx);
    SETUP_PARCELDEF(p_def);
    ymDm_BindParcelDef(dm, "p/q/r", p_def);
    auto a = ymCtx_Import(ctx, "p/q/r");
    auto b = ymCtx_Import(ctx, "p  /  q /      \r r");
    auto c = ymCtx_Import(ctx, "    p/  q  \n\n\n /r   ");
    ASSERT_TRUE(a);
    EXPECT_EQ(a, b);
    EXPECT_EQ(a, c);
}

TEST(Contexts, Import_AcrossCtxBoundaries) {
    SETUP_ERRCOUNTER;
    SETUP_DM;
    SETUP_PARCELDEF(p_def);
    ymDm_BindParcelDef(dm, "p", p_def);
    SETUP_CTX(ctx1);
    SETUP_CTX(ctx2);
    EXPECT_EQ(ymCtx_Import(ctx1, "p"), ymCtx_Import(ctx2, "p"));
}

// NOTE: See special/loading.cpp for unit tests covering loading semantics.

TEST(Contexts, Load) {
    SETUP_ALL(ctx);
    SETUP_PARCELDEF(p_def);
    std::string name = taul::utf8_s(u8"ab魂💩cd"); // Ensure can handle UTF-8.
    std::string fullname = taul::utf8_s(u8"p:ab魂💩cd"); // Ensure can handle UTF-8.
    ymParcelDef_AddStruct(p_def, name.c_str());
    BIND_AND_IMPORT(ctx, p, p_def, "p");
    YmItem* item = ymCtx_Load(ctx, fullname.c_str());
    ASSERT_NE(item, nullptr);
    EXPECT_EQ(ymItem_Parcel(item), ymCtx_Import(ctx, "p"));
    EXPECT_STREQ(ymItem_Fullname(item), fullname.c_str());
    EXPECT_EQ(ymItem_Kind(item), YmKind_Struct);
}

TEST(Contexts, Load_Normalizes) {
    SETUP_ALL(ctx);
    SETUP_PARCELDEF(p_def);
    ymParcelDef_AddStruct(p_def, "A");
    ymDm_BindParcelDef(dm, "p/q/r", p_def);
    auto a = ymCtx_Load(ctx, "p/q/r:A");
    auto b = ymCtx_Load(ctx, "p  /  q /      \r r  :A");
    auto c = ymCtx_Load(ctx, "    p/  q  \n\n\n /r  :   A ");
    ASSERT_TRUE(a);
    EXPECT_EQ(a, b);
    EXPECT_EQ(a, c);
}

TEST(Contexts, Load_AcrossCtxBoundaries) {
    SETUP_ERRCOUNTER;
    SETUP_DM;
    SETUP_PARCELDEF(p_def);
    ymParcelDef_AddStruct(p_def, "A");
    ymDm_BindParcelDef(dm, "p", p_def);
    SETUP_CTX(ctx1);
    SETUP_CTX(ctx2);
    EXPECT_EQ(ymCtx_Load(ctx1, "p:A"), ymCtx_Load(ctx2, "p:A"));
}

