

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
    SETUP_ERRCOUNTER;
    SETUP_DM;
    SETUP_CTX(ctx);
    EXPECT_EQ(ymCtx_Dm(ctx), dm);
}

TEST(Contexts, Import) {
    SETUP_ERRCOUNTER;
    SETUP_DM;
    SETUP_CTX(ctx);
    SETUP_PARCELDEF(p);
    std::string path = taul::utf8_s(u8"ab魂💩cd"); // Ensure can handle UTF-8.
    ymDm_BindParcelDef(dm, path.c_str(), p);
    auto a = ymCtx_Import(ctx, path.c_str());
    auto b = ymCtx_Import(ctx, path.c_str()); // Should yield same result.
    ASSERT_TRUE(a);
    ASSERT_TRUE(b);
    ASSERT_EQ(a, b);
}

TEST(Contexts, Import_IllegalPath) {
    for (const auto& path : illegalPaths) {
        SETUP_ERRCOUNTER;
        SETUP_DM;
        SETUP_CTX(ctx);
        SETUP_PARCELDEF(p);
        EXPECT_EQ(ymCtx_Import(ctx, path.c_str()), nullptr)
            << "path == \"" << path << "\"";
        EXPECT_GE(err[YmErrCode_IllegalPath], 1);
    }
}

TEST(Contexts, Import_ParcelNotFound) {
    SETUP_ERRCOUNTER;
    SETUP_DM;
    SETUP_CTX(ctx);
    EXPECT_EQ(ymCtx_Import(ctx, "missing"), nullptr);
    EXPECT_GE(err[YmErrCode_ParcelNotFound], 1);
}

TEST(Contexts, Import_AcrossCtxBoundaries) {
    SETUP_ERRCOUNTER;
    SETUP_DM;
    YmPID expected_pid{};
    // Setup first ctx and discern PID.
    SETUP_CTX(ctx1);
    // We setup and import this 'other' parcel first in order to make it harder
    // to get false positives on this test. (ie. If impl has ctx's have local
    // PID counters, this stops them from easily lining up w/ one another.)
    SETUP_PARCELDEF(other_def);
    SETUP_PARCELDEF(p_def);
    BIND_AND_IMPORT(ctx1, other, other_def, "other");
    BIND_AND_IMPORT(ctx1, p, p_def, "p");
    expected_pid = ymParcel_PID(p);
    // Setup second ctx and test across boundary.
    SETUP_CTX(ctx2);
    YmParcel* result = ymCtx_Import(ctx2, "p"); // Call under test.
    ASSERT_NE(result, nullptr);
    EXPECT_EQ(ymParcel_PID(result), expected_pid);
}

TEST(Contexts, ImportByPID) {
    SETUP_ERRCOUNTER;
    SETUP_DM;
    SETUP_CTX(ctx);
    SETUP_PARCELDEF(p_def);
    BIND_AND_IMPORT(ctx, p, p_def, "p");
    EXPECT_EQ(ymCtx_ImportByPID(ctx, ymParcel_PID(p)), p);
}

TEST(Contexts, ImportByPID_ParcelNotFound) {
    SETUP_ERRCOUNTER;
    SETUP_DM;
    SETUP_CTX(ctx);
    EXPECT_EQ(ymCtx_ImportByPID(ctx, 500), nullptr);
    EXPECT_GE(err[YmErrCode_ParcelNotFound], 1);
}

TEST(Contexts, ImportByPID_AcrossCtxBoundaries) {
    SETUP_ERRCOUNTER;
    SETUP_DM;
    YmPID expected_pid{};
    // Setup first ctx and discern PID.
    SETUP_CTX(ctx1);
    // We setup and import this 'other' parcel first in order to make it harder
    // to get false positives on this test. (ie. If impl has ctx's have local
    // PID counters, this stops them from easily lining up w/ one another.)
    SETUP_PARCELDEF(other_def);
    SETUP_PARCELDEF(p_def);
    BIND_AND_IMPORT(ctx1, other, other_def, "other");
    BIND_AND_IMPORT(ctx1, p, p_def, "p");
    expected_pid = ymParcel_PID(p);
    // Setup second ctx and test across boundary.
    SETUP_CTX(ctx2);
    YmParcel* result = ymCtx_ImportByPID(ctx2, expected_pid); // Call under test.
    ASSERT_NE(result, nullptr);
    EXPECT_EQ(ymParcel_PID(result), expected_pid);
}

