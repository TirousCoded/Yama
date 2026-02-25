

#include <gtest/gtest.h>
#include <yama/yama.h>

#include "../utils/utils.h"


TEST(Types, Parcel) {
    SETUP_ALL(ctx);
    SETUP_PARCELDEF(p_def);
    ymParcelDef_AddStruct(p_def, "A");
    ymDm_BindParcelDef(dm, "p", p_def);
    auto A = ymCtx_Load(ctx, "p:A");
    ASSERT_TRUE(A);
    YmParcel* p = ymCtx_Import(ctx, "p");
    ASSERT_NE(p, nullptr);
    EXPECT_EQ(ymType_Parcel(A), p);
}

TEST(Types, Fullname) {
    SETUP_ALL(ctx);
    SETUP_PARCELDEF(p_def);
    ymParcelDef_AddStruct(p_def, "A");
    ymDm_BindParcelDef(dm, "p", p_def);
    auto A = ymCtx_Load(ctx, "p:A");
    ASSERT_TRUE(A);
    EXPECT_STREQ(ymType_Fullname(A), "p:A");
}

TEST(Types, Kind) {
    SETUP_ALL(ctx);
    SETUP_PARCELDEF(p_def);
    ymParcelDef_AddStruct(p_def, "A");
    ymDm_BindParcelDef(dm, "p", p_def);
    auto A = ymCtx_Load(ctx, "p:A");
    ASSERT_TRUE(A);
    EXPECT_EQ(ymType_Kind(A), YmKind_Struct);
}

TEST(Types, Owner_MemberType) {
    SETUP_ALL(ctx);
    SETUP_PARCELDEF(p_def);
    auto A_index = ymParcelDef_AddStruct(p_def, "A");
    auto A_m_index = ymParcelDef_AddMethod(p_def, A_index, "m", "p:A", ymInertCallBhvrFn, nullptr);
    ymDm_BindParcelDef(dm, "p", p_def);
    auto A = ymCtx_Load(ctx, "p:A");
    auto A_m = ymCtx_Load(ctx, "p:A::m");
    ASSERT_TRUE(A);
    ASSERT_TRUE(A_m);
    EXPECT_EQ(ymType_Owner(A_m), A);
}

TEST(Types, Owner_NonMemberType) {
    SETUP_ALL(ctx);
    SETUP_PARCELDEF(p_def);
    ymParcelDef_AddStruct(p_def, "A");
    ymDm_BindParcelDef(dm, "p", p_def);
    auto A = ymCtx_Load(ctx, "p:A");
    ASSERT_TRUE(A);
    EXPECT_EQ(ymType_Owner(A), nullptr); // p:A is not a member type.
}

TEST(Types, Members_OwnerType) {
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
    EXPECT_EQ(ymType_Members(A), 2);
}

TEST(Types, Members_NonOwnerType) {
    SETUP_ALL(ctx);
    SETUP_PARCELDEF(p_def);
    auto A_index = ymParcelDef_AddStruct(p_def, "A");
    auto A_m_index = ymParcelDef_AddMethod(p_def, A_index, "m", "p:A", ymInertCallBhvrFn, nullptr);
    ymDm_BindParcelDef(dm, "p", p_def);
    auto A = ymCtx_Load(ctx, "p:A");
    auto A_m = ymCtx_Load(ctx, "p:A::m");
    ASSERT_TRUE(A);
    ASSERT_TRUE(A_m);
    EXPECT_EQ(ymType_Members(A_m), 0); // p:A::m is not an owner type.
}

TEST(Types, MemberByIndex_OwnerType) {
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
    EXPECT_EQ(ymType_MemberByIndex(A, 0), A_m1);
    EXPECT_EQ(ymType_MemberByIndex(A, 1), A_m2);
    EXPECT_EQ(ymType_MemberByIndex(A, 2), nullptr); // Out-Of-Bounds
}

TEST(Types, MemberByIndex_NonOwnerType) {
    SETUP_ALL(ctx);
    SETUP_PARCELDEF(p_def);
    auto A_index = ymParcelDef_AddStruct(p_def, "A");
    auto A_m_index = ymParcelDef_AddMethod(p_def, A_index, "m", "p:A", ymInertCallBhvrFn, nullptr);
    ymDm_BindParcelDef(dm, "p", p_def);
    auto A = ymCtx_Load(ctx, "p:A");
    auto A_m = ymCtx_Load(ctx, "p:A::m");
    ASSERT_TRUE(A);
    ASSERT_TRUE(A_m);
    EXPECT_EQ(ymType_MemberByIndex(A_m, 0), nullptr); // p:A::m is not an owner type.
}

TEST(Types, MemberByName_OwnerType) {
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
    EXPECT_EQ(ymType_MemberByName(A, "m1"), A_m1);
    EXPECT_EQ(ymType_MemberByName(A, "m2"), A_m2);
    EXPECT_EQ(ymType_MemberByName(A, "missing"), nullptr); // Missing
}

