

#include <gtest/gtest.h>
#include <taul/strings.h>
#include <yama/yama.h>
#include <yama++/print.h>

#include "../utils/utils.h"


TEST(ParcelDefs, CreateAndDestroy) {
    SETUP_ERRCOUNTER;
    SETUP_PARCELDEF(p_def); // Macro will do the create/destroy.
}

TEST(ParcelDefs, RefCounting) {
    SETUP_ERRCOUNTER;
    auto parceldef = ymParcelDef_Create();
    ASSERT_TRUE(parceldef);
    EXPECT_EQ(ymParcelDef_RefCount(parceldef), 1); // Initial
    EXPECT_EQ(ymParcelDef_Secure(parceldef), 1); // 1 -> 2
    EXPECT_EQ(ymParcelDef_RefCount(parceldef), 2);
    EXPECT_EQ(ymParcelDef_Secure(parceldef), 2); // 2 -> 3
    EXPECT_EQ(ymParcelDef_RefCount(parceldef), 3);
    EXPECT_EQ(ymParcelDef_Release(parceldef), 3); // 3 -> 2
    EXPECT_EQ(ymParcelDef_RefCount(parceldef), 2);
    EXPECT_EQ(ymParcelDef_Release(parceldef), 2); // 2 -> 1
    EXPECT_EQ(ymParcelDef_RefCount(parceldef), 1);
    EXPECT_EQ(ymParcelDef_Release(parceldef), 1); // Destroys
}

TEST(ParcelDefs, AddStruct) {
    SETUP_ALL(ctx);
    SETUP_PARCELDEF(p_def);
    std::string nonASCIIName = taul::utf8_s(u8"ab魂💩cd"); // Ensure can handle UTF-8.
    std::string nonASCIIFullname = taul::utf8_s(u8"p:ab魂💩cd"); // Ensure can handle UTF-8.
    YmTypeIndex foo_index = ymParcelDef_AddStruct(p_def, "foo");
    YmTypeIndex bar_index = ymParcelDef_AddStruct(p_def, "bar");
    YmTypeIndex other_index = ymParcelDef_AddStruct(p_def, nonASCIIName.c_str());
    ASSERT_NE(foo_index, YM_NO_TYPE_INDEX);
    ASSERT_NE(bar_index, YM_NO_TYPE_INDEX);
    ASSERT_NE(other_index, YM_NO_TYPE_INDEX);

    BIND_AND_IMPORT(ctx, parcel, p_def, "p");
    auto foo = ymCtx_Load(ctx, "p:foo");
    auto bar = ymCtx_Load(ctx, "p:bar");
    auto other = ymCtx_Load(ctx, nonASCIIFullname.c_str());
    ASSERT_TRUE(foo);
    ASSERT_TRUE(bar);
    ASSERT_TRUE(other);
    EXPECT_EQ(ymType_Parcel(foo), parcel);
    EXPECT_EQ(ymType_Parcel(bar), parcel);
    EXPECT_EQ(ymType_Parcel(other), parcel);
    EXPECT_STREQ(ymType_Fullname(foo), "p:foo");
    EXPECT_STREQ(ymType_Fullname(bar), "p:bar");
    EXPECT_STREQ(ymType_Fullname(other), nonASCIIFullname.c_str());
    EXPECT_EQ(ymType_Kind(foo), YmKind_Struct);
    EXPECT_EQ(ymType_Kind(bar), YmKind_Struct);
    EXPECT_EQ(ymType_Kind(other), YmKind_Struct);
    EXPECT_EQ(ymType_ReturnType(foo), nullptr);
    EXPECT_EQ(ymType_ReturnType(bar), nullptr);
    EXPECT_EQ(ymType_ReturnType(other), nullptr);
    EXPECT_EQ(ymType_Params(foo), 0);
    EXPECT_EQ(ymType_Params(bar), 0);
    EXPECT_EQ(ymType_Params(other), 0);
}

TEST(ParcelDefs, AddStruct_NameConflict) {
    SETUP_ERRCOUNTER;
    SETUP_PARCELDEF(p_def);
    ASSERT_NE(ymParcelDef_AddStruct(p_def, "foo"), YM_NO_TYPE_INDEX);
    ASSERT_EQ(ymParcelDef_AddStruct(p_def, "foo"), YM_NO_TYPE_INDEX);
    EXPECT_EQ(err[YmErrCode_NameConflict], 1);
}

TEST(ParcelDefs, AddProtocol) {
    SETUP_ALL(ctx);
    SETUP_PARCELDEF(p_def);
    std::string nonASCIIName = taul::utf8_s(u8"ab魂💩cd"); // Ensure can handle UTF-8.
    std::string nonASCIIFullname = taul::utf8_s(u8"p:ab魂💩cd"); // Ensure can handle UTF-8.
    YmTypeIndex foo_index = ymParcelDef_AddProtocol(p_def, "foo");
    YmTypeIndex bar_index = ymParcelDef_AddProtocol(p_def, "bar");
    YmTypeIndex other_index = ymParcelDef_AddProtocol(p_def, nonASCIIName.c_str());
    ASSERT_NE(foo_index, YM_NO_TYPE_INDEX);
    ASSERT_NE(bar_index, YM_NO_TYPE_INDEX);
    ASSERT_NE(other_index, YM_NO_TYPE_INDEX);

    BIND_AND_IMPORT(ctx, parcel, p_def, "p");
    auto foo = ymCtx_Load(ctx, "p:foo");
    auto bar = ymCtx_Load(ctx, "p:bar");
    auto other = ymCtx_Load(ctx, nonASCIIFullname.c_str());
    ASSERT_TRUE(foo);
    ASSERT_TRUE(bar);
    ASSERT_TRUE(other);
    EXPECT_EQ(ymType_Parcel(foo), parcel);
    EXPECT_EQ(ymType_Parcel(bar), parcel);
    EXPECT_EQ(ymType_Parcel(other), parcel);
    EXPECT_STREQ(ymType_Fullname(foo), "p:foo");
    EXPECT_STREQ(ymType_Fullname(bar), "p:bar");
    EXPECT_STREQ(ymType_Fullname(other), nonASCIIFullname.c_str());
    EXPECT_EQ(ymType_Kind(foo), YmKind_Protocol);
    EXPECT_EQ(ymType_Kind(bar), YmKind_Protocol);
    EXPECT_EQ(ymType_Kind(other), YmKind_Protocol);
    EXPECT_EQ(ymType_ReturnType(foo), nullptr);
    EXPECT_EQ(ymType_ReturnType(bar), nullptr);
    EXPECT_EQ(ymType_ReturnType(other), nullptr);
    EXPECT_EQ(ymType_Params(foo), 0);
    EXPECT_EQ(ymType_Params(bar), 0);
    EXPECT_EQ(ymType_Params(other), 0);
}

