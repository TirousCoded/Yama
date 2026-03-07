

#include <gtest/gtest.h>
#include <taul/strings.h>
#include <yama/yama.h>

#include "../utils/utils.h"


TEST(Objects, CreateAndDestroy) {
    SETUP_ALL(ctx);
    // This'll handle the destruction of obj.
    SETUP_OBJ(obj, ymCtx_NewInt(ctx, 50));
}

TEST(Objects, RefCounting) {
    SETUP_ALL(ctx);
    auto obj = ymCtx_NewInt(ctx, 50);
    ASSERT_TRUE(obj);
    EXPECT_EQ(ymObj_RefCount(obj), 1); // Initial
    EXPECT_EQ(ymObj_Secure(obj), 1); // 1 -> 2
    EXPECT_EQ(ymObj_RefCount(obj), 2);
    EXPECT_EQ(ymObj_Secure(obj), 2); // 2 -> 3
    EXPECT_EQ(ymObj_RefCount(obj), 3);
    EXPECT_EQ(ymObj_Release(obj), 3); // 3 -> 2
    EXPECT_EQ(ymObj_RefCount(obj), 2);
    EXPECT_EQ(ymObj_Release(obj), 2); // 2 -> 1
    EXPECT_EQ(ymObj_RefCount(obj), 1);
    EXPECT_EQ(ymObj_Release(obj), 1); // Destroys
}

TEST(Objects, Type) {
    SETUP_ALL(ctx);
    SETUP_OBJ(obj, ymCtx_NewInt(ctx, 50));
    EXPECT_EQ(ymObj_Type(obj), ymCtx_LdInt(ctx));
}

TEST(Objects, Fmt) {
    SETUP_ALL(ctx);
    SETUP_OBJ(none, ymCtx_NewNone(ctx));
    SETUP_OBJ(int0, ymCtx_NewInt(ctx, -50));
    SETUP_OBJ(uint, ymCtx_NewUInt(ctx, 50));
    SETUP_OBJ(float0, ymCtx_NewFloat(ctx, 3.14159));
    SETUP_OBJ(bool0, ymCtx_NewBool(ctx, true));
    SETUP_OBJ(rune, ymCtx_NewRune(ctx, 'y'));
    {
        ScopedStr s(ymObj_Fmt(none));
        EXPECT_STREQ(s, "n/a");
    }
    {
        ScopedStr s(ymObj_Fmt(int0));
        ScopedStr e(ymInt_Fmt(-50, YM_TRUE, YmIntFmt_Dec, nullptr));
        EXPECT_STREQ(s, e);
    }
    {
        ScopedStr s(ymObj_Fmt(uint));
        ScopedStr e(ymUInt_Fmt(50, YM_TRUE, YmIntFmt_Dec, nullptr));
        EXPECT_STREQ(s, e);
    }
    {
        ScopedStr s(ymObj_Fmt(float0));
        ScopedStr e(ymFloat_Fmt(3.14159, nullptr));
        EXPECT_STREQ(s, e);
    }
    {
        ScopedStr s(ymObj_Fmt(bool0));
        EXPECT_STREQ(s, ymBool_Fmt(true));
    }
    {
        ScopedStr s(ymObj_Fmt(rune));
        ScopedStr e(ymRune_Fmt('y', YM_TRUE, YM_TRUE, YM_TRUE, YM_TRUE, nullptr));
        EXPECT_STREQ(s, e);
    }
}

TEST(Objects, ToInt) {
    SETUP_ALL(ctx);
    SETUP_OBJ(obj, ymCtx_NewInt(ctx, -50));
    ASSERT_EQ(ymObj_Type(obj), ymCtx_LdInt(ctx));
    EXPECT_EQ(ymObj_ToInt(obj), -50);
}

TEST(Objects, ToUInt) {
    SETUP_ALL(ctx);
    SETUP_OBJ(obj, ymCtx_NewUInt(ctx, 50));
    ASSERT_EQ(ymObj_Type(obj), ymCtx_LdUInt(ctx));
    EXPECT_EQ(ymObj_ToUInt(obj), 50);
}

TEST(Objects, ToFloat) {
    SETUP_ALL(ctx);
    SETUP_OBJ(obj, ymCtx_NewFloat(ctx, 3.14159));
    ASSERT_EQ(ymObj_Type(obj), ymCtx_LdFloat(ctx));
    EXPECT_EQ(ymObj_ToFloat(obj), 3.14159);
}

TEST(Objects, ToBool) {
    SETUP_ALL(ctx);
    SETUP_OBJ(obj, ymCtx_NewBool(ctx, YM_TRUE));
    ASSERT_EQ(ymObj_Type(obj), ymCtx_LdBool(ctx));
    EXPECT_EQ(ymObj_ToBool(obj), YM_TRUE);
}

TEST(Objects, ToRune) {
    SETUP_ALL(ctx);
    SETUP_OBJ(obj, ymCtx_NewRune(ctx, U'y'));
    ASSERT_EQ(ymObj_Type(obj), ymCtx_LdRune(ctx));
    EXPECT_EQ(ymObj_ToRune(obj), U'y');
}

