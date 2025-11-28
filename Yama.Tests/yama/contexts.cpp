

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

TEST(Contexts, Load_AutoLoadsDeps) {
    SETUP_ERRCOUNTER;
    SETUP_DM;
    SETUP_CTX(ctx);
    SETUP_PARCELDEF(A_def);
    SETUP_PARCELDEF(B_def);
    YmLID A_f_lid = ymParcelDef_FnItem(A_def, "f");
    YmLID B_f_lid = ymParcelDef_FnItem(B_def, "f");
    ASSERT_NE(ymParcelDef_RefConst(B_def, B_f_lid, "A:f"), YM_NO_CONST); // First const of B:f is ref-const to A:f.
    ASSERT_EQ(ymDm_BindParcelDef(dm, "A", A_def), YM_TRUE);
    ASSERT_EQ(ymDm_BindParcelDef(dm, "B", B_def), YM_TRUE);
    // Test that B:f auto-loads A:f as a dependency during B:f load.
    YmItem* B_f = ymCtx_Load(ctx, "B:f"); // Should load A:f.
    ASSERT_NE(B_f, nullptr);
    // Having already called ymCtx_Load to auto-load A:f, and having confirmed that B_f
    // isn't nullptr, now it's okay to load A:f explicitly to compare w/.
    YmItem* A_f = ymCtx_Load(ctx, "A:f");
    ASSERT_NE(A_f, nullptr);
    // Query first const of B:f (which should be A:f.)
    ASSERT_EQ(ymItem_Consts(B_f), 1);
    ASSERT_EQ(ymItem_ConstType(B_f, 0), YmConstType_Ref);
    YmItem* B_f_first_const = ymItem_RefConst(B_f, 0);
    ASSERT_NE(B_f_first_const, nullptr);
    // Now compare B_f_first_const against A_f to see that B:f auto-loaded A:f.
    EXPECT_EQ(B_f_first_const, A_f);
}