TEST(ParcelDefs, AddProtocol_NameConflict) {
    SETUP_ERRCOUNTER;
    SETUP_PARCELDEF(p_def);
    ASSERT_NE(ymParcelDef_AddStruct(p_def, "foo"), YM_NO_TYPE_INDEX);
    ASSERT_EQ(ymParcelDef_AddProtocol(p_def, "foo"), YM_NO_TYPE_INDEX);
    EXPECT_EQ(err[YmErrCode_NameConflict], 1);
}

TEST(ParcelDefs, AddFn) {
    SETUP_ALL(ctx);
    SETUP_PARCELDEF(p_def);
    std::string nonASCIIName = taul::utf8_s(u8"ab魂💩cd"); // Ensure can handle UTF-8.
    std::string nonASCIIFullname = taul::utf8_s(u8"p:ab魂💩cd"); // Ensure can handle UTF-8.
    ymParcelDef_AddStruct(p_def, "A");
    ymParcelDef_AddStruct(p_def, "B");
    ymParcelDef_AddStruct(p_def, "C");

    YmTypeIndex foo_index = ymParcelDef_AddFn(p_def, "foo", "p:A", ymInertCallBhvrFn, nullptr); // w/out params

    YmTypeIndex bar_index = ymParcelDef_AddFn(p_def, "bar", "p:A", ymInertCallBhvrFn, nullptr); // w/ params
    ymParcelDef_AddParam(p_def, bar_index, "x", "p:A");
    ymParcelDef_AddParam(p_def, bar_index, "y", "p:B");
    ymParcelDef_AddParam(p_def, bar_index, "z", "p:C");

    YmTypeIndex other_index = ymParcelDef_AddFn(p_def, nonASCIIName.c_str(), "p:A", ymInertCallBhvrFn, nullptr); // w/out params

    BIND_AND_IMPORT(ctx, parcel, p_def, "p");
    auto A = ymCtx_Load(ctx, "p:A");
    auto B = ymCtx_Load(ctx, "p:B");
    auto C = ymCtx_Load(ctx, "p:C");
    auto foo = ymCtx_Load(ctx, "p:foo");
    auto bar = ymCtx_Load(ctx, "p:bar");
    auto other = ymCtx_Load(ctx, nonASCIIFullname.c_str());
    ASSERT_TRUE(A);
    ASSERT_TRUE(B);
    ASSERT_TRUE(C);
    ASSERT_TRUE(foo);
    ASSERT_TRUE(bar);
    ASSERT_TRUE(other);
    EXPECT_EQ(ymType_Parcel(foo), parcel);
    EXPECT_EQ(ymType_Parcel(bar), parcel);
    EXPECT_EQ(ymType_Parcel(other), parcel);
    EXPECT_STREQ(ymType_Fullname(foo), "p:foo");
    EXPECT_STREQ(ymType_Fullname(bar), "p:bar");
    EXPECT_STREQ(ymType_Fullname(other), nonASCIIFullname.c_str());
    EXPECT_EQ(ymType_Kind(foo), YmKind_Fn);
    EXPECT_EQ(ymType_Kind(bar), YmKind_Fn);
    EXPECT_EQ(ymType_Kind(other), YmKind_Fn);
    EXPECT_EQ(ymType_ReturnType(foo), A);
    EXPECT_EQ(ymType_ReturnType(bar), A);
    EXPECT_EQ(ymType_ReturnType(other), A);
    EXPECT_EQ(ymType_Params(foo), 0);
    EXPECT_EQ(ymType_Params(bar), 3);
    EXPECT_EQ(ymType_Params(other), 0);
    EXPECT_STREQ(ymType_ParamName(bar, 0), "x");
    EXPECT_STREQ(ymType_ParamName(bar, 1), "y");
    EXPECT_STREQ(ymType_ParamName(bar, 2), "z");
    EXPECT_EQ(ymType_ParamType(bar, 0), A);
    EXPECT_EQ(ymType_ParamType(bar, 1), B);
    EXPECT_EQ(ymType_ParamType(bar, 2), C);
}

TEST(ParcelDefs, AddFn_Normalizes_ReturnType) {
    SETUP_ALL(ctx);
    SETUP_PARCELDEF(p_def);

    auto A_index = ymParcelDef_AddStruct(p_def, "A");
    auto f_returnType = "  p  : \n\n A  ";
    auto f_index = ymParcelDef_AddFn(p_def, "f", f_returnType, ymInertCallBhvrFn, nullptr);

    ymDm_BindParcelDef(dm, "p", p_def);
    auto A = ymCtx_Load(ctx, "p:A");
    auto f = ymCtx_Load(ctx, "p:f");
    ASSERT_TRUE(A);
    ASSERT_TRUE(f);

    ASSERT_EQ(ymType_ReturnType(f), A);
    EXPECT_STREQ(ymType_Fullname(ymType_ReturnType(f)), "p:A");
}

TEST(ParcelDefs, AddFn_NameConflict) {
    SETUP_ERRCOUNTER;
    SETUP_PARCELDEF(p_def);
    ASSERT_NE(ymParcelDef_AddStruct(p_def, "A"), YM_NO_TYPE_INDEX);
    ASSERT_EQ(ymParcelDef_AddFn(p_def, "A", "p:B", ymInertCallBhvrFn, nullptr), YM_NO_TYPE_INDEX);
    EXPECT_EQ(err[YmErrCode_NameConflict], 1);
}

TEST(ParcelDefs, AddFn_IllegalSpecifier_InvalidReturnTypeSymbol) {
    SETUP_ERRCOUNTER;
    SETUP_PARCELDEF(p_def);
    ASSERT_EQ(ymParcelDef_AddFn(p_def, "A", "/", ymInertCallBhvrFn, nullptr), YM_NO_TYPE_INDEX);
    EXPECT_EQ(err[YmErrCode_IllegalSpecifier], 1);
}

