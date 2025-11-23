

#include <gtest/gtest.h>
#include <yama/yama.h>

#include "../utils/utils.h"


TEST(Items, GID) {
    SETUP_ERRCOUNTER;
    SETUP_DM;
    SETUP_CTX(ctx);
    SETUP_PARCELDEF(p_def);
    YmLID f_lid = ymParcelDef_FnItem(p_def, "f");
    ASSERT_NE(f_lid, YM_NO_LID);
    ASSERT_EQ(ymDm_BindParcelDef(dm, "p", p_def), YM_TRUE);
    auto item = ymCtx_Load(ctx, "p:f");
    ASSERT_TRUE(item);
    YmParcel* p = ymCtx_Import(ctx, "p");
    ASSERT_NE(p, nullptr);
    EXPECT_EQ(ymItem_GID(item), ymGID(ymParcel_PID(p), f_lid));
}

TEST(Items, Fullname) {
    SETUP_ERRCOUNTER;
    SETUP_DM;
    SETUP_CTX(ctx);
    SETUP_PARCELDEF(p_def);
    ASSERT_NE(ymParcelDef_FnItem(p_def, "f"), YM_NO_LID);
    ASSERT_EQ(ymDm_BindParcelDef(dm, "p", p_def), YM_TRUE);
    auto item = ymCtx_Load(ctx, "p:f");
    ASSERT_TRUE(item);
    EXPECT_STREQ(ymItem_Fullname(item), "p:f");
}

TEST(Items, Kind) {
    SETUP_ERRCOUNTER;
    SETUP_DM;
    SETUP_CTX(ctx);
    SETUP_PARCELDEF(p_def);
    ASSERT_NE(ymParcelDef_FnItem(p_def, "f"), YM_NO_LID);
    ASSERT_EQ(ymDm_BindParcelDef(dm, "p", p_def), YM_TRUE);
    auto item = ymCtx_Load(ctx, "p:f");
    ASSERT_TRUE(item);
    EXPECT_EQ(ymItem_Kind(item), YmKind_Fn);
}

TEST(Items, Consts) {
    SETUP_ERRCOUNTER;
    SETUP_DM;
    SETUP_CTX(ctx);
    {
        SETUP_PARCELDEF(empty_def);
        ymParcelDef_FnItem(empty_def, "f");
        ymDm_BindParcelDef(dm, "empty", empty_def);
        YmItem* f = ymCtx_Load(ctx, "empty:f");
        ASSERT_TRUE(f);

        EXPECT_EQ(ymItem_Consts(f), 0);
    }
    {
        SETUP_PARCELDEF(nonempty_def);
        YmLID f_lid = ymParcelDef_FnItem(nonempty_def, "f");
        ymParcelDef_IntConst(nonempty_def, f_lid, 10);
        ymParcelDef_IntConst(nonempty_def, f_lid, 11);
        ymParcelDef_IntConst(nonempty_def, f_lid, 12);
        ymDm_BindParcelDef(dm, "nonempty", nonempty_def);
        YmItem* f = ymCtx_Load(ctx, "nonempty:f");
        ASSERT_TRUE(f);

        EXPECT_EQ(ymItem_Consts(f), 3);
    }
}

TEST(Items, ConstType) {
    SETUP_ERRCOUNTER;
    SETUP_DM;
    SETUP_CTX(ctx);

    SETUP_PARCELDEF(p_def);
    YmLID f_lid = ymParcelDef_FnItem(p_def, "f");
    
    static_assert(YmConstType_Num == 5);
    YmConst int_c = ymParcelDef_IntConst(p_def, f_lid, 10);
    YmConst uint_c = ymParcelDef_UIntConst(p_def, f_lid, 10);
    YmConst float_c = ymParcelDef_FloatConst(p_def, f_lid, 3.14159);
    YmConst bool_c = ymParcelDef_BoolConst(p_def, f_lid, true);
    YmConst rune_c = ymParcelDef_RuneConst(p_def, f_lid, U'💩');

    ymDm_BindParcelDef(dm, "p", p_def);
    YmItem* f = ymCtx_Load(ctx, "p:f");
    ASSERT_TRUE(f);
    ASSERT_EQ(ymItem_Consts(f), YmConstType_Num);

    static_assert(YmConstType_Num == 5);
    EXPECT_EQ(ymItem_ConstType(f, int_c), YmConstType_Int);
    EXPECT_EQ(ymItem_ConstType(f, uint_c), YmConstType_UInt);
    EXPECT_EQ(ymItem_ConstType(f, float_c), YmConstType_Float);
    EXPECT_EQ(ymItem_ConstType(f, bool_c), YmConstType_Bool);
    EXPECT_EQ(ymItem_ConstType(f, rune_c), YmConstType_Rune);
}

TEST(Items, ConstValueAccess) {
    SETUP_ERRCOUNTER;
    SETUP_DM;
    SETUP_CTX(ctx);

    SETUP_PARCELDEF(p_def);
    YmLID f_lid = ymParcelDef_FnItem(p_def, "f");

    static_assert(YmConstType_Num == 5);
    YmConst int_c = ymParcelDef_IntConst(p_def, f_lid, 10);
    YmConst uint_c = ymParcelDef_UIntConst(p_def, f_lid, 10);
    YmConst float_c = ymParcelDef_FloatConst(p_def, f_lid, 3.14159);
    YmConst bool_c = ymParcelDef_BoolConst(p_def, f_lid, true);
    YmConst rune_c = ymParcelDef_RuneConst(p_def, f_lid, U'💩');

    ymDm_BindParcelDef(dm, "p", p_def);
    YmItem* f = ymCtx_Load(ctx, "p:f");
    ASSERT_TRUE(f);
    ASSERT_EQ(ymItem_Consts(f), YmConstType_Num);

    static_assert(YmConstType_Num == 5);
    EXPECT_EQ(ymItem_IntConst(f, int_c), 10);
    EXPECT_EQ(ymItem_UIntConst(f, uint_c), 10);
    EXPECT_DOUBLE_EQ(ymItem_FloatConst(f, float_c), 3.14159);
    EXPECT_EQ(ymItem_BoolConst(f, bool_c), true);
    EXPECT_EQ(ymItem_RuneConst(f, rune_c), U'💩');
}