TEST(Types, MemberByName_NonOwnerType) {
    SETUP_ALL(ctx);
    SETUP_PARCELDEF(p_def);
    auto A_index = ymParcelDef_AddStruct(p_def, "A");
    auto A_m_index = ymParcelDef_AddMethod(p_def, A_index, "m", "p:A", ymInertCallBhvrFn, nullptr);
    ymDm_BindParcelDef(dm, "p", p_def);
    auto A = ymCtx_Load(ctx, "p:A");
    auto A_m = ymCtx_Load(ctx, "p:A::m");
    ASSERT_TRUE(A);
    ASSERT_TRUE(A_m);
    EXPECT_EQ(ymType_MemberByName(A_m, "m"), nullptr); // p:A::m is not an owner type.
}

static void setup_for_type_params_tests(YmDm* dm, YmParcelDef* p_def) {
    ymAssert(dm != nullptr);
    ymAssert(p_def != nullptr);
    ymParcelDef_AddProtocol(p_def, "Any");
    ymParcelDef_AddStruct(p_def, "Int");
    ymParcelDef_AddStruct(p_def, "Float");
    auto A_index = ymParcelDef_AddStruct(p_def, "A");
    ymParcelDef_AddTypeParam(p_def, A_index, "T", "p:Any");
    ymParcelDef_AddTypeParam(p_def, A_index, "U", "p:Any");
    ymParcelDef_AddMethod(p_def, A_index, "m", "p:Int", ymInertCallBhvrFn, nullptr);
    ymDm_BindParcelDef(dm, "p", p_def);
}

TEST(Types, TypeParams_OwnerType) {
    SETUP_ALL(ctx);
    SETUP_PARCELDEF(p_def);
    setup_for_type_params_tests(dm, p_def);
    auto A = ymCtx_Load(ctx, "p:A[p:Int, p:Float]");
    ASSERT_TRUE(A);
    EXPECT_EQ(ymType_TypeParams(A), 2);
}

TEST(Types, TypeParams_NonOwnerType) {
    SETUP_ALL(ctx);
    SETUP_PARCELDEF(p_def);
    setup_for_type_params_tests(dm, p_def);
    auto A_m = ymCtx_Load(ctx, "p:A[p:Int, p:Float]::m");
    ASSERT_TRUE(A_m);
    EXPECT_EQ(ymType_TypeParams(A_m), 2);
}

TEST(Types, TypeParamByIndex_OwnerType) {
    SETUP_ALL(ctx);
    SETUP_PARCELDEF(p_def);
    setup_for_type_params_tests(dm, p_def);
    auto A = ymCtx_Load(ctx, "p:A[p:Int, p:Float]");
    auto Int = ymCtx_Load(ctx, "p:Int");
    auto Float = ymCtx_Load(ctx, "p:Float");
    ASSERT_TRUE(A);
    ASSERT_TRUE(Int);
    ASSERT_TRUE(Float);
    ASSERT_EQ(ymType_TypeParams(A), 2);
    EXPECT_EQ(ymType_TypeParamByIndex(A, 0), Int);
    EXPECT_EQ(ymType_TypeParamByIndex(A, 1), Float);
    EXPECT_EQ(ymType_TypeParamByIndex(A, 2), nullptr);
}

TEST(Types, TypeParamByIndex_NonOwnerType) {
    SETUP_ALL(ctx);
    SETUP_PARCELDEF(p_def);
    setup_for_type_params_tests(dm, p_def);
    auto A_m = ymCtx_Load(ctx, "p:A[p:Int, p:Float]::m");
    auto Int = ymCtx_Load(ctx, "p:Int");
    auto Float = ymCtx_Load(ctx, "p:Float");
    ASSERT_TRUE(A_m);
    ASSERT_TRUE(Int);
    ASSERT_TRUE(Float);
    ASSERT_EQ(ymType_TypeParams(A_m), 2);
    EXPECT_EQ(ymType_TypeParamByIndex(A_m, 0), Int);
    EXPECT_EQ(ymType_TypeParamByIndex(A_m, 1), Float);
    EXPECT_EQ(ymType_TypeParamByIndex(A_m, 2), nullptr);
}

TEST(Types, TypeParamByName_OwnerType) {
    SETUP_ALL(ctx);
    SETUP_PARCELDEF(p_def);
    setup_for_type_params_tests(dm, p_def);
    auto A = ymCtx_Load(ctx, "p:A[p:Int, p:Float]");
    auto Int = ymCtx_Load(ctx, "p:Int");
    auto Float = ymCtx_Load(ctx, "p:Float");
    ASSERT_TRUE(A);
    ASSERT_TRUE(Int);
    ASSERT_TRUE(Float);
    ASSERT_EQ(ymType_TypeParams(A), 2);
    EXPECT_EQ(ymType_TypeParamByName(A, "T"), Int);
    EXPECT_EQ(ymType_TypeParamByName(A, "U"), Float);
    EXPECT_EQ(ymType_TypeParamByName(A, "V"), nullptr);
}

