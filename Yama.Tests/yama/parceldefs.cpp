

#include <format>

#include <gtest/gtest.h>
#include <taul/strings.h>
#include <taul/encoding.h>
#include <yama/yama.h>
#include <yama++/print.h>

#include "../utils/utils.h"


void testNameLegalityHelper(
    std::function<void(std::string name)> success,
    std::function<void(std::string name)> failure
) {
    // Skip '\0', as that'll just add null terminator.
    for (YmRune c = U'\0' + 1; c < U'\x80'; c++) {
        if ((c >= U'a' && c <= U'z') || (c >= U'A' && c <= U'Z') || c == U'_') {
            success(std::format("{}abc", (YmChar)c));
        }
        else {
            failure(std::format("{}abc", (YmChar)c));
        }
    }
    // Digits are okay, just not at start.
    success("a0123456789");
    // Non-ASCII Unicode is okay.
    success(taul::utf8_s(u8"Σ"));
    success(taul::utf8_s(u8"魂"));
    success(taul::utf8_s(u8"💩"));
    // Empty name is illegal.
    failure("");
}

void testNameLegality(
    // Returns expected fullname.
    std::function<std::string(std::string name, YmParcelDef* parceldef, bool shouldSucceed)> testfn
) {
    testNameLegalityHelper(
        [&testfn](std::string name) {
            SETUP_ALL(ctx);
            SETUP_PARCELDEF(p_def);
            auto fln = testfn(name, p_def, true);
            ymDm_BindParcelDef(dm, "p", p_def);
            auto t = ymCtx_Load(ctx, fln.c_str());
            ASSERT_TRUE(t);
            EXPECT_STREQ(ymType_Fullname(t), fln.c_str());
        },
        [&testfn](std::string name) {
            SETUP_ERRCOUNTER;
            SETUP_PARCELDEF(p_def);
            (void)testfn(name, p_def, false);
            EXPECT_EQ(err[YmErrCode_IllegalName], 1);
        });
}

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
    ASSERT_EQ(ymParcelDef_AddStruct(p_def, "foo"), YM_TRUE);
    ASSERT_EQ(ymParcelDef_AddStruct(p_def, "bar"), YM_TRUE);
    ASSERT_EQ(ymParcelDef_AddStruct(p_def, nonASCIIName.c_str()), YM_TRUE);

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
    EXPECT_EQ(ymType_Params(foo, YM_FALSE), 0);
    EXPECT_EQ(ymType_Params(bar, YM_FALSE), 0);
    EXPECT_EQ(ymType_Params(other, YM_FALSE), 0);
}

TEST(ParcelDefs, AddStruct_NameConflict) {
    SETUP_ERRCOUNTER;
    SETUP_PARCELDEF(p_def);
    ASSERT_EQ(ymParcelDef_AddStruct(p_def, "foo"), YM_TRUE);
    ASSERT_EQ(ymParcelDef_AddStruct(p_def, "foo"), YM_FALSE);
    EXPECT_EQ(err[YmErrCode_NameConflict], 1);
}

TEST(ParcelDefs, AddStruct_IllegalName) {
    testNameLegality(
        [](std::string name, YmParcelDef* parceldef, bool shouldSucceed) -> std::string {
            if (shouldSucceed) {
                EXPECT_EQ(ymParcelDef_AddStruct(parceldef, name.c_str()), YM_TRUE)
                    << "name==\"" << name << "\"";
            }
            else {
                EXPECT_EQ(ymParcelDef_AddStruct(parceldef, name.c_str()), YM_FALSE)
                    << "name==\"" << name << "\"";
            }
            return std::format("p:{}", name);
        });
}

TEST(ParcelDefs, AddProtocol) {
    SETUP_ALL(ctx);
    SETUP_PARCELDEF(p_def);
    std::string nonASCIIName = taul::utf8_s(u8"ab魂💩cd"); // Ensure can handle UTF-8.
    std::string nonASCIIFullname = taul::utf8_s(u8"p:ab魂💩cd"); // Ensure can handle UTF-8.
    ASSERT_EQ(ymParcelDef_AddProtocol(p_def, "foo"), YM_TRUE);
    ASSERT_EQ(ymParcelDef_AddProtocol(p_def, "bar"), YM_TRUE);
    ASSERT_EQ(ymParcelDef_AddProtocol(p_def, nonASCIIName.c_str()), YM_TRUE);

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
    EXPECT_EQ(ymType_Params(foo, YM_FALSE), 0);
    EXPECT_EQ(ymType_Params(bar, YM_FALSE), 0);
    EXPECT_EQ(ymType_Params(other, YM_FALSE), 0);
}

TEST(ParcelDefs, AddProtocol_NameConflict) {
    SETUP_ERRCOUNTER;
    SETUP_PARCELDEF(p_def);
    ASSERT_EQ(ymParcelDef_AddStruct(p_def, "foo"), YM_TRUE);
    ASSERT_EQ(ymParcelDef_AddProtocol(p_def, "foo"), YM_FALSE);
    EXPECT_EQ(err[YmErrCode_NameConflict], 1);
}

TEST(ParcelDefs, AddProtocol_IllegalName) {
    testNameLegality(
        [](std::string name, YmParcelDef* parceldef, bool shouldSucceed) -> std::string {
            if (shouldSucceed) {
                EXPECT_EQ(ymParcelDef_AddProtocol(parceldef, name.c_str()), YM_TRUE);
            }
            else {
                EXPECT_EQ(ymParcelDef_AddProtocol(parceldef, name.c_str()), YM_FALSE);
            }
            return std::format("p:{}", name);
        });
}

TEST(ParcelDefs, AddFn) {
    SETUP_ALL(ctx);
    SETUP_PARCELDEF(p_def);
    std::string nonASCIIName = taul::utf8_s(u8"ab魂💩cd"); // Ensure can handle UTF-8.
    std::string nonASCIIFullname = taul::utf8_s(u8"p:ab魂💩cd"); // Ensure can handle UTF-8.
    ymParcelDef_AddStruct(p_def, "A");
    ymParcelDef_AddStruct(p_def, "B");
    ymParcelDef_AddStruct(p_def, "C");

    ymParcelDef_AddFn(p_def, "foo", "p:A", ymInertCallBhvrFn, nullptr); // w/out params

    ymParcelDef_AddFn(p_def, "bar", "p:A", ymInertCallBhvrFn, nullptr); // w/ params
    ymParcelDef_AddParam(p_def, "bar", "x", "p:A");
    ymParcelDef_AddParam(p_def, "bar", "y", "p:B");
    ymParcelDef_AddParam(p_def, "bar", "z", "p:C");

    ymParcelDef_AddFn(p_def, nonASCIIName.c_str(), "p:A", ymInertCallBhvrFn, nullptr); // w/out params

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
    EXPECT_EQ(ymType_Params(foo, YM_FALSE), 0);
    EXPECT_EQ(ymType_Params(bar, YM_FALSE), 3);
    EXPECT_EQ(ymType_Params(other, YM_FALSE), 0);
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
    ASSERT_EQ(ymParcelDef_AddStruct(p_def, "A"), YM_TRUE);
    ASSERT_EQ(ymParcelDef_AddFn(p_def, "A", "p:B", ymInertCallBhvrFn, nullptr), YM_FALSE);
    EXPECT_EQ(err[YmErrCode_NameConflict], 1);
}

TEST(ParcelDefs, AddFn_IllegalName) {
    testNameLegality(
        [](std::string name, YmParcelDef* parceldef, bool shouldSucceed) -> std::string {
            if (shouldSucceed) {
                EXPECT_EQ(ymParcelDef_AddFn(parceldef, name.c_str(), "yama:None", ymInertCallBhvrFn, nullptr), YM_TRUE);
            }
            else {
                EXPECT_EQ(ymParcelDef_AddFn(parceldef, name.c_str(), "yama:None", ymInertCallBhvrFn, nullptr), YM_FALSE);
            }
            return std::format("p:{}", name);
        });
}

TEST(ParcelDefs, AddFn_IllegalSpecifier_InvalidReturnTypeSymbol) {
    SETUP_ERRCOUNTER;
    SETUP_PARCELDEF(p_def);
    ASSERT_EQ(ymParcelDef_AddFn(p_def, "A", "/", ymInertCallBhvrFn, nullptr), YM_FALSE);
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

    ymParcelDef_AddMethod(p_def, "A", "foo", "p:C", ymInertCallBhvrFn, nullptr); // w/out params

    ymParcelDef_AddMethod(p_def, "A", "bar", "p:C", ymInertCallBhvrFn, nullptr); // w/ params
    ymParcelDef_AddParam(p_def, "A::bar", "x", "p:C");
    ymParcelDef_AddParam(p_def, "A::bar", "y", "p:B");
    ymParcelDef_AddParam(p_def, "A::bar", "z", "p:C");

    ymParcelDef_AddMethod(p_def, "A", nonASCIIName.c_str(), "p:C", ymInertCallBhvrFn, nullptr); // w/out params

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
    EXPECT_EQ(ymType_Params(A_foo, YM_FALSE), 0);
    EXPECT_EQ(ymType_Params(A_bar, YM_FALSE), 3);
    EXPECT_EQ(ymType_Params(A_other, YM_FALSE), 0);
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
    auto A_m_index = ymParcelDef_AddMethod(p_def, "A", "m", A_m_returnType, ymInertCallBhvrFn, nullptr);

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
    ASSERT_EQ(ymParcelDef_AddStruct(p_def, "A"), YM_TRUE);
    ASSERT_EQ(ymParcelDef_AddMethod(p_def, "A", "m", "p:B", ymInertCallBhvrFn, nullptr), YM_TRUE);
    ASSERT_EQ(ymParcelDef_AddMethod(p_def, "A", "m", "p:B", ymInertCallBhvrFn, nullptr), YM_FALSE);
    EXPECT_EQ(err[YmErrCode_NameConflict], 1);
}

TEST(ParcelDefs, AddMethod_NameConflict_TypeParam) {
    SETUP_ERRCOUNTER;
    SETUP_PARCELDEF(p_def);
    ymParcelDef_AddProtocol(p_def, "Any");
    ASSERT_EQ(ymParcelDef_AddStruct(p_def, "A"), YM_TRUE);
    ASSERT_NE(ymParcelDef_AddTypeParam(p_def, "A", "m", "p:Any"), YM_NO_TYPE_PARAM_INDEX);
    ASSERT_EQ(ymParcelDef_AddMethod(p_def, "A", "m", "p:B", ymInertCallBhvrFn, nullptr), YM_FALSE);
    EXPECT_EQ(err[YmErrCode_NameConflict], 1);
}

