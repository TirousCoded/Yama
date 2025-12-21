

#include <gtest/gtest.h>
#include <taul/strings.h>
#include <yama/yama.h>
#include <yama++/print.h>

#include "../utils/utils.h"


TEST(ParcelDefs, CreateAndDestroy) {
    SETUP_ERRCOUNTER;
    SETUP_PARCELDEF(p_def); // Macro will do the create/destroy.
}

TEST(ParcelDefs, AddStruct) {
    SETUP_ALL(ctx);
    SETUP_PARCELDEF(p_def);
    std::string otherName = taul::utf8_s(u8"ab魂💩cd"); // Ensure can handle UTF-8.
    std::string otherFullname = taul::utf8_s(u8"p:ab魂💩cd"); // Ensure can handle UTF-8.
    YmItemIndex foo_index = ymParcelDef_AddStruct(p_def, "foo");
    YmItemIndex bar_index = ymParcelDef_AddStruct(p_def, "bar");
    YmItemIndex other_index = ymParcelDef_AddStruct(p_def, otherName.c_str());
    ASSERT_NE(foo_index, YM_NO_ITEM_INDEX);
    ASSERT_NE(bar_index, YM_NO_ITEM_INDEX);
    ASSERT_NE(other_index, YM_NO_ITEM_INDEX);

    BIND_AND_IMPORT(ctx, parcel, p_def, "p");
    auto foo = ymCtx_Load(ctx, "p:foo");
    auto bar = ymCtx_Load(ctx, "p:bar");
    auto other = ymCtx_Load(ctx, otherFullname.c_str());
    ASSERT_TRUE(foo);
    ASSERT_TRUE(bar);
    ASSERT_TRUE(other);
    EXPECT_EQ(ymItem_Parcel(foo), parcel);
    EXPECT_EQ(ymItem_Parcel(bar), parcel);
    EXPECT_EQ(ymItem_Parcel(other), parcel);
    EXPECT_STREQ(ymItem_Fullname(foo), "p:foo");
    EXPECT_STREQ(ymItem_Fullname(bar), "p:bar");
    EXPECT_STREQ(ymItem_Fullname(other), otherFullname.c_str());
    EXPECT_EQ(ymItem_Kind(foo), YmKind_Struct);
    EXPECT_EQ(ymItem_Kind(bar), YmKind_Struct);
    EXPECT_EQ(ymItem_Kind(other), YmKind_Struct);
    EXPECT_EQ(ymItem_ReturnType(foo), nullptr);
    EXPECT_EQ(ymItem_ReturnType(bar), nullptr);
    EXPECT_EQ(ymItem_ReturnType(other), nullptr);
    EXPECT_EQ(ymItem_Params(foo), 0);
    EXPECT_EQ(ymItem_Params(bar), 0);
    EXPECT_EQ(ymItem_Params(other), 0);
}

TEST(ParcelDefs, AddStruct_ItemNameConflict) {
    SETUP_ERRCOUNTER;
    SETUP_PARCELDEF(p_def);
    ASSERT_NE(ymParcelDef_AddStruct(p_def, "foo"), YM_NO_ITEM_INDEX);
    ASSERT_EQ(ymParcelDef_AddStruct(p_def, "foo"), YM_NO_ITEM_INDEX);
    EXPECT_EQ(err[YmErrCode_ItemNameConflict], 1);
}

