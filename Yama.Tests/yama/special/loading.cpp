

#include "loading-helpers.h"


TEST(Loading, WorksWithAllItemKinds) {
    static_assert(YmKind_Num == 4);
    // Dep Graph:
    //      p:A                     (Struct)
    //      p:B                     (Protocol)
    //      p:C -> p:A              (Function) (Need back ref to set return type.)
    //      p:A::m -> p:A           (Method) (Need back ref to set return type.)

    SETUP_ALL(ctx);
    SETUP_PARCELDEF(p_def);

    YmItemIndex p_A_index = setup_struct(p_def, "A", {});
    YmItemIndex p_B_index = setup_protocol(p_def, "B", {});
    YmItemIndex p_C_index = setup_fn(p_def, "C", "p:A", {});
    YmItemIndex p_A_m_index = setup_method(p_def, p_A_index, "m", "p:A", {});

    ymDm_BindParcelDef(dm, "p", p_def);

    YmItem* p_A = ymCtx_Load(ctx, "p:A");
    YmItem* p_B = ymCtx_Load(ctx, "p:B");
    YmItem* p_C = ymCtx_Load(ctx, "p:C");
    YmItem* p_A_m = ymCtx_Load(ctx, "p:A::m");

    test_struct(p_A, "p:A", {});
    test_protocol(p_B, "p:B", {});
    test_fn(p_C, "p:C", { p_A });
    test_method(p_A_m, "p:A::m", { p_A });
}

TEST(Loading, NoRefConsts) {
    // Dep Graph:
    //      p:A

    // NOTE: Don't call ymCtx_Load for any item before the initial p:A load, so that
    //       it's recursive loading of others is properly tested.
    SETUP_ALL(ctx);
    SETUP_PARCELDEF(p_def);

    YmItemIndex p_A_index = setup_struct(p_def, "A", {});

    ymDm_BindParcelDef(dm, "p", p_def);
    
    YmItem* p_A = ymCtx_Load(ctx, "p:A"); // The recursive load under test.

    test_struct(p_A, "p:A", {});
}

TEST(Loading, RefConsts) {
    // Dep Graph:
    //      p:A -> p:B
    //          -> p:C

    // NOTE: Don't call ymCtx_Load for any item before the initial p:A load, so that
    //       it's recursive loading of others is properly tested.
    SETUP_ALL(ctx);
    SETUP_PARCELDEF(p_def);

    YmItemIndex p_A_index = setup_struct(p_def, "A", { "p:B", "p:C" });
    YmItemIndex p_B_index = setup_struct(p_def, "B", {});
    YmItemIndex p_C_index = setup_struct(p_def, "C", {});

    ymDm_BindParcelDef(dm, "p", p_def);
    
    YmItem* p_A = ymCtx_Load(ctx, "p:A"); // The recursive load under test.
    YmItem* p_B = ymCtx_Load(ctx, "p:B");
    YmItem* p_C = ymCtx_Load(ctx, "p:C");

    test_struct(p_A, "p:A", { p_B, p_C });
    test_struct(p_B, "p:B", {});
    test_struct(p_C, "p:C", {});
}