TEST(ParcelDefs, AddMethod_NameConflict_Self) {
    SETUP_ERRCOUNTER;
    SETUP_PARCELDEF(p_def);
    ASSERT_EQ(ymParcelDef_AddStruct(p_def, "A"), YM_TRUE);
    ASSERT_EQ(ymParcelDef_AddMethod(p_def, "A", "Self", "p:B", ymInertCallBhvrFn, nullptr), YM_FALSE);
    EXPECT_EQ(err[YmErrCode_NameConflict], 1);
}

TEST(ParcelDefs, AddMethod_TypeNotFound_InvalidOwnerName) {
    SETUP_ERRCOUNTER;
    SETUP_PARCELDEF(p_def);
    ASSERT_EQ(ymParcelDef_AddMethod(p_def, "missing", "m", "p:B", ymInertCallBhvrFn, nullptr), YM_FALSE);
    EXPECT_EQ(err[YmErrCode_TypeNotFound], 1);
}

TEST(ParcelDefs, AddMethod_TypeCannotHaveMembers) {
    SETUP_ERRCOUNTER;
    SETUP_PARCELDEF(p_def);
    ASSERT_EQ(ymParcelDef_AddFn(p_def, "A", "p:B", ymInertCallBhvrFn, nullptr), YM_TRUE);
    ASSERT_EQ(ymParcelDef_AddMethod(p_def, "A", "m", "p:B", ymInertCallBhvrFn, nullptr), YM_FALSE);
    EXPECT_EQ(err[YmErrCode_TypeCannotHaveMembers], 1);
}

TEST(ParcelDefs, AddMethod_ProtocolType) {
    SETUP_ERRCOUNTER;
    SETUP_PARCELDEF(p_def);
    ASSERT_EQ(ymParcelDef_AddProtocol(p_def, "A"), YM_TRUE);
    ASSERT_EQ(ymParcelDef_AddMethod(p_def, "A", "m", "p:B", ymInertCallBhvrFn, nullptr), YM_FALSE);
    EXPECT_EQ(err[YmErrCode_ProtocolType], 1);
}

TEST(ParcelDefs, AddMethod_IllegalName) {
    testNameLegality(
        [](std::string name, YmParcelDef* parceldef, bool shouldSucceed) -> std::string {
            if (shouldSucceed) {
                EXPECT_EQ(ymParcelDef_AddStruct(parceldef, "A"), YM_TRUE);
                EXPECT_EQ(ymParcelDef_AddMethod(parceldef, "A", name.c_str(), "yama:None", ymInertCallBhvrFn, nullptr), YM_TRUE);
            }
            else {
                EXPECT_EQ(ymParcelDef_AddStruct(parceldef, "A"), YM_TRUE);
                EXPECT_EQ(ymParcelDef_AddMethod(parceldef, "A", name.c_str(), "yama:None", ymInertCallBhvrFn, nullptr), YM_FALSE);
            }
            return std::format("p:A::{}", name);
        });
}

TEST(ParcelDefs, AddMethod_IllegalSpecifier_InvalidReturnTypeSymbol) {
    SETUP_ERRCOUNTER;
    SETUP_PARCELDEF(p_def);
    ASSERT_EQ(ymParcelDef_AddStruct(p_def, "A"), YM_TRUE);
    ASSERT_EQ(ymParcelDef_AddMethod(p_def, "A", "m", "/", ymInertCallBhvrFn, nullptr), YM_FALSE);
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

    ymParcelDef_AddMethodReq(p_def, "A", "foo", "p:C"); // w/out params

    ymParcelDef_AddMethodReq(p_def, "A", "bar", "p:C"); // w/ params
    ymParcelDef_AddParam(p_def, "A::bar", "x", "p:C");
    ymParcelDef_AddParam(p_def, "A::bar", "y", "p:B");
    ymParcelDef_AddParam(p_def, "A::bar", "z", "p:C");

    ymParcelDef_AddMethodReq(p_def, "A", nonASCIIName.c_str(), "p:C"); // w/out params

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
    EXPECT_EQ(ymType_Params(A_foo, YM_FALSE), 0);
    EXPECT_EQ(ymType_Params(A_bar, YM_FALSE), 3);
    EXPECT_EQ(ymType_Params(A_other, YM_FALSE), 0);
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
    auto P_m_index = ymParcelDef_AddMethodReq(p_def, "P", "m", P_m_returnType);

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
    ASSERT_EQ(ymParcelDef_AddProtocol(p_def, "A"), YM_TRUE);
    ASSERT_EQ(ymParcelDef_AddMethodReq(p_def, "A", "m", "p:B"), YM_TRUE);
    ASSERT_EQ(ymParcelDef_AddMethodReq(p_def, "A", "m", "p:B"), YM_FALSE);
    EXPECT_EQ(err[YmErrCode_NameConflict], 1);
}

TEST(ParcelDefs, AddMethodReq_NameConflict_TypeParam) {
    SETUP_ERRCOUNTER;
    SETUP_PARCELDEF(p_def);
    ymParcelDef_AddProtocol(p_def, "Any");
    ASSERT_EQ(ymParcelDef_AddProtocol(p_def, "A"), YM_TRUE);
    ASSERT_NE(ymParcelDef_AddTypeParam(p_def, "A", "m", "p:Any"), YM_NO_TYPE_PARAM_INDEX);
    ASSERT_EQ(ymParcelDef_AddMethodReq(p_def, "A", "m", "p:B"), YM_FALSE);
    EXPECT_EQ(err[YmErrCode_NameConflict], 1);
}

TEST(ParcelDefs, AddMethodReq_NameConflict_Self) {
    SETUP_ERRCOUNTER;
    SETUP_PARCELDEF(p_def);
    auto A_index = ymParcelDef_AddProtocol(p_def, "A");
    ASSERT_EQ(A_index, YM_TRUE);
    ASSERT_EQ(ymParcelDef_AddMethodReq(p_def, "A", "Self", "p:B"), YM_FALSE);
    EXPECT_EQ(err[YmErrCode_NameConflict], 1);
}

TEST(ParcelDefs, AddMethodReq_TypeNotFound_InvalidOwnerName) {
    SETUP_ERRCOUNTER;
    SETUP_PARCELDEF(p_def);
    ASSERT_EQ(ymParcelDef_AddMethodReq(p_def, "missing", "m", "p:B"), YM_FALSE);
    EXPECT_EQ(err[YmErrCode_TypeNotFound], 1);
}

TEST(ParcelDefs, AddMethodReq_NonProtocolType) {
    SETUP_ERRCOUNTER;
    SETUP_PARCELDEF(p_def);
    auto A_index = ymParcelDef_AddStruct(p_def, "A");
    ASSERT_EQ(A_index, YM_TRUE);
    ASSERT_EQ(ymParcelDef_AddMethodReq(p_def, "A", "m", "p:B"), YM_FALSE);
    EXPECT_EQ(err[YmErrCode_NonProtocolType], 1);
}

TEST(ParcelDefs, AddMethodReq_IllegalName) {
    testNameLegality(
        [](std::string name, YmParcelDef* parceldef, bool shouldSucceed) -> std::string {
            if (shouldSucceed) {
                EXPECT_EQ(ymParcelDef_AddProtocol(parceldef, "A"), YM_TRUE);
                EXPECT_EQ(ymParcelDef_AddMethodReq(parceldef, "A", name.c_str(), "yama:None"), YM_TRUE);
            }
            else {
                EXPECT_EQ(ymParcelDef_AddProtocol(parceldef, "A"), YM_TRUE);
                EXPECT_EQ(ymParcelDef_AddMethodReq(parceldef, "A", name.c_str(), "yama:None"), YM_FALSE);
            }
            return std::format("p:A::{}", name);
        });
}

TEST(ParcelDefs, AddMethodReq_IllegalSpecifier_InvalidReturnTypeSymbol) {
    SETUP_ERRCOUNTER;
    SETUP_PARCELDEF(p_def);
    auto A_index = ymParcelDef_AddProtocol(p_def, "A");
    ASSERT_EQ(A_index, YM_TRUE);
    ASSERT_EQ(ymParcelDef_AddMethodReq(p_def, "A", "m", "/"), YM_FALSE);
    EXPECT_EQ(err[YmErrCode_IllegalSpecifier], 1);
}

TEST(ParcelDefs, AddReadOnlyStoredProperty) {
    SETUP_ALL(ctx);
    SETUP_PARCELDEF(p_def);
    std::string nonASCIIName = taul::utf8_s(u8"ab魂💩cd"); // Ensure can handle UTF-8.
    std::string nonASCIIFullname = taul::utf8_s(u8"p:A::ab魂💩cd"); // Ensure can handle UTF-8.
    ymParcelDef_AddStruct(p_def, "A");
    ymParcelDef_AddStruct(p_def, "B");
    ymParcelDef_AddStruct(p_def, "C");

    ymParcelDef_AddReadOnlyStoredProperty(p_def, "A", "foo", "p:B");
    ymParcelDef_AddReadOnlyStoredProperty(p_def, "A", nonASCIIName.c_str(), "p:C");

    BIND_AND_IMPORT(ctx, parcel, p_def, "p");
    auto A = ymCtx_Load(ctx, "p:A");
    auto B = ymCtx_Load(ctx, "p:B");
    auto C = ymCtx_Load(ctx, "p:C");
    auto A_foo = ymCtx_Load(ctx, "p:A::foo");
    auto A_other = ymCtx_Load(ctx, nonASCIIFullname.c_str());
    ASSERT_TRUE(A);
    ASSERT_TRUE(B);
    ASSERT_TRUE(C);
    ASSERT_TRUE(A_foo);
    ASSERT_TRUE(A_other);
    EXPECT_EQ(ymType_Parcel(A_foo), parcel);
    EXPECT_EQ(ymType_Parcel(A_other), parcel);
    EXPECT_STREQ(ymType_Fullname(A_foo), "p:A::foo");
    EXPECT_STREQ(ymType_Fullname(A_other), nonASCIIFullname.c_str());
    EXPECT_EQ(ymType_Kind(A_foo), YmKind_Property);
    EXPECT_EQ(ymType_Kind(A_other), YmKind_Property);
    EXPECT_EQ(ymType_Owner(A_foo), A);
    EXPECT_EQ(ymType_Owner(A_other), A);
    EXPECT_EQ(ymType_Type(A_foo), B);
    EXPECT_EQ(ymType_Type(A_other), C);
    EXPECT_EQ(ymType_ReturnType(A_foo), B);
    EXPECT_EQ(ymType_ReturnType(A_other), C);
    EXPECT_EQ(ymType_Params(A_foo, YM_FALSE), 1);
    EXPECT_EQ(ymType_Params(A_other, YM_FALSE), 1);
    EXPECT_EQ(ymType_ParamType(A_foo, 0), A);
    EXPECT_EQ(ymType_ParamType(A_other, 0), A);
}

