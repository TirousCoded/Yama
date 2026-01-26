

#include <gtest/gtest.h>
#include <yama/yama.h>

#include "../utils/utils.h"


TEST(Items, Parcel) {
    SETUP_ALL(ctx);
    SETUP_PARCELDEF(p_def);
    ymParcelDef_AddStruct(p_def, "A");
    ymDm_BindParcelDef(dm, "p", p_def);
    auto A = ymCtx_Load(ctx, "p:A");
    ASSERT_TRUE(A);
    YmParcel* p = ymCtx_Import(ctx, "p");
    ASSERT_NE(p, nullptr);
    EXPECT_EQ(ymItem_Parcel(A), p);
}

TEST(Items, Fullname) {
    SETUP_ALL(ctx);
    SETUP_PARCELDEF(p_def);
    ymParcelDef_AddStruct(p_def, "A");
    ymDm_BindParcelDef(dm, "p", p_def);
    auto A = ymCtx_Load(ctx, "p:A");
    ASSERT_TRUE(A);
    EXPECT_STREQ(ymItem_Fullname(A), "p:A");
}

TEST(Items, Kind) {
    SETUP_ALL(ctx);
    SETUP_PARCELDEF(p_def);
    ymParcelDef_AddStruct(p_def, "A");
    ymDm_BindParcelDef(dm, "p", p_def);
    auto A = ymCtx_Load(ctx, "p:A");
    ASSERT_TRUE(A);
    EXPECT_EQ(ymItem_Kind(A), YmKind_Struct);
}

TEST(Items, Owner_MemberType) {
    SETUP_ALL(ctx);
    SETUP_PARCELDEF(p_def);
    auto A_index = ymParcelDef_AddStruct(p_def, "A");
    auto A_m_index = ymParcelDef_AddMethod(p_def, A_index, "m", "p:A", ymInertCallBhvrFn, nullptr);
    ymDm_BindParcelDef(dm, "p", p_def);
    auto A = ymCtx_Load(ctx, "p:A");
    auto A_m = ymCtx_Load(ctx, "p:A::m");
    ASSERT_TRUE(A);
    ASSERT_TRUE(A_m);
    EXPECT_EQ(ymItem_Owner(A_m), A);
}

TEST(Items, Owner_NonMemberType) {
    SETUP_ALL(ctx);
    SETUP_PARCELDEF(p_def);
    ymParcelDef_AddStruct(p_def, "A");
    ymDm_BindParcelDef(dm, "p", p_def);
    auto A = ymCtx_Load(ctx, "p:A");
    ASSERT_TRUE(A);
    EXPECT_EQ(ymItem_Owner(A), nullptr); // p:A is not a member type.
}

TEST(Items, Members_OwnerType) {
    SETUP_ALL(ctx);
    SETUP_PARCELDEF(p_def);
    auto A_index = ymParcelDef_AddStruct(p_def, "A");
    auto A_m1_index = ymParcelDef_AddMethod(p_def, A_index, "m1", "p:A", ymInertCallBhvrFn, nullptr);
    auto A_m2_index = ymParcelDef_AddMethod(p_def, A_index, "m2", "p:A", ymInertCallBhvrFn, nullptr);
    ymDm_BindParcelDef(dm, "p", p_def);
    auto A = ymCtx_Load(ctx, "p:A");
    auto A_m1 = ymCtx_Load(ctx, "p:A::m1");
    auto A_m2 = ymCtx_Load(ctx, "p:A::m2");
    ASSERT_TRUE(A);
    ASSERT_TRUE(A_m1);
    ASSERT_TRUE(A_m2);
    EXPECT_EQ(ymItem_Members(A), 2);
}

TEST(Items, Members_NonOwnerType) {
    SETUP_ALL(ctx);
    SETUP_PARCELDEF(p_def);
    auto A_index = ymParcelDef_AddStruct(p_def, "A");
    auto A_m_index = ymParcelDef_AddMethod(p_def, A_index, "m", "p:A", ymInertCallBhvrFn, nullptr);
    ymDm_BindParcelDef(dm, "p", p_def);
    auto A = ymCtx_Load(ctx, "p:A");
    auto A_m = ymCtx_Load(ctx, "p:A::m");
    ASSERT_TRUE(A);
    ASSERT_TRUE(A_m);
    EXPECT_EQ(ymItem_Members(A_m), 0); // p:A::m is not an owner type.
}