TEST(Loading, MultipleLayersOfIndirectRefConstReferences) {
    // Dep Graph:
    //      p:A -> p:B -> p:D
    //                 -> p:E
    //          -> p:C -> p:F

    // NOTE: Don't call ymCtx_Load for any item before the initial p:A load, so that
    //       it's recursive loading of others is properly tested.
    SETUP_ALL(ctx);
    SETUP_PARCELDEF(p_def);

    YmItemIndex p_A_index = setup_struct(p_def, "A", { "p:B", "p:C" });
    YmItemIndex p_B_index = setup_struct(p_def, "B", { "p:D", "p:E" });
    YmItemIndex p_C_index = setup_struct(p_def, "C", { "p:F" });
    YmItemIndex p_D_index = setup_struct(p_def, "D", {});
    YmItemIndex p_E_index = setup_struct(p_def, "E", {});
    YmItemIndex p_F_index = setup_struct(p_def, "F", {});

    ymDm_BindParcelDef(dm, "p", p_def);
    
    YmItem* p_A = ymCtx_Load(ctx, "p:A"); // The recursive load under test.
    YmItem* p_B = ymCtx_Load(ctx, "p:B");
    YmItem* p_C = ymCtx_Load(ctx, "p:C");
    YmItem* p_D = ymCtx_Load(ctx, "p:D");
    YmItem* p_E = ymCtx_Load(ctx, "p:E");
    YmItem* p_F = ymCtx_Load(ctx, "p:F");

    test_struct(p_A, "p:A", { p_B, p_C });
    test_struct(p_B, "p:B", { p_D, p_E });
    test_struct(p_C, "p:C", { p_F });
    test_struct(p_D, "p:D", {});
    test_struct(p_E, "p:E", {});
    test_struct(p_F, "p:F", {});
}

TEST(Loading, ItemReferencedMultipleTimesInAcyclicDepGraph) {
    // Dep Graph:
    //      p:A -> p:B -> p:D
    //          -> p:C -> p:D
    //          -> p:D

    // NOTE: Don't call ymCtx_Load for any item before the initial p:A load, so that
    //       it's recursive loading of others is properly tested.
    SETUP_ALL(ctx);
    SETUP_PARCELDEF(p_def);

    YmItemIndex p_A_index = setup_struct(p_def, "A", { "p:B", "p:C", "p:D" });
    YmItemIndex p_B_index = setup_struct(p_def, "B", { "p:D" });
    YmItemIndex p_C_index = setup_struct(p_def, "C", { "p:D" });
    YmItemIndex p_D_index = setup_struct(p_def, "D", {});

    ymDm_BindParcelDef(dm, "p", p_def);
    
    YmItem* p_A = ymCtx_Load(ctx, "p:A"); // The recursive load under test.
    YmItem* p_B = ymCtx_Load(ctx, "p:B");
    YmItem* p_C = ymCtx_Load(ctx, "p:C");
    YmItem* p_D = ymCtx_Load(ctx, "p:D");

    test_struct(p_A, "p:A", { p_B, p_C, p_D });
    test_struct(p_B, "p:B", { p_D });
    test_struct(p_C, "p:C", { p_D });
    test_struct(p_D, "p:D", {});
}

TEST(Loading, DepGraphCycle) {
    // Dep Graph:
    //      p:A -> p:B -> p:A       (back ref)
    //          -> p:A              (back ref)

    // NOTE: Don't call ymCtx_Load for any item before the initial p:A load, so that
    //       it's recursive loading of others is properly tested.
    SETUP_ALL(ctx);
    SETUP_PARCELDEF(p_def);

    YmItemIndex p_A_index = setup_struct(p_def, "A", { "p:B", "p:A" });
    YmItemIndex p_B_index = setup_struct(p_def, "B", { "p:A" });

    ymDm_BindParcelDef(dm, "p", p_def);
    
    YmItem* p_A = ymCtx_Load(ctx, "p:A"); // The recursive load under test.
    YmItem* p_B = ymCtx_Load(ctx, "p:B");

    test_struct(p_A, "p:A", { p_B, p_A });
    test_struct(p_B, "p:B", { p_A });
}

TEST(Loading, ItemsReferencedFromDifferentParcels) {
    // Dep Graph:
    //      p:A -> q:A -> p:B

    // NOTE: Don't call ymCtx_Load for any item before the initial p:A load, so that
    //       it's recursive loading of others is properly tested.
    SETUP_ALL(ctx);
    SETUP_PARCELDEF(p_def);
    SETUP_PARCELDEF(q_def);

    YmItemIndex p_A_index = setup_struct(p_def, "A", { "q:A" });
    YmItemIndex p_B_index = setup_struct(p_def, "B", {});
    YmItemIndex q_A_index = setup_struct(q_def, "A", { "p:B" });

    ymDm_BindParcelDef(dm, "p", p_def);
    ymDm_BindParcelDef(dm, "q", q_def);
    
    YmItem* p_A = ymCtx_Load(ctx, "p:A"); // The recursive load under test.
    YmItem* p_B = ymCtx_Load(ctx, "p:B");
    YmItem* q_A = ymCtx_Load(ctx, "q:A");

    test_struct(p_A, "p:A", { q_A });
    test_struct(p_B, "p:B", {});
    test_struct(q_A, "q:A", { p_B });
}