TEST(Contexts, Load) {
    SETUP_ERRCOUNTER;
    SETUP_DM;
    SETUP_CTX(ctx);
    SETUP_PARCELDEF(p_def);
    std::string name = taul::utf8_s(u8"ab魂💩cd"); // Ensure can handle UTF-8.
    std::string fullname = taul::utf8_s(u8"p:ab魂💩cd"); // Ensure can handle UTF-8.
    YmLID item_lid = ymParcelDef_FnItem(p_def, name.c_str());
    ASSERT_NE(item_lid, YM_NO_LID);
    BIND_AND_IMPORT(ctx, p, p_def, "p");
    YmItem* item = ymCtx_Load(ctx, fullname.c_str());
    ASSERT_NE(item, nullptr);
    EXPECT_EQ(ymItem_GID(item), ymGID(ymParcel_PID(p), item_lid));
    EXPECT_STREQ(ymItem_Fullname(item), fullname.c_str());
    EXPECT_EQ(ymItem_Kind(item), YmKind_Fn);
}

TEST(Contexts, Load_IllegalFullname) {
    for (const auto& fullname : illegalFullnames) {
        SETUP_ERRCOUNTER;
        SETUP_DM;
        SETUP_CTX(ctx);
        EXPECT_EQ(ymCtx_Load(ctx, fullname.c_str()), nullptr)
            << "fullname == \"" << fullname << "\"";
        EXPECT_GE(err[YmErrCode_IllegalFullname], 1);
    }
}

TEST(Contexts, Load_ParcelNotFound) {
    SETUP_ERRCOUNTER;
    SETUP_DM;
    SETUP_CTX(ctx);
    EXPECT_EQ(ymCtx_Load(ctx, "missing:xyz"), nullptr);
    EXPECT_GE(err[YmErrCode_ParcelNotFound], 1);
}

TEST(Contexts, Load_ItemNotFound) {
    SETUP_ERRCOUNTER;
    SETUP_DM;
    SETUP_CTX(ctx);
    SETUP_PARCELDEF(p_def);
    BIND_AND_IMPORT(ctx, p, p_def, "p");
    EXPECT_EQ(ymCtx_Load(ctx, "p:xyz"), nullptr);
    EXPECT_GE(err[YmErrCode_ItemNotFound], 1);
}

TEST(Contexts, Load_AutoImportsDeps) {
    SETUP_ERRCOUNTER;
    SETUP_DM;
    SETUP_CTX(ctx);
    SETUP_PARCELDEF(p_def);
    YmLID item_lid = ymParcelDef_FnItem(p_def, "f");
    ASSERT_NE(item_lid, YM_NO_LID);
    ASSERT_EQ(ymDm_BindParcelDef(dm, "p", p_def), YM_TRUE);
    // This test tests that ymCtx_Load can import parcel p equiv to calling ymCtx_Import,
    // such that p gets imported w/out any prior ymCtx_Import call by end-user.
    YmItem* item = ymCtx_Load(ctx, "p:f"); // Should import p.
    ASSERT_NE(item, nullptr);
    // Having already called ymCtx_Load to auto-import p, and having confirmed that item
    // isn't nullptr, now it's okay to import p explicitly to query its PID.
    YmParcel* p = ymCtx_Import(ctx, "p");
    ASSERT_NE(p, nullptr);
    EXPECT_EQ(ymItem_GID(item), ymGID(ymParcel_PID(p), item_lid));
    EXPECT_STREQ(ymItem_Fullname(item), "p:f");
    EXPECT_EQ(ymItem_Kind(item), YmKind_Fn);
}
#error tomorrow add ref-constants, NO MORE DELAYING!!! get to generics as soon as we can!!!
TEST(Contexts, Load_AutoLoadsDeps) {
    FAIL();
}

TEST(Contexts, Load_DepGraphCycle) {
    FAIL();
}

TEST(Contexts, Load_AcrossCtxBoundaries) {
    YmGID expected_gid{};
    SETUP_ERRCOUNTER;
    SETUP_DM;
    SETUP_CTX(ctx1);
    {
        // Setup parcel p for dm.
        SETUP_PARCELDEF(p_def);
        YmLID item_lid = ymParcelDef_FnItem(p_def, "f");
        ASSERT_NE(item_lid, YM_NO_LID);
        BIND_AND_IMPORT(ctx1, p, p_def, "p");
        expected_gid = ymGID(ymParcel_PID(p), item_lid);

        // Load in ctx1.
        YmItem* item = ymCtx_Load(ctx1, "p:f");
        ASSERT_NE(item, nullptr);
        EXPECT_EQ(ymItem_GID(item), expected_gid);
        EXPECT_STREQ(ymItem_Fullname(item), "p:f");
        EXPECT_EQ(ymItem_Kind(item), YmKind_Fn);
    }
    SETUP_CTX(ctx2);
    {
        // Load in ctx2 (ie. across context boundary.)
        YmItem* item = ymCtx_Load(ctx2, "p:f");
        ASSERT_NE(item, nullptr);
        EXPECT_EQ(ymItem_GID(item), expected_gid);
        EXPECT_STREQ(ymItem_Fullname(item), "p:f");
        EXPECT_EQ(ymItem_Kind(item), YmKind_Fn);
    }
}