TEST(Items, MemberByIndex_OwnerType) {
    SETUP_ALL(ctx);
    SETUP_PARCELDEF(p_def);
    auto A_index = ymParcelDef_AddStruct(p_def, "A");
    auto A_m1_index = ymParcelDef_AddMethod(p_def, A_index, "m1", "p:A", ymInertCallBhvrFn, nullptr);
    auto A_m2_index = ymParcelDef_AddMethod(p_def, A_index, "m2", "p:A", ymInertCallBhvrFn, nullptr);
    ymDm_BindParcelDef(dm, "p", p_def);
    auto A = ymCtx_Load(ctx, "p:A");
    auto A_m1 = ymCtx_Load(ctx, "p:A::m1");
    auto A_m2 = ymCtx_Load(ctx, "p:A::m2");
    ASSERT_TRUE(A);
    ASSERT_TRUE(A_m1);
    ASSERT_TRUE(A_m2);
    EXPECT_EQ(ymItem_MemberByIndex(A, 0), A_m1);
    EXPECT_EQ(ymItem_MemberByIndex(A, 1), A_m2);
    EXPECT_EQ(ymItem_MemberByIndex(A, 2), nullptr); // Out-Of-Bounds
}

TEST(Items, MemberByIndex_NonOwnerType) {
    SETUP_ALL(ctx);
    SETUP_PARCELDEF(p_def);
    auto A_index = ymParcelDef_AddStruct(p_def, "A");
    auto A_m_index = ymParcelDef_AddMethod(p_def, A_index, "m", "p:A", ymInertCallBhvrFn, nullptr);
    ymDm_BindParcelDef(dm, "p", p_def);
    auto A = ymCtx_Load(ctx, "p:A");
    auto A_m = ymCtx_Load(ctx, "p:A::m");
    ASSERT_TRUE(A);
    ASSERT_TRUE(A_m);
    EXPECT_EQ(ymItem_MemberByIndex(A_m, 0), nullptr); // p:A::m is not an owner type.
}

TEST(Items, MemberByName_OwnerType) {
    SETUP_ALL(ctx);
    SETUP_PARCELDEF(p_def);
    auto A_index = ymParcelDef_AddStruct(p_def, "A");
    auto A_m1_index = ymParcelDef_AddMethod(p_def, A_index, "m1", "p:A", ymInertCallBhvrFn, nullptr);
    auto A_m2_index = ymParcelDef_AddMethod(p_def, A_index, "m2", "p:A", ymInertCallBhvrFn, nullptr);
    ymDm_BindParcelDef(dm, "p", p_def);
    auto A = ymCtx_Load(ctx, "p:A");
    auto A_m1 = ymCtx_Load(ctx, "p:A::m1");
    auto A_m2 = ymCtx_Load(ctx, "p:A::m2");
    ASSERT_TRUE(A);
    ASSERT_TRUE(A_m1);
    ASSERT_TRUE(A_m2);
    EXPECT_EQ(ymItem_MemberByName(A, "m1"), A_m1);
    EXPECT_EQ(ymItem_MemberByName(A, "m2"), A_m2);
    EXPECT_EQ(ymItem_MemberByName(A, "missing"), nullptr); // Missing
}

TEST(Items, MemberByName_NonOwnerType) {
    SETUP_ALL(ctx);
    SETUP_PARCELDEF(p_def);
    auto A_index = ymParcelDef_AddStruct(p_def, "A");
    auto A_m_index = ymParcelDef_AddMethod(p_def, A_index, "m", "p:A", ymInertCallBhvrFn, nullptr);
    ymDm_BindParcelDef(dm, "p", p_def);
    auto A = ymCtx_Load(ctx, "p:A");
    auto A_m = ymCtx_Load(ctx, "p:A::m");
    ASSERT_TRUE(A);
    ASSERT_TRUE(A_m);
    EXPECT_EQ(ymItem_MemberByName(A_m, "m"), nullptr); // p:A::m is not an owner type.
}

static void setup_for_item_params_tests(YmDm* dm, YmParcelDef* p_def) {
    ymAssert(dm != nullptr);
    ymAssert(p_def != nullptr);
    ymParcelDef_AddProtocol(p_def, "Any");
    ymParcelDef_AddStruct(p_def, "Int");
    ymParcelDef_AddStruct(p_def, "Float");
    auto A_index = ymParcelDef_AddStruct(p_def, "A");
    ymParcelDef_AddItemParam(p_def, A_index, "T", "p:Any");
    ymParcelDef_AddItemParam(p_def, A_index, "U", "p:Any");
    ymParcelDef_AddMethod(p_def, A_index, "m", "p:Int", ymInertCallBhvrFn, nullptr);
    ymDm_BindParcelDef(dm, "p", p_def);
}