TEST(ParcelDefs, AddMethod) {
    SETUP_ALL(ctx);
    SETUP_PARCELDEF(p_def);
    std::string nonASCIIName = taul::utf8_s(u8"ab魂💩cd"); // Ensure can handle UTF-8.
    std::string nonASCIIFullname = taul::utf8_s(u8"p:A::ab魂💩cd"); // Ensure can handle UTF-8.
    auto A_index = ymParcelDef_AddStruct(p_def, "A");
    ymParcelDef_AddStruct(p_def, "B");
    ymParcelDef_AddStruct(p_def, "C");

    YmTypeIndex foo_index = ymParcelDef_AddMethod(p_def, A_index, "foo", "p:C", ymInertCallBhvrFn, nullptr); // w/out params

    YmTypeIndex bar_index = ymParcelDef_AddMethod(p_def, A_index, "bar", "p:C", ymInertCallBhvrFn, nullptr); // w/ params
    ymParcelDef_AddParam(p_def, bar_index, "x", "p:C");
    ymParcelDef_AddParam(p_def, bar_index, "y", "p:B");
    ymParcelDef_AddParam(p_def, bar_index, "z", "p:C");

    YmTypeIndex other_index = ymParcelDef_AddMethod(p_def, A_index, nonASCIIName.c_str(), "p:C", ymInertCallBhvrFn, nullptr); // w/out params

    BIND_AND_IMPORT(ctx, parcel, p_def, "p");
    auto A = ymCtx_Load(ctx, "p:A");
    auto B = ymCtx_Load(ctx, "p:B");
    auto C = ymCtx_Load(ctx, "p:C");
    auto A_foo = ymCtx_Load(ctx, "p:A::foo");
    auto A_bar = ymCtx_Load(ctx, "p:A::bar");
    auto A_other = ymCtx_Load(ctx, nonASCIIFullname.c_str());
    ASSERT_TRUE(A);
    ASSERT_TRUE(B);
    ASSERT_TRUE(C);
    ASSERT_TRUE(A_foo);
    ASSERT_TRUE(A_bar);
    ASSERT_TRUE(A_other);
    EXPECT_EQ(ymType_Parcel(A_foo), parcel);
    EXPECT_EQ(ymType_Parcel(A_bar), parcel);
    EXPECT_EQ(ymType_Parcel(A_other), parcel);
    EXPECT_STREQ(ymType_Fullname(A_foo), "p:A::foo");
    EXPECT_STREQ(ymType_Fullname(A_bar), "p:A::bar");
    EXPECT_STREQ(ymType_Fullname(A_other), nonASCIIFullname.c_str());
    EXPECT_EQ(ymType_Kind(A_foo), YmKind_Method);
    EXPECT_EQ(ymType_Kind(A_bar), YmKind_Method);
    EXPECT_EQ(ymType_Kind(A_other), YmKind_Method);
    EXPECT_EQ(ymType_Owner(A_foo), A);
    EXPECT_EQ(ymType_Owner(A_bar), A);
    EXPECT_EQ(ymType_Owner(A_other), A);
    EXPECT_EQ(ymType_ReturnType(A_foo), C);
    EXPECT_EQ(ymType_ReturnType(A_bar), C);
    EXPECT_EQ(ymType_ReturnType(A_other), C);
    EXPECT_EQ(ymType_Params(A_foo), 0);
    EXPECT_EQ(ymType_Params(A_bar), 3);
    EXPECT_EQ(ymType_Params(A_other), 0);
    EXPECT_STREQ(ymType_ParamName(A_bar, 0), "x");
    EXPECT_STREQ(ymType_ParamName(A_bar, 1), "y");
    EXPECT_STREQ(ymType_ParamName(A_bar, 2), "z");
    EXPECT_EQ(ymType_ParamType(A_bar, 0), C);
    EXPECT_EQ(ymType_ParamType(A_bar, 1), B);
    EXPECT_EQ(ymType_ParamType(A_bar, 2), C);
}

TEST(ParcelDefs, AddMethod_Normalizes_ReturnType) {
    SETUP_ALL(ctx);
    SETUP_PARCELDEF(p_def);

    auto A_index = ymParcelDef_AddStruct(p_def, "A");
    auto A_m_returnType = "  p  : \n\n A  ";
    auto A_m_index = ymParcelDef_AddMethod(p_def, A_index, "m", A_m_returnType, ymInertCallBhvrFn, nullptr);

    ymDm_BindParcelDef(dm, "p", p_def);
    auto A = ymCtx_Load(ctx, "p:A");
    auto A_m = ymCtx_Load(ctx, "p:A::m");
    ASSERT_TRUE(A);
    ASSERT_TRUE(A_m);

    ASSERT_EQ(ymType_ReturnType(A_m), A);
    EXPECT_STREQ(ymType_Fullname(ymType_ReturnType(A_m)), "p:A");
}

TEST(ParcelDefs, AddMethod_NameConflict_Type) {
    SETUP_ERRCOUNTER;
    SETUP_PARCELDEF(p_def);
    auto A_index = ymParcelDef_AddStruct(p_def, "A");
    ASSERT_NE(A_index, YM_NO_TYPE_INDEX);
    ASSERT_NE(ymParcelDef_AddMethod(p_def, A_index, "m", "p:B", ymInertCallBhvrFn, nullptr), YM_NO_TYPE_INDEX);
    ASSERT_EQ(ymParcelDef_AddMethod(p_def, A_index, "m", "p:B", ymInertCallBhvrFn, nullptr), YM_NO_TYPE_INDEX);
    EXPECT_EQ(err[YmErrCode_NameConflict], 1);
}

TEST(ParcelDefs, AddMethod_NameConflict_TypeParam) {
    SETUP_ERRCOUNTER;
    SETUP_PARCELDEF(p_def);
    ymParcelDef_AddProtocol(p_def, "Any");
    auto A_index = ymParcelDef_AddStruct(p_def, "A");
    ASSERT_NE(A_index, YM_NO_TYPE_INDEX);
    ASSERT_NE(ymParcelDef_AddTypeParam(p_def, A_index, "m", "p:Any"), YM_NO_TYPE_PARAM_INDEX);
    ASSERT_EQ(ymParcelDef_AddMethod(p_def, A_index, "m", "p:B", ymInertCallBhvrFn, nullptr), YM_NO_TYPE_INDEX);
    EXPECT_EQ(err[YmErrCode_NameConflict], 1);
}

