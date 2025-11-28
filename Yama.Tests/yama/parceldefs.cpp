

#include <gtest/gtest.h>
#include <taul/strings.h>
#include <yama/yama.h>
#include <yama++/print.h>

#include "../utils/utils.h"


TEST(ParcelDefs, CreateAndDestroy) {
    SETUP_ERRCOUNTER;
    SETUP_PARCELDEF(parceldef); // Macro will do the create/destroy.
}

TEST(ParcelDefs, FnItem) {
    SETUP_ERRCOUNTER;
    SETUP_PARCELDEF(parceldef);
    std::string otherName = taul::utf8_s(u8"ab魂💩cd"); // Ensure can handle UTF-8.
    std::string otherFullname = taul::utf8_s(u8"p:ab魂💩cd"); // Ensure can handle UTF-8.
    YmLID foo_lid = ymParcelDef_FnItem(parceldef, "foo");
    YmLID bar_lid = ymParcelDef_FnItem(parceldef, "bar");
    YmLID other_lid = ymParcelDef_FnItem(parceldef, otherName.c_str());
    ASSERT_NE(foo_lid, YM_NO_LID);
    ASSERT_NE(bar_lid, YM_NO_LID);
    ASSERT_NE(other_lid, YM_NO_LID);
    {
        SETUP_DM;
        SETUP_CTX(ctx);
        BIND_AND_IMPORT(ctx, parcel, parceldef, "p");
        auto foo = ymCtx_Load(ctx, "p:foo");
        auto bar = ymCtx_Load(ctx, "p:bar");
        auto other = ymCtx_Load(ctx, otherFullname.c_str());
        ASSERT_TRUE(foo);
        ASSERT_TRUE(bar);
        ASSERT_TRUE(other);
        EXPECT_EQ(ymItem_GID(foo), ymGID(ymParcel_PID(parcel), foo_lid));
        EXPECT_EQ(ymItem_GID(bar), ymGID(ymParcel_PID(parcel), bar_lid));
        EXPECT_EQ(ymItem_GID(other), ymGID(ymParcel_PID(parcel), other_lid));
        EXPECT_STREQ(ymItem_Fullname(foo), "p:foo");
        EXPECT_STREQ(ymItem_Fullname(bar), "p:bar");
        EXPECT_STREQ(ymItem_Fullname(other), otherFullname.c_str());
        EXPECT_EQ(ymItem_Kind(foo), YmKind_Fn);
        EXPECT_EQ(ymItem_Kind(bar), YmKind_Fn);
        EXPECT_EQ(ymItem_Kind(other), YmKind_Fn);
    }
}

TEST(ParcelDefs, FnItem_ItemNameConflict) {
    SETUP_ERRCOUNTER;
    SETUP_PARCELDEF(parceldef);
    ASSERT_NE(ymParcelDef_FnItem(parceldef, "foo"), YM_NO_LID);
    ASSERT_EQ(ymParcelDef_FnItem(parceldef, "foo"), YM_NO_LID);
    EXPECT_GE(err[YmErrCode_ItemNameConflict], 1);
}