TEST(Loading, DirectLoadsAutoImportParcels) {
    // Dep Graph:
    //      p:A                     (We're testing that p gets auto-imported.)

    // NOTE: Don't call ymCtx_Load for any item before the initial p:A load, so that
    //       it's recursive loading of others is properly tested.
    // NOTE: Vary important to not call ymCtx_Import until AFTER our load.
    SETUP_ALL(ctx);
    SETUP_PARCELDEF(p_def);

    YmItemIndex p_A_index = setup_struct(p_def, "A", {});

    ymDm_BindParcelDef(dm, "p", p_def);

    YmItem* p_A = ymCtx_Load(ctx, "p:A"); // The recursive load under test.

    ASSERT_NE(p_A, nullptr);
    EXPECT_EQ(ymItem_Parcel(p_A), ymCtx_Import(ctx, "p"));
}

TEST(Loading, IndirectLoadsAutoImportParcels) {
    // Dep Graph:
    //      p:A -> q:A              (We're testing that q gets auto-imported.)

    // NOTE: Don't call ymCtx_Load for any item before the initial p:A load, so that
    //       it's recursive loading of others is properly tested.
    // NOTE: Vary important to not call ymCtx_Import until AFTER our load.
    SETUP_ALL(ctx);
    SETUP_PARCELDEF(p_def);
    SETUP_PARCELDEF(q_def);

    YmItemIndex p_A_index = setup_struct(p_def, "A", { "q:A" });
    YmItemIndex q_A_index = setup_struct(q_def, "A", {});

    ymDm_BindParcelDef(dm, "p", p_def);
    ymDm_BindParcelDef(dm, "q", q_def);

    YmItem* p_A = ymCtx_Load(ctx, "p:A"); // The recursive load under test.
    YmItem* q_A = ymCtx_Load(ctx, "q:A");

    ASSERT_NE(q_A, nullptr);
    EXPECT_EQ(ymItem_Parcel(q_A), ymCtx_Import(ctx, "q"));
}