TEST(Contexts, Load_DepGraphCycle) {
    SETUP_ERRCOUNTER;
    SETUP_DM;
    SETUP_CTX(ctx);
    SETUP_PARCELDEF(A_def);
    SETUP_PARCELDEF(B_def);
    // Given A:f and B:f which have first constant table ref-constants referring to one
    // another, test that these can load successfully despite dep graph cycle.
    YmLID A_f_lid = ymParcelDef_FnItem(A_def, "f");
    YmLID B_f_lid = ymParcelDef_FnItem(B_def, "f");
    ASSERT_NE(ymParcelDef_RefConst(A_def, A_f_lid, "B:f"), YM_NO_CONST); // First const of A:f is ref-const to B:f.
    ASSERT_NE(ymParcelDef_RefConst(B_def, B_f_lid, "A:f"), YM_NO_CONST); // First const of B:f is ref-const to A:f.
    ASSERT_EQ(ymDm_BindParcelDef(dm, "A", A_def), YM_TRUE);
    ASSERT_EQ(ymDm_BindParcelDef(dm, "B", B_def), YM_TRUE);
    YmItem* A_f = ymCtx_Load(ctx, "A:f");
    YmItem* B_f = ymCtx_Load(ctx, "B:f");
    ASSERT_NE(A_f, nullptr);
    ASSERT_NE(B_f, nullptr);
    ASSERT_EQ(ymItem_Consts(A_f), 1);
    ASSERT_EQ(ymItem_Consts(B_f), 1);
    ASSERT_EQ(ymItem_ConstType(A_f, 0), YmConstType_Ref);
    ASSERT_EQ(ymItem_ConstType(B_f, 0), YmConstType_Ref);
    YmItem* A_f_first_const = ymItem_RefConst(A_f, 0);
    YmItem* B_f_first_const = ymItem_RefConst(B_f, 0);
    ASSERT_NE(A_f_first_const, nullptr);
    ASSERT_NE(B_f_first_const, nullptr);
    EXPECT_EQ(A_f_first_const, B_f);
    EXPECT_EQ(B_f_first_const, A_f);
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

TEST(Contexts, LoadByGID) {
    SETUP_ERRCOUNTER;
    SETUP_DM;
    SETUP_CTX(ctx);
    SETUP_PARCELDEF(p_def);
    YmLID item_lid = ymParcelDef_FnItem(p_def, "abc");
    ASSERT_NE(item_lid, YM_NO_LID);
    BIND_AND_IMPORT(ctx, p, p_def, "p");
    YmItem* item = ymCtx_LoadByGID(ctx, ymGID(ymParcel_PID(p), item_lid));
    ASSERT_NE(item, nullptr);
    EXPECT_EQ(ymItem_GID(item), ymGID(ymParcel_PID(p), item_lid));
    EXPECT_STREQ(ymItem_Fullname(item), "p:abc");
    EXPECT_EQ(ymItem_Kind(item), YmKind_Fn);
}

TEST(Contexts, LoadByGID_ParcelNotFound) {
    SETUP_ERRCOUNTER;
    SETUP_DM;
    SETUP_CTX(ctx);
    EXPECT_EQ(ymCtx_LoadByGID(ctx, ymGID(500, 0)), nullptr);
    EXPECT_GE(err[YmErrCode_ParcelNotFound], 1);
}

TEST(Contexts, LoadByGID_ItemNotFound) {
    SETUP_ERRCOUNTER;
    SETUP_DM;
    SETUP_CTX(ctx);
    SETUP_PARCELDEF(p_def);
    BIND_AND_IMPORT(ctx, p, p_def, "p");
    EXPECT_EQ(ymCtx_LoadByGID(ctx, ymGID(ymParcel_PID(p), 0)), nullptr);
    EXPECT_GE(err[YmErrCode_ItemNotFound], 1);
}

TEST(Contexts, LoadByGID_AutoImportsDeps) {
    // TODO: I don't know how to test this, as we lack a way to get the PID w/out importing.
}

TEST(Contexts, LoadByGID_AutoLoadsDeps) {
    SETUP_ERRCOUNTER;
    SETUP_DM;
    SETUP_CTX(ctx);
    SETUP_PARCELDEF(A_def);
    SETUP_PARCELDEF(B_def);
    YmLID A_f_lid = ymParcelDef_FnItem(A_def, "f");
    YmLID B_f_lid = ymParcelDef_FnItem(B_def, "f");
    ASSERT_NE(ymParcelDef_RefConst(B_def, B_f_lid, "A:f"), YM_NO_CONST); // First const of B:f is ref-const to A:f.
    ASSERT_EQ(ymDm_BindParcelDef(dm, "A", A_def), YM_TRUE);
    ASSERT_EQ(ymDm_BindParcelDef(dm, "B", B_def), YM_TRUE);
    YmParcel* A = ymCtx_Import(ctx, "A");
    YmParcel* B = ymCtx_Import(ctx, "B");
    ASSERT_NE(A, nullptr);
    ASSERT_NE(B, nullptr);
    // Test that B:f auto-loads A:f as a dependency during B:f load.
    YmItem* B_f = ymCtx_LoadByGID(ctx, ymGID(ymParcel_PID(B), B_f_lid)); // Should load A:f.
    ASSERT_NE(B_f, nullptr);
    // Having already called ymCtx_Load to auto-load A:f, and having confirmed that B_f
    // isn't nullptr, now it's okay to load A:f explicitly to compare w/.
    YmItem* A_f = ymCtx_LoadByGID(ctx, ymGID(ymParcel_PID(A), A_f_lid));
    ASSERT_NE(A_f, nullptr);
    // Query first const of B:f (which should be A:f.)
    ASSERT_EQ(ymItem_Consts(B_f), 1);
    ASSERT_EQ(ymItem_ConstType(B_f, 0), YmConstType_Ref);
    YmItem* B_f_first_const = ymItem_RefConst(B_f, 0);
    ASSERT_NE(B_f_first_const, nullptr);
    // Now compare B_f_first_const against A_f to see that B:f auto-loaded A:f.
    EXPECT_EQ(B_f_first_const, A_f);
}

TEST(Contexts, LoadByGID_DepGraphCycle) {
    SETUP_ERRCOUNTER;
    SETUP_DM;
    SETUP_CTX(ctx);
    SETUP_PARCELDEF(A_def);
    SETUP_PARCELDEF(B_def);
    // Given A:f and B:f which have first constant table ref-constants referring to one
    // another, test that these can load successfully despite dep graph cycle.
    YmLID A_f_lid = ymParcelDef_FnItem(A_def, "f");
    YmLID B_f_lid = ymParcelDef_FnItem(B_def, "f");
    ASSERT_NE(ymParcelDef_RefConst(A_def, A_f_lid, "B:f"), YM_NO_CONST); // First const of A:f is ref-const to B:f.
    ASSERT_NE(ymParcelDef_RefConst(B_def, B_f_lid, "A:f"), YM_NO_CONST); // First const of B:f is ref-const to A:f.
    ASSERT_EQ(ymDm_BindParcelDef(dm, "A", A_def), YM_TRUE);
    ASSERT_EQ(ymDm_BindParcelDef(dm, "B", B_def), YM_TRUE);
    YmParcel* A = ymCtx_Import(ctx, "A");
    YmParcel* B = ymCtx_Import(ctx, "B");
    ASSERT_NE(A, nullptr);
    ASSERT_NE(B, nullptr);
    YmItem* A_f = ymCtx_LoadByGID(ctx, ymGID(ymParcel_PID(A), A_f_lid));
    YmItem* B_f = ymCtx_LoadByGID(ctx, ymGID(ymParcel_PID(B), B_f_lid));
    ASSERT_NE(A_f, nullptr);
    ASSERT_NE(B_f, nullptr);
    ASSERT_EQ(ymItem_Consts(A_f), 1);
    ASSERT_EQ(ymItem_Consts(B_f), 1);
    ASSERT_EQ(ymItem_ConstType(A_f, 0), YmConstType_Ref);
    ASSERT_EQ(ymItem_ConstType(B_f, 0), YmConstType_Ref);
    YmItem* A_f_first_const = ymItem_RefConst(A_f, 0);
    YmItem* B_f_first_const = ymItem_RefConst(B_f, 0);
    ASSERT_NE(A_f_first_const, nullptr);
    ASSERT_NE(B_f_first_const, nullptr);
    EXPECT_EQ(A_f_first_const, B_f);
    EXPECT_EQ(B_f_first_const, A_f);
}

TEST(Contexts, LoadByGID_AcrossCtxBoundaries) {
    YmGID item_gid{};
    SETUP_ERRCOUNTER;
    SETUP_DM;
    SETUP_CTX(ctx1);
    {
        // Setup parcel p for dm.
        SETUP_PARCELDEF(p_def);
        YmLID item_lid = ymParcelDef_FnItem(p_def, "f");
        ASSERT_NE(item_lid, YM_NO_LID);
        BIND_AND_IMPORT(ctx1, p, p_def, "p");
        item_gid = ymGID(ymParcel_PID(p), item_lid);

        // Load in ctx1.
        YmItem* item = ymCtx_LoadByGID(ctx1, item_gid);
        ASSERT_NE(item, nullptr);
        EXPECT_EQ(ymItem_GID(item), item_gid);
        EXPECT_STREQ(ymItem_Fullname(item), "p:f");
        EXPECT_EQ(ymItem_Kind(item), YmKind_Fn);
    }
    SETUP_CTX(ctx2);
    {
        // Load in ctx2 (ie. across context boundary.)
        YmItem* item = ymCtx_LoadByGID(ctx2, item_gid);
        ASSERT_NE(item, nullptr);
        EXPECT_EQ(ymItem_GID(item), item_gid);
        EXPECT_STREQ(ymItem_Fullname(item), "p:f");
        EXPECT_EQ(ymItem_Kind(item), YmKind_Fn);
    }
}