TEST(ParcelDefs, AddReadOnlyStoredProperty_Normalizes_Type) {
    SETUP_ALL(ctx);
    SETUP_PARCELDEF(p_def);

    ymParcelDef_AddStruct(p_def, "A");
    ymParcelDef_AddReadOnlyStoredProperty(p_def, "A", "p", "  p  : \n\n A  ");

    ymDm_BindParcelDef(dm, "p", p_def);
    auto A = ymCtx_Load(ctx, "p:A");
    auto A_p = ymCtx_Load(ctx, "p:A::p");
    ASSERT_TRUE(A);
    ASSERT_TRUE(A_p);

    ASSERT_EQ(ymType_Type(A_p), A);
    EXPECT_STREQ(ymType_Fullname(ymType_Type(A_p)), "p:A");
}

TEST(ParcelDefs, AddReadOnlyStoredProperty_NameConflict_Type) {
    SETUP_ERRCOUNTER;
    SETUP_PARCELDEF(p_def);
    ASSERT_EQ(ymParcelDef_AddStruct(p_def, "A"), YM_TRUE);
    ASSERT_EQ(ymParcelDef_AddReadOnlyStoredProperty(p_def, "A", "p", "p:B"), YM_TRUE);
    ASSERT_EQ(ymParcelDef_AddReadOnlyStoredProperty(p_def, "A", "p", "p:B"), YM_FALSE);
    EXPECT_EQ(err[YmErrCode_NameConflict], 1);
}

TEST(ParcelDefs, AddReadOnlyStoredProperty_NameConflict_TypeParam) {
    SETUP_ERRCOUNTER;
    SETUP_PARCELDEF(p_def);
    ymParcelDef_AddProtocol(p_def, "Any");
    ASSERT_EQ(ymParcelDef_AddStruct(p_def, "A"), YM_TRUE);
    ASSERT_NE(ymParcelDef_AddTypeParam(p_def, "A", "p", "p:Any"), YM_NO_TYPE_PARAM_INDEX);
    ASSERT_EQ(ymParcelDef_AddReadOnlyStoredProperty(p_def, "A", "p", "p:B"), YM_FALSE);
    EXPECT_EQ(err[YmErrCode_NameConflict], 1);
}

TEST(ParcelDefs, AddReadOnlyStoredProperty_NameConflict_Self) {
    SETUP_ERRCOUNTER;
    SETUP_PARCELDEF(p_def);
    ASSERT_EQ(ymParcelDef_AddStruct(p_def, "A"), YM_TRUE);
    ASSERT_EQ(ymParcelDef_AddReadOnlyStoredProperty(p_def, "A", "Self", "p:B"), YM_FALSE);
    EXPECT_EQ(err[YmErrCode_NameConflict], 1);
}

TEST(ParcelDefs, AddReadOnlyStoredProperty_TypeNotFound_InvalidOwnerName) {
    SETUP_ERRCOUNTER;
    SETUP_PARCELDEF(p_def);
    ASSERT_EQ(ymParcelDef_AddReadOnlyStoredProperty(p_def, "missing", "p", "p:B"), YM_FALSE);
    EXPECT_EQ(err[YmErrCode_TypeNotFound], 1);
}

TEST(ParcelDefs, AddReadOnlyStoredProperty_TypeCannotHaveMembers) {
    SETUP_ERRCOUNTER;
    SETUP_PARCELDEF(p_def);
    ASSERT_EQ(ymParcelDef_AddFn(p_def, "A", "p:B", ymInertCallBhvrFn, nullptr), YM_TRUE);
    ASSERT_EQ(ymParcelDef_AddReadOnlyStoredProperty(p_def, "A", "p", "p:B"), YM_FALSE);
    EXPECT_EQ(err[YmErrCode_TypeCannotHaveMembers], 1);
}

TEST(ParcelDefs, AddReadOnlyStoredProperty_ProtocolType) {
    SETUP_ERRCOUNTER;
    SETUP_PARCELDEF(p_def);
    ASSERT_EQ(ymParcelDef_AddProtocol(p_def, "A"), YM_TRUE);
    ASSERT_EQ(ymParcelDef_AddReadOnlyStoredProperty(p_def, "A", "p", "p:B"), YM_FALSE);
    EXPECT_EQ(err[YmErrCode_ProtocolType], 1);
}

TEST(ParcelDefs, AddReadOnlyStoredProperty_IllegalName) {
    testNameLegality(
        [](std::string name, YmParcelDef* parceldef, bool shouldSucceed) -> std::string {
            if (shouldSucceed) {
                EXPECT_EQ(ymParcelDef_AddStruct(parceldef, "A"), YM_TRUE);
                EXPECT_EQ(ymParcelDef_AddReadOnlyStoredProperty(parceldef, "A", name.c_str(), "yama:None"), YM_TRUE);
            }
            else {
                EXPECT_EQ(ymParcelDef_AddStruct(parceldef, "A"), YM_TRUE);
                EXPECT_EQ(ymParcelDef_AddReadOnlyStoredProperty(parceldef, "A", name.c_str(), "yama:None"), YM_FALSE);
            }
            return std::format("p:A::{}", name);
        });
}

TEST(ParcelDefs, AddReadOnlyStoredProperty_IllegalSpecifier_InvalidTypeSymbol) {
    SETUP_ERRCOUNTER;
    SETUP_PARCELDEF(p_def);
    ASSERT_EQ(ymParcelDef_AddStruct(p_def, "A"), YM_TRUE);
    ASSERT_EQ(ymParcelDef_AddReadOnlyStoredProperty(p_def, "A", "p", "/"), YM_FALSE);
    EXPECT_EQ(err[YmErrCode_IllegalSpecifier], 1);
}

TEST(ParcelDefs, AddStoredProperty) {
    SETUP_ALL(ctx);
    SETUP_PARCELDEF(p_def);
    std::string nonASCIIName = taul::utf8_s(u8"ab魂💩cd"); // Ensure can handle UTF-8.
    std::string nonASCIIFullname = taul::utf8_s(u8"p:A::ab魂💩cd"); // Ensure can handle UTF-8.
    auto nonASCIIFullname_assigner = nonASCIIFullname + "$assigner";
    ymParcelDef_AddStruct(p_def, "A");
    ymParcelDef_AddStruct(p_def, "B");
    auto C_ind = ymParcelDef_AddStruct(p_def, "C");

    ymParcelDef_AddStoredProperty(p_def, "A", "foo", "p:B");
    ymParcelDef_AddStoredProperty(p_def, "A", nonASCIIName.c_str(), "p:C");

    BIND_AND_IMPORT(ctx, parcel, p_def, "p");
    auto A = ymCtx_Load(ctx, "p:A");
    auto B = ymCtx_Load(ctx, "p:B");
    auto C = ymCtx_Load(ctx, "p:C");
    auto A_foo = ymCtx_Load(ctx, "p:A::foo");
    auto A_foo_assigner = ymCtx_Load(ctx, "p:A::foo$assigner");
    auto A_other = ymCtx_Load(ctx, nonASCIIFullname.c_str());
    auto A_other_assigner = ymCtx_Load(ctx, nonASCIIFullname_assigner.c_str());
    ASSERT_TRUE(A);
    ASSERT_TRUE(B);
    ASSERT_TRUE(C);
    ASSERT_TRUE(A_foo);
    ASSERT_TRUE(A_foo_assigner);
    ASSERT_TRUE(A_other);
    ASSERT_TRUE(A_other_assigner);
    EXPECT_EQ(ymType_Parcel(A_foo), parcel);
    EXPECT_EQ(ymType_Parcel(A_foo_assigner), parcel);
    EXPECT_EQ(ymType_Parcel(A_other), parcel);
    EXPECT_EQ(ymType_Parcel(A_other_assigner), parcel);
    EXPECT_STREQ(ymType_Fullname(A_foo), "p:A::foo");
    EXPECT_STREQ(ymType_Fullname(A_foo_assigner), "p:A::foo$assigner");
    EXPECT_STREQ(ymType_Fullname(A_other), nonASCIIFullname.c_str());
    EXPECT_STREQ(ymType_Fullname(A_other_assigner), nonASCIIFullname_assigner.c_str());
    EXPECT_EQ(ymType_Kind(A_foo), YmKind_Property);
    EXPECT_EQ(ymType_Kind(A_foo_assigner), YmKind_PropertyAssigner);
    EXPECT_EQ(ymType_Kind(A_other), YmKind_Property);
    EXPECT_EQ(ymType_Kind(A_other_assigner), YmKind_PropertyAssigner);
    EXPECT_EQ(ymType_Owner(A_foo), A);
    EXPECT_EQ(ymType_Owner(A_foo_assigner), A);
    EXPECT_EQ(ymType_Owner(A_other), A);
    EXPECT_EQ(ymType_Owner(A_other_assigner), A);
    EXPECT_EQ(ymType_Type(A_foo), B);
    EXPECT_EQ(ymType_Type(A_foo_assigner), nullptr);
    EXPECT_EQ(ymType_Type(A_other), C);
    EXPECT_EQ(ymType_Type(A_other_assigner), nullptr);
    EXPECT_EQ(ymType_ReturnType(A_foo), B);
    EXPECT_EQ(ymType_ReturnType(A_foo_assigner), ymCtx_LdNone(ctx));
    EXPECT_EQ(ymType_ReturnType(A_other), C);
    EXPECT_EQ(ymType_ReturnType(A_other_assigner), ymCtx_LdNone(ctx));
    EXPECT_EQ(ymType_Params(A_foo, YM_FALSE), 1);
    EXPECT_EQ(ymType_Params(A_foo_assigner, YM_FALSE), 2);
    EXPECT_EQ(ymType_Params(A_other, YM_FALSE), 1);
    EXPECT_EQ(ymType_Params(A_other_assigner, YM_FALSE), 2);
    EXPECT_EQ(ymType_ParamType(A_foo, 0), A);
    EXPECT_EQ(ymType_ParamType(A_foo_assigner, 0), A);
    EXPECT_EQ(ymType_ParamType(A_foo_assigner, 1), B);
    EXPECT_EQ(ymType_ParamType(A_other, 0), A);
    EXPECT_EQ(ymType_ParamType(A_other_assigner, 0), A);
    EXPECT_EQ(ymType_ParamType(A_other_assigner, 1), C);
}