TEST(Types, TypeParamByName_NonOwnerType) {
    SETUP_ALL(ctx);
    SETUP_PARCELDEF(p_def);
    setup_for_type_params_tests(dm, p_def);
    auto A_m = ymCtx_Load(ctx, "p:A[p:Int, p:Float]::m");
    auto Int = ymCtx_Load(ctx, "p:Int");
    auto Float = ymCtx_Load(ctx, "p:Float");
    ASSERT_TRUE(A_m);
    ASSERT_TRUE(Int);
    ASSERT_TRUE(Float);
    ASSERT_EQ(ymType_TypeParams(A_m), 2);
    EXPECT_EQ(ymType_TypeParamByName(A_m, "T"), Int);
    EXPECT_EQ(ymType_TypeParamByName(A_m, "U"), Float);
    EXPECT_EQ(ymType_TypeParamByName(A_m, "V"), nullptr);
}

static void setup_fn_type_with_three_params(YmDm* dm, YmParcelDef* p_def) {
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

TEST(Types, ReturnType_Callable) {
    SETUP_ALL(ctx);
    SETUP_PARCELDEF(p_def);
    setup_fn_type_with_three_params(dm, p_def);
    auto A = ymCtx_Load(ctx, "p:A");
    auto B = ymCtx_Load(ctx, "p:B");
    ASSERT_TRUE(A);
    ASSERT_TRUE(B);
    EXPECT_EQ(ymType_ReturnType(A), B);
}

TEST(Types, ReturnType_NonCallable) {
    SETUP_ALL(ctx);
    SETUP_PARCELDEF(p_def);
    ymParcelDef_AddStruct(p_def, "A");
    ymDm_BindParcelDef(dm, "p", p_def);
    auto A = ymCtx_Load(ctx, "p:A");
    ASSERT_TRUE(A);
    EXPECT_EQ(ymType_ReturnType(A), nullptr);
}

TEST(Types, Params_Callable) {
    SETUP_ALL(ctx);
    SETUP_PARCELDEF(p_def);
    setup_fn_type_with_three_params(dm, p_def);
    auto A = ymCtx_Load(ctx, "p:A");
    auto B = ymCtx_Load(ctx, "p:B");
    auto C = ymCtx_Load(ctx, "p:C");
    auto D = ymCtx_Load(ctx, "p:D");
    ASSERT_TRUE(A);
    ASSERT_TRUE(B);
    ASSERT_TRUE(C);
    ASSERT_TRUE(D);
    EXPECT_EQ(ymType_Params(A), 3);
}

TEST(Types, Params_NonCallable) {
    SETUP_ALL(ctx);
    SETUP_PARCELDEF(p_def);
    ymParcelDef_AddStruct(p_def, "A");
    ymDm_BindParcelDef(dm, "p", p_def);
    auto A = ymCtx_Load(ctx, "p:A");
    ASSERT_TRUE(A);
    EXPECT_EQ(ymType_Params(A), 0);
}

TEST(Types, ParamName_Callable) {
    SETUP_ALL(ctx);
    SETUP_PARCELDEF(p_def);
    setup_fn_type_with_three_params(dm, p_def);
    auto A = ymCtx_Load(ctx, "p:A");
    auto B = ymCtx_Load(ctx, "p:B");
    auto C = ymCtx_Load(ctx, "p:C");
    auto D = ymCtx_Load(ctx, "p:D");
    ASSERT_TRUE(A);
    ASSERT_TRUE(B);
    ASSERT_TRUE(C);
    ASSERT_TRUE(D);
    EXPECT_STREQ(ymType_ParamName(A, 0), "x");
    EXPECT_STREQ(ymType_ParamName(A, 1), "y");
    EXPECT_STREQ(ymType_ParamName(A, 2), "z");
}

TEST(Types, ParamName_ParamNotFound_Callable) {
    SETUP_ALL(ctx);
    SETUP_PARCELDEF(p_def);
    setup_fn_type_with_three_params(dm, p_def);
    auto A = ymCtx_Load(ctx, "p:A");
    auto B = ymCtx_Load(ctx, "p:B");
    auto C = ymCtx_Load(ctx, "p:C");
    auto D = ymCtx_Load(ctx, "p:D");
    ASSERT_TRUE(A);
    ASSERT_TRUE(B);
    ASSERT_TRUE(C);
    ASSERT_TRUE(D);
    EXPECT_EQ(ymType_ParamName(A, 3), nullptr);
    EXPECT_EQ(err[YmErrCode_ParamNotFound], 1);
}

TEST(Types, ParamName_ParamNotFound_NonCallable) {
    SETUP_ALL(ctx);
    SETUP_PARCELDEF(p_def);
    ymParcelDef_AddStruct(p_def, "A");
    ymDm_BindParcelDef(dm, "p", p_def);
    auto A = ymCtx_Load(ctx, "p:A");
    ASSERT_TRUE(A);
    EXPECT_EQ(ymType_ParamName(A, 0), nullptr);
    EXPECT_EQ(err[YmErrCode_ParamNotFound], 1);
}

TEST(Types, ParamType_Callable) {
    SETUP_ALL(ctx);
    SETUP_PARCELDEF(p_def);
    setup_fn_type_with_three_params(dm, p_def);
    auto A = ymCtx_Load(ctx, "p:A");
    auto B = ymCtx_Load(ctx, "p:B");
    auto C = ymCtx_Load(ctx, "p:C");
    auto D = ymCtx_Load(ctx, "p:D");
    ASSERT_TRUE(A);
    ASSERT_TRUE(B);
    ASSERT_TRUE(C);
    ASSERT_TRUE(D);
    EXPECT_EQ(ymType_ParamType(A, 0), B);
    EXPECT_EQ(ymType_ParamType(A, 1), C);
    EXPECT_EQ(ymType_ParamType(A, 2), D);
}

TEST(Types, ParamType_ParamNotFound_Callable) {
    SETUP_ALL(ctx);
    SETUP_PARCELDEF(p_def);
    setup_fn_type_with_three_params(dm, p_def);
    auto A = ymCtx_Load(ctx, "p:A");
    auto B = ymCtx_Load(ctx, "p:B");
    auto C = ymCtx_Load(ctx, "p:C");
    auto D = ymCtx_Load(ctx, "p:D");
    ASSERT_TRUE(A);
    ASSERT_TRUE(B);
    ASSERT_TRUE(C);
    ASSERT_TRUE(D);
    EXPECT_EQ(ymType_ParamType(A, 3), nullptr);
    EXPECT_EQ(err[YmErrCode_ParamNotFound], 1);
}

TEST(Types, ParamType_ParamNotFound_NonCallable) {
    SETUP_ALL(ctx);
    SETUP_PARCELDEF(p_def);
    ymParcelDef_AddStruct(p_def, "A");
    ymDm_BindParcelDef(dm, "p", p_def);
    auto A = ymCtx_Load(ctx, "p:A");
    ASSERT_TRUE(A);
    EXPECT_EQ(ymType_ParamType(A, 0), nullptr);
    EXPECT_EQ(err[YmErrCode_ParamNotFound], 1);
}

TEST(Types, Ref) {
    SETUP_ALL(ctx);
    SETUP_PARCELDEF(p_def);
    YmTypeIndex A_index = ymParcelDef_AddStruct(p_def, "A");
    YmRef A_ref_B = ymParcelDef_AddRef(p_def, A_index, "p:B");
    ymParcelDef_AddStruct(p_def, "B");
    ymDm_BindParcelDef(dm, "p", p_def);
    YmType* A = ymCtx_Load(ctx, "p:A");
    YmType* B = ymCtx_Load(ctx, "p:B");
    ASSERT_TRUE(A);
    ASSERT_TRUE(B);

    EXPECT_EQ(ymType_Ref(A, A_ref_B), B);
}

TEST(Types, Ref_Failure) {
    SETUP_ALL(ctx);
    SETUP_PARCELDEF(p_def);
    ymParcelDef_AddStruct(p_def, "A");
    ymDm_BindParcelDef(dm, "p", p_def);
    YmType* A = ymCtx_Load(ctx, "p:A");
    ASSERT_TRUE(A);

    EXPECT_EQ(ymType_Ref(A, 100), nullptr);
}

TEST(Types, FindRef) {
    SETUP_ALL(ctx);
    SETUP_PARCELDEF(p_def);
    YmTypeIndex A_index = ymParcelDef_AddStruct(p_def, "A");
    YmRef A_ref_B = ymParcelDef_AddRef(p_def, A_index, "p:B");
    ymParcelDef_AddStruct(p_def, "B");
    ymParcelDef_AddStruct(p_def, "C");
    ymDm_BindParcelDef(dm, "p", p_def);
    YmType* A = ymCtx_Load(ctx, "p:A");
    YmType* B = ymCtx_Load(ctx, "p:B");
    YmType* C = ymCtx_Load(ctx, "p:C");
    ASSERT_TRUE(A);
    ASSERT_TRUE(B);
    ASSERT_TRUE(C);

    EXPECT_EQ(ymType_FindRef(A, B), A_ref_B);
    EXPECT_EQ(ymType_FindRef(A, C), YM_NO_REF);
}