TEST(Loading, OwnersAndTheirMembersAreLoadedTogether_DirectlyLoadOwner) {
    // Dep Graph:
    //      p:A     -> p:A::m1      (Member)
    //              -> p:A::m2      (Member)
    //      p:A::m1 -> p:A          (Owner)
    //              -> p:B          (Return Type)
    //      p:A::m2 -> p:A          (Owner)
    //              -> p:B          (Return Type)
    //      p:B                     (This is just for m1/m2 return types.)

    // NOTE: Don't call ymCtx_Load for any item before the initial p:A load, so that
    //       it's recursive loading of others is properly tested.
    SETUP_ALL(ctx);
    SETUP_PARCELDEF(p_def);

    YmItemIndex p_A_index = setup_struct(p_def, "A", { "p:A::m1", "p:A::m2" });
    YmItemIndex p_A_m1_index = setup_method(p_def, p_A_index, "m1", "p:B", { "p:A", "p:B" });
    YmItemIndex p_A_m2_index = setup_method(p_def, p_A_index, "m2", "p:B", { "p:A", "p:B" });
    YmItemIndex p_B_index = setup_struct(p_def, "B", {});

    ymDm_BindParcelDef(dm, "p", p_def);

    // NOTE: This test covers principle load being of the owner.
    YmItem* p_A = ymCtx_Load(ctx, "p:A"); // The recursive load under test.
    YmItem* p_A_m1 = ymCtx_Load(ctx, "p:A::m1");
    YmItem* p_A_m2 = ymCtx_Load(ctx, "p:A::m2");
    YmItem* p_B = ymCtx_Load(ctx, "p:B");

    ASSERT_NE(p_A, nullptr);
    ASSERT_NE(p_A_m1, nullptr);
    ASSERT_NE(p_A_m2, nullptr);

    test_struct(p_A, "p:A", { p_A_m1, p_A_m2 });
    test_method(p_A_m1, "p:A::m1", { p_A, p_B });
    test_method(p_A_m2, "p:A::m2", { p_A, p_B });
    test_struct(p_B, "p:B", {});

    EXPECT_EQ(ymItem_Owner(p_A), nullptr);
    EXPECT_EQ(ymItem_Owner(p_A_m1), p_A);
    EXPECT_EQ(ymItem_Owner(p_A_m2), p_A);
    EXPECT_EQ(ymItem_Members(p_A), 2);
    EXPECT_EQ(ymItem_Members(p_A_m1), 0);
    EXPECT_EQ(ymItem_Members(p_A_m2), 0);
    EXPECT_EQ(ymItem_MemberByIndex(p_A, 0), p_A_m1);
    EXPECT_EQ(ymItem_MemberByIndex(p_A, 1), p_A_m2);
    EXPECT_EQ(ymItem_MemberByName(p_A, "m1"), p_A_m1);
    EXPECT_EQ(ymItem_MemberByName(p_A, "m2"), p_A_m2);
}

TEST(Loading, OwnersAndTheirMembersAreLoadedTogether_DirectlyLoadMember) {
    // Dep Graph:
    //      p:A     -> p:A::m1      (Member)
    //              -> p:A::m2      (Member)
    //      p:A::m1 -> p:A          (Owner)
    //              -> p:B          (Return Type)
    //      p:A::m2 -> p:A          (Owner)
    //              -> p:B          (Return Type)
    //      p:B                     (This is just for m1/m2 return types.)

    // NOTE: Don't call ymCtx_Load for any item before the initial p:A::m1 load, so that
    //       it's recursive loading of others is properly tested.
    SETUP_ALL(ctx);
    SETUP_PARCELDEF(p_def);

    YmItemIndex p_A_index = setup_struct(p_def, "A", { "p:A::m1", "p:A::m2" });
    YmItemIndex p_A_m1_index = setup_method(p_def, p_A_index, "m1", "p:B", { "p:A", "p:B" });
    YmItemIndex p_A_m2_index = setup_method(p_def, p_A_index, "m2", "p:B", { "p:A", "p:B" });
    YmItemIndex p_B_index = setup_struct(p_def, "B", {});

    ymDm_BindParcelDef(dm, "p", p_def);

    // NOTE: This test covers principle load being of a member.
    YmItem* p_A_m1 = ymCtx_Load(ctx, "p:A::m1"); // The recursive load under test.
    YmItem* p_A = ymCtx_Load(ctx, "p:A");
    YmItem* p_A_m2 = ymCtx_Load(ctx, "p:A::m2");
    YmItem* p_B = ymCtx_Load(ctx, "p:B");

    ASSERT_NE(p_A, nullptr);
    ASSERT_NE(p_A_m1, nullptr);
    ASSERT_NE(p_A_m2, nullptr);

    test_struct(p_A, "p:A", { p_A_m1, p_A_m2 });
    test_method(p_A_m1, "p:A::m1", { p_A, p_B });
    test_method(p_A_m2, "p:A::m2", { p_A, p_B });
    test_struct(p_B, "p:B", {});

    EXPECT_EQ(ymItem_Owner(p_A), nullptr);
    EXPECT_EQ(ymItem_Owner(p_A_m1), p_A);
    EXPECT_EQ(ymItem_Owner(p_A_m2), p_A);
    EXPECT_EQ(ymItem_Members(p_A), 2);
    EXPECT_EQ(ymItem_Members(p_A_m1), 0);
    EXPECT_EQ(ymItem_Members(p_A_m2), 0);
    EXPECT_EQ(ymItem_MemberByIndex(p_A, 0), p_A_m1);
    EXPECT_EQ(ymItem_MemberByIndex(p_A, 1), p_A_m2);
    EXPECT_EQ(ymItem_MemberByName(p_A, "m1"), p_A_m1);
    EXPECT_EQ(ymItem_MemberByName(p_A, "m2"), p_A_m2);
}