TEST(ParcelDefs, AddProtocol) {
    SETUP_ALL(ctx);
    SETUP_PARCELDEF(p_def);
    std::string otherName = taul::utf8_s(u8"ab魂💩cd"); // Ensure can handle UTF-8.
    std::string otherFullname = taul::utf8_s(u8"p:ab魂💩cd"); // Ensure can handle UTF-8.
    YmItemIndex foo_index = ymParcelDef_AddProtocol(p_def, "foo");
    YmItemIndex bar_index = ymParcelDef_AddProtocol(p_def, "bar");
    YmItemIndex other_index = ymParcelDef_AddProtocol(p_def, otherName.c_str());
    ASSERT_NE(foo_index, YM_NO_ITEM_INDEX);
    ASSERT_NE(bar_index, YM_NO_ITEM_INDEX);
    ASSERT_NE(other_index, YM_NO_ITEM_INDEX);

    BIND_AND_IMPORT(ctx, parcel, p_def, "p");
    auto foo = ymCtx_Load(ctx, "p:foo");
    auto bar = ymCtx_Load(ctx, "p:bar");
    auto other = ymCtx_Load(ctx, otherFullname.c_str());
    ASSERT_TRUE(foo);
    ASSERT_TRUE(bar);
    ASSERT_TRUE(other);
    EXPECT_EQ(ymItem_Parcel(foo), parcel);
    EXPECT_EQ(ymItem_Parcel(bar), parcel);
    EXPECT_EQ(ymItem_Parcel(other), parcel);
    EXPECT_STREQ(ymItem_Fullname(foo), "p:foo");
    EXPECT_STREQ(ymItem_Fullname(bar), "p:bar");
    EXPECT_STREQ(ymItem_Fullname(other), otherFullname.c_str());
    EXPECT_EQ(ymItem_Kind(foo), YmKind_Protocol);
    EXPECT_EQ(ymItem_Kind(bar), YmKind_Protocol);
    EXPECT_EQ(ymItem_Kind(other), YmKind_Protocol);
    EXPECT_EQ(ymItem_ReturnType(foo), nullptr);
    EXPECT_EQ(ymItem_ReturnType(bar), nullptr);
    EXPECT_EQ(ymItem_ReturnType(other), nullptr);
    EXPECT_EQ(ymItem_Params(foo), 0);
    EXPECT_EQ(ymItem_Params(bar), 0);
    EXPECT_EQ(ymItem_Params(other), 0);
}

TEST(ParcelDefs, AddProtocol_ItemNameConflict) {
    SETUP_ERRCOUNTER;
    SETUP_PARCELDEF(p_def);
    ASSERT_NE(ymParcelDef_AddStruct(p_def, "foo"), YM_NO_ITEM_INDEX);
    ASSERT_EQ(ymParcelDef_AddProtocol(p_def, "foo"), YM_NO_ITEM_INDEX);
    EXPECT_EQ(err[YmErrCode_ItemNameConflict], 1);
}

TEST(ParcelDefs, AddFn) {
    SETUP_ALL(ctx);
    SETUP_PARCELDEF(p_def);
    std::string otherName = taul::utf8_s(u8"ab魂💩cd"); // Ensure can handle UTF-8.
    std::string otherFullname = taul::utf8_s(u8"p:ab魂💩cd"); // Ensure can handle UTF-8.
    ymParcelDef_AddStruct(p_def, "A");
    ymParcelDef_AddStruct(p_def, "B");
    ymParcelDef_AddStruct(p_def, "C");

    YmItemIndex foo_index = ymParcelDef_AddFn(p_def, "foo", "p:A", ymInertCallBhvrFn, nullptr); // w/out params

    YmItemIndex bar_index = ymParcelDef_AddFn(p_def, "bar", "p:A", ymInertCallBhvrFn, nullptr); // w/ params
    ymParcelDef_AddParam(p_def, bar_index, "x", "p:A");
    ymParcelDef_AddParam(p_def, bar_index, "y", "p:B");
    ymParcelDef_AddParam(p_def, bar_index, "z", "p:C");

    YmItemIndex other_index = ymParcelDef_AddFn(p_def, otherName.c_str(), "p:A", ymInertCallBhvrFn, nullptr); // w/out params

    BIND_AND_IMPORT(ctx, parcel, p_def, "p");
    auto A = ymCtx_Load(ctx, "p:A");
    auto B = ymCtx_Load(ctx, "p:B");
    auto C = ymCtx_Load(ctx, "p:C");
    auto foo = ymCtx_Load(ctx, "p:foo");
    auto bar = ymCtx_Load(ctx, "p:bar");
    auto other = ymCtx_Load(ctx, otherFullname.c_str());
    ASSERT_TRUE(A);
    ASSERT_TRUE(B);
    ASSERT_TRUE(C);
    ASSERT_TRUE(foo);
    ASSERT_TRUE(bar);
    ASSERT_TRUE(other);
    EXPECT_EQ(ymItem_Parcel(foo), parcel);
    EXPECT_EQ(ymItem_Parcel(bar), parcel);
    EXPECT_EQ(ymItem_Parcel(other), parcel);
    EXPECT_STREQ(ymItem_Fullname(foo), "p:foo");
    EXPECT_STREQ(ymItem_Fullname(bar), "p:bar");
    EXPECT_STREQ(ymItem_Fullname(other), otherFullname.c_str());
    EXPECT_EQ(ymItem_Kind(foo), YmKind_Fn);
    EXPECT_EQ(ymItem_Kind(bar), YmKind_Fn);
    EXPECT_EQ(ymItem_Kind(other), YmKind_Fn);
    EXPECT_EQ(ymItem_ReturnType(foo), A);
    EXPECT_EQ(ymItem_ReturnType(bar), A);
    EXPECT_EQ(ymItem_ReturnType(other), A);
    EXPECT_EQ(ymItem_Params(foo), 0);
    EXPECT_EQ(ymItem_Params(bar), 3);
    EXPECT_EQ(ymItem_Params(other), 0);
    EXPECT_STREQ(ymItem_ParamName(bar, 0), "x");
    EXPECT_STREQ(ymItem_ParamName(bar, 1), "y");
    EXPECT_STREQ(ymItem_ParamName(bar, 2), "z");
    EXPECT_EQ(ymItem_ParamType(bar, 0), A);
    EXPECT_EQ(ymItem_ParamType(bar, 1), B);
    EXPECT_EQ(ymItem_ParamType(bar, 2), C);
}