TEST(Items, ItemParams_OwnerType) {
    SETUP_ALL(ctx);
    SETUP_PARCELDEF(p_def);
    setup_for_item_params_tests(dm, p_def);
    auto A = ymCtx_Load(ctx, "p:A[p:Int, p:Float]");
    ASSERT_TRUE(A);
    EXPECT_EQ(ymItem_ItemParams(A), 2);
}

TEST(Items, ItemParams_NonOwnerType) {
    SETUP_ALL(ctx);
    SETUP_PARCELDEF(p_def);
    setup_for_item_params_tests(dm, p_def);
    auto A_m = ymCtx_Load(ctx, "p:A[p:Int, p:Float]::m");
    ASSERT_TRUE(A_m);
    EXPECT_EQ(ymItem_ItemParams(A_m), 2);
}

TEST(Items, ItemParamByIndex_OwnerType) {
    SETUP_ALL(ctx);
    SETUP_PARCELDEF(p_def);
    setup_for_item_params_tests(dm, p_def);
    auto A = ymCtx_Load(ctx, "p:A[p:Int, p:Float]");
    auto Int = ymCtx_Load(ctx, "p:Int");
    auto Float = ymCtx_Load(ctx, "p:Float");
    ASSERT_TRUE(A);
    ASSERT_TRUE(Int);
    ASSERT_TRUE(Float);
    ASSERT_EQ(ymItem_ItemParams(A), 2);
    EXPECT_EQ(ymItem_ItemParamByIndex(A, 0), Int);
    EXPECT_EQ(ymItem_ItemParamByIndex(A, 1), Float);
    EXPECT_EQ(ymItem_ItemParamByIndex(A, 2), nullptr);
}

TEST(Items, ItemParamByIndex_NonOwnerType) {
    SETUP_ALL(ctx);
    SETUP_PARCELDEF(p_def);
    setup_for_item_params_tests(dm, p_def);
    auto A_m = ymCtx_Load(ctx, "p:A[p:Int, p:Float]::m");
    auto Int = ymCtx_Load(ctx, "p:Int");
    auto Float = ymCtx_Load(ctx, "p:Float");
    ASSERT_TRUE(A_m);
    ASSERT_TRUE(Int);
    ASSERT_TRUE(Float);
    ASSERT_EQ(ymItem_ItemParams(A_m), 2);
    EXPECT_EQ(ymItem_ItemParamByIndex(A_m, 0), Int);
    EXPECT_EQ(ymItem_ItemParamByIndex(A_m, 1), Float);
    EXPECT_EQ(ymItem_ItemParamByIndex(A_m, 2), nullptr);
}

TEST(Items, ItemParamByName_OwnerType) {
    SETUP_ALL(ctx);
    SETUP_PARCELDEF(p_def);
    setup_for_item_params_tests(dm, p_def);
    auto A = ymCtx_Load(ctx, "p:A[p:Int, p:Float]");
    auto Int = ymCtx_Load(ctx, "p:Int");
    auto Float = ymCtx_Load(ctx, "p:Float");
    ASSERT_TRUE(A);
    ASSERT_TRUE(Int);
    ASSERT_TRUE(Float);
    ASSERT_EQ(ymItem_ItemParams(A), 2);
    EXPECT_EQ(ymItem_ItemParamByName(A, "T"), Int);
    EXPECT_EQ(ymItem_ItemParamByName(A, "U"), Float);
    EXPECT_EQ(ymItem_ItemParamByName(A, "V"), nullptr);
}

TEST(Items, ItemParamByName_NonOwnerType) {
    SETUP_ALL(ctx);
    SETUP_PARCELDEF(p_def);
    setup_for_item_params_tests(dm, p_def);
    auto A_m = ymCtx_Load(ctx, "p:A[p:Int, p:Float]::m");
    auto Int = ymCtx_Load(ctx, "p:Int");
    auto Float = ymCtx_Load(ctx, "p:Float");
    ASSERT_TRUE(A_m);
    ASSERT_TRUE(Int);
    ASSERT_TRUE(Float);
    ASSERT_EQ(ymItem_ItemParams(A_m), 2);
    EXPECT_EQ(ymItem_ItemParamByName(A_m, "T"), Int);
    EXPECT_EQ(ymItem_ItemParamByName(A_m, "U"), Float);
    EXPECT_EQ(ymItem_ItemParamByName(A_m, "V"), nullptr);
}