TEST(ParcelDefs, AddStoredProperty_Normalizes_Type) {
    SETUP_ALL(ctx);
    SETUP_PARCELDEF(p_def);

    ymParcelDef_AddStruct(p_def, "A");
    ymParcelDef_AddStoredProperty(p_def, "A", "p", "  p  : \n\n A  ");

    ymDm_BindParcelDef(dm, "p", p_def);
    auto A = ymCtx_Load(ctx, "p:A");
    auto A_p = ymCtx_Load(ctx, "p:A::p");
    ASSERT_TRUE(A);
    ASSERT_TRUE(A_p);

    ASSERT_EQ(ymType_Type(A_p), A);
    EXPECT_STREQ(ymType_Fullname(ymType_Type(A_p)), "p:A");
}

TEST(ParcelDefs, AddStoredProperty_NameConflict_Type) {
    SETUP_ERRCOUNTER;
    SETUP_PARCELDEF(p_def);
    ASSERT_EQ(ymParcelDef_AddStruct(p_def, "A"), YM_TRUE);
    ASSERT_EQ(ymParcelDef_AddStoredProperty(p_def, "A", "p", "p:B"), YM_TRUE);
    ASSERT_EQ(ymParcelDef_AddStoredProperty(p_def, "A", "p", "p:B"), YM_FALSE);
    EXPECT_EQ(err[YmErrCode_NameConflict], 1);
}

TEST(ParcelDefs, AddStoredProperty_NameConflict_TypeParam) {
    SETUP_ERRCOUNTER;
    SETUP_PARCELDEF(p_def);
    ymParcelDef_AddProtocol(p_def, "Any");
    ASSERT_EQ(ymParcelDef_AddStruct(p_def, "A"), YM_TRUE);
    ASSERT_NE(ymParcelDef_AddTypeParam(p_def, "A", "p", "p:Any"), YM_NO_TYPE_PARAM_INDEX);
    ASSERT_EQ(ymParcelDef_AddStoredProperty(p_def, "A", "p", "p:B"), YM_FALSE);
    EXPECT_EQ(err[YmErrCode_NameConflict], 1);
}

TEST(ParcelDefs, AddStoredProperty_NameConflict_Self) {
    SETUP_ERRCOUNTER;
    SETUP_PARCELDEF(p_def);
    ASSERT_EQ(ymParcelDef_AddStruct(p_def, "A"), YM_TRUE);
    ASSERT_EQ(ymParcelDef_AddStoredProperty(p_def, "A", "Self", "p:B"), YM_FALSE);
    EXPECT_EQ(err[YmErrCode_NameConflict], 1);
}

TEST(ParcelDefs, AddStoredProperty_TypeNotFound_InvalidOwnerName) {
    SETUP_ERRCOUNTER;
    SETUP_PARCELDEF(p_def);
    ASSERT_EQ(ymParcelDef_AddStoredProperty(p_def, "missing", "p", "p:B"), YM_FALSE);
    EXPECT_EQ(err[YmErrCode_TypeNotFound], 1);
}

TEST(ParcelDefs, AddStoredProperty_TypeCannotHaveMembers) {
    SETUP_ERRCOUNTER;
    SETUP_PARCELDEF(p_def);
    ASSERT_EQ(ymParcelDef_AddFn(p_def, "A", "p:B", ymInertCallBhvrFn, nullptr), YM_TRUE);
    ASSERT_EQ(ymParcelDef_AddStoredProperty(p_def, "A", "p", "p:B"), YM_FALSE);
    EXPECT_EQ(err[YmErrCode_TypeCannotHaveMembers], 1);
}

TEST(ParcelDefs, AddStoredProperty_ProtocolType) {
    SETUP_ERRCOUNTER;
    SETUP_PARCELDEF(p_def);
    ASSERT_EQ(ymParcelDef_AddProtocol(p_def, "A"), YM_TRUE);
    ASSERT_EQ(ymParcelDef_AddStoredProperty(p_def, "A", "p", "p:B"), YM_FALSE);
    EXPECT_EQ(err[YmErrCode_ProtocolType], 1);
}

TEST(ParcelDefs, AddStoredProperty_IllegalName) {
    testNameLegality(
        [](std::string name, YmParcelDef* parceldef, bool shouldSucceed) -> std::string {
            if (shouldSucceed) {
                EXPECT_EQ(ymParcelDef_AddStruct(parceldef, "A"), YM_TRUE);
                EXPECT_EQ(ymParcelDef_AddStoredProperty(parceldef, "A", name.c_str(), "yama:None"), YM_TRUE);
            }
            else {
                EXPECT_EQ(ymParcelDef_AddStruct(parceldef, "A"), YM_TRUE);
                EXPECT_EQ(ymParcelDef_AddStoredProperty(parceldef, "A", name.c_str(), "yama:None"), YM_FALSE);
            }
            return std::format("p:A::{}", name);
        });
}

TEST(ParcelDefs, AddStoredProperty_IllegalSpecifier_InvalidTypeSymbol) {
    SETUP_ERRCOUNTER;
    SETUP_PARCELDEF(p_def);
    ASSERT_EQ(ymParcelDef_AddStruct(p_def, "A"), YM_TRUE);
    ASSERT_EQ(ymParcelDef_AddStoredProperty(p_def, "A", "p", "/"), YM_FALSE);
    EXPECT_EQ(err[YmErrCode_IllegalSpecifier], 1);
}

TEST(ParcelDefs, AddReadOnlyComputedProperty) {
    SETUP_ALL(ctx);
    SETUP_PARCELDEF(p_def);
    std::string nonASCIIName = taul::utf8_s(u8"ab魂💩cd"); // Ensure can handle UTF-8.
    std::string nonASCIIFullname = taul::utf8_s(u8"p:A::ab魂💩cd"); // Ensure can handle UTF-8.
    ymParcelDef_AddStruct(p_def, "A");
    ymParcelDef_AddStruct(p_def, "B");
    ymParcelDef_AddStruct(p_def, "C");

    ymParcelDef_AddReadOnlyComputedProperty(p_def, "A", "foo", "p:B", ymInertCallBhvrFn, nullptr);
    ymParcelDef_AddReadOnlyComputedProperty(p_def, "A", nonASCIIName.c_str(), "p:C", ymInertCallBhvrFn, nullptr);

    BIND_AND_IMPORT(ctx, parcel, p_def, "p");
    auto A = ymCtx_Load(ctx, "p:A");
    auto B = ymCtx_Load(ctx, "p:B");
    auto C = ymCtx_Load(ctx, "p:C");
    auto A_foo = ymCtx_Load(ctx, "p:A::foo");
    auto A_other = ymCtx_Load(ctx, nonASCIIFullname.c_str());
    ASSERT_TRUE(A);
    ASSERT_TRUE(B);
    ASSERT_TRUE(C);
    ASSERT_TRUE(A_foo);
    ASSERT_TRUE(A_other);
    EXPECT_EQ(ymType_Parcel(A_foo), parcel);
    EXPECT_EQ(ymType_Parcel(A_other), parcel);
    EXPECT_STREQ(ymType_Fullname(A_foo), "p:A::foo");
    EXPECT_STREQ(ymType_Fullname(A_other), nonASCIIFullname.c_str());
    EXPECT_EQ(ymType_Kind(A_foo), YmKind_Property);
    EXPECT_EQ(ymType_Kind(A_other), YmKind_Property);
    EXPECT_EQ(ymType_Owner(A_foo), A);
    EXPECT_EQ(ymType_Owner(A_other), A);
    EXPECT_EQ(ymType_Type(A_foo), B);
    EXPECT_EQ(ymType_Type(A_other), C);
    EXPECT_EQ(ymType_ReturnType(A_foo), B);
    EXPECT_EQ(ymType_ReturnType(A_other), C);
    EXPECT_EQ(ymType_Params(A_foo, YM_FALSE), 1);
    EXPECT_EQ(ymType_Params(A_other, YM_FALSE), 1);
    EXPECT_EQ(ymType_ParamType(A_foo, 0), A);
    EXPECT_EQ(ymType_ParamType(A_other, 0), A);
}

TEST(ParcelDefs, AddReadOnlyComputedProperty_Normalizes_Type) {
    SETUP_ALL(ctx);
    SETUP_PARCELDEF(p_def);

    ymParcelDef_AddStruct(p_def, "A");
    ymParcelDef_AddReadOnlyComputedProperty(p_def, "A", "p", "  p  : \n\n A  ", ymInertCallBhvrFn, nullptr);

    ymDm_BindParcelDef(dm, "p", p_def);
    auto A = ymCtx_Load(ctx, "p:A");
    auto A_p = ymCtx_Load(ctx, "p:A::p");
    ASSERT_TRUE(A);
    ASSERT_TRUE(A_p);

    ASSERT_EQ(ymType_Type(A_p), A);
    EXPECT_STREQ(ymType_Fullname(ymType_Type(A_p)), "p:A");
}

TEST(ParcelDefs, AddReadOnlyComputedProperty_NameConflict_Type) {
    SETUP_ERRCOUNTER;
    SETUP_PARCELDEF(p_def);
    ASSERT_EQ(ymParcelDef_AddStruct(p_def, "A"), YM_TRUE);
    ASSERT_EQ(ymParcelDef_AddReadOnlyComputedProperty(p_def, "A", "p", "p:B", ymInertCallBhvrFn, nullptr), YM_TRUE);
    ASSERT_EQ(ymParcelDef_AddReadOnlyComputedProperty(p_def, "A", "p", "p:B", ymInertCallBhvrFn, nullptr), YM_FALSE);
    EXPECT_EQ(err[YmErrCode_NameConflict], 1);
}

TEST(ParcelDefs, AddReadOnlyComputedProperty_NameConflict_TypeParam) {
    SETUP_ERRCOUNTER;
    SETUP_PARCELDEF(p_def);
    ymParcelDef_AddProtocol(p_def, "Any");
    ASSERT_EQ(ymParcelDef_AddStruct(p_def, "A"), YM_TRUE);
    ASSERT_NE(ymParcelDef_AddTypeParam(p_def, "A", "p", "p:Any"), YM_NO_TYPE_PARAM_INDEX);
    ASSERT_EQ(ymParcelDef_AddReadOnlyComputedProperty(p_def, "A", "p", "p:B", ymInertCallBhvrFn, nullptr), YM_FALSE);
    EXPECT_EQ(err[YmErrCode_NameConflict], 1);
}

TEST(ParcelDefs, AddReadOnlyComputedProperty_NameConflict_Self) {
    SETUP_ERRCOUNTER;
    SETUP_PARCELDEF(p_def);
    ASSERT_EQ(ymParcelDef_AddStruct(p_def, "A"), YM_TRUE);
    ASSERT_EQ(ymParcelDef_AddReadOnlyComputedProperty(p_def, "A", "Self", "p:B", ymInertCallBhvrFn, nullptr), YM_FALSE);
    EXPECT_EQ(err[YmErrCode_NameConflict], 1);
}