TEST(ParcelDefs, AddFn_ItemNameConflict) {
    SETUP_ERRCOUNTER;
    SETUP_PARCELDEF(p_def);
    ASSERT_NE(ymParcelDef_AddStruct(p_def, "A"), YM_NO_ITEM_INDEX);
    ASSERT_EQ(ymParcelDef_AddFn(p_def, "A", "p:B", ymInertCallBhvrFn, nullptr), YM_NO_ITEM_INDEX);
    EXPECT_EQ(err[YmErrCode_ItemNameConflict], 1);
}

TEST(ParcelDefs, AddFn_IllegalRefSym_InvalidReturnTypeSymbol) {
    SETUP_ERRCOUNTER;
    SETUP_PARCELDEF(p_def);
    ASSERT_EQ(ymParcelDef_AddFn(p_def, "A", "/", ymInertCallBhvrFn, nullptr), YM_NO_ITEM_INDEX);
    EXPECT_EQ(err[YmErrCode_IllegalRefSym], 1);
}

TEST(ParcelDefs, AddMethod) {
    SETUP_ALL(ctx);
    SETUP_PARCELDEF(p_def);
    std::string otherName = taul::utf8_s(u8"ab魂💩cd"); // Ensure can handle UTF-8.
    std::string otherFullname = taul::utf8_s(u8"p:A::ab魂💩cd"); // Ensure can handle UTF-8.
    auto A_index = ymParcelDef_AddStruct(p_def, "A");
    ymParcelDef_AddStruct(p_def, "B");
    ymParcelDef_AddStruct(p_def, "C");

    YmItemIndex foo_index = ymParcelDef_AddMethod(p_def, A_index, "foo", "p:C", ymInertCallBhvrFn, nullptr); // w/out params

    YmItemIndex bar_index = ymParcelDef_AddMethod(p_def, A_index, "bar", "p:C", ymInertCallBhvrFn, nullptr); // w/ params
    ymParcelDef_AddParam(p_def, bar_index, "x", "p:C");
    ymParcelDef_AddParam(p_def, bar_index, "y", "p:B");
    ymParcelDef_AddParam(p_def, bar_index, "z", "p:C");

    YmItemIndex other_index = ymParcelDef_AddMethod(p_def, A_index, otherName.c_str(), "p:C", ymInertCallBhvrFn, nullptr); // w/out params

    BIND_AND_IMPORT(ctx, parcel, p_def, "p");
    auto A = ymCtx_Load(ctx, "p:A");
    auto B = ymCtx_Load(ctx, "p:B");
    auto C = ymCtx_Load(ctx, "p:C");
    auto A_foo = ymCtx_Load(ctx, "p:A::foo");
    auto A_bar = ymCtx_Load(ctx, "p:A::bar");
    auto A_other = ymCtx_Load(ctx, otherFullname.c_str());
    ASSERT_TRUE(A);
    ASSERT_TRUE(B);
    ASSERT_TRUE(C);
    ASSERT_TRUE(A_foo);
    ASSERT_TRUE(A_bar);
    ASSERT_TRUE(A_other);
    EXPECT_EQ(ymItem_Parcel(A_foo), parcel);
    EXPECT_EQ(ymItem_Parcel(A_bar), parcel);
    EXPECT_EQ(ymItem_Parcel(A_other), parcel);
    EXPECT_STREQ(ymItem_Fullname(A_foo), "p:A::foo");
    EXPECT_STREQ(ymItem_Fullname(A_bar), "p:A::bar");
    EXPECT_STREQ(ymItem_Fullname(A_other), otherFullname.c_str());
    EXPECT_EQ(ymItem_Kind(A_foo), YmKind_Method);
    EXPECT_EQ(ymItem_Kind(A_bar), YmKind_Method);
    EXPECT_EQ(ymItem_Kind(A_other), YmKind_Method);
    EXPECT_EQ(ymItem_Owner(A_foo), A);
    EXPECT_EQ(ymItem_Owner(A_bar), A);
    EXPECT_EQ(ymItem_Owner(A_other), A);
    EXPECT_EQ(ymItem_ReturnType(A_foo), C);
    EXPECT_EQ(ymItem_ReturnType(A_bar), C);
    EXPECT_EQ(ymItem_ReturnType(A_other), C);
    EXPECT_EQ(ymItem_Params(A_foo), 0);
    EXPECT_EQ(ymItem_Params(A_bar), 3);
    EXPECT_EQ(ymItem_Params(A_other), 0);
    EXPECT_STREQ(ymItem_ParamName(A_bar, 0), "x");
    EXPECT_STREQ(ymItem_ParamName(A_bar, 1), "y");
    EXPECT_STREQ(ymItem_ParamName(A_bar, 2), "z");
    EXPECT_EQ(ymItem_ParamType(A_bar, 0), C);
    EXPECT_EQ(ymItem_ParamType(A_bar, 1), B);
    EXPECT_EQ(ymItem_ParamType(A_bar, 2), C);
}