static void setup_fn_item_with_three_params(YmDm* dm, YmParcelDef* p_def) {
    ymAssert(dm != nullptr);
    ymAssert(p_def != nullptr);
    auto A_index = ymParcelDef_AddFn(p_def, "A", "p:B", ymInertCallBhvrFn, nullptr);
    ymParcelDef_AddStruct(p_def, "B");
    ymParcelDef_AddStruct(p_def, "C");
    ymParcelDef_AddStruct(p_def, "D");
    ymParcelDef_AddParam(p_def, A_index, "x", "p:B");
    ymParcelDef_AddParam(p_def, A_index, "y", "p:C");
    ymParcelDef_AddParam(p_def, A_index, "z", "p:D");
    ymDm_BindParcelDef(dm, "p", p_def);
}

TEST(Items, ReturnType_Callable) {
    SETUP_ALL(ctx);
    SETUP_PARCELDEF(p_def);
    setup_fn_item_with_three_params(dm, p_def);
    auto A = ymCtx_Load(ctx, "p:A");
    auto B = ymCtx_Load(ctx, "p:B");
    ASSERT_TRUE(A);
    ASSERT_TRUE(B);
    EXPECT_EQ(ymItem_ReturnType(A), B);
}

TEST(Items, ReturnType_NonCallable) {
    SETUP_ALL(ctx);
    SETUP_PARCELDEF(p_def);
    ymParcelDef_AddStruct(p_def, "A");
    ymDm_BindParcelDef(dm, "p", p_def);
    auto A = ymCtx_Load(ctx, "p:A");
    ASSERT_TRUE(A);
    EXPECT_EQ(ymItem_ReturnType(A), nullptr);
}

TEST(Items, Params_Callable) {
    SETUP_ALL(ctx);
    SETUP_PARCELDEF(p_def);
    setup_fn_item_with_three_params(dm, p_def);
    auto A = ymCtx_Load(ctx, "p:A");
    auto B = ymCtx_Load(ctx, "p:B");
    auto C = ymCtx_Load(ctx, "p:C");
    auto D = ymCtx_Load(ctx, "p:D");
    ASSERT_TRUE(A);
    ASSERT_TRUE(B);
    ASSERT_TRUE(C);
    ASSERT_TRUE(D);
    EXPECT_EQ(ymItem_Params(A), 3);
}

TEST(Items, Params_NonCallable) {
    SETUP_ALL(ctx);
    SETUP_PARCELDEF(p_def);
    ymParcelDef_AddStruct(p_def, "A");
    ymDm_BindParcelDef(dm, "p", p_def);
    auto A = ymCtx_Load(ctx, "p:A");
    ASSERT_TRUE(A);
    EXPECT_EQ(ymItem_Params(A), 0);
}

TEST(Items, ParamName_Callable) {
    SETUP_ALL(ctx);
    SETUP_PARCELDEF(p_def);
    setup_fn_item_with_three_params(dm, p_def);
    auto A = ymCtx_Load(ctx, "p:A");
    auto B = ymCtx_Load(ctx, "p:B");
    auto C = ymCtx_Load(ctx, "p:C");
    auto D = ymCtx_Load(ctx, "p:D");
    ASSERT_TRUE(A);
    ASSERT_TRUE(B);
    ASSERT_TRUE(C);
    ASSERT_TRUE(D);
    EXPECT_STREQ(ymItem_ParamName(A, 0), "x");
    EXPECT_STREQ(ymItem_ParamName(A, 1), "y");
    EXPECT_STREQ(ymItem_ParamName(A, 2), "z");
}

TEST(Items, ParamName_ParamNotFound_Callable) {
    SETUP_ALL(ctx);
    SETUP_PARCELDEF(p_def);
    setup_fn_item_with_three_params(dm, p_def);
    auto A = ymCtx_Load(ctx, "p:A");
    auto B = ymCtx_Load(ctx, "p:B");
    auto C = ymCtx_Load(ctx, "p:C");
    auto D = ymCtx_Load(ctx, "p:D");
    ASSERT_TRUE(A);
    ASSERT_TRUE(B);
    ASSERT_TRUE(C);
    ASSERT_TRUE(D);
    EXPECT_EQ(ymItem_ParamName(A, 3), nullptr);
    EXPECT_EQ(err[YmErrCode_ParamNotFound], 1);
}