TEST(Loading, Self) {
    static_assert(YmKind_Num == 4);
    // NOTE: This test covers (ideally) ALL places in ymParcelDef_*** API where a reference
    //       symbol can be injected, and tests that they can interpret the direct loading
    //       of 'Self' correctly. Also, this tests that 'Self' is interpreted correctly
    //       in protocols and non-protocols alike.
    // NOTE: yama/special/protocol-conformance.cpp is responsible for testing that 'Self'
    //       operates as expected w/ regards to protocol conformance checking.
    SETUP_ALL(ctx);
    SETUP_PARCELDEF(p_def);

    auto STRUCT_index = setup_struct(p_def, "STRUCT", {});
    auto STRUCT_ref = ymParcelDef_AddRef(p_def, STRUCT_index, "Self"); // Covers ref for this kind.
    auto STRUCT_m_index = setup_method(p_def, STRUCT_index, "m", "Self", {}); // Covers method kind.
    ymParcelDef_AddParam(p_def, STRUCT_m_index, "a", "Self");
    auto STRUCT_m_ref = ymParcelDef_AddRef(p_def, STRUCT_m_index, "Self"); // Covers ref for this kind.
    
    auto PROTOCOL_index = setup_protocol(p_def, "PROTOCOL", {});
    auto PROTOCOL_ref = ymParcelDef_AddRef(p_def, PROTOCOL_index, "Self"); // Covers ref for this kind.
    auto PROTOCOL_m_index = ymParcelDef_AddMethodReq(p_def, PROTOCOL_index, "m", "Self");
    ymParcelDef_AddParam(p_def, PROTOCOL_m_index, "a", "Self");
    auto PROTOCOL_m_ref = ymParcelDef_AddRef(p_def, PROTOCOL_m_index, "Self"); // Covers ref for this kind.
    
    auto FN_index = setup_fn(p_def, "FN", "Self", {});
    ymParcelDef_AddParam(p_def, FN_index, "a", "Self");
    auto FN_ref = ymParcelDef_AddRef(p_def, FN_index, "Self"); // Covers ref for this kind.

    ymDm_BindParcelDef(dm, "p", p_def);
    auto STRUCT = ymCtx_Load(ctx, "p:STRUCT");
    auto STRUCT_m = ymCtx_Load(ctx, "p:STRUCT::m");
    auto PROTOCOL = ymCtx_Load(ctx, "p:PROTOCOL");
    auto PROTOCOL_m = ymCtx_Load(ctx, "p:PROTOCOL::m");
    auto FN = ymCtx_Load(ctx, "p:FN");
    ASSERT_TRUE(STRUCT);
    ASSERT_TRUE(STRUCT_m);
    ASSERT_TRUE(PROTOCOL);
    ASSERT_TRUE(PROTOCOL_m);
    ASSERT_TRUE(FN);

    EXPECT_EQ(ymItem_ReturnType(STRUCT_m), STRUCT);
    EXPECT_EQ(ymItem_ReturnType(PROTOCOL_m), PROTOCOL);
    EXPECT_EQ(ymItem_ReturnType(FN), FN);

    ASSERT_EQ(ymItem_Params(STRUCT_m), 1);
    ASSERT_EQ(ymItem_Params(PROTOCOL_m), 1);
    ASSERT_EQ(ymItem_Params(FN), 1);

    EXPECT_EQ(ymItem_ParamType(STRUCT_m, 0), STRUCT);
    EXPECT_EQ(ymItem_ParamType(PROTOCOL_m, 0), PROTOCOL);
    EXPECT_EQ(ymItem_ParamType(FN, 0), FN);

    EXPECT_EQ(ymItem_Ref(STRUCT, STRUCT_ref), STRUCT);
    EXPECT_EQ(ymItem_Ref(STRUCT_m, STRUCT_m_ref), STRUCT);
    EXPECT_EQ(ymItem_Ref(PROTOCOL, PROTOCOL_ref), PROTOCOL);
    EXPECT_EQ(ymItem_Ref(PROTOCOL_m, PROTOCOL_m_ref), PROTOCOL);
    EXPECT_EQ(ymItem_Ref(FN, FN_ref), FN);
}

