

#include <gtest/gtest.h>
#include <taul/strings.h>
#include <yama/yama.h>
#include <yama++/general.h>

#include "../../utils/utils.h"


// NOTE: These unit tests cover defined Yama conversions.

// NOTE: Yama defines explicit and implicit conversions, the ladder called 'coercions'.


static void conv(YmItem* from, YmItem* to, bool succeedIfExplicit, bool succeedIfImplicit, int line) {
    EXPECT_EQ(ymItem_Converts(from, to, false) == YM_TRUE, succeedIfExplicit)
        << "line: " << line << ", " << ymItem_Fullname(from) << " -> " << ymItem_Fullname(to);

    EXPECT_EQ(ymItem_Converts(from, to, true) == YM_TRUE, succeedIfImplicit)
        << "line: " << line << ", " << ymItem_Fullname(from) << " -> " << ymItem_Fullname(to) << " (coercion)";
}

#define CONV(from, to, succeedIfExplicit, succeedIfImplicit) \
conv((from), (to), (succeedIfExplicit), (succeedIfImplicit), __LINE__)


static std::optional<std::vector<YmItem*>> mkItemsVec(YmCtx* ctx, std::vector<std::string> fullnames) {
    std::vector<YmItem*> result{};
    result.reserve(fullnames.size());
    for (const auto& fullname : fullnames) {
        result.push_back(ymCtx_Load(ctx, fullname.c_str()));
        EXPECT_TRUE(result.back()) << "fullname==" << fullname;
        if (!result.back()) {
            return std::nullopt;
        }
    }
    return ym::retopt(result);
}


TEST(Conversions, Identity_IncludeGenerics) {
    static_assert(YmKind_Num == 4);
    SETUP_ALL(ctx);
    SETUP_PARCELDEF(p_def);

    ymParcelDef_AddProtocol(p_def, "Any");

    auto A_ind = ymParcelDef_AddStruct(p_def, "A");
    auto B_ind = ymParcelDef_AddStruct(p_def, "B");
    auto C_ind = ymParcelDef_AddStruct(p_def, "C");
    ymParcelDef_AddItemParam(p_def, C_ind, "T", "p:Any");
    
    auto P_ind = ymParcelDef_AddProtocol(p_def, "P");
    auto Q_ind = ymParcelDef_AddProtocol(p_def, "Q");
    ymParcelDef_AddItemParam(p_def, Q_ind, "T", "p:Any");
    
    auto f_ind = ymParcelDef_AddFn(p_def, "f", "p:A", ymInertCallBhvrFn, nullptr);
    auto g_ind = ymParcelDef_AddFn(p_def, "g", "p:A", ymInertCallBhvrFn, nullptr);
    ymParcelDef_AddItemParam(p_def, g_ind, "T", "p:Any");
    
    auto A_m_ind = ymParcelDef_AddMethod(p_def, A_ind, "m", "p:A", ymInertCallBhvrFn, nullptr);
    auto C_m_ind = ymParcelDef_AddMethod(p_def, C_ind, "m", "p:A", ymInertCallBhvrFn, nullptr);
    
    ymDm_BindParcelDef(dm, "p", p_def);

    auto items = mkItemsVec(ctx, {
        "p:A",
        "p:B",
        "p:C[p:A]",
        "p:C[p:B]",
        "p:P",
        "p:Q[p:A]",
        "p:Q[p:B]",
        "p:f",
        "p:g[p:A]",
        "p:g[p:B]",
        "p:A::m",
        "p:C[p:A]::m",
        "p:C[p:B]::m",
        })
        .value();

    for (const auto& t : items) {
        CONV(t, t, true, true);
    }
}

// NOTE: We exclude protocols from below as their conversion rules are more nuanced.

// NOTE: Test below cover the fact that for conversions T -> U, where T and U differ, and
//       are non-protocols, the default is for the conversion to be illegal, w/ being legal
//       being a special case.