TEST(ParcelDefs, AddReadOnlyComputedProperty_TypeNotFound_InvalidOwnerName) {
    SETUP_ERRCOUNTER;
    SETUP_PARCELDEF(p_def);
    ASSERT_EQ(ymParcelDef_AddReadOnlyComputedProperty(p_def, "missing", "p", "p:B", ymInertCallBhvrFn, nullptr), YM_FALSE);
    EXPECT_EQ(err[YmErrCode_TypeNotFound], 1);
}

TEST(ParcelDefs, AddReadOnlyComputedProperty_TypeCannotHaveMembers) {
    SETUP_ERRCOUNTER;
    SETUP_PARCELDEF(p_def);
    ASSERT_EQ(ymParcelDef_AddFn(p_def, "A", "p:B", ymInertCallBhvrFn, nullptr), YM_TRUE);
    ASSERT_EQ(ymParcelDef_AddReadOnlyComputedProperty(p_def, "A", "p", "p:B", ymInertCallBhvrFn, nullptr), YM_FALSE);
    EXPECT_EQ(err[YmErrCode_TypeCannotHaveMembers], 1);
}

TEST(ParcelDefs, AddReadOnlyComputedProperty_ProtocolType) {
    SETUP_ERRCOUNTER;
    SETUP_PARCELDEF(p_def);
    ASSERT_EQ(ymParcelDef_AddProtocol(p_def, "A"), YM_TRUE);
    ASSERT_EQ(ymParcelDef_AddReadOnlyComputedProperty(p_def, "A", "p", "p:B", ymInertCallBhvrFn, nullptr), YM_FALSE);
    EXPECT_EQ(err[YmErrCode_ProtocolType], 1);
}

TEST(ParcelDefs, AddReadOnlyComputedProperty_IllegalName) {
    testNameLegality(
        [](std::string name, YmParcelDef* parceldef, bool shouldSucceed) -> std::string {
            if (shouldSucceed) {
                EXPECT_EQ(ymParcelDef_AddStruct(parceldef, "A"), YM_TRUE);
                EXPECT_EQ(ymParcelDef_AddReadOnlyComputedProperty(parceldef, "A", name.c_str(), "yama:None", ymInertCallBhvrFn, nullptr), YM_TRUE);
            }
            else {
                EXPECT_EQ(ymParcelDef_AddStruct(parceldef, "A"), YM_TRUE);
                EXPECT_EQ(ymParcelDef_AddReadOnlyComputedProperty(parceldef, "A", name.c_str(), "yama:None", ymInertCallBhvrFn, nullptr), YM_FALSE);
            }
            return std::format("p:A::{}", name);
        });
}

TEST(ParcelDefs, AddReadOnlyComputedProperty_IllegalSpecifier_InvalidTypeSymbol) {
    SETUP_ERRCOUNTER;
    SETUP_PARCELDEF(p_def);
    ASSERT_EQ(ymParcelDef_AddStruct(p_def, "A"), YM_TRUE);
    ASSERT_EQ(ymParcelDef_AddReadOnlyComputedProperty(p_def, "A", "p", "/", ymInertCallBhvrFn, nullptr), YM_FALSE);
    EXPECT_EQ(err[YmErrCode_IllegalSpecifier], 1);
}

TEST(ParcelDefs, AddComputedProperty) {
    SETUP_ALL(ctx);
    SETUP_PARCELDEF(p_def);
    std::string nonASCIIName = taul::utf8_s(u8"ab魂💩cd"); // Ensure can handle UTF-8.
    std::string nonASCIIFullname = taul::utf8_s(u8"p:A::ab魂💩cd"); // Ensure can handle UTF-8.
    auto nonASCIIFullname_assigner = nonASCIIFullname + "$assigner";
    ymParcelDef_AddStruct(p_def, "A");
    ymParcelDef_AddStruct(p_def, "B");
    auto C_ind = ymParcelDef_AddStruct(p_def, "C");

    ymParcelDef_AddComputedProperty(p_def, "A", "foo", "p:B",
        ymInertCallBhvrFn, nullptr, ymInertCallBhvrFn, nullptr);
    ymParcelDef_AddComputedProperty(p_def, "A", nonASCIIName.c_str(), "p:C",
        ymInertCallBhvrFn, nullptr, ymInertCallBhvrFn, nullptr);

    BIND_AND_IMPORT(ctx, parcel, p_def, "p");
    auto A = ymCtx_Load(ctx, "p:A");
    auto B = ymCtx_Load(ctx, "p:B");
    auto C = ymCtx_Load(ctx, "p:C");
    auto A_foo = ymCtx_Load(ctx, "p:A::foo");
    auto A_foo_assigner = ymCtx_Load(ctx, "p:A::foo$assigner");
    auto A_other = ymCtx_Load(ctx, nonASCIIFullname.c_str());
    auto A_other_assigner = ymCtx_Load(ctx, nonASCIIFullname_assigner.c_str());
    ASSERT_TRUE(A);
    ASSERT_TRUE(B);
    ASSERT_TRUE(C);
    ASSERT_TRUE(A_foo);
    ASSERT_TRUE(A_foo_assigner);
    ASSERT_TRUE(A_other);
    ASSERT_TRUE(A_other_assigner);
    EXPECT_EQ(ymType_Parcel(A_foo), parcel);
    EXPECT_EQ(ymType_Parcel(A_foo_assigner), parcel);
    EXPECT_EQ(ymType_Parcel(A_other), parcel);
    EXPECT_EQ(ymType_Parcel(A_other_assigner), parcel);
    EXPECT_STREQ(ymType_Fullname(A_foo), "p:A::foo");
    EXPECT_STREQ(ymType_Fullname(A_foo_assigner), "p:A::foo$assigner");
    EXPECT_STREQ(ymType_Fullname(A_other), nonASCIIFullname.c_str());
    EXPECT_STREQ(ymType_Fullname(A_other_assigner), nonASCIIFullname_assigner.c_str());
    EXPECT_EQ(ymType_Kind(A_foo), YmKind_Property);
    EXPECT_EQ(ymType_Kind(A_foo_assigner), YmKind_PropertyAssigner);
    EXPECT_EQ(ymType_Kind(A_other), YmKind_Property);
    EXPECT_EQ(ymType_Kind(A_other_assigner), YmKind_PropertyAssigner);
    EXPECT_EQ(ymType_Owner(A_foo), A);
    EXPECT_EQ(ymType_Owner(A_foo_assigner), A);
    EXPECT_EQ(ymType_Owner(A_other), A);
    EXPECT_EQ(ymType_Owner(A_other_assigner), A);
    EXPECT_EQ(ymType_Type(A_foo), B);
    EXPECT_EQ(ymType_Type(A_foo_assigner), nullptr);
    EXPECT_EQ(ymType_Type(A_other), C);
    EXPECT_EQ(ymType_Type(A_other_assigner), nullptr);
    EXPECT_EQ(ymType_ReturnType(A_foo), B);
    EXPECT_EQ(ymType_ReturnType(A_foo_assigner), ymCtx_LdNone(ctx));
    EXPECT_EQ(ymType_ReturnType(A_other), C);
    EXPECT_EQ(ymType_ReturnType(A_other_assigner), ymCtx_LdNone(ctx));
    EXPECT_EQ(ymType_Params(A_foo, YM_FALSE), 1);
    EXPECT_EQ(ymType_Params(A_foo_assigner, YM_FALSE), 2);
    EXPECT_EQ(ymType_Params(A_other, YM_FALSE), 1);
    EXPECT_EQ(ymType_Params(A_other_assigner, YM_FALSE), 2);
    EXPECT_EQ(ymType_ParamType(A_foo, 0), A);
    EXPECT_EQ(ymType_ParamType(A_foo_assigner, 0), A);
    EXPECT_EQ(ymType_ParamType(A_foo_assigner, 1), B);
    EXPECT_EQ(ymType_ParamType(A_other, 0), A);
    EXPECT_EQ(ymType_ParamType(A_other_assigner, 0), A);
    EXPECT_EQ(ymType_ParamType(A_other_assigner, 1), C);
}

TEST(ParcelDefs, AddComputedProperty_Normalizes_Type) {
    SETUP_ALL(ctx);
    SETUP_PARCELDEF(p_def);

    ymParcelDef_AddStruct(p_def, "A");
    ymParcelDef_AddComputedProperty(p_def, "A", "p", "  p  : \n\n A  ",
        ymInertCallBhvrFn, nullptr, ymInertCallBhvrFn, nullptr);

    ymDm_BindParcelDef(dm, "p", p_def);
    auto A = ymCtx_Load(ctx, "p:A");
    auto A_p = ymCtx_Load(ctx, "p:A::p");
    ASSERT_TRUE(A);
    ASSERT_TRUE(A_p);

    ASSERT_EQ(ymType_Type(A_p), A);
    EXPECT_STREQ(ymType_Fullname(ymType_Type(A_p)), "p:A");
}

TEST(ParcelDefs, AddComputedProperty_NameConflict_Type) {
    SETUP_ERRCOUNTER;
    SETUP_PARCELDEF(p_def);
    ASSERT_EQ(ymParcelDef_AddStruct(p_def, "A"), YM_TRUE);
    ASSERT_NE(ymParcelDef_AddComputedProperty(p_def, "A", "p", "p:B",
        ymInertCallBhvrFn, nullptr, ymInertCallBhvrFn, nullptr), YM_FALSE);
    ASSERT_EQ(ymParcelDef_AddComputedProperty(p_def, "A", "p", "p:B",
        ymInertCallBhvrFn, nullptr, ymInertCallBhvrFn, nullptr), YM_FALSE);
    EXPECT_EQ(err[YmErrCode_NameConflict], 1);
}

TEST(ParcelDefs, AddComputedProperty_NameConflict_TypeParam) {
    SETUP_ERRCOUNTER;
    SETUP_PARCELDEF(p_def);
    ymParcelDef_AddProtocol(p_def, "Any");
    ASSERT_EQ(ymParcelDef_AddStruct(p_def, "A"), YM_TRUE);
    ASSERT_NE(ymParcelDef_AddTypeParam(p_def, "A", "p", "p:Any"), YM_NO_TYPE_PARAM_INDEX);
    ASSERT_EQ(ymParcelDef_AddComputedProperty(p_def, "A", "p", "p:B",
        ymInertCallBhvrFn, nullptr, ymInertCallBhvrFn, nullptr), YM_FALSE);
    EXPECT_EQ(err[YmErrCode_NameConflict], 1);
}