TEST(ParcelDefs, AddMethod_NameConflict_Self) {
    SETUP_ERRCOUNTER;
    SETUP_PARCELDEF(p_def);
    auto A_index = ymParcelDef_AddStruct(p_def, "A");
    ASSERT_NE(A_index, YM_NO_TYPE_INDEX);
    ASSERT_EQ(ymParcelDef_AddMethod(p_def, A_index, "Self", "p:B", ymInertCallBhvrFn, nullptr), YM_NO_TYPE_INDEX);
    EXPECT_EQ(err[YmErrCode_NameConflict], 1);
}

TEST(ParcelDefs, AddMethod_TypeNotFound_InvalidOwner) {
    SETUP_ERRCOUNTER;
    SETUP_PARCELDEF(p_def);
    ASSERT_EQ(ymParcelDef_AddMethod(p_def, 5'000, "m", "p:B", ymInertCallBhvrFn, nullptr), YM_NO_TYPE_INDEX);
    EXPECT_EQ(err[YmErrCode_TypeNotFound], 1);
}

TEST(ParcelDefs, AddMethod_TypeCannotHaveMembers) {
    SETUP_ERRCOUNTER;
    SETUP_PARCELDEF(p_def);
    auto A_index = ymParcelDef_AddFn(p_def, "A", "p:B", ymInertCallBhvrFn, nullptr);
    ASSERT_NE(A_index, YM_NO_TYPE_INDEX);
    ASSERT_EQ(ymParcelDef_AddMethod(p_def, A_index, "m", "p:B", ymInertCallBhvrFn, nullptr), YM_NO_TYPE_INDEX);
    EXPECT_EQ(err[YmErrCode_TypeCannotHaveMembers], 1);
}

TEST(ParcelDefs, AddMethod_ProtocolType) {
    SETUP_ERRCOUNTER;
    SETUP_PARCELDEF(p_def);
    auto A_index = ymParcelDef_AddProtocol(p_def, "A");
    ASSERT_NE(A_index, YM_NO_TYPE_INDEX);
    ASSERT_EQ(ymParcelDef_AddMethod(p_def, A_index, "m", "p:B", ymInertCallBhvrFn, nullptr), YM_NO_TYPE_INDEX);
    EXPECT_EQ(err[YmErrCode_ProtocolType], 1);
}

TEST(ParcelDefs, AddMethod_IllegalSpecifier_InvalidReturnTypeSymbol) {
    SETUP_ERRCOUNTER;
    SETUP_PARCELDEF(p_def);
    auto A_index = ymParcelDef_AddStruct(p_def, "A");
    ASSERT_NE(A_index, YM_NO_TYPE_INDEX);
    ASSERT_EQ(ymParcelDef_AddMethod(p_def, A_index, "m", "/", ymInertCallBhvrFn, nullptr), YM_NO_TYPE_INDEX);
    EXPECT_EQ(err[YmErrCode_IllegalSpecifier], 1);
}

TEST(ParcelDefs, AddMethodReq) {
    SETUP_ALL(ctx);
    SETUP_PARCELDEF(p_def);
    std::string nonASCIIName = taul::utf8_s(u8"ab魂💩cd"); // Ensure can handle UTF-8.
    std::string nonASCIIFullname = taul::utf8_s(u8"p:A::ab魂💩cd"); // Ensure can handle UTF-8.
    auto A_index = ymParcelDef_AddProtocol(p_def, "A");
    ymParcelDef_AddStruct(p_def, "B");
    ymParcelDef_AddStruct(p_def, "C");

    YmTypeIndex foo_index = ymParcelDef_AddMethodReq(p_def, A_index, "foo", "p:C"); // w/out params

    YmTypeIndex bar_index = ymParcelDef_AddMethodReq(p_def, A_index, "bar", "p:C"); // w/ params
    ymParcelDef_AddParam(p_def, bar_index, "x", "p:C");
    ymParcelDef_AddParam(p_def, bar_index, "y", "p:B");
    ymParcelDef_AddParam(p_def, bar_index, "z", "p:C");

    YmTypeIndex other_index = ymParcelDef_AddMethodReq(p_def, A_index, nonASCIIName.c_str(), "p:C"); // w/out params

    BIND_AND_IMPORT(ctx, parcel, p_def, "p");
    auto A = ymCtx_Load(ctx, "p:A");
    auto B = ymCtx_Load(ctx, "p:B");
    auto C = ymCtx_Load(ctx, "p:C");
    auto A_foo = ymCtx_Load(ctx, "p:A::foo");
    auto A_bar = ymCtx_Load(ctx, "p:A::bar");
    auto A_other = ymCtx_Load(ctx, nonASCIIFullname.c_str());
    ASSERT_TRUE(A);
    ASSERT_TRUE(B);
    ASSERT_TRUE(C);
    ASSERT_TRUE(A_foo);
    ASSERT_TRUE(A_bar);
    ASSERT_TRUE(A_other);
    EXPECT_EQ(ymType_Parcel(A_foo), parcel);
    EXPECT_EQ(ymType_Parcel(A_bar), parcel);
    EXPECT_EQ(ymType_Parcel(A_other), parcel);
    EXPECT_STREQ(ymType_Fullname(A_foo), "p:A::foo");
    EXPECT_STREQ(ymType_Fullname(A_bar), "p:A::bar");
    EXPECT_STREQ(ymType_Fullname(A_other), nonASCIIFullname.c_str());
    EXPECT_EQ(ymType_Kind(A_foo), YmKind_Method);
    EXPECT_EQ(ymType_Kind(A_bar), YmKind_Method);
    EXPECT_EQ(ymType_Kind(A_other), YmKind_Method);
    EXPECT_EQ(ymType_Owner(A_foo), A);
    EXPECT_EQ(ymType_Owner(A_bar), A);
    EXPECT_EQ(ymType_Owner(A_other), A);
    EXPECT_EQ(ymType_ReturnType(A_foo), C);
    EXPECT_EQ(ymType_ReturnType(A_bar), C);
    EXPECT_EQ(ymType_ReturnType(A_other), C);
    EXPECT_EQ(ymType_Params(A_foo), 0);
    EXPECT_EQ(ymType_Params(A_bar), 3);
    EXPECT_EQ(ymType_Params(A_other), 0);
    EXPECT_STREQ(ymType_ParamName(A_bar, 0), "x");
    EXPECT_STREQ(ymType_ParamName(A_bar, 1), "y");
    EXPECT_STREQ(ymType_ParamName(A_bar, 2), "z");
    EXPECT_EQ(ymType_ParamType(A_bar, 0), C);
    EXPECT_EQ(ymType_ParamType(A_bar, 1), B);
    EXPECT_EQ(ymType_ParamType(A_bar, 2), C);
}

TEST(ParcelDefs, AddMethodReq_Normalizes_ReturnType) {
    SETUP_ALL(ctx);
    SETUP_PARCELDEF(p_def);

    auto A_index = ymParcelDef_AddStruct(p_def, "A");
    auto P_index = ymParcelDef_AddProtocol(p_def, "P");
    auto P_m_returnType = "  p  : \n\n A  ";
    auto P_m_index = ymParcelDef_AddMethodReq(p_def, P_index, "m", P_m_returnType);

    ymDm_BindParcelDef(dm, "p", p_def);
    auto A = ymCtx_Load(ctx, "p:A");
    auto P_m = ymCtx_Load(ctx, "p:P::m");
    ASSERT_TRUE(A);
    ASSERT_TRUE(P_m);

    ASSERT_EQ(ymType_ReturnType(P_m), A);
    EXPECT_STREQ(ymType_Fullname(ymType_ReturnType(P_m)), "p:A");
}

TEST(ParcelDefs, AddMethodReq_NameConflict_Type) {
    SETUP_ERRCOUNTER;
    SETUP_PARCELDEF(p_def);
    auto A_index = ymParcelDef_AddProtocol(p_def, "A");
    ASSERT_NE(A_index, YM_NO_TYPE_INDEX);
    ASSERT_NE(ymParcelDef_AddMethodReq(p_def, A_index, "m", "p:B"), YM_NO_TYPE_INDEX);
    ASSERT_EQ(ymParcelDef_AddMethodReq(p_def, A_index, "m", "p:B"), YM_NO_TYPE_INDEX);
    EXPECT_EQ(err[YmErrCode_NameConflict], 1);
}

TEST(ParcelDefs, AddMethodReq_NameConflict_TypeParam) {
    SETUP_ERRCOUNTER;
    SETUP_PARCELDEF(p_def);
    ymParcelDef_AddProtocol(p_def, "Any");
    auto A_index = ymParcelDef_AddProtocol(p_def, "A");
    ASSERT_NE(A_index, YM_NO_TYPE_INDEX);
    ASSERT_NE(ymParcelDef_AddTypeParam(p_def, A_index, "m", "p:Any"), YM_NO_TYPE_PARAM_INDEX);
    ASSERT_EQ(ymParcelDef_AddMethodReq(p_def, A_index, "m", "p:B"), YM_NO_TYPE_INDEX);
    EXPECT_EQ(err[YmErrCode_NameConflict], 1);
}

TEST(ParcelDefs, AddMethodReq_NameConflict_Self) {
    SETUP_ERRCOUNTER;
    SETUP_PARCELDEF(p_def);
    auto A_index = ymParcelDef_AddProtocol(p_def, "A");
    ASSERT_NE(A_index, YM_NO_TYPE_INDEX);
    ASSERT_EQ(ymParcelDef_AddMethodReq(p_def, A_index, "Self", "p:B"), YM_NO_TYPE_INDEX);
    EXPECT_EQ(err[YmErrCode_NameConflict], 1);
}

TEST(ParcelDefs, AddMethodReq_TypeNotFound_InvalidOwner) {
    SETUP_ERRCOUNTER;
    SETUP_PARCELDEF(p_def);
    ASSERT_EQ(ymParcelDef_AddMethodReq(p_def, 5'000, "m", "p:B"), YM_NO_TYPE_INDEX);
    EXPECT_EQ(err[YmErrCode_TypeNotFound], 1);
}

TEST(ParcelDefs, AddMethodReq_NonProtocolType) {
    SETUP_ERRCOUNTER;
    SETUP_PARCELDEF(p_def);
    auto A_index = ymParcelDef_AddStruct(p_def, "A");
    ASSERT_NE(A_index, YM_NO_TYPE_INDEX);
    ASSERT_EQ(ymParcelDef_AddMethodReq(p_def, A_index, "m", "p:B"), YM_NO_TYPE_INDEX);
    EXPECT_EQ(err[YmErrCode_NonProtocolType], 1);
}

TEST(ParcelDefs, AddMethodReq_IllegalSpecifier_InvalidReturnTypeSymbol) {
    SETUP_ERRCOUNTER;
    SETUP_PARCELDEF(p_def);
    auto A_index = ymParcelDef_AddProtocol(p_def, "A");
    ASSERT_NE(A_index, YM_NO_TYPE_INDEX);
    ASSERT_EQ(ymParcelDef_AddMethodReq(p_def, A_index, "m", "/"), YM_NO_TYPE_INDEX);
    EXPECT_EQ(err[YmErrCode_IllegalSpecifier], 1);
}

TEST(ParcelDefs, AddTypeParam) {
    SETUP_ALL(ctx);
    SETUP_PARCELDEF(p_def);
    ymParcelDef_AddProtocol(p_def, "Any");
    ymParcelDef_AddStruct(p_def, "Int");
    ymParcelDef_AddStruct(p_def, "Float");
    auto A_index = ymParcelDef_AddStruct(p_def, "A");
    ASSERT_EQ(ymParcelDef_AddTypeParam(p_def, A_index, "T", "p:Any"), 0);
    ASSERT_EQ(ymParcelDef_AddTypeParam(p_def, A_index, "U", "p:Any"), 1);
    ymDm_BindParcelDef(dm, "p", p_def);
    auto A_Int_Float = ymCtx_Load(ctx, "p:A[p:Int, p:Float]");
    auto Int = ymCtx_Load(ctx, "p:Int");
    auto Float = ymCtx_Load(ctx, "p:Float");
    ASSERT_TRUE(A_Int_Float);
    ASSERT_TRUE(Int);
    ASSERT_TRUE(Float);
    ASSERT_EQ(ymType_TypeParams(A_Int_Float), 2);
    EXPECT_EQ(ymType_TypeParamByIndex(A_Int_Float, 0), Int);
    EXPECT_EQ(ymType_TypeParamByIndex(A_Int_Float, 1), Float);
}

TEST(ParcelDefs, AddTypeParam_Normalizes_ConstraintType) {
    SETUP_ALL(ctx);
    SETUP_PARCELDEF(p_def);

    auto Any_index = ymParcelDef_AddProtocol(p_def, "Any");
    auto Int_index = ymParcelDef_AddStruct(p_def, "Int");
    auto A_index = ymParcelDef_AddStruct(p_def, "A");
    auto A_T_constraintType = "  p  : \n\n Any  ";
    ymParcelDef_AddTypeParam(p_def, A_index, "T", A_T_constraintType);

    ymDm_BindParcelDef(dm, "p", p_def);
    auto A_Int = ymCtx_Load(ctx, "p:A[p:Int]");
    auto Int = ymCtx_Load(ctx, "p:Int");
    ASSERT_TRUE(A_Int);
    ASSERT_TRUE(Int);

    ASSERT_EQ(ymType_TypeParamByName(A_Int, "T"), Int);
    EXPECT_STREQ(ymType_Fullname(ymType_TypeParamByName(A_Int, "T")), "p:Int");
}

TEST(ParcelDefs, AddTypeParam_MaxTypeParams) {
    SETUP_ALL(ctx);
    SETUP_PARCELDEF(p_def);
    ymParcelDef_AddProtocol(p_def, "Any");
    ymParcelDef_AddStruct(p_def, "Int");
    ymParcelDef_AddStruct(p_def, "Float");
    auto A_index = ymParcelDef_AddStruct(p_def, "A");
    for (YmTypeParamIndex i = 0; i < YM_MAX_TYPE_PARAMS; i++) {
        auto name = std::format("T{}", i + 1);
        ASSERT_EQ(ymParcelDef_AddTypeParam(p_def, A_index, name.c_str(), "p:Any"), i);
    }
    ymDm_BindParcelDef(dm, "p", p_def);
    auto A = ymCtx_Load(ctx, "p:A[p:Int, p:Int, p:Int, p:Int, p:Float, p:Int, p:Int, p:Int, p:Int, p:Float, p:Int, p:Int, p:Int, p:Int, p:Float, p:Int, p:Int, p:Int, p:Int, p:Float, p:Int, p:Int, p:Int, p:Int, p:Float]");
    auto Int = ymCtx_Load(ctx, "p:Int");
    auto Float = ymCtx_Load(ctx, "p:Float");
    ASSERT_TRUE(A);
    ASSERT_TRUE(Int);
    ASSERT_TRUE(Float);
    ASSERT_EQ(ymType_TypeParams(A), YM_MAX_TYPE_PARAMS);
    for (YmTypeParamIndex i = 0; i < YM_MAX_TYPE_PARAMS; i++) {
        auto name = std::format("T{}", i + 1);
        auto t = i % 5 == 4 ? Float : Int;
        EXPECT_EQ(ymType_TypeParamByIndex(A, i), t);
        EXPECT_EQ(ymType_TypeParamByName(A, name.c_str()), t);
    }
}

TEST(ParcelDefs, AddTypeParam_TypeNotFound) {
    SETUP_ALL(ctx);
    SETUP_PARCELDEF(p_def);
    ymParcelDef_AddProtocol(p_def, "Any");
    EXPECT_EQ(ymParcelDef_AddTypeParam(p_def, 500, "T", "p:Any"), YM_NO_TYPE_PARAM_INDEX);
    EXPECT_EQ(err[YmErrCode_TypeNotFound], 1);
}

TEST(ParcelDefs, AddTypeParam_MemberType) {
    SETUP_ALL(ctx);
    SETUP_PARCELDEF(p_def);
    ymParcelDef_AddProtocol(p_def, "Any");
    ymParcelDef_AddStruct(p_def, "Int");
    auto A_index = ymParcelDef_AddStruct(p_def, "A");
    auto A_m_index = ymParcelDef_AddMethod(p_def, A_index, "m", "p:Int", ymInertCallBhvrFn, nullptr);
    EXPECT_EQ(ymParcelDef_AddTypeParam(p_def, A_m_index, "T", "p:Any"), YM_NO_TYPE_PARAM_INDEX);
    EXPECT_EQ(err[YmErrCode_MemberType], 1);
}

TEST(ParcelDefs, AddTypeParam_NameConflict_MemberType) {
    SETUP_ALL(ctx);
    SETUP_PARCELDEF(p_def);
    ymParcelDef_AddProtocol(p_def, "Any");
    ymParcelDef_AddStruct(p_def, "Int");
    auto A_index = ymParcelDef_AddStruct(p_def, "A");
    ymParcelDef_AddMethod(p_def, A_index, "m", "p:Int", ymInertCallBhvrFn, nullptr);
    EXPECT_EQ(ymParcelDef_AddTypeParam(p_def, A_index, "m", "p:Any"), YM_NO_TYPE_PARAM_INDEX);
    EXPECT_EQ(err[YmErrCode_NameConflict], 1);
}

TEST(ParcelDefs, AddTypeParam_NameConflict_TypeParam) {
    SETUP_ALL(ctx);
    SETUP_PARCELDEF(p_def);
    ymParcelDef_AddProtocol(p_def, "Any");
    ymParcelDef_AddStruct(p_def, "Int");
    auto A_index = ymParcelDef_AddStruct(p_def, "A");
    EXPECT_NE(ymParcelDef_AddTypeParam(p_def, A_index, "T", "p:Any"), YM_NO_TYPE_PARAM_INDEX);
    EXPECT_EQ(ymParcelDef_AddTypeParam(p_def, A_index, "T", "p:Any"), YM_NO_TYPE_PARAM_INDEX);
    EXPECT_EQ(err[YmErrCode_NameConflict], 1);
}

TEST(ParcelDefs, AddTypeParam_NameConflict_Self) {
    SETUP_ALL(ctx);
    SETUP_PARCELDEF(p_def);
    ymParcelDef_AddProtocol(p_def, "Any");
    auto A_index = ymParcelDef_AddStruct(p_def, "A");
    EXPECT_EQ(ymParcelDef_AddTypeParam(p_def, A_index, "Self", "p:Any"), YM_NO_TYPE_PARAM_INDEX);
    EXPECT_EQ(err[YmErrCode_NameConflict], 1);
}

TEST(ParcelDefs, AddTypeParam_IllegalSpecifier_InvalidConstraint) {
    SETUP_ALL(ctx);
    SETUP_PARCELDEF(p_def);
    ymParcelDef_AddProtocol(p_def, "Any");
    auto A_index = ymParcelDef_AddStruct(p_def, "A");
    EXPECT_EQ(ymParcelDef_AddTypeParam(p_def, A_index, "T", "/"), YM_NO_TYPE_PARAM_INDEX);
    EXPECT_EQ(err[YmErrCode_IllegalSpecifier], 1);
}

TEST(ParcelDefs, AddTypeParam_IllegalSpecifier_ContainsRefsToTypeParamsNotYetDefined) {
    // NOTE: This test can be implemented via checking for any EXISTING type param, but the actual
    //       semantic being enforced is more about, given say 'A[T: P[U], U: Q]', type param T
    //       not being allowed to refer to U, as a matter of definition order, REGARDLESS of whether
    //       U has/hasn't been formally defined during parcel construction yet.
    SETUP_ALL(ctx);
    SETUP_PARCELDEF(p_def);
    ymParcelDef_AddProtocol(p_def, "Any");
    auto P_index = ymParcelDef_AddProtocol(p_def, "P");
    ymParcelDef_AddTypeParam(p_def, P_index, "T", "p:Any");
    auto A_index = ymParcelDef_AddStruct(p_def, "A");
    EXPECT_EQ(ymParcelDef_AddTypeParam(p_def, A_index, "T", "p:P[$U]"), YM_NO_TYPE_PARAM_INDEX);
    EXPECT_EQ(err[YmErrCode_IllegalSpecifier], 1);
}

TEST(ParcelDefs, AddTypeParam_IllegalConstraint_ImmediateTypeParamRefCannotBeUsedAsConstraintType) {
    // NOTE: To be clear, something like 'A[T: Any, U: B[T]]' is fine, as U's usage of T
    //       wraps it in B. The problem only arises in cases like 'A[T: Any, U: T]', where
    //       we're trying to use T as the constraint itself, in which case we run into the
    //       problem of not knowing STATICALLY whether T is a protocol or not.
    //
    //       Now, DYNAMICALLY we can discern if T is a protocol, but I've decided that since
    //       allowing that would mean that U's interface is indeterminate statically, as we
    //       won't know T's interface until load-time, that it's best to just disallow such
    //       usage for the time being.
    //          * TODO: Does T having constraint Any mean we could *deduce* T's interface (and
    //                  thus U's) based on T's constraint?
    //
    //       Maybe later we can look into this again, but for now we're just gonna forbid it.
    SETUP_ALL(ctx);
    SETUP_PARCELDEF(p_def);
    ymParcelDef_AddProtocol(p_def, "Any");
    auto A_index = ymParcelDef_AddStruct(p_def, "A");
    EXPECT_NE(ymParcelDef_AddTypeParam(p_def, A_index, "T", "p:Any"), YM_NO_TYPE_PARAM_INDEX);
    EXPECT_EQ(ymParcelDef_AddTypeParam(p_def, A_index, "U", "$T"), YM_NO_TYPE_PARAM_INDEX);
    EXPECT_EQ(err[YmErrCode_IllegalConstraint], 1);
}

TEST(ParcelDefs, AddTypeParam_LimitReached_MaxTypeParams) {
    SETUP_ALL(ctx);
    SETUP_PARCELDEF(p_def);
    ymParcelDef_AddProtocol(p_def, "Any");
    ymParcelDef_AddStruct(p_def, "Int");
    auto A_index = ymParcelDef_AddStruct(p_def, "A");
    for (YmTypeParamIndex i = 0; i < YM_MAX_TYPE_PARAMS; i++) {
        auto name = std::format("T{}", i + 1);
        ASSERT_EQ(ymParcelDef_AddTypeParam(p_def, A_index, name.c_str(), "p:Any"), i);
    }
    EXPECT_EQ(ymParcelDef_AddTypeParam(p_def, A_index, "TXX", "p:Int"), YM_NO_TYPE_PARAM_INDEX);
    EXPECT_EQ(err[YmErrCode_LimitReached], 1);
}

TEST(ParcelDefs, AddParam) {
    SETUP_ALL(ctx);
    SETUP_PARCELDEF(p_def);
    auto A_index = ymParcelDef_AddFn(p_def, "A", "p:B", ymInertCallBhvrFn, nullptr);
    ymParcelDef_AddStruct(p_def, "B");
    ymParcelDef_AddStruct(p_def, "C");
    ymParcelDef_AddStruct(p_def, "D");
    EXPECT_EQ(ymParcelDef_AddParam(p_def, A_index, "x", "p:B"), 0);
    EXPECT_EQ(ymParcelDef_AddParam(p_def, A_index, "y", "p:C"), 1);
    EXPECT_EQ(ymParcelDef_AddParam(p_def, A_index, "z", "p:D"), 2);
    ymDm_BindParcelDef(dm, "p", p_def);
    auto A = ymCtx_Load(ctx, "p:A");
    auto B = ymCtx_Load(ctx, "p:B");
    auto C = ymCtx_Load(ctx, "p:C");
    auto D = ymCtx_Load(ctx, "p:D");
    ASSERT_TRUE(A);
    ASSERT_TRUE(B);
    ASSERT_TRUE(C);
    ASSERT_TRUE(D);
    ASSERT_EQ(ymType_Params(A), 3);
    EXPECT_STREQ(ymType_ParamName(A, 0), "x");
    EXPECT_STREQ(ymType_ParamName(A, 1), "y");
    EXPECT_STREQ(ymType_ParamName(A, 2), "z");
    EXPECT_EQ(ymType_ParamType(A, 0), B);
    EXPECT_EQ(ymType_ParamType(A, 1), C);
    EXPECT_EQ(ymType_ParamType(A, 2), D);
}

TEST(ParcelDefs, AddParam_Normalizes_ParamType) {
    SETUP_ALL(ctx);
    SETUP_PARCELDEF(p_def);

    auto Int_index = ymParcelDef_AddStruct(p_def, "Int");
    auto f_index = ymParcelDef_AddFn(p_def, "f", "p:Int", ymInertCallBhvrFn, nullptr);
    auto f_paramType = "  p  : \n\n Int  ";
    ymParcelDef_AddParam(p_def, f_index, "a", f_paramType);

    ymDm_BindParcelDef(dm, "p", p_def);
    auto f = ymCtx_Load(ctx, "p:f");
    auto Int = ymCtx_Load(ctx, "p:Int");
    ASSERT_TRUE(f);
    ASSERT_TRUE(Int);

    ASSERT_EQ(ymType_ParamType(f, 0), Int);
    EXPECT_STREQ(ymType_Fullname(ymType_ParamType(f, 0)), "p:Int");
}

TEST(ParcelDefs, AddParam_MaxParams) {
    SETUP_ALL(ctx);
    SETUP_PARCELDEF(p_def);
    auto A_index = ymParcelDef_AddFn(p_def, "A", "p:B", ymInertCallBhvrFn, nullptr);
    for (YmParamIndex i = 0; i < YM_MAX_PARAMS; i++) {
        std::string paramName = std::format("a{}", i);
        ASSERT_EQ(ymParcelDef_AddParam(p_def, A_index, paramName.c_str(), "p:B"), i) << "i == " << i;
    }
    ymParcelDef_AddStruct(p_def, "B");
    ymDm_BindParcelDef(dm, "p", p_def);
    auto A = ymCtx_Load(ctx, "p:A");
    auto B = ymCtx_Load(ctx, "p:B");
    ASSERT_TRUE(A);
    ASSERT_TRUE(B);
    ASSERT_EQ(ymType_Params(A), YM_MAX_PARAMS);
    for (YmParamIndex i = 0; i < YM_MAX_PARAMS; i++) {
        std::string paramName = std::format("a{}", i);
        EXPECT_STREQ(ymType_ParamName(A, i), paramName.c_str()) << "i == " << i;
        EXPECT_EQ(ymType_ParamType(A, i), B) << "i == " << i;
    }
}

TEST(ParcelDefs, AddParam_TypeNotFound) {
    SETUP_ALL(ctx);
    SETUP_PARCELDEF(p_def);
    EXPECT_EQ(ymParcelDef_AddParam(p_def, 500, "x", "p:B"), YM_NO_PARAM_INDEX);
    EXPECT_EQ(err[YmErrCode_TypeNotFound], 1);
}

TEST(ParcelDefs, AddParam_NonCallableType) {
    SETUP_ALL(ctx);
    SETUP_PARCELDEF(p_def);
    auto A_index = ymParcelDef_AddStruct(p_def, "A");
    EXPECT_EQ(ymParcelDef_AddParam(p_def, A_index, "x", "p:B"), YM_NO_PARAM_INDEX);
    EXPECT_EQ(err[YmErrCode_NonCallableType], 1);
}

TEST(ParcelDefs, AddParam_NameConflict_BetweenParams) {
    SETUP_ALL(ctx);
    SETUP_PARCELDEF(p_def);
    auto A_index = ymParcelDef_AddFn(p_def, "A", "p:B", ymInertCallBhvrFn, nullptr);
    EXPECT_EQ(ymParcelDef_AddParam(p_def, A_index, "x", "p:B"), 0);
    EXPECT_EQ(ymParcelDef_AddParam(p_def, A_index, "x", "p:C"), YM_NO_PARAM_INDEX);
    EXPECT_EQ(err[YmErrCode_NameConflict], 1);
}

TEST(ParcelDefs, AddParam_IllegalSpecifier) {
    SETUP_ALL(ctx);
    SETUP_PARCELDEF(p_def);
    auto A_index = ymParcelDef_AddFn(p_def, "A", "p:B", ymInertCallBhvrFn, nullptr);
    EXPECT_EQ(ymParcelDef_AddParam(p_def, A_index, "x", "/"), YM_NO_PARAM_INDEX);
    EXPECT_EQ(err[YmErrCode_IllegalSpecifier], 1);
}

TEST(ParcelDefs, AddParam_LimitReached_MaxParams) {
    SETUP_ALL(ctx);
    SETUP_PARCELDEF(p_def);
    auto A_index = ymParcelDef_AddFn(p_def, "A", "p:B", ymInertCallBhvrFn, nullptr);
    for (YmParamIndex i = 0; i < YM_MAX_PARAMS; i++) {
        std::string paramName = std::format("a{}", i);
        ASSERT_EQ(ymParcelDef_AddParam(p_def, A_index, paramName.c_str(), "p:B"), i) << "i == " << i;
    }
    EXPECT_EQ(ymParcelDef_AddParam(p_def, A_index, "b", "p:B"), YM_NO_PARAM_INDEX);
    EXPECT_EQ(err[YmErrCode_LimitReached], 1);
}

TEST(ParcelDefs, AddRef) {
    SETUP_ALL(ctx);
    SETUP_PARCELDEF(p_def);
    auto A_index = ymParcelDef_AddStruct(p_def, "A");
    ymParcelDef_AddStruct(p_def, "B");
    YmRef A_ref_B = ymParcelDef_AddRef(p_def, A_index, "p:B");
    ASSERT_NE(A_ref_B, YM_NO_REF);
    ymDm_BindParcelDef(dm, "p", p_def);
    auto A = ymCtx_Load(ctx, "p:A");
    auto B = ymCtx_Load(ctx, "p:B");
    ASSERT_TRUE(A);
    ASSERT_TRUE(B);
    EXPECT_EQ(ymType_Ref(A, A_ref_B), B);
}

TEST(ParcelDefs, AddRef_Normalizes_Symbol) {
    SETUP_ALL(ctx);
    SETUP_PARCELDEF(p_def);

    auto Int_index = ymParcelDef_AddStruct(p_def, "Int");
    auto A_index = ymParcelDef_AddStruct(p_def, "A");
    auto A_symbol = "  p  : \n\n Int  ";
    auto A_ref = ymParcelDef_AddRef(p_def, A_index, A_symbol);
    ASSERT_NE(A_ref, YM_NO_REF);

    ymDm_BindParcelDef(dm, "p", p_def);
    auto A = ymCtx_Load(ctx, "p:A");
    auto Int = ymCtx_Load(ctx, "p:Int");
    ASSERT_TRUE(A);
    ASSERT_TRUE(Int);

    ASSERT_EQ(ymType_Ref(A, A_ref), Int);
    EXPECT_STREQ(ymType_Fullname(ymType_Ref(A, A_ref)), "p:Int");
}

TEST(ParcelDefs, AddRef_TypeNotFound) {
    SETUP_ERRCOUNTER;
    SETUP_PARCELDEF(p_def);
    ASSERT_EQ(ymParcelDef_AddRef(p_def, 5'000, "p:B"), YM_NO_REF);
    EXPECT_EQ(err[YmErrCode_TypeNotFound], 1);
}

TEST(ParcelDefs, AddRef_IllegalSpecifier) {
    for (const auto& fullname : illegalFullnames) {
        SETUP_ERRCOUNTER;
        SETUP_PARCELDEF(p_def);
        YmTypeIndex p_A_index = ymParcelDef_AddStruct(p_def, "A");
        ASSERT_NE(p_A_index, YM_NO_TYPE_INDEX);
        EXPECT_EQ(ymParcelDef_AddRef(p_def, p_A_index, fullname.c_str()), YM_NO_REF) << "fullname==" << fullname;
        EXPECT_EQ(err[YmErrCode_IllegalSpecifier], 1) << "fullname==" << fullname;
    }
}

