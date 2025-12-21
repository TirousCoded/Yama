

#include <gtest/gtest.h>
#include <taul/strings.h>
#include <yama/yama.h>

#include "../../utils/utils.h"


// NOTE: These unit tests cover defined Yama conversions.

// NOTE: Yama defines explicit and implicit conversions, the ladder called 'coercions'.


void conv(YmItem* from, YmItem* to, bool succeedIfExplicit, bool succeedIfImplicit, int line) {
    EXPECT_EQ(ymItem_Converts(from, to, false) == YM_TRUE, succeedIfExplicit)
        << "line: " << line << ", " << ymItem_Fullname(from) << " -> " << ymItem_Fullname(to);

    EXPECT_EQ(ymItem_Converts(from, to, true) == YM_TRUE, succeedIfImplicit)
        << "line: " << line << ", " << ymItem_Fullname(from) << " -> " << ymItem_Fullname(to) << " (coercion)";
}

#define CONV(from, to, succeedIfExplicit, succeedIfImplicit) \
conv((from), (to), (succeedIfExplicit), (succeedIfImplicit), __LINE__)


TEST(Conversions, Identity) {
    static_assert(YmKind_Num == 4);
    SETUP_ALL(ctx);
    SETUP_PARCELDEF(p_def);
    auto STRUCT_index = ymParcelDef_AddStruct(p_def, "STRUCT");
    auto PROTOCOL_index = ymParcelDef_AddProtocol(p_def, "PROTOCOL");
    auto FN_index = ymParcelDef_AddFn(p_def, "FN", "p:STRUCT", ymInertCallBhvrFn, nullptr);
    auto METHOD_index = ymParcelDef_AddMethod(p_def, STRUCT_index, "METHOD", "p:STRUCT", ymInertCallBhvrFn, nullptr);
    ymDm_BindParcelDef(dm, "p", p_def);
    auto STRUCT = ymCtx_Load(ctx, "p:STRUCT");
    auto PROTOCOL = ymCtx_Load(ctx, "p:PROTOCOL");
    auto FN = ymCtx_Load(ctx, "p:FN");
    auto METHOD = ymCtx_Load(ctx, "p:STRUCT::METHOD");
    ASSERT_TRUE(STRUCT);
    ASSERT_TRUE(PROTOCOL);
    ASSERT_TRUE(FN);
    ASSERT_TRUE(METHOD);

    for (const auto& t : { STRUCT, PROTOCOL, FN, METHOD }) {
        CONV(t, t, true, true);
    }
}

// NOTE: We exclude protocols from below as their conversion rules are more nuanced.

// NOTE: The two NonIdentity_***_ExcludingProtocols tests below cover the fact that for
//		 conversions T -> U, where T and U differ, and are non-protocols, the default
//		 is for the conversion to be illegal, w/ being legal being a special case.

TEST(Conversions, NonIdentity_BetweenSameKinds_ExcludingProtocols) {
    static_assert(YmKind_Num == 4);
    SETUP_ALL(ctx);
    SETUP_PARCELDEF(p_def);
    auto STRUCT1_index = ymParcelDef_AddStruct(p_def, "STRUCT1");
    auto STRUCT2_index = ymParcelDef_AddStruct(p_def, "STRUCT2");
    auto FN1_index = ymParcelDef_AddFn(p_def, "FN1", "p:STRUCT1", ymInertCallBhvrFn, nullptr);
    auto FN2_index = ymParcelDef_AddFn(p_def, "FN2", "p:STRUCT1", ymInertCallBhvrFn, nullptr);
    auto METHOD1_index = ymParcelDef_AddMethod(p_def, STRUCT1_index, "METHOD1", "p:STRUCT1", ymInertCallBhvrFn, nullptr);
    auto METHOD2_index = ymParcelDef_AddMethod(p_def, STRUCT1_index, "METHOD2", "p:STRUCT1", ymInertCallBhvrFn, nullptr);
    ymDm_BindParcelDef(dm, "p", p_def);
    auto STRUCT1 = ymCtx_Load(ctx, "p:STRUCT1");
    auto STRUCT2 = ymCtx_Load(ctx, "p:STRUCT2");
    auto FN1 = ymCtx_Load(ctx, "p:FN1");
    auto FN2 = ymCtx_Load(ctx, "p:FN2");
    auto METHOD1 = ymCtx_Load(ctx, "p:STRUCT1::METHOD1");
    auto METHOD2 = ymCtx_Load(ctx, "p:STRUCT1::METHOD2");
    ASSERT_TRUE(STRUCT1);
    ASSERT_TRUE(STRUCT2);
    ASSERT_TRUE(FN1);
    ASSERT_TRUE(FN2);
    ASSERT_TRUE(METHOD1);
    ASSERT_TRUE(METHOD2);

    std::vector<std::pair<YmItem*, YmItem*>> pairs{
        { STRUCT1,	STRUCT2 },
        { FN1,		FN2 },
        { METHOD1,	METHOD2 },
    };

    for (const auto& [from, to] : pairs) {
        CONV(from, to, false, false);
    }
}