TEST(ParcelDefs, AddComputedProperty_NameConflict_Self) {
    SETUP_ERRCOUNTER;
    SETUP_PARCELDEF(p_def);
    ASSERT_EQ(ymParcelDef_AddStruct(p_def, "A"), YM_TRUE);
    ASSERT_EQ(ymParcelDef_AddComputedProperty(p_def, "A", "Self", "p:B",
        ymInertCallBhvrFn, nullptr, ymInertCallBhvrFn, nullptr), YM_FALSE);
    EXPECT_EQ(err[YmErrCode_NameConflict], 1);
}

TEST(ParcelDefs, AddComputedProperty_TypeNotFound_InvalidOwnerName) {
    SETUP_ERRCOUNTER;
    SETUP_PARCELDEF(p_def);
    ASSERT_EQ(ymParcelDef_AddComputedProperty(p_def, "missing", "p", "p:B",
        ymInertCallBhvrFn, nullptr, ymInertCallBhvrFn, nullptr), YM_FALSE);
    EXPECT_EQ(err[YmErrCode_TypeNotFound], 1);
}

TEST(ParcelDefs, AddComputedProperty_TypeCannotHaveMembers) {
    SETUP_ERRCOUNTER;
    SETUP_PARCELDEF(p_def);
    ASSERT_EQ(ymParcelDef_AddFn(p_def, "A", "p:B", ymInertCallBhvrFn, nullptr), YM_TRUE);
    ASSERT_EQ(ymParcelDef_AddComputedProperty(p_def, "A", "p", "p:B",
        ymInertCallBhvrFn, nullptr, ymInertCallBhvrFn, nullptr), YM_FALSE);
    EXPECT_EQ(err[YmErrCode_TypeCannotHaveMembers], 1);
}

TEST(ParcelDefs, AddComputedProperty_ProtocolType) {
    SETUP_ERRCOUNTER;
    SETUP_PARCELDEF(p_def);
    ASSERT_EQ(ymParcelDef_AddProtocol(p_def, "A"), YM_TRUE);
    ASSERT_EQ(ymParcelDef_AddComputedProperty(p_def, "A", "p", "p:B",
        ymInertCallBhvrFn, nullptr, ymInertCallBhvrFn, nullptr), YM_FALSE);
    EXPECT_EQ(err[YmErrCode_ProtocolType], 1);
}

TEST(ParcelDefs, AddComputedProperty_IllegalName) {
    testNameLegality(
        [](std::string name, YmParcelDef* parceldef, bool shouldSucceed) -> std::string {
            if (shouldSucceed) {
                EXPECT_EQ(ymParcelDef_AddStruct(parceldef, "A"), YM_TRUE);
                EXPECT_NE(ymParcelDef_AddComputedProperty(parceldef, "A", name.c_str(), "yama:None",
                    ymInertCallBhvrFn, nullptr, ymInertCallBhvrFn, nullptr), YM_FALSE);
            }
            else {
                EXPECT_EQ(ymParcelDef_AddStruct(parceldef, "A"), YM_TRUE);
                EXPECT_EQ(ymParcelDef_AddComputedProperty(parceldef, "A", name.c_str(), "yama:None",
                    ymInertCallBhvrFn, nullptr, ymInertCallBhvrFn, nullptr), YM_FALSE);
            }
            return std::format("p:A::{}", name);
        });
}

TEST(ParcelDefs, AddComputedProperty_IllegalSpecifier_InvalidTypeSymbol) {
    SETUP_ERRCOUNTER;
    SETUP_PARCELDEF(p_def);
    ASSERT_EQ(ymParcelDef_AddStruct(p_def, "A"), YM_TRUE);
    ASSERT_EQ(ymParcelDef_AddComputedProperty(p_def, "A", "p", "/",
        ymInertCallBhvrFn, nullptr, ymInertCallBhvrFn, nullptr), YM_FALSE);
    EXPECT_EQ(err[YmErrCode_IllegalSpecifier], 1);
}