// TODO: Maybe we exclude protocols, but what about their methods?

TEST(Conversions, NonIdentity_IncludeGenerics_ExcludeProtocols) {
    static_assert(YmKind_Num == 4);
    SETUP_ALL(ctx);
    SETUP_PARCELDEF(p_def);

    ymParcelDef_AddProtocol(p_def, "Any");

    auto A_ind = ymParcelDef_AddStruct(p_def, "A");
    auto B_ind = ymParcelDef_AddStruct(p_def, "B");
    auto C_ind = ymParcelDef_AddStruct(p_def, "C");
    ymParcelDef_AddItemParam(p_def, C_ind, "T", "p:Any");

    ymParcelDef_AddFn(p_def, "f", "p:A", ymInertCallBhvrFn, nullptr);
    ymParcelDef_AddFn(p_def, "g", "p:A", ymInertCallBhvrFn, nullptr);
    auto h_ind = ymParcelDef_AddFn(p_def, "h", "p:A", ymInertCallBhvrFn, nullptr);
    ymParcelDef_AddItemParam(p_def, h_ind, "T", "p:Any");

    ymParcelDef_AddMethod(p_def, A_ind, "m1", "p:A", ymInertCallBhvrFn, nullptr);
    ymParcelDef_AddMethod(p_def, A_ind, "m2", "p:A", ymInertCallBhvrFn, nullptr);
    ymParcelDef_AddMethod(p_def, B_ind, "m1", "p:A", ymInertCallBhvrFn, nullptr);
    ymParcelDef_AddMethod(p_def, C_ind, "m1", "p:A", ymInertCallBhvrFn, nullptr);

    ymDm_BindParcelDef(dm, "p", p_def);

    auto items = mkItemsVec(ctx, {
        "p:A",
        "p:B",
        "p:C[p:A]",
        "p:C[p:B]",
        "p:f",
        "p:g",
        "p:h[p:A]",
        "p:h[p:B]",
        "p:A::m1",
        "p:A::m2",
        "p:B::m1",
        "p:C[p:A]::m1",
        "p:C[p:B]::m1",
        })
        .value();

    for (const auto& from : items) {
        for (const auto& to : items) {
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
    
    ymParcelDef_AddProtocol(p_def, "Any");

    auto A_ind = ymParcelDef_AddStruct(p_def, "A");
    ymParcelDef_AddMethod(p_def, A_ind, "mA", "p:A", ymInertCallBhvrFn, nullptr);
    ymParcelDef_AddMethod(p_def, A_ind, "mB", "p:A", ymInertCallBhvrFn, nullptr);

    ymParcelDef_AddStruct(p_def, "B");
    
    auto P1_ind = ymParcelDef_AddProtocol(p_def, "P1");
    ymParcelDef_AddMethodReq(p_def, P1_ind, "mA", "p:A");
    
    auto P2_ind = ymParcelDef_AddProtocol(p_def, "P2");
    ymParcelDef_AddMethodReq(p_def, P2_ind, "mB", "p:A");

    auto P3_ind = ymParcelDef_AddProtocol(p_def, "P3");
    ymParcelDef_AddMethodReq(p_def, P3_ind, "mC", "p:A");

    auto P4_ind = ymParcelDef_AddProtocol(p_def, "P4");
    ymParcelDef_AddItemParam(p_def, P4_ind, "T", "p:Any");
    ymParcelDef_AddMethodReq(p_def, P4_ind, "mA", "$T");
    
    ymDm_BindParcelDef(dm, "p", p_def);

    auto A = load(ctx, "p:A");
    auto P1 = load(ctx, "p:P1");
    auto P2 = load(ctx, "p:P2");
    auto P3 = load(ctx, "p:P3");
    auto P4_A = load(ctx, "p:P4[p:A]");
    auto P4_B = load(ctx, "p:P4[p:B]");

    CONV(A, P1, true, true);
    CONV(A, P2, true, true);
    CONV(A, P3, false, false); // A is not P3.
    CONV(A, P4_A, true, true);
    CONV(A, P4_B, false, false); // A is not P4[B].
}

// Given P -> A, where P is a protocol, and A is not, and A conforms to P, the
// conversion is statically allowed, and checked for full legality at runtime.

// Given P -> A, where P is a protocol, and A is not, and A does NOT conform
// to P, the conversion is illegal (as no value of P could be a legal A.)

TEST(Conversions, ProtocolToNonProtocol) {
    static_assert(YmKind_Num == 4);
    SETUP_ALL(ctx);
    SETUP_PARCELDEF(p_def);

    ymParcelDef_AddProtocol(p_def, "Any");

    auto A_ind = ymParcelDef_AddStruct(p_def, "A");
    ymParcelDef_AddMethod(p_def, A_ind, "mA", "p:A", ymInertCallBhvrFn, nullptr);
    ymParcelDef_AddMethod(p_def, A_ind, "mB", "p:A", ymInertCallBhvrFn, nullptr);

    ymParcelDef_AddStruct(p_def, "B");

    auto P1_ind = ymParcelDef_AddProtocol(p_def, "P1");
    ymParcelDef_AddMethodReq(p_def, P1_ind, "mA", "p:A");
    
    auto P2_ind = ymParcelDef_AddProtocol(p_def, "P2");
    ymParcelDef_AddMethodReq(p_def, P2_ind, "mB", "p:A");
    
    auto P3_ind = ymParcelDef_AddProtocol(p_def, "P3");
    ymParcelDef_AddMethodReq(p_def, P3_ind, "mC", "p:A");

    auto P4_ind = ymParcelDef_AddProtocol(p_def, "P4");
    ymParcelDef_AddItemParam(p_def, P4_ind, "T", "p:Any");
    ymParcelDef_AddMethodReq(p_def, P4_ind, "mA", "$T");
    
    ymDm_BindParcelDef(dm, "p", p_def);

    auto A = load(ctx, "p:A");
    auto P1 = load(ctx, "p:P1");
    auto P2 = load(ctx, "p:P2");
    auto P3 = load(ctx, "p:P3");
    auto P4_A = load(ctx, "p:P4[p:A]");
    auto P4_B = load(ctx, "p:P4[p:B]");

    CONV(P1, A, true, false);
    CONV(P2, A, true, false);
    CONV(P3, A, false, false); // A is not P3
    CONV(P4_A, A, true, false);
    CONV(P4_B, A, false, false); // A is not P4[B]
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

    ymParcelDef_AddProtocol(p_def, "Any");
    ymParcelDef_AddStruct(p_def, "A");
    ymParcelDef_AddStruct(p_def, "B");

    auto P_ind = ymParcelDef_AddProtocol(p_def, "P");
    ymParcelDef_AddMethodReq(p_def, P_ind, "mA", "p:A");
    
    auto Q_ind = ymParcelDef_AddProtocol(p_def, "Q");
    ymParcelDef_AddMethodReq(p_def, Q_ind, "mB", "p:A");
    
    auto R_ind = ymParcelDef_AddProtocol(p_def, "R");
    ymParcelDef_AddItemParam(p_def, R_ind, "T", "p:Any");
    ymParcelDef_AddMethodReq(p_def, R_ind, "mA", "$T");
    
    ymDm_BindParcelDef(dm, "p", p_def);
    
    auto items = mkItemsVec(ctx, {
        "p:P",
        "p:Q",
        "p:R[p:A]",
        "p:R[p:B]",
        })
        .value();

    // NOTE: For P -> Q, then Q -> P should also be true.

    for (const auto& from : items) {
        for (const auto& to : items) {
            if (&from == &to) continue; // Skip identity convs.
            CONV(from, to, true, true);
        }
    }
}