TEST(Items, ParamName_ParamNotFound_NonCallable) {
    SETUP_ALL(ctx);
    SETUP_PARCELDEF(p_def);
    ymParcelDef_AddStruct(p_def, "A");
    ymDm_BindParcelDef(dm, "p", p_def);
    auto A = ymCtx_Load(ctx, "p:A");
    ASSERT_TRUE(A);
    EXPECT_EQ(ymItem_ParamName(A, 0), nullptr);
    EXPECT_EQ(err[YmErrCode_ParamNotFound], 1);
}

TEST(Items, ParamType_Callable) {
    SETUP_ALL(ctx);
    SETUP_PARCELDEF(p_def);
    setup_fn_item_with_three_params(dm, p_def);
    auto A = ymCtx_Load(ctx, "p:A");
    auto B = ymCtx_Load(ctx, "p:B");
    auto C = ymCtx_Load(ctx, "p:C");
    auto D = ymCtx_Load(ctx, "p:D");
    ASSERT_TRUE(A);
    ASSERT_TRUE(B);
    ASSERT_TRUE(C);
    ASSERT_TRUE(D);
    EXPECT_EQ(ymItem_ParamType(A, 0), B);
    EXPECT_EQ(ymItem_ParamType(A, 1), C);
    EXPECT_EQ(ymItem_ParamType(A, 2), D);
}

TEST(Items, ParamType_ParamNotFound_Callable) {
    SETUP_ALL(ctx);
    SETUP_PARCELDEF(p_def);
    setup_fn_item_with_three_params(dm, p_def);
    auto A = ymCtx_Load(ctx, "p:A");
    auto B = ymCtx_Load(ctx, "p:B");
    auto C = ymCtx_Load(ctx, "p:C");
    auto D = ymCtx_Load(ctx, "p:D");
    ASSERT_TRUE(A);
    ASSERT_TRUE(B);
    ASSERT_TRUE(C);
    ASSERT_TRUE(D);
    EXPECT_EQ(ymItem_ParamType(A, 3), nullptr);
    EXPECT_EQ(err[YmErrCode_ParamNotFound], 1);
}

TEST(Items, ParamType_ParamNotFound_NonCallable) {
    SETUP_ALL(ctx);
    SETUP_PARCELDEF(p_def);
    ymParcelDef_AddStruct(p_def, "A");
    ymDm_BindParcelDef(dm, "p", p_def);
    auto A = ymCtx_Load(ctx, "p:A");
    ASSERT_TRUE(A);
    EXPECT_EQ(ymItem_ParamType(A, 0), nullptr);
    EXPECT_EQ(err[YmErrCode_ParamNotFound], 1);
}

TEST(Items, Ref) {
    SETUP_ALL(ctx);
    SETUP_PARCELDEF(p_def);
    YmItemIndex A_index = ymParcelDef_AddStruct(p_def, "A");
    YmRef A_ref_B = ymParcelDef_AddRef(p_def, A_index, "p:B");
    ymParcelDef_AddStruct(p_def, "B");
    ymDm_BindParcelDef(dm, "p", p_def);
    YmItem* A = ymCtx_Load(ctx, "p:A");
    YmItem* B = ymCtx_Load(ctx, "p:B");
    ASSERT_TRUE(A);
    ASSERT_TRUE(B);

    EXPECT_EQ(ymItem_Ref(A, A_ref_B), B);
}

TEST(Items, Ref_Failure) {
    SETUP_ALL(ctx);
    SETUP_PARCELDEF(p_def);
    ymParcelDef_AddStruct(p_def, "A");
    ymDm_BindParcelDef(dm, "p", p_def);
    YmItem* A = ymCtx_Load(ctx, "p:A");
    ASSERT_TRUE(A);

    EXPECT_EQ(ymItem_Ref(A, 100), nullptr);
}

TEST(Items, FindRef) {
    SETUP_ALL(ctx);
    SETUP_PARCELDEF(p_def);
    YmItemIndex A_index = ymParcelDef_AddStruct(p_def, "A");
    YmRef A_ref_B = ymParcelDef_AddRef(p_def, A_index, "p:B");
    ymParcelDef_AddStruct(p_def, "B");
    ymParcelDef_AddStruct(p_def, "C");
    ymDm_BindParcelDef(dm, "p", p_def);
    YmItem* A = ymCtx_Load(ctx, "p:A");
    YmItem* B = ymCtx_Load(ctx, "p:B");
    YmItem* C = ymCtx_Load(ctx, "p:C");
    ASSERT_TRUE(A);
    ASSERT_TRUE(B);
    ASSERT_TRUE(C);

    EXPECT_EQ(ymItem_FindRef(A, B), A_ref_B);
    EXPECT_EQ(ymItem_FindRef(A, C), YM_NO_REF);
}