TEST(ParcelDefs, ConstAddingFns) {
    SETUP_ERRCOUNTER;
    SETUP_PARCELDEF(p_def);
    YmLID f_lid = ymParcelDef_FnItem(p_def, "f");
    YmLID A_lid = ymParcelDef_FnItem(p_def, "A");
    YmLID B_lid = ymParcelDef_FnItem(p_def, "B");
    ASSERT_NE(f_lid, YM_NO_LID);
    ASSERT_NE(A_lid, YM_NO_LID);
    ASSERT_NE(B_lid, YM_NO_LID);

    // Test each of our ymParcelDef_***Const fns, testing them adding new entries, querying
    // existing entries, and that LIDs are sequential as expected.

    // We put this code in a loop and run it twice such that iteration #1 tests adding of new
    // constants, and iteration #2 tests querying of existing constants.
    for (YmWord i = 0; i < 2; i++) {
        if (i == 0) ym::println("Iteration #1 (Adding New Constants)");
        if (i == 1) ym::println("Iteration #2 (Querying Existing Constants)");

        // Our 'offset' variable is used to help ease the process of add new constant types.
        YmConst offset = 0;

        // Add w/out existing of same const type.
        static_assert(YmConstType_Num == 6);
        EXPECT_EQ(ymParcelDef_IntConst(p_def, f_lid, 10), offset++);
        EXPECT_EQ(ymParcelDef_UIntConst(p_def, f_lid, 10), offset++);
        EXPECT_EQ(ymParcelDef_FloatConst(p_def, f_lid, 3.14159), offset++);
        EXPECT_EQ(ymParcelDef_BoolConst(p_def, f_lid, true), offset++);
        EXPECT_EQ(ymParcelDef_RuneConst(p_def, f_lid, U'💩'), offset++);
        EXPECT_EQ(ymParcelDef_RefConst(p_def, f_lid, "p:A"), offset++);

        // Add w/ existing of same const type.
        static_assert(YmConstType_Num == 6);
        EXPECT_EQ(ymParcelDef_IntConst(p_def, f_lid, 11), offset++);
        EXPECT_EQ(ymParcelDef_UIntConst(p_def, f_lid, 11), offset++);
        EXPECT_EQ(ymParcelDef_FloatConst(p_def, f_lid, 5.01), offset++);
        EXPECT_EQ(ymParcelDef_BoolConst(p_def, f_lid, false), offset++);
        EXPECT_EQ(ymParcelDef_RuneConst(p_def, f_lid, U'&'), offset++);
        EXPECT_EQ(ymParcelDef_RefConst(p_def, f_lid, "p:B"), offset++);
    }

    SETUP_DM;
    SETUP_CTX(ctx);
    ymDm_BindParcelDef(dm, "p", p_def);
    YmItem* f = ymCtx_Load(ctx, "p:f");
    YmItem* A = ymCtx_Load(ctx, "p:A");
    YmItem* B = ymCtx_Load(ctx, "p:B");
    ASSERT_TRUE(f);
    ASSERT_TRUE(A);
    ASSERT_TRUE(B);

    // Check were added.
    ASSERT_EQ(ymItem_Consts(f), YmConstType_Num * 2);

    // Check are correct types.
    std::vector<YmConstType> expected_const_types{
        YmConstType_Int,
        YmConstType_UInt,
        YmConstType_Float,
        YmConstType_Bool,
        YmConstType_Rune,
        YmConstType_Ref,

        YmConstType_Int,
        YmConstType_UInt,
        YmConstType_Float,
        YmConstType_Bool,
        YmConstType_Rune,
        YmConstType_Ref,
    };
    ASSERT_EQ(expected_const_types.size(), YmConstType_Num * 2);
    for (YmConst i = 0; i < YmConstType_Num * 2; i++) {
        ASSERT_EQ(ymItem_ConstType(f, i), expected_const_types[i])
            << "i == " << i;
    }

    // Check are correct values.
    static_assert(YmConstType_Num == 6);
    YmConst index = 0;
    EXPECT_EQ(ymItem_IntConst(f, index++), 10);
    EXPECT_EQ(ymItem_UIntConst(f, index++), 10);
    EXPECT_DOUBLE_EQ(ymItem_FloatConst(f, index++), 3.14159);
    EXPECT_EQ(ymItem_BoolConst(f, index++), true);
    EXPECT_EQ(ymItem_RuneConst(f, index++), U'💩');
    EXPECT_EQ(ymItem_RefConst(f, index++), A);

    EXPECT_EQ(ymItem_IntConst(f, index++), 11);
    EXPECT_EQ(ymItem_UIntConst(f, index++), 11);
    EXPECT_DOUBLE_EQ(ymItem_FloatConst(f, index++), 5.01);
    EXPECT_EQ(ymItem_BoolConst(f, index++), false);
    EXPECT_EQ(ymItem_RuneConst(f, index++), U'&');
    EXPECT_EQ(ymItem_RefConst(f, index++), B);
}