TEST(ParcelDefs, AddTypeParam) {
    SETUP_ALL(ctx);
    SETUP_PARCELDEF(p_def);
    ymParcelDef_AddProtocol(p_def, "Any");
    ymParcelDef_AddStruct(p_def, "Int");
    ymParcelDef_AddStruct(p_def, "Float");
    ymParcelDef_AddStruct(p_def, "A");
    ASSERT_EQ(ymParcelDef_AddTypeParam(p_def, "A", "T", "p:Any"), 0);
    ASSERT_EQ(ymParcelDef_AddTypeParam(p_def, "A", "U", "p:Any"), 1);
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

    ymParcelDef_AddProtocol(p_def, "Any");
    ymParcelDef_AddStruct(p_def, "Int");
    ymParcelDef_AddStruct(p_def, "A");
    auto A_T_constraintType = "  p  : \n\n Any  ";
    ymParcelDef_AddTypeParam(p_def, "A", "T", A_T_constraintType);

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
    ymParcelDef_AddStruct(p_def, "A");
    for (YmTypeParamIndex i = 0; i < YM_MAX_TYPE_PARAMS; i++) {
        auto name = std::format("T{}", i + 1);
        ASSERT_EQ(ymParcelDef_AddTypeParam(p_def, "A", name.c_str(), "p:Any"), i);
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
    EXPECT_EQ(ymParcelDef_AddTypeParam(p_def, "missing", "T", "p:Any"), YM_NO_TYPE_PARAM_INDEX);
    EXPECT_EQ(err[YmErrCode_TypeNotFound], 1);
}

TEST(ParcelDefs, AddTypeParam_MemberType) {
    SETUP_ALL(ctx);
    SETUP_PARCELDEF(p_def);
    ymParcelDef_AddProtocol(p_def, "Any");
    ymParcelDef_AddStruct(p_def, "Int");
    ymParcelDef_AddStruct(p_def, "A");
    ymParcelDef_AddMethod(p_def, "A", "m", "p:Int", ymInertCallBhvrFn, nullptr);
    EXPECT_EQ(ymParcelDef_AddTypeParam(p_def, "A::m", "T", "p:Any"), YM_NO_TYPE_PARAM_INDEX);
    EXPECT_EQ(err[YmErrCode_MemberType], 1);
}

TEST(ParcelDefs, AddTypeParam_NameConflict_MemberType) {
    SETUP_ALL(ctx);
    SETUP_PARCELDEF(p_def);
    ymParcelDef_AddProtocol(p_def, "Any");
    ymParcelDef_AddStruct(p_def, "Int");
    ymParcelDef_AddStruct(p_def, "A");
    ymParcelDef_AddMethod(p_def, "A", "m", "p:Int", ymInertCallBhvrFn, nullptr);
    EXPECT_EQ(ymParcelDef_AddTypeParam(p_def, "A", "m", "p:Any"), YM_NO_TYPE_PARAM_INDEX);
    EXPECT_EQ(err[YmErrCode_NameConflict], 1);
}

TEST(ParcelDefs, AddTypeParam_NameConflict_TypeParam) {
    SETUP_ALL(ctx);
    SETUP_PARCELDEF(p_def);
    ymParcelDef_AddProtocol(p_def, "Any");
    ymParcelDef_AddStruct(p_def, "Int");
    ymParcelDef_AddStruct(p_def, "A");
    EXPECT_NE(ymParcelDef_AddTypeParam(p_def, "A", "T", "p:Any"), YM_NO_TYPE_PARAM_INDEX);
    EXPECT_EQ(ymParcelDef_AddTypeParam(p_def, "A", "T", "p:Any"), YM_NO_TYPE_PARAM_INDEX);
    EXPECT_EQ(err[YmErrCode_NameConflict], 1);
}

TEST(ParcelDefs, AddTypeParam_NameConflict_Self) {
    SETUP_ALL(ctx);
    SETUP_PARCELDEF(p_def);
    ymParcelDef_AddProtocol(p_def, "Any");
    ymParcelDef_AddStruct(p_def, "A");
    EXPECT_EQ(ymParcelDef_AddTypeParam(p_def, "A", "Self", "p:Any"), YM_NO_TYPE_PARAM_INDEX);
    EXPECT_EQ(err[YmErrCode_NameConflict], 1);
}

TEST(ParcelDefs, AddTypeParam_IllegalName) {
    testNameLegalityHelper(
        [](std::string name) {
            SETUP_ALL(ctx);
            SETUP_PARCELDEF(p_def);
            ASSERT_EQ(ymParcelDef_AddStruct(p_def, "A"), YM_TRUE);
            ASSERT_NE(ymParcelDef_AddTypeParam(p_def, "A", name.c_str(), "yama:Any"), YM_NO_TYPE_PARAM_INDEX);
            ymDm_BindParcelDef(dm, "p", p_def);
            auto t = ymCtx_Load(ctx, "p:A[yama:Int]");
            ASSERT_TRUE(t);
            EXPECT_EQ(ymType_TypeParamByIndex(t, 0), ymCtx_LdInt(ctx));
        },
        [](std::string name) {
            SETUP_ERRCOUNTER;
            SETUP_PARCELDEF(p_def);
            ASSERT_EQ(ymParcelDef_AddStruct(p_def, "A"), YM_TRUE);
            ASSERT_EQ(ymParcelDef_AddTypeParam(p_def, "A", name.c_str(), "yama:Any"), YM_NO_TYPE_PARAM_INDEX);
            EXPECT_EQ(err[YmErrCode_IllegalName], 1);
        });
}

TEST(ParcelDefs, AddTypeParam_IllegalSpecifier_InvalidConstraint) {
    SETUP_ALL(ctx);
    SETUP_PARCELDEF(p_def);
    ymParcelDef_AddProtocol(p_def, "Any");
    ymParcelDef_AddStruct(p_def, "A");
    EXPECT_EQ(ymParcelDef_AddTypeParam(p_def, "A", "T", "/"), YM_NO_TYPE_PARAM_INDEX);
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
    ymParcelDef_AddProtocol(p_def, "P");
    ymParcelDef_AddTypeParam(p_def, "P", "T", "p:Any");
    ymParcelDef_AddStruct(p_def, "A");
    EXPECT_EQ(ymParcelDef_AddTypeParam(p_def, "A", "T", "p:P[$U]"), YM_NO_TYPE_PARAM_INDEX);
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
    ymParcelDef_AddStruct(p_def, "A");
    EXPECT_NE(ymParcelDef_AddTypeParam(p_def, "A", "T", "p:Any"), YM_NO_TYPE_PARAM_INDEX);
    EXPECT_EQ(ymParcelDef_AddTypeParam(p_def, "A", "U", "$T"), YM_NO_TYPE_PARAM_INDEX);
    EXPECT_EQ(err[YmErrCode_IllegalConstraint], 1);
}

TEST(ParcelDefs, AddTypeParam_LimitReached_MaxTypeParams) {
    SETUP_ALL(ctx);
    SETUP_PARCELDEF(p_def);
    ymParcelDef_AddProtocol(p_def, "Any");
    ymParcelDef_AddStruct(p_def, "Int");
    ymParcelDef_AddStruct(p_def, "A");
    for (YmTypeParamIndex i = 0; i < YM_MAX_TYPE_PARAMS; i++) {
        auto name = std::format("T{}", i + 1);
        ASSERT_EQ(ymParcelDef_AddTypeParam(p_def, "A", name.c_str(), "p:Any"), i);
    }
    EXPECT_EQ(ymParcelDef_AddTypeParam(p_def, "A", "TXX", "p:Int"), YM_NO_TYPE_PARAM_INDEX);
    EXPECT_EQ(err[YmErrCode_LimitReached], 1);
}

TEST(ParcelDefs, AddParam_PositionalAndNamed) {
    SETUP_ALL(ctx);
    SETUP_PARCELDEF(p_def);
    ymParcelDef_AddFn(p_def, "A", "p:B", ymInertCallBhvrFn, nullptr);
    ymParcelDef_AddStruct(p_def, "B");
    ymParcelDef_AddStruct(p_def, "C");
    ymParcelDef_AddStruct(p_def, "D");
    EXPECT_EQ(ymParcelDef_AddParam(p_def, "A", "x", "p:B"), 0);
    EXPECT_EQ(ymParcelDef_AddParam(p_def, "A", "y", "p:C"), 1);
    EXPECT_EQ(ymParcelDef_AddParam(p_def, "A", "z", "p:D"), 2);
    ymParcelDef_BeginNamedParams(p_def, "A");
    EXPECT_EQ(ymParcelDef_AddParam(p_def, "A", "a", "p:B"), 3);
    EXPECT_EQ(ymParcelDef_AddParam(p_def, "A", "b", "p:C"), 4);
    EXPECT_EQ(ymParcelDef_AddParam(p_def, "A", "c", "p:D"), 5);
    ymDm_BindParcelDef(dm, "p", p_def);
    auto A = ymCtx_Load(ctx, "p:A");
    auto B = ymCtx_Load(ctx, "p:B");
    auto C = ymCtx_Load(ctx, "p:C");
    auto D = ymCtx_Load(ctx, "p:D");
    ASSERT_TRUE(A);
    ASSERT_TRUE(B);
    ASSERT_TRUE(C);
    ASSERT_TRUE(D);
    ASSERT_EQ(ymType_Params(A, YM_TRUE), 6);
    ASSERT_EQ(ymType_Params(A, YM_FALSE), 3);
    EXPECT_STREQ(ymType_ParamName(A, 0), "x");
    EXPECT_STREQ(ymType_ParamName(A, 1), "y");
    EXPECT_STREQ(ymType_ParamName(A, 2), "z");
    EXPECT_STREQ(ymType_ParamName(A, 3), "a");
    EXPECT_STREQ(ymType_ParamName(A, 4), "b");
    EXPECT_STREQ(ymType_ParamName(A, 5), "c");
    EXPECT_EQ(ymType_ParamType(A, 0), B);
    EXPECT_EQ(ymType_ParamType(A, 1), C);
    EXPECT_EQ(ymType_ParamType(A, 2), D);
    EXPECT_EQ(ymType_ParamType(A, 3), B);
    EXPECT_EQ(ymType_ParamType(A, 4), C);
    EXPECT_EQ(ymType_ParamType(A, 5), D);
}

TEST(ParcelDefs, AddParam_Normalizes_ParamType) {
    SETUP_ALL(ctx);
    SETUP_PARCELDEF(p_def);

    ymParcelDef_AddStruct(p_def, "Int");
    ymParcelDef_AddFn(p_def, "f", "p:Int", ymInertCallBhvrFn, nullptr);
    auto f_paramType = "  p  : \n\n Int  ";
    ymParcelDef_AddParam(p_def, "f", "a", f_paramType); // Positional
    ymParcelDef_BeginNamedParams(p_def, "f");
    ymParcelDef_AddParam(p_def, "f", "b", f_paramType); // Named

    ymDm_BindParcelDef(dm, "p", p_def);
    auto f = ymCtx_Load(ctx, "p:f");
    auto Int = ymCtx_Load(ctx, "p:Int");
    ASSERT_TRUE(f);
    ASSERT_TRUE(Int);

    ASSERT_EQ(ymType_ParamType(f, 0), Int);
    ASSERT_EQ(ymType_ParamType(f, 1), Int);
    EXPECT_STREQ(ymType_Fullname(ymType_ParamType(f, 0)), "p:Int");
    EXPECT_STREQ(ymType_Fullname(ymType_ParamType(f, 1)), "p:Int");
}

TEST(ParcelDefs, AddParam_MaxPositionalAndNamedParams) {
    SETUP_ALL(ctx);
    SETUP_PARCELDEF(p_def);
    ymParcelDef_AddFn(p_def, "A", "p:B", ymInertCallBhvrFn, nullptr);
    for (YmParamIndex i = 0; i < YM_MAX_POSITIONAL_PARAMS; i++) {
        std::string paramName = std::format("a{}", i);
        ASSERT_EQ(ymParcelDef_AddParam(p_def, "A", paramName.c_str(), "p:B"), i)
            << "positional, i == " << i;
    }
    ymParcelDef_BeginNamedParams(p_def, "A");
    for (YmParamIndex i = 0; i < YM_MAX_NAMED_PARAMS; i++) {
        std::string paramName = std::format("b{}", i);
        ASSERT_EQ(ymParcelDef_AddParam(p_def, "A", paramName.c_str(), "p:B"), YM_MAX_POSITIONAL_PARAMS + i)
            << "named, YM_MAX_POSITIONAL_PARAMS + i == " << YM_MAX_POSITIONAL_PARAMS + i;
    }
    ymParcelDef_AddStruct(p_def, "B");
    ymDm_BindParcelDef(dm, "p", p_def);
    auto A = ymCtx_Load(ctx, "p:A");
    auto B = ymCtx_Load(ctx, "p:B");
    ASSERT_TRUE(A);
    ASSERT_TRUE(B);
    ASSERT_EQ(ymType_Params(A, YM_TRUE), YM_MAX_POSITIONAL_PARAMS + YM_MAX_NAMED_PARAMS);
    ASSERT_EQ(ymType_Params(A, YM_FALSE), YM_MAX_POSITIONAL_PARAMS);
    for (YmParamIndex i = 0; i < YM_MAX_POSITIONAL_PARAMS; i++) {
        std::string paramName = std::format("a{}", i);
        EXPECT_STREQ(ymType_ParamName(A, i), paramName.c_str())
            << "positional, i == " << i;
        EXPECT_EQ(ymType_ParamType(A, i), B)
            << "positional, i == " << i;
    }
    for (YmParamIndex i = 0; i < YM_MAX_NAMED_PARAMS; i++) {
        std::string paramName = std::format("b{}", i);
        EXPECT_STREQ(ymType_ParamName(A, YM_MAX_POSITIONAL_PARAMS + i), paramName.c_str())
            << "named, YM_MAX_POSITIONAL_PARAMS + i == " << YM_MAX_POSITIONAL_PARAMS + i;
        EXPECT_EQ(ymType_ParamType(A, YM_MAX_POSITIONAL_PARAMS + i), B)
            << "named, YM_MAX_POSITIONAL_PARAMS + i == " << YM_MAX_POSITIONAL_PARAMS + i;
    }
}

TEST(ParcelDefs, AddParam_TypeNotFound) {
    SETUP_ALL(ctx);
    SETUP_PARCELDEF(p_def);
    EXPECT_EQ(ymParcelDef_AddParam(p_def, "missing", "x", "p:B"), YM_NO_PARAM_INDEX);
    EXPECT_EQ(err[YmErrCode_TypeNotFound], 1);
}

TEST(ParcelDefs, AddParam_CallSigNotFound) {
    SETUP_ALL(ctx);
    SETUP_PARCELDEF(p_def);
    ymParcelDef_AddStruct(p_def, "A");
    EXPECT_EQ(ymParcelDef_AddParam(p_def, "A", "x", "p:B"), YM_NO_PARAM_INDEX);
    EXPECT_EQ(err[YmErrCode_CallSigNotFound], 1);
}

TEST(ParcelDefs, AddParam_PropertyType) {
    SETUP_ALL(ctx);
    SETUP_PARCELDEF(p_def);
    ymParcelDef_AddStruct(p_def, "A");
    ymParcelDef_AddReadOnlyStoredProperty(p_def, "A", "p", "yama:Int");
    EXPECT_EQ(ymParcelDef_AddParam(p_def, "A::p", "x", "p:B"), YM_NO_PARAM_INDEX);
    EXPECT_EQ(err[YmErrCode_PropertyType], 1);
}

TEST(ParcelDefs, AddParam_PropertyAssignerType) {
    SETUP_ALL(ctx);
    SETUP_PARCELDEF(p_def);
    ymParcelDef_AddStruct(p_def, "A");
    ymParcelDef_AddStoredProperty(p_def, "A", "p", "yama:Int");
    EXPECT_EQ(ymParcelDef_AddParam(p_def, "A::p$assigner", "x", "p:B"), YM_NO_PARAM_INDEX);
    EXPECT_EQ(err[YmErrCode_PropertyAssignerType], 1);
}

TEST(ParcelDefs, AddParam_NameConflict_BetweenParams) {
    SETUP_ALL(ctx);
    SETUP_PARCELDEF(p_def);
    ymParcelDef_AddFn(p_def, "A", "p:B", ymInertCallBhvrFn, nullptr);
    EXPECT_EQ(ymParcelDef_AddParam(p_def, "A", "x", "p:B"), 0);
    EXPECT_EQ(ymParcelDef_AddParam(p_def, "A", "x", "p:C"), YM_NO_PARAM_INDEX);
    EXPECT_EQ(err[YmErrCode_NameConflict], 1);
}

TEST(ParcelDefs, AddParam_IllegalName) {
    testNameLegalityHelper(
        [](std::string name) {
            SETUP_ALL(ctx);
            SETUP_PARCELDEF(p_def);
            ASSERT_EQ(ymParcelDef_AddFn(p_def, "f", "yama:None", ymInertCallBhvrFn, nullptr), YM_TRUE);
            ASSERT_NE(ymParcelDef_AddParam(p_def, "f", name.c_str(), "yama:Int"), YM_NO_PARAM_INDEX)
                << "name==\"" << name << "\"";
            ymDm_BindParcelDef(dm, "p", p_def);
            auto t = ymCtx_Load(ctx, "p:f");
            ASSERT_TRUE(t);
            EXPECT_STREQ(ymType_ParamName(t, 0), name.c_str());
        },
        [](std::string name) {
            SETUP_ERRCOUNTER;
            SETUP_PARCELDEF(p_def);
            ASSERT_EQ(ymParcelDef_AddFn(p_def, "f", "yama:None", ymInertCallBhvrFn, nullptr), YM_TRUE);
            ASSERT_EQ(ymParcelDef_AddParam(p_def, "f", name.c_str(), "yama:Int"), YM_NO_PARAM_INDEX)
                << "name==\"" << name << "\"";
            EXPECT_EQ(err[YmErrCode_IllegalName], 1);
        });
}

TEST(ParcelDefs, AddParam_IllegalSpecifier) {
    SETUP_ALL(ctx);
    SETUP_PARCELDEF(p_def);
    ymParcelDef_AddFn(p_def, "A", "p:B", ymInertCallBhvrFn, nullptr);
    EXPECT_EQ(ymParcelDef_AddParam(p_def, "A", "x", "/"), YM_NO_PARAM_INDEX);
    EXPECT_EQ(err[YmErrCode_IllegalSpecifier], 1);
}

TEST(ParcelDefs, AddParam_LimitReached_MaxPositionalParams) {
    SETUP_ALL(ctx);
    SETUP_PARCELDEF(p_def);
    ymParcelDef_AddFn(p_def, "A", "p:B", ymInertCallBhvrFn, nullptr);
    for (YmParamIndex i = 0; i < YM_MAX_POSITIONAL_PARAMS; i++) {
        std::string paramName = std::format("a{}", i);
        ASSERT_EQ(ymParcelDef_AddParam(p_def, "A", paramName.c_str(), "p:B"), i) << "i == " << i;
    }
    EXPECT_EQ(ymParcelDef_AddParam(p_def, "A", "b", "p:B"), YM_NO_PARAM_INDEX);
    EXPECT_EQ(err[YmErrCode_LimitReached], 1);
}

TEST(ParcelDefs, AddParam_LimitReached_MaxNamedParams) {
    SETUP_ALL(ctx);
    SETUP_PARCELDEF(p_def);
    ymParcelDef_AddFn(p_def, "A", "p:B", ymInertCallBhvrFn, nullptr);
    ymParcelDef_BeginNamedParams(p_def, "A");
    for (YmParamIndex i = 0; i < YM_MAX_NAMED_PARAMS; i++) {
        std::string paramName = std::format("a{}", i);
        ASSERT_EQ(ymParcelDef_AddParam(p_def, "A", paramName.c_str(), "p:B"), i) << "i == " << i;
    }
    EXPECT_EQ(ymParcelDef_AddParam(p_def, "A", "b", "p:B"), YM_NO_PARAM_INDEX);
    EXPECT_EQ(err[YmErrCode_LimitReached], 1);
}

TEST(ParcelDefs, BeginNamedParams) {
    SETUP_ALL(ctx);
    SETUP_PARCELDEF(p_def);

    ymParcelDef_AddFn(p_def, "f", "yama:None", ymInertCallBhvrFn, nullptr);
    ymParcelDef_AddParam(p_def, "f", "a", "yama:Int");
    ymParcelDef_BeginNamedParams(p_def, "f");
    ymParcelDef_AddParam(p_def, "f", "b", "yama:Float");
    ymParcelDef_AddParam(p_def, "f", "c", "yama:Float");
    ymDm_BindParcelDef(dm, "p", p_def);
    auto f = ymCtx_Load(ctx, "p:f");
    ASSERT_TRUE(f);

    EXPECT_EQ(ymType_Params(f, YM_TRUE), 3);
    EXPECT_EQ(ymType_Params(f, YM_FALSE), 1);

    EXPECT_EQ(ymType_ParamCategory(f, 0), YmParamCategory_Positional);
    EXPECT_EQ(ymType_ParamCategory(f, 1), YmParamCategory_Named);
    EXPECT_EQ(ymType_ParamCategory(f, 2), YmParamCategory_Named);
}

TEST(ParcelDefs, BeginNamedParams_MultipleTimes) {
    SETUP_ALL(ctx);
    SETUP_PARCELDEF(p_def);

    ymParcelDef_AddFn(p_def, "f", "yama:None", ymInertCallBhvrFn, nullptr);
    ymParcelDef_AddParam(p_def, "f", "a", "yama:Int");
    ymParcelDef_BeginNamedParams(p_def, "f");
    ymParcelDef_BeginNamedParams(p_def, "f");
    ymParcelDef_BeginNamedParams(p_def, "f");
    ymParcelDef_BeginNamedParams(p_def, "f");
    ymParcelDef_AddParam(p_def, "f", "b", "yama:Float");
    ymParcelDef_BeginNamedParams(p_def, "f");
    ymParcelDef_BeginNamedParams(p_def, "f");
    ymParcelDef_AddParam(p_def, "f", "c", "yama:Float");
    ymParcelDef_BeginNamedParams(p_def, "f");
    ymDm_BindParcelDef(dm, "p", p_def);
    auto f = ymCtx_Load(ctx, "p:f");
    ASSERT_TRUE(f);

    EXPECT_EQ(ymType_Params(f, YM_TRUE), 3);
    EXPECT_EQ(ymType_Params(f, YM_FALSE), 1);

    EXPECT_EQ(ymType_ParamCategory(f, 0), YmParamCategory_Positional);
    EXPECT_EQ(ymType_ParamCategory(f, 1), YmParamCategory_Named);
    EXPECT_EQ(ymType_ParamCategory(f, 2), YmParamCategory_Named);
}

TEST(ParcelDefs, BeginNamedParams_TypeNotFound) {
    SETUP_ALL(ctx);
    SETUP_PARCELDEF(p_def);

    ymParcelDef_BeginNamedParams(p_def, "missing");
    EXPECT_GE(err[YmErrCode_TypeNotFound], 1);
}

TEST(ParcelDefs, BeginNamedParams_CallSigNotFound) {
    SETUP_ALL(ctx);
    SETUP_PARCELDEF(p_def);

    ymParcelDef_AddStruct(p_def, "A");
    ymParcelDef_BeginNamedParams(p_def, "A");
    EXPECT_GE(err[YmErrCode_CallSigNotFound], 1);
}

TEST(ParcelDefs, BeginNamedParams_PropertyType) {
    SETUP_ALL(ctx);
    SETUP_PARCELDEF(p_def);

    ymParcelDef_AddStruct(p_def, "A");
    ymParcelDef_AddReadOnlyStoredProperty(p_def, "A", "p", "yama:Int");
    ymParcelDef_BeginNamedParams(p_def, "A::p");
    EXPECT_GE(err[YmErrCode_PropertyType], 1);
}

TEST(ParcelDefs, BeginNamedParams_PropertyAssignerType) {
    SETUP_ALL(ctx);
    SETUP_PARCELDEF(p_def);

    ymParcelDef_AddStruct(p_def, "A");
    ymParcelDef_AddStoredProperty(p_def, "A", "p", "yama:Int");
    ymParcelDef_BeginNamedParams(p_def, "A::p$assigner");
    EXPECT_GE(err[YmErrCode_PropertyAssignerType], 1);
}

TEST(ParcelDefs, BeginNamedParams_ProtocolMemberType) {
    SETUP_ALL(ctx);
    SETUP_PARCELDEF(p_def);

    ymParcelDef_AddProtocol(p_def, "P");
    ymParcelDef_AddMethodReq(p_def, "P", "m", "yama:Int");
    ymParcelDef_BeginNamedParams(p_def, "P::m");
    EXPECT_GE(err[YmErrCode_ProtocolMemberType], 1);
}

TEST(ParcelDefs, AddRef) {
    SETUP_ALL(ctx);
    SETUP_PARCELDEF(p_def);

    ymParcelDef_AddStruct(p_def, "A");
    ymParcelDef_AddStruct(p_def, "B");
    ymParcelDef_AddStruct(p_def, "C");
    
    YmRef A_ref_B_1 = ymParcelDef_AddRef(p_def, "A", "p:B");
    YmRef A_ref_C = ymParcelDef_AddRef(p_def, "A", "p:C");
    YmRef A_ref_B_2 = ymParcelDef_AddRef(p_def, "A", "p:B"); // Duplicates get unique IDs.
    // Ref IDs should be sequential as expected.
    ASSERT_EQ(A_ref_B_1, 0);
    ASSERT_EQ(A_ref_C, 1);
    ASSERT_EQ(A_ref_B_2, 2);
    
    ymDm_BindParcelDef(dm, "p", p_def);
    auto A = ymCtx_Load(ctx, "p:A");
    auto B = ymCtx_Load(ctx, "p:B");
    auto C = ymCtx_Load(ctx, "p:C");
    ASSERT_TRUE(A);
    ASSERT_TRUE(B);
    ASSERT_TRUE(C);
    
    EXPECT_EQ(ymType_Ref(A, A_ref_B_1), B);
    EXPECT_EQ(ymType_Ref(A, A_ref_C), C);
    EXPECT_EQ(ymType_Ref(A, A_ref_B_2), B);
}

TEST(ParcelDefs, AddRef_Normalizes_Symbol) {
    SETUP_ALL(ctx);
    SETUP_PARCELDEF(p_def);

    ymParcelDef_AddStruct(p_def, "Int");
    ymParcelDef_AddStruct(p_def, "A");
    auto A_symbol = "  p  : \n\n Int  ";
    auto A_ref = ymParcelDef_AddRef(p_def, "A", A_symbol);
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
    ASSERT_EQ(ymParcelDef_AddRef(p_def, "missing", "p:B"), YM_NO_REF);
    EXPECT_EQ(err[YmErrCode_TypeNotFound], 1);
}

TEST(ParcelDefs, AddRef_IllegalSpecifier) {
    for (const auto& fullname : illegalFullnames) {
        SETUP_ERRCOUNTER;
        SETUP_PARCELDEF(p_def);
        ASSERT_EQ(ymParcelDef_AddStruct(p_def, "A"), YM_TRUE);
        EXPECT_EQ(ymParcelDef_AddRef(p_def, "A", fullname.c_str()), YM_NO_REF) << "fullname==" << fullname;
        EXPECT_EQ(err[YmErrCode_IllegalSpecifier], 1) << "fullname==" << fullname;
    }
}