TEST(Conversions, NonIdentity_BetweenDiffKinds_ExcludingProtocols) {
    static_assert(YmKind_Num == 4);
    SETUP_ALL(ctx);
    SETUP_PARCELDEF(p_def);
    auto STRUCT_index = ymParcelDef_AddStruct(p_def, "STRUCT");
    auto FN_index = ymParcelDef_AddFn(p_def, "FN", "p:STRUCT", ymInertCallBhvrFn, nullptr);
    auto METHOD_index = ymParcelDef_AddMethod(p_def, STRUCT_index, "METHOD", "p:STRUCT", ymInertCallBhvrFn, nullptr);
    ymDm_BindParcelDef(dm, "p", p_def);
    auto STRUCT = ymCtx_Load(ctx, "p:STRUCT");
    auto FN = ymCtx_Load(ctx, "p:FN");
    auto METHOD = ymCtx_Load(ctx, "p:STRUCT::METHOD");
    ASSERT_TRUE(STRUCT);
    ASSERT_TRUE(FN);
    ASSERT_TRUE(METHOD);

    std::vector<YmItem*> types{
        STRUCT,
        FN,
        METHOD,
    };

    for (const auto& from : types) {
        for (const auto& to : types) {
            if (&from == &to) continue; // Skip identity convs.
            CONV(from, to, false, false);
        }
    }
}

// TODO: Add tests for the nuances of when a type conforms to a protocol.

TEST(Conversions, NonProtocolToProtocol) {
    static_assert(YmKind_Num == 4);
    SETUP_ALL(ctx);
    SETUP_PARCELDEF(p_def);
    auto T_index = ymParcelDef_AddStruct(p_def, "T");
    ymParcelDef_AddMethod(p_def, T_index, "mA", "p:T", ymInertCallBhvrFn, nullptr);
    ymParcelDef_AddMethod(p_def, T_index, "mB", "p:T", ymInertCallBhvrFn, nullptr);
    auto P1_index = ymParcelDef_AddProtocol(p_def, "P1");
    ymParcelDef_AddMethodReq(p_def, P1_index, "mA", "p:T");
    auto P2_index = ymParcelDef_AddProtocol(p_def, "P2");
    ymParcelDef_AddMethodReq(p_def, P2_index, "mB", "p:T");
    auto P3_index = ymParcelDef_AddProtocol(p_def, "P3");
    ymParcelDef_AddMethodReq(p_def, P3_index, "mC", "p:T");
    ymDm_BindParcelDef(dm, "p", p_def);
    auto T = ymCtx_Load(ctx, "p:T");
    auto P1 = ymCtx_Load(ctx, "p:P1");
    auto P2 = ymCtx_Load(ctx, "p:P2");
    auto P3 = ymCtx_Load(ctx, "p:P3");
    ASSERT_TRUE(T);
    ASSERT_TRUE(P1);
    ASSERT_TRUE(P2);
    ASSERT_TRUE(P3);

    CONV(T, P1, true, true);
    CONV(T, P2, true, true);
    CONV(T, P3, false, false); // T is not P3.
}

// Given P -> T, where P is a protocol, and T is not, and T conforms to P, the
// conversion is statically allowed, and checked for full legality at runtime.

// Given P -> T, where P is a protocol, and T is not, and T does NOT conform
// to P, the conversion is illegal (as no value of P could be a legal T.)

TEST(Conversions, ProtocolToNonProtocol) {
    static_assert(YmKind_Num == 4);
    SETUP_ALL(ctx);
    SETUP_PARCELDEF(p_def);
    auto T_index = ymParcelDef_AddStruct(p_def, "T");
    ymParcelDef_AddMethod(p_def, T_index, "mA", "p:T", ymInertCallBhvrFn, nullptr);
    ymParcelDef_AddMethod(p_def, T_index, "mB", "p:T", ymInertCallBhvrFn, nullptr);
    auto P1_index = ymParcelDef_AddProtocol(p_def, "P1");
    ymParcelDef_AddMethodReq(p_def, P1_index, "mA", "p:T");
    auto P2_index = ymParcelDef_AddProtocol(p_def, "P2");
    ymParcelDef_AddMethodReq(p_def, P2_index, "mB", "p:T");
    auto P3_index = ymParcelDef_AddProtocol(p_def, "P3");
    ymParcelDef_AddMethodReq(p_def, P3_index, "mC", "p:T");
    ymDm_BindParcelDef(dm, "p", p_def);
    auto T = ymCtx_Load(ctx, "p:T");
    auto P1 = ymCtx_Load(ctx, "p:P1");
    auto P2 = ymCtx_Load(ctx, "p:P2");
    auto P3 = ymCtx_Load(ctx, "p:P3");
    ASSERT_TRUE(T);
    ASSERT_TRUE(P1);
    ASSERT_TRUE(P2);
    ASSERT_TRUE(P3);

    CONV(P1, T, true, false);
    CONV(P2, T, true, false);
    CONV(P3, T, false, false); // T is not P3
}

// TODO: If we ever have situations be possible where protocols P and Q can be found
//		 to be mutually exclusive, it might be worth it to make such P -> Q conversions
//		 statically illegal in those cases.

// Given P -> Q, where P and Q are protocols, the conversion is always statically
// allowed, and checked for legality at runtime (as it's possible that the object
// could conform to both P and Q.)

TEST(Conversions, ProtocolToProtocol) {
    static_assert(YmKind_Num == 4);
    SETUP_ALL(ctx);
    SETUP_PARCELDEF(p_def);
    ymParcelDef_AddStruct(p_def, "T");
    auto P_index = ymParcelDef_AddProtocol(p_def, "P");
    ymParcelDef_AddMethodReq(p_def, P_index, "mA", "p:T");
    auto Q_index = ymParcelDef_AddProtocol(p_def, "Q");
    ymParcelDef_AddMethodReq(p_def, Q_index, "mB", "p:T");
    ymDm_BindParcelDef(dm, "p", p_def);
    auto P = ymCtx_Load(ctx, "p:P");
    auto Q = ymCtx_Load(ctx, "p:Q");
    ASSERT_TRUE(P);
    ASSERT_TRUE(Q);

    CONV(P, Q, true, true);
    CONV(Q, P, true, true); // Should also be true.
}