TEST(Loading, Fail_IllegalFullname_DirectLoad) {
    for (const auto& fullname : illegalFullnames) {
        SETUP_ALL(ctx);
        EXPECT_EQ(ymCtx_Load(ctx, fullname.c_str()), nullptr)
            << "fullname == \"" << fullname << "\"";
        EXPECT_EQ(err[YmErrCode_IllegalFullname], 1);
    }
}

TEST(Loading, Fail_IllegalRefSym_IndirectLoad) {
    // NOTE: ymParcelDef_AddRef will cover checking fullname syntax for indirect loading.
}

TEST(Loading, Fail_ParcelNotFound_DirectLoad) {
    // Dep Graph:
    //      p:A                     (p and p:A doesn't exist.)

    // NOTE: Don't call ymCtx_Load for any item before the initial p:A load, so that
    //       it's recursive loading of others is properly tested.
    SETUP_ALL(ctx);
    EXPECT_EQ(ymCtx_Load(ctx, "p:A"), nullptr);
    EXPECT_EQ(err[YmErrCode_ParcelNotFound], 1);
}

TEST(Loading, Fail_ParcelNotFound_IndirectLoad) {
    // Dep Graph:
    //      p:A -> q:A              (q and q:A doesn't exist.)

    // NOTE: Don't call ymCtx_Load for any item before the initial p:A load, so that
    //       it's recursive loading of others is properly tested.
    SETUP_ALL(ctx);
    SETUP_PARCELDEF(p_def);

    YmItemIndex p_A_index = setup_struct(p_def, "A", { "q:A" });

    ymDm_BindParcelDef(dm, "p", p_def);

    EXPECT_EQ(ymCtx_Load(ctx, "p:A"), nullptr);
    EXPECT_EQ(err[YmErrCode_ParcelNotFound], 1);
}

TEST(Loading, Fail_ItemNotFound_DirectLoad) {
    // Dep Graph:
    //      p:A                     (p does exist, but p:A doesn't.)

    // NOTE: Don't call ymCtx_Load for any item before the initial p:A load, so that
    //       it's recursive loading of others is properly tested.
    SETUP_ALL(ctx);
    SETUP_PARCELDEF(p_def);
    
    ymDm_BindParcelDef(dm, "p", p_def);

    EXPECT_EQ(ymCtx_Load(ctx, "p:A"), nullptr);
    EXPECT_EQ(err[YmErrCode_ItemNotFound], 1);
}

TEST(Loading, Fail_ItemNotFound_IndirectLoad) {
    // Dep Graph:
    //      p:A -> q:A              (q does exist, but q:A doesn't.)

    // NOTE: Don't call ymCtx_Load for any item before the initial p:A load, so that
    //       it's recursive loading of others is properly tested.
    SETUP_ALL(ctx);
    SETUP_PARCELDEF(p_def);
    SETUP_PARCELDEF(q_def);

    YmItemIndex p_A_index = setup_struct(p_def, "A", { "q:A" });
    
    ymDm_BindParcelDef(dm, "p", p_def);
    ymDm_BindParcelDef(dm, "q", q_def);

    EXPECT_EQ(ymCtx_Load(ctx, "p:A"), nullptr);
    EXPECT_EQ(err[YmErrCode_ItemNotFound], 1);
}