TEST(ParcelDefs, AddMethod_ItemNameConflict) {
    SETUP_ERRCOUNTER;
    SETUP_PARCELDEF(p_def);
    auto A_index = ymParcelDef_AddStruct(p_def, "A");
    ASSERT_NE(A_index, YM_NO_ITEM_INDEX);
    ASSERT_NE(ymParcelDef_AddMethod(p_def, A_index, "m", "p:B", ymInertCallBhvrFn, nullptr), YM_NO_ITEM_INDEX);
    ASSERT_EQ(ymParcelDef_AddMethod(p_def, A_index, "m", "p:B", ymInertCallBhvrFn, nullptr), YM_NO_ITEM_INDEX);
    EXPECT_EQ(err[YmErrCode_ItemNameConflict], 1);
}

TEST(ParcelDefs, AddMethod_ItemNotFound_InvalidOwner) {
    SETUP_ERRCOUNTER;
    SETUP_PARCELDEF(p_def);
    ASSERT_EQ(ymParcelDef_AddMethod(p_def, 5'000, "m", "p:B", ymInertCallBhvrFn, nullptr), YM_NO_ITEM_INDEX);
    EXPECT_EQ(err[YmErrCode_ItemNotFound], 1);
}

TEST(ParcelDefs, AddMethod_ItemCannotHaveMembers) {
    SETUP_ERRCOUNTER;
    SETUP_PARCELDEF(p_def);
    auto A_index = ymParcelDef_AddFn(p_def, "A", "p:B", ymInertCallBhvrFn, nullptr);
    ASSERT_NE(A_index, YM_NO_ITEM_INDEX);
    ASSERT_EQ(ymParcelDef_AddMethod(p_def, A_index, "m", "p:B", ymInertCallBhvrFn, nullptr), YM_NO_ITEM_INDEX);
    EXPECT_EQ(err[YmErrCode_ItemCannotHaveMembers], 1);
}

TEST(ParcelDefs, AddMethod_ProtocolItem) {
    SETUP_ERRCOUNTER;
    SETUP_PARCELDEF(p_def);
    auto A_index = ymParcelDef_AddProtocol(p_def, "A");
    ASSERT_NE(A_index, YM_NO_ITEM_INDEX);
    ASSERT_EQ(ymParcelDef_AddMethod(p_def, A_index, "m", "p:B", ymInertCallBhvrFn, nullptr), YM_NO_ITEM_INDEX);
    EXPECT_EQ(err[YmErrCode_ProtocolItem], 1);
}

TEST(ParcelDefs, AddMethod_IllegalRefSym_InvalidReturnTypeSymbol) {
    SETUP_ERRCOUNTER;
    SETUP_PARCELDEF(p_def);
    auto A_index = ymParcelDef_AddStruct(p_def, "A");
    ASSERT_NE(A_index, YM_NO_ITEM_INDEX);
    ASSERT_EQ(ymParcelDef_AddMethod(p_def, A_index, "m", "/", ymInertCallBhvrFn, nullptr), YM_NO_ITEM_INDEX);
    EXPECT_EQ(err[YmErrCode_IllegalRefSym], 1);
}

TEST(ParcelDefs, AddMethodReq) {
    SETUP_ALL(ctx);
    SETUP_PARCELDEF(p_def);
    std::string otherName = taul::utf8_s(u8"ab魂💩cd"); // Ensure can handle UTF-8.
    std::string otherFullname = taul::utf8_s(u8"p:A::ab魂💩cd"); // Ensure can handle UTF-8.
    auto A_index = ymParcelDef_AddProtocol(p_def, "A");
    ymParcelDef_AddStruct(p_def, "B");
    ymParcelDef_AddStruct(p_def, "C");

    YmItemIndex foo_index = ymParcelDef_AddMethodReq(p_def, A_index, "foo", "p:C"); // w/out params

    YmItemIndex bar_index = ymParcelDef_AddMethodReq(p_def, A_index, "bar", "p:C"); // w/ params
    ymParcelDef_AddParam(p_def, bar_index, "x", "p:C");
    ymParcelDef_AddParam(p_def, bar_index, "y", "p:B");
    ymParcelDef_AddParam(p_def, bar_index, "z", "p:C");

    YmItemIndex other_index = ymParcelDef_AddMethodReq(p_def, A_index, otherName.c_str(), "p:C"); // w/out params

    BIND_AND_IMPORT(ctx, parcel, p_def, "p");
    auto A = ymCtx_Load(ctx, "p:A");
    auto B = ymCtx_Load(ctx, "p:B");
    auto C = ymCtx_Load(ctx, "p:C");
    auto A_foo = ymCtx_Load(ctx, "p:A::foo");
    auto A_bar = ymCtx_Load(ctx, "p:A::bar");
    auto A_other = ymCtx_Load(ctx, otherFullname.c_str());
    ASSERT_TRUE(A);
    ASSERT_TRUE(B);
    ASSERT_TRUE(C);
    ASSERT_TRUE(A_foo);
    ASSERT_TRUE(A_bar);
    ASSERT_TRUE(A_other);
    EXPECT_EQ(ymItem_Parcel(A_foo), parcel);
    EXPECT_EQ(ymItem_Parcel(A_bar), parcel);
    EXPECT_EQ(ymItem_Parcel(A_other), parcel);
    EXPECT_STREQ(ymItem_Fullname(A_foo), "p:A::foo");
    EXPECT_STREQ(ymItem_Fullname(A_bar), "p:A::bar");
    EXPECT_STREQ(ymItem_Fullname(A_other), otherFullname.c_str());
    EXPECT_EQ(ymItem_Kind(A_foo), YmKind_Method);
    EXPECT_EQ(ymItem_Kind(A_bar), YmKind_Method);
    EXPECT_EQ(ymItem_Kind(A_other), YmKind_Method);
    EXPECT_EQ(ymItem_Owner(A_foo), A);
    EXPECT_EQ(ymItem_Owner(A_bar), A);
    EXPECT_EQ(ymItem_Owner(A_other), A);
    EXPECT_EQ(ymItem_ReturnType(A_foo), C);
    EXPECT_EQ(ymItem_ReturnType(A_bar), C);
    EXPECT_EQ(ymItem_ReturnType(A_other), C);
    EXPECT_EQ(ymItem_Params(A_foo), 0);
    EXPECT_EQ(ymItem_Params(A_bar), 3);
    EXPECT_EQ(ymItem_Params(A_other), 0);
    EXPECT_STREQ(ymItem_ParamName(A_bar, 0), "x");
    EXPECT_STREQ(ymItem_ParamName(A_bar, 1), "y");
    EXPECT_STREQ(ymItem_ParamName(A_bar, 2), "z");
    EXPECT_EQ(ymItem_ParamType(A_bar, 0), C);
    EXPECT_EQ(ymItem_ParamType(A_bar, 1), B);
    EXPECT_EQ(ymItem_ParamType(A_bar, 2), C);
}

TEST(ParcelDefs, AddMethodReq_ItemNameConflict) {
    SETUP_ERRCOUNTER;
    SETUP_PARCELDEF(p_def);
    auto A_index = ymParcelDef_AddProtocol(p_def, "A");
    ASSERT_NE(A_index, YM_NO_ITEM_INDEX);
    ASSERT_NE(ymParcelDef_AddMethodReq(p_def, A_index, "m", "p:B"), YM_NO_ITEM_INDEX);
    ASSERT_EQ(ymParcelDef_AddMethodReq(p_def, A_index, "m", "p:B"), YM_NO_ITEM_INDEX);
    EXPECT_EQ(err[YmErrCode_ItemNameConflict], 1);
}

TEST(ParcelDefs, AddMethodReq_ItemNotFound_InvalidOwner) {
    SETUP_ERRCOUNTER;
    SETUP_PARCELDEF(p_def);
    ASSERT_EQ(ymParcelDef_AddMethodReq(p_def, 5'000, "m", "p:B"), YM_NO_ITEM_INDEX);
    EXPECT_EQ(err[YmErrCode_ItemNotFound], 1);
}

TEST(ParcelDefs, AddMethodReq_NonProtocolItem) {
    SETUP_ERRCOUNTER;
    SETUP_PARCELDEF(p_def);
    auto A_index = ymParcelDef_AddStruct(p_def, "A");
    ASSERT_NE(A_index, YM_NO_ITEM_INDEX);
    ASSERT_EQ(ymParcelDef_AddMethodReq(p_def, A_index, "m", "p:B"), YM_NO_ITEM_INDEX);
    EXPECT_EQ(err[YmErrCode_NonProtocolItem], 1);
}

TEST(ParcelDefs, AddMethodReq_IllegalRefSym_InvalidReturnTypeSymbol) {
    SETUP_ERRCOUNTER;
    SETUP_PARCELDEF(p_def);
    auto A_index = ymParcelDef_AddProtocol(p_def, "A");
    ASSERT_NE(A_index, YM_NO_ITEM_INDEX);
    ASSERT_EQ(ymParcelDef_AddMethodReq(p_def, A_index, "m", "/"), YM_NO_ITEM_INDEX);
    EXPECT_EQ(err[YmErrCode_IllegalRefSym], 1);
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
    ASSERT_EQ(ymItem_Params(A), 3);
    EXPECT_STREQ(ymItem_ParamName(A, 0), "x");
    EXPECT_STREQ(ymItem_ParamName(A, 1), "y");
    EXPECT_STREQ(ymItem_ParamName(A, 2), "z");
    EXPECT_EQ(ymItem_ParamType(A, 0), B);
    EXPECT_EQ(ymItem_ParamType(A, 1), C);
    EXPECT_EQ(ymItem_ParamType(A, 2), D);
}

TEST(ParcelDefs, AddParam_ItemNotFound) {
    SETUP_ALL(ctx);
    SETUP_PARCELDEF(p_def);
    EXPECT_EQ(ymParcelDef_AddParam(p_def, 500, "x", "p:B"), YM_NO_PARAM_INDEX);
    EXPECT_EQ(err[YmErrCode_ItemNotFound], 1);
}

TEST(ParcelDefs, AddParam_NonCallableItem) {
    SETUP_ALL(ctx);
    SETUP_PARCELDEF(p_def);
    auto A_index = ymParcelDef_AddStruct(p_def, "A");
    EXPECT_EQ(ymParcelDef_AddParam(p_def, A_index, "x", "p:B"), YM_NO_PARAM_INDEX);
    EXPECT_EQ(err[YmErrCode_NonCallableItem], 1);
}

TEST(ParcelDefs, AddParam_ParamNameConflict) {
    SETUP_ALL(ctx);
    SETUP_PARCELDEF(p_def);
    auto A_index = ymParcelDef_AddFn(p_def, "A", "p:B", ymInertCallBhvrFn, nullptr);
    EXPECT_EQ(ymParcelDef_AddParam(p_def, A_index, "x", "p:B"), 0);
    EXPECT_EQ(ymParcelDef_AddParam(p_def, A_index, "x", "p:C"), YM_NO_PARAM_INDEX);
    EXPECT_EQ(err[YmErrCode_ParamNameConflict], 1);
}

TEST(ParcelDefs, AddParam_IllegalRefSym) {
    SETUP_ALL(ctx);
    SETUP_PARCELDEF(p_def);
    auto A_index = ymParcelDef_AddFn(p_def, "A", "p:B", ymInertCallBhvrFn, nullptr);
    EXPECT_EQ(ymParcelDef_AddParam(p_def, A_index, "x", "/"), YM_NO_PARAM_INDEX);
    EXPECT_EQ(err[YmErrCode_IllegalRefSym], 1);
}

TEST(ParcelDefs, AddParam_MaxParamsLimit) {
    SETUP_ALL(ctx);
    SETUP_PARCELDEF(p_def);
    auto A_index = ymParcelDef_AddFn(p_def, "A", "p:B", ymInertCallBhvrFn, nullptr);
    for (YmParamIndex i = 0; i < YM_MAX_PARAMS; i++) {
        std::string paramName = std::format("a{}", i);
        ASSERT_EQ(ymParcelDef_AddParam(p_def, A_index, paramName.c_str(), "p:B"), i) << "i == " << i;
    }
    EXPECT_EQ(ymParcelDef_AddParam(p_def, A_index, "b", "p:B"), YM_NO_PARAM_INDEX);
    EXPECT_EQ(err[YmErrCode_MaxParamsLimit], 1);
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
    EXPECT_EQ(ymItem_Ref(A, A_ref_B), B);
}

TEST(ParcelDefs, AddRef_ItemNotFound) {
    SETUP_ERRCOUNTER;
    SETUP_PARCELDEF(p_def);
    ASSERT_EQ(ymParcelDef_AddRef(p_def, 5'000, "p:B"), YM_NO_REF);
    EXPECT_EQ(err[YmErrCode_ItemNotFound], 1);
}

TEST(ParcelDefs, AddRef_IllegalRefSym) {
    for (const auto& fullname : illegalFullnames) {
        SETUP_ERRCOUNTER;
        SETUP_PARCELDEF(p_def);
        YmItemIndex p_A_index = ymParcelDef_AddStruct(p_def, "A");
        ASSERT_NE(p_A_index, YM_NO_ITEM_INDEX);
        EXPECT_EQ(ymParcelDef_AddRef(p_def, p_A_index, fullname.c_str()), YM_NO_REF);
        EXPECT_EQ(err[YmErrCode_IllegalRefSym], 1);
    }
}