TEST(ParcelDefs, ConstAddingFns_ItemNotFound) {
    SETUP_ERRCOUNTER;
    SETUP_PARCELDEF(p_def);
    static_assert(YmConstType_Num == 6);

    EXPECT_EQ(ymParcelDef_IntConst(p_def, 500, 10), YM_NO_CONST);
    EXPECT_GE(err[YmErrCode_ItemNotFound], 1);
    err.reset();

    EXPECT_EQ(ymParcelDef_UIntConst(p_def, 500, 10), YM_NO_CONST);
    EXPECT_GE(err[YmErrCode_ItemNotFound], 1);
    err.reset();

    EXPECT_EQ(ymParcelDef_FloatConst(p_def, 500, 3.14159), YM_NO_CONST);
    EXPECT_GE(err[YmErrCode_ItemNotFound], 1);
    err.reset();

    EXPECT_EQ(ymParcelDef_BoolConst(p_def, 500, true), YM_NO_CONST);
    EXPECT_GE(err[YmErrCode_ItemNotFound], 1);
    err.reset();

    EXPECT_EQ(ymParcelDef_RuneConst(p_def, 500, U'a'), YM_NO_CONST);
    EXPECT_GE(err[YmErrCode_ItemNotFound], 1);
    err.reset();

    EXPECT_EQ(ymParcelDef_RefConst(p_def, 500, "yama:Int"), YM_NO_CONST);
    EXPECT_GE(err[YmErrCode_ItemNotFound], 1);
    err.reset();
}

TEST(ParcelDefs, ConstAddingFns_MaxConstsLimit) {
    SETUP_ERRCOUNTER;
    SETUP_PARCELDEF(p_def);

    YmLID f_lid = ymParcelDef_FnItem(p_def, "f");
    ASSERT_NE(f_lid, YM_NO_LID);

    // Saturate the constant table.
    for (YmInt i = 0; i <= YmInt(YM_MAX_CONST); i++) {
        // Add 50'000 to ensure int constant adding further below doesn't overlap.
        ASSERT_EQ(ymParcelDef_IntConst(p_def, f_lid, i + 50'000), YmConst(i));
    }

    // All below should fail due to constant table being full.
    static_assert(YmConstType_Num == 6);

    EXPECT_EQ(ymParcelDef_IntConst(p_def, f_lid, 10), YM_NO_CONST);
    EXPECT_GE(err[YmErrCode_MaxConstsLimit], 1);
    err.reset();

    EXPECT_EQ(ymParcelDef_UIntConst(p_def, f_lid, 10), YM_NO_CONST);
    EXPECT_GE(err[YmErrCode_MaxConstsLimit], 1);
    err.reset();

    EXPECT_EQ(ymParcelDef_FloatConst(p_def, f_lid, 3.14159), YM_NO_CONST);
    EXPECT_GE(err[YmErrCode_MaxConstsLimit], 1);
    err.reset();

    EXPECT_EQ(ymParcelDef_BoolConst(p_def, f_lid, true), YM_NO_CONST);
    EXPECT_GE(err[YmErrCode_MaxConstsLimit], 1);
    err.reset();

    EXPECT_EQ(ymParcelDef_RuneConst(p_def, f_lid, U'a'), YM_NO_CONST);
    EXPECT_GE(err[YmErrCode_MaxConstsLimit], 1);
    err.reset();

    EXPECT_EQ(ymParcelDef_RefConst(p_def, f_lid, "yama:Int"), YM_NO_CONST);
    EXPECT_GE(err[YmErrCode_MaxConstsLimit], 1);
    err.reset();
}

