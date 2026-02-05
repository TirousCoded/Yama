

#include "loading-helpers.h"


// TODO: Below, our little semi-formal language of 'Generics:' and 'Dep Graph:' is alright,
//       but as generics were added it has become increasingly *loose* in terms of how comprehensive
//       it is.
//
//       At some point we'd be best to figure out what our priorities regarding it are, and try
//       to revise it to be more uniform and consistent in terms of comprehensiveness.


TEST(Loading, WorksWithAllItemKinds) {
    static_assert(YmKind_Num == 4);
    // Dep Graph:
    //      p:A                     (Struct)
    //      p:B                     (Protocol)
    //      p:C     -> p:Int        (Function)
    //      p:A::m  -> p:A          (Method)
    //              -> p:Int
    //      p:B::m  -> p:B          (Method Req.)
    //              -> p:Int
    //
    //      p:Int
    SETUP_ALL(ctx);
    SETUP_PARCELDEF(p_def);

    YmItemIndex Int_index = setup_struct(p_def, "Int", {});

    YmItemIndex A_index = setup_struct(p_def, "A", {});
    YmItemIndex B_index = setup_protocol(p_def, "B", {});
    YmItemIndex C_index = setup_fn(p_def, "C", "p:Int", {});
    YmItemIndex A_m_index = setup_method(p_def, A_index, "m", "p:Int", {});
    YmItemIndex B_m_index = ymParcelDef_AddMethodReq(p_def, B_index, "m", "p:Int");

    ymDm_BindParcelDef(dm, "p", p_def);

    YmItem* A = load(ctx, "p:A");
    YmItem* B = load(ctx, "p:B");
    YmItem* C = load(ctx, "p:C");
    YmItem* A_m = load(ctx, "p:A::m");
    YmItem* B_m = load(ctx, "p:B::m");
    YmItem* Int = load(ctx, "p:Int");

    test_struct(A, "p:A", { A_m });
    test_protocol(B, "p:B", { B_m });
    test_fn(C, "p:C", { Int });
    test_method(A_m, "p:A::m", { A, Int });
    test_method(B_m, "p:B::m", { B, Int });
}

TEST(Loading, WorksWithAllItemKinds_Generics) {
    static_assert(YmKind_Num == 4);
    // Generics:
    //      A[X: Any]
    //      B[X: Any]
    //      C[X: Any]() -> X
    //      A[X: Any]::m() -> X
    //      B[X: Any]::m() -> X
    // 
    // Dep Graph:
    //      p:A[X]          -> X            (Struct)
    //      p:B[X]          -> X            (Protocol)
    //      p:C[X]          -> X            (Function)
    //      p:A[X]::m       -> p:A[X]       (Method)
    //                      -> X
    //      p:B[X]::m       -> p:B[X]       (Method Req.)
    //                      -> X
    //
    //      p:A[p:Int]      -> p:Int
    //      p:B[p:Int]      -> p:Int
    //      p:C[p:Int]      -> p:Int
    //      p:A[p:Int]::m   -> p:A[p:Int]
    //                      -> p:Int
    //      p:B[p:Int]::m   -> p:B[p:Int]
    //                      -> p:Int
    SETUP_ALL(ctx);
    SETUP_PARCELDEF(p_def);

    YmItemIndex Any_index = setup_protocol(p_def, "Any", {});
    YmItemIndex Int_index = setup_struct(p_def, "Int", {});

    YmItemIndex A_index = setup_struct(p_def, "A", {}, { { "X", "p:Any" } });
    YmItemIndex B_index = setup_protocol(p_def, "B", {}, { { "X", "p:Any" } });
    YmItemIndex C_index = setup_fn(p_def, "C", "p:Int", {}, { { "X", "p:Any" } });
    YmItemIndex A_m_index = setup_method(p_def, A_index, "m", "p:Int", {});
    YmItemIndex B_m_index = ymParcelDef_AddMethodReq(p_def, B_index, "m", "p:Int");

    ymDm_BindParcelDef(dm, "p", p_def);

    YmItem* A_Int = load(ctx, "p:A[p:Int]");
    YmItem* B_Int = load(ctx, "p:B[p:Int]");
    YmItem* C_Int = load(ctx, "p:C[p:Int]");
    YmItem* A_Int_m = load(ctx, "p:A[p:Int]::m");
    YmItem* B_Int_m = load(ctx, "p:B[p:Int]::m");
    YmItem* Int = load(ctx, "p:Int");

    test_struct(A_Int, "p:A[p:Int]", {}, { Int });
    test_protocol(B_Int, "p:B[p:Int]", {}, { Int });
    test_fn(C_Int, "p:C[p:Int]", {}, { Int });
    test_method(A_Int_m, "p:A[p:Int]::m", { A_Int }, { Int });
    test_method(B_Int_m, "p:B[p:Int]::m", { B_Int }, { Int });
}

TEST(Loading, NoRefs) {
    // Dep Graph:
    //      p:A

    // NOTE: Don't call ymCtx_Load for any item before the initial p:A load, so that
    //       it's recursive loading of others is properly tested.
    SETUP_ALL(ctx);
    SETUP_PARCELDEF(p_def);

    YmItemIndex p_A_index = setup_struct(p_def, "A", {});

    ymDm_BindParcelDef(dm, "p", p_def);
    
    YmItem* p_A = load(ctx, "p:A"); // The recursive load under test.

    test_struct(p_A, "p:A", {});
}

TEST(Loading, Refs) {
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
    
    YmItem* p_A = load(ctx, "p:A"); // The recursive load under test.
    YmItem* p_B = load(ctx, "p:B");
    YmItem* p_C = load(ctx, "p:C");

    test_struct(p_A, "p:A", { p_B, p_C });
    test_struct(p_B, "p:B", {});
    test_struct(p_C, "p:C", {});
}

TEST(Loading, MultipleLayersOfIndirectRefs) {
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
    
    YmItem* p_A = load(ctx, "p:A"); // The recursive load under test.
    YmItem* p_B = load(ctx, "p:B");
    YmItem* p_C = load(ctx, "p:C");
    YmItem* p_D = load(ctx, "p:D");
    YmItem* p_E = load(ctx, "p:E");
    YmItem* p_F = load(ctx, "p:F");

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
    
    YmItem* p_A = load(ctx, "p:A"); // The recursive load under test.
    YmItem* p_B = load(ctx, "p:B");
    YmItem* p_C = load(ctx, "p:C");
    YmItem* p_D = load(ctx, "p:D");

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
    
    YmItem* p_A = load(ctx, "p:A"); // The recursive load under test.
    YmItem* p_B = load(ctx, "p:B");

    test_struct(p_A, "p:A", { p_B, p_A });
    test_struct(p_B, "p:B", { p_A });
}

TEST(Loading, DepGraphCycle_ArisingDueToSpecificTypeArgUsed) {
    // Generics:
    //      T[X: Any]
    //
    // Dep Graph:
    //      p:T[X]      -> X
    //
    //      p:A         -> p:T[p:A] -> p:A      (back ref)
    // 
    //      p:T[p:A]    -> p:A      -> p:T[p:A] (back ref)

    // NOTE: Don't call ymCtx_Load for any item before the initial p:T[~] load, so that
    //       it's recursive loading of others is properly tested.
    SETUP_ALL(ctx);
    SETUP_PARCELDEF(p_def);

    auto Any_index = setup_protocol(p_def, "Any", {});

    auto A_index = setup_struct(p_def, "A", { "p:T[p:A]" });

    auto T_index = setup_struct(p_def, "T", {}, { { "X", "p:Any" } });

    ymDm_BindParcelDef(dm, "p", p_def);

    auto T_A = load(ctx, "p:T[p:A]");
    auto A = load(ctx, "p:A");

    test_struct(T_A, "p:T[p:A]", {}, { A });
    test_struct(A, "p:A", { T_A });
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
    
    YmItem* p_A = load(ctx, "p:A"); // The recursive load under test.
    YmItem* p_B = load(ctx, "p:B");
    YmItem* q_A = load(ctx, "q:A");

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

    YmItem* p_A = load(ctx, "p:A"); // The recursive load under test.

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

    YmItem* p_A = load(ctx, "p:A"); // The recursive load under test.
    YmItem* q_A = load(ctx, "q:A");

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
    YmItem* p_A = load(ctx, "p:A"); // The recursive load under test.
    YmItem* p_A_m1 = load(ctx, "p:A::m1");
    YmItem* p_A_m2 = load(ctx, "p:A::m2");
    YmItem* p_B = load(ctx, "p:B");

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
    YmItem* p_A_m1 = load(ctx, "p:A::m1"); // The recursive load under test.
    YmItem* p_A = load(ctx, "p:A");
    YmItem* p_A_m2 = load(ctx, "p:A::m2");
    YmItem* p_B = load(ctx, "p:B");

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

TEST(Loading, OwnersAndTheirMembersAreLoadedTogether_Generics) {
    // Generics:
    //      T[X: Any]
    //      T[X: Any]::m1() -> X
    //      T[X: Any]::m2(X) -> Int
    // 
    // Dep Graph:
    //      p:T[X]      -> X
    //                  -> p:T[X]::m1   -> p:T[X]
    //                                  -> X
    //                  -> p:T[X]::m2   -> p:T[X]
    //                                  -> X
    //                                  -> p:Int
    // 
    //      p:T[p:A]    -> p:A
    //                  -> p:T[p:A]::m1 -> p:T[p:A]
    //                                  -> p:A
    //                  -> p:T[p:A]::m2 -> p:T[p:A]
    //                                  -> p:A
    //                                  -> p:Int

    // NOTE: Don't call ymCtx_Load for any item before the initial p:T[~] load, so that
    //       it's recursive loading of others is properly tested.
    SETUP_ALL(ctx);
    SETUP_PARCELDEF(p_def);

    auto Int_index = setup_struct(p_def, "Int", {});
    auto Any_index = setup_protocol(p_def, "Any", {});

    auto T_index = setup_struct(p_def, "T", { "p:T[$X]::m1", "p:T[$X]::m2" }, { { "X", "p:Any" } });
    auto T_m1_index = setup_method(p_def, T_index, "m1", "$X", { "$X" });
    auto T_m2_index = setup_method(p_def, T_index, "m2", "p:Int", { "$X", "p:Int" });

    auto A_index = setup_struct(p_def, "A", {});

    ymDm_BindParcelDef(dm, "p", p_def);

    // NOTE: This test covers principle load being of the owner.
    YmItem* T_A = load(ctx, "p:T[p:A]"); // The recursive load under test.
    YmItem* T_A_m1 = load(ctx, "p:T[p:A]::m1");
    YmItem* T_A_m2 = load(ctx, "p:T[p:A]::m2");
    YmItem* A = load(ctx, "p:A");
    YmItem* Int = load(ctx, "p:Int");

    test_struct(T_A, "p:T[p:A]", { T_A_m1, T_A_m2 }, { A });
    test_method(T_A_m1, "p:T[p:A]::m1", {}, { A });
    test_method(T_A_m2, "p:T[p:A]::m2", { Int }, { A });

    EXPECT_EQ(ymItem_Owner(T_A), nullptr);
    EXPECT_EQ(ymItem_Owner(T_A_m1), T_A);
    EXPECT_EQ(ymItem_Owner(T_A_m2), T_A);
    EXPECT_EQ(ymItem_Members(T_A), 2);
    EXPECT_EQ(ymItem_Members(T_A_m1), 0);
    EXPECT_EQ(ymItem_Members(T_A_m2), 0);
    EXPECT_EQ(ymItem_MemberByIndex(T_A, 0), T_A_m1);
    EXPECT_EQ(ymItem_MemberByIndex(T_A, 1), T_A_m2);
    EXPECT_EQ(ymItem_MemberByName(T_A, "m1"), T_A_m1);
    EXPECT_EQ(ymItem_MemberByName(T_A, "m2"), T_A_m2);
}

TEST(Loading, SelfRef_IncludingRefsWithinMembers) {
    // NOTE: yama/special/protocol-conformance.cpp is responsible for testing that '$Self'
    //       operates as expected w/ regards to protocol conformance checking.
    SETUP_ALL(ctx);
    SETUP_PARCELDEF(p_def);

    // NOTE: Here, struct is for testing non-protocol types generally.

    auto STRUCT_index = setup_struct(p_def, "STRUCT", {});
    auto STRUCT_ref = ymParcelDef_AddRef(p_def, STRUCT_index, "$Self");
    auto STRUCT_m_index = setup_method(p_def, STRUCT_index, "m", "$Self", {}); // Covers method kind.
    ymParcelDef_AddParam(p_def, STRUCT_m_index, "a", "$Self");
    auto STRUCT_m_ref = ymParcelDef_AddRef(p_def, STRUCT_m_index, "$Self");

    auto PROTOCOL_index = setup_protocol(p_def, "PROTOCOL", {});
    auto PROTOCOL_ref = ymParcelDef_AddRef(p_def, PROTOCOL_index, "$Self");
    auto PROTOCOL_m_index = ymParcelDef_AddMethodReq(p_def, PROTOCOL_index, "m", "$Self");
    ymParcelDef_AddParam(p_def, PROTOCOL_m_index, "a", "$Self");
    auto PROTOCOL_m_ref = ymParcelDef_AddRef(p_def, PROTOCOL_m_index, "$Self");

    auto FN_index = setup_fn(p_def, "FN", "$Self", {});
    ymParcelDef_AddParam(p_def, FN_index, "a", "$Self");
    auto FN_ref = ymParcelDef_AddRef(p_def, FN_index, "$Self");

    ymDm_BindParcelDef(dm, "p", p_def);

    auto STRUCT = load(ctx, "p:STRUCT");
    auto STRUCT_m = load(ctx, "p:STRUCT::m");
    auto PROTOCOL = load(ctx, "p:PROTOCOL");
    auto PROTOCOL_m = load(ctx, "p:PROTOCOL::m");
    auto FN = load(ctx, "p:FN");

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

TEST(Loading, ItemArgRef_IncludingRefsWithinMembers) {
    // Generics:
    //      T[X: Any]
    //      T[X: Any]::m() -> X
    //      U[X: Any]
    //
    // Dep Graph:
    //      p:T[X]      -> X
    //                  -> p:T[X]::m        -> X
    //                                      -> p:U[X]
    //                  -> p:U[p:U[X]]
    //
    //      p:U[X]      -> X
    //
    //      p:T[p:A]    -> p:A
    //                  -> p:T[p:A]::m      -> p:A
    //                                      -> p:U[p:A]
    //                  -> p:U[p:U[p:A]]

    // NOTE: Don't call ymCtx_Load for any item before the initial p:T[~] loads, so that
    //       it's recursive loading of others is properly tested.
    SETUP_ALL(ctx);
    SETUP_PARCELDEF(p_def);

    auto Any_index = setup_protocol(p_def, "Any", {});

    auto A_index = setup_struct(p_def, "A", {});

    // NOTE: All the '$X' below are the item arg refs under test.

    auto T_index = setup_struct(p_def, "T", { "p:U[p:U[$X]]" }, { { "X", "p:Any" } });
    auto T_m_index = setup_method(p_def, T_index, "m", "$X", { "p:U[$X]" });

    auto U_index = setup_struct(p_def, "U", {}, { { "X", "p:Any" } });

    ymDm_BindParcelDef(dm, "p", p_def);

    auto T_A = load(ctx, "p:T[p:A]"); // The recursive load under test.
    auto T_A_m = load(ctx, "p:T[p:A]::m"); // The recursive load under test.
    auto A = load(ctx, "p:A");
    auto U_A = load(ctx, "p:U[p:A]");
    auto U_U_A = load(ctx, "p:U[p:U[p:A]]");

    test_struct(T_A, "p:T[p:A]", { T_A_m, U_U_A }, { A });
    test_method(T_A_m, "p:T[p:A]::m", { A, U_A }, { A });
    test_struct(A, "p:A", {});
    test_struct(U_A, "p:U[p:A]", {}, { A });
    test_struct(U_U_A, "p:U[p:U[p:A]]", {}, { U_A });

    EXPECT_EQ(ymItem_ReturnType(T_A_m), A);
}

TEST(Loading, Here) {
    // Dep Graph:
    //      p:A
    //
    //      p:B -> p:A  (p:B exists to indirectly load p:A via %here.)

    // NOTE: Don't call ymCtx_Load for any item before the initial p:B load, so that
    //       it's recursive loading of others is properly tested.
    SETUP_ALL(ctx);
    SETUP_PARCELDEF(p_def);

    auto A_index = setup_struct(p_def, "A", {});
    auto B_index = setup_struct(p_def, "B", {});
    auto B_ref = ymParcelDef_AddRef(p_def, B_index, "%here:A"); // Reference symbol under test.
    ASSERT_NE(B_ref, YM_NO_REF);

    ymDm_BindParcelDef(dm, "p", p_def);

    auto B = load(ctx, "p:B"); // The recursive load under test.
    auto A = load(ctx, "p:A");

    test_struct(B, "p:B", { A });
    test_struct(A, "p:A", {});
    
    EXPECT_EQ(ymItem_Ref(B, B_ref), A); // Ensure %here:A and p:A are the same.
}

TEST(Loading, ItemParams_IncludingForMemberTypes) {
    // Generics:
    //      T[X: P, Y: Q]
    //      T[X: P, Y: Q]::m() -> Int
    // 
    // Dep Graph:
    //      p:T[X, Y]               -> p:T[X, Y]::m     -> p:T[X, Y]
    //                                                  -> X
    //                                                  -> Y
    //                              -> X
    //                              -> Y
    // 
    //      p:T[p:A, p:B]           -> p:T[p:A, p:B]::m -> p:T[p:A, p:B]
    //                                                  -> p:A              (X == p:A)
    //                                                  -> p:B              (Y == p:B)
    //                              -> p:A                                  (X == p:A)
    //                              -> p:B                                  (Y == p:B)
    // 
    //      p:T[p:C, p:D]           -> p:T[p:C, p:D]::m -> p:T[p:C, p:D]
    //                                                  -> p:C              (X == p:C)
    //                                                  -> p:D              (Y == p:D)
    //                              -> p:C                                  (X == p:C)
    //                              -> p:D                                  (Y == p:D)
    // 
    //      p:T[p:C, p:T[p:A, p:B]] -> p:T[~, ~]::m     -> p:T[~, ~]
    //                                                  -> p:C              (X == p:C)
    //                                                  -> p:T[p:A, p:B]    (Y == p:T[p:A, p:B])
    //                              -> p:C                                  (X == p:C)
    //                              -> p:T[p:A, p:B]                        (Y == p:T[p:A, p:B])

    // NOTE: Don't call ymCtx_Load for any item before the initial p:T[~] loads, so that
    //       it's recursive loading of others is properly tested.
    SETUP_ALL(ctx);
    SETUP_PARCELDEF(p_def);

    auto Int_index = setup_struct(p_def, "Int", {});

    auto P_index = setup_protocol(p_def, "P", {});
    auto P_m_index = ymParcelDef_AddMethodReq(p_def, P_index, "m", "p:Int");
    ymParcelDef_AddParam(p_def, P_m_index, "x", "p:Int");

    auto Q_index = setup_protocol(p_def, "Q", {});
    auto Q_m_index = ymParcelDef_AddMethodReq(p_def, Q_index, "m", "p:Int");

    auto T_index = setup_struct(p_def, "T", {}, { { "X", "p:P" }, { "Y", "p:Q" } });
    ymParcelDef_AddMethod(p_def, T_index, "m", "p:Int", ymInertCallBhvrFn, nullptr);

    auto A_index = setup_struct(p_def, "A", {});
    auto A_m_index = ymParcelDef_AddMethod(p_def, A_index, "m", "p:Int", ymInertCallBhvrFn, nullptr);
    ymParcelDef_AddParam(p_def, A_m_index, "x", "p:Int");

    auto B_index = setup_struct(p_def, "B", {});
    auto B_m_index = ymParcelDef_AddMethod(p_def, B_index, "m", "p:Int", ymInertCallBhvrFn, nullptr);

    auto C_index = setup_struct(p_def, "C", {});
    auto C_m_index = ymParcelDef_AddMethod(p_def, C_index, "m", "p:Int", ymInertCallBhvrFn, nullptr);
    ymParcelDef_AddParam(p_def, C_m_index, "x", "p:Int");

    auto D_index = setup_struct(p_def, "D", {});
    auto D_m_index = ymParcelDef_AddMethod(p_def, D_index, "m", "p:Int", ymInertCallBhvrFn, nullptr);

    ymDm_BindParcelDef(dm, "p", p_def);

    auto T_C_T_A_B = load(ctx, "p:T[p:C, p:T[p:A, p:B]]"); // The recursive load under test.
    auto T_C_T_A_B_m = load(ctx, "p:T[p:C, p:T[p:A, p:B]]::m"); // The recursive load under test.
    auto T_A_B = load(ctx, "p:T[p:A, p:B]"); // The recursive load under test.
    auto T_A_B_m = load(ctx, "p:T[p:A, p:B]::m"); // The recursive load under test.
    auto T_C_D = load(ctx, "p:T[p:C, p:D]"); // The recursive load under test.
    auto T_C_D_m = load(ctx, "p:T[p:C, p:D]::m"); // The recursive load under test.
    auto A = load(ctx, "p:A");
    auto B = load(ctx, "p:B");
    auto C = load(ctx, "p:C");
    auto D = load(ctx, "p:D");

    test_struct(T_A_B, "p:T[p:A, p:B]", { T_A_B_m }, { A, B });
    test_method(T_A_B_m, "p:T[p:A, p:B]::m", {}, { A, B });
    test_struct(T_C_D, "p:T[p:C, p:D]", { T_C_D_m }, { C, D });
    test_method(T_C_D_m, "p:T[p:C, p:D]::m", {}, { C, D });
    test_struct(T_C_T_A_B, "p:T[p:C, p:T[p:A, p:B]]", { T_C_T_A_B_m }, { C, T_A_B });
    test_method(T_C_T_A_B_m, "p:T[p:C, p:T[p:A, p:B]]::m", {}, { C, T_A_B });
}

TEST(Loading, ItemParams_ParamsInterReferenceOneAnother) {
    // Generics:
    //      T[X: Any, Y: P[U[X]]]
    // 
    // Any is a top type.
    // 
    // P expects a method 'm() -> X', where P's X will above be 'U[X]'.
    // 
    // U[X] is used instead of just X to test w/ nesting.
    //
    // Dep Graph:
    //      p:T[X, Y]       -> X
    //                      -> Y
    //
    //      p:T[p:A, p:B]   -> p:A  (X == p:A)
    //                      -> p:B  (Y == p:B)

    // NOTE: Don't call ymCtx_Load for any item before the initial p:T[~] loads, so that
    //       it's recursive loading of others is properly tested.
    SETUP_ALL(ctx);
    SETUP_PARCELDEF(p_def);

    auto Any_index = setup_protocol(p_def, "Any", {});

    auto P_index = setup_protocol(p_def, "P", {}, { { "X", "p:Any" } });
    ymParcelDef_AddMethodReq(p_def, P_index, "m", "$X");

    auto U_index = setup_struct(p_def, "U", {}, { { "X", "p:Any" } });

    auto T_index = setup_struct(p_def, "T", {}, { { "X", "p:Any" }, { "Y", "p:P[p:U[$X]]" } });

    auto A_index = setup_struct(p_def, "A", {});

    auto B_index = setup_struct(p_def, "B", {});
    ymParcelDef_AddMethod(p_def, B_index, "m", "p:U[p:A]", ymInertCallBhvrFn, nullptr);

    ymDm_BindParcelDef(dm, "p", p_def);

    auto T_A_B = load(ctx, "p:T[p:A, p:B]"); // The recursive load under test.
    auto A = load(ctx, "p:A");
    auto B = load(ctx, "p:B");

    test_struct(T_A_B, "p:T[p:A, p:B]", {}, { A, B });
}

TEST(Loading, ItemParams_ParamsReferenceSelf) {
    // Generics:
    //      T[X: P[U[Self]]]
    // 
    // P expects a method 'm() -> X', where P's X will above be 'U[Self]'.
    // 
    // U[Self] is used instead of just Self to test w/ nesting.
    //
    // Dep Graph:
    //      p:T[X]      -> X
    //
    //      p:T[p:A]    -> p:A  (X == p:A)

    // NOTE: Don't call ymCtx_Load for any item before the initial p:T[~] loads, so that
    //       it's recursive loading of others is properly tested.
    SETUP_ALL(ctx);
    SETUP_PARCELDEF(p_def);

    auto Any_index = setup_protocol(p_def, "Any", {});

    auto P_index = setup_protocol(p_def, "P", {}, { { "X", "p:Any" } });
    ymParcelDef_AddMethodReq(p_def, P_index, "m", "$X");

    auto U_index = setup_struct(p_def, "U", {}, { { "X", "p:Any" } });

    auto T_index = setup_struct(p_def, "T", {}, { { "X", "p:P[p:U[$Self]]" } });

    auto A_index = setup_struct(p_def, "A", {});
    ymParcelDef_AddMethod(p_def, A_index, "m", "p:U[p:T[p:A]]", ymInertCallBhvrFn, nullptr);

    ymDm_BindParcelDef(dm, "p", p_def);

    auto T_A = load(ctx, "p:T[p:A]"); // The recursive load under test.
    auto A = load(ctx, "p:A");

    test_struct(T_A, "p:T[p:A]", {}, { A });
}

TEST(Loading, ItemParams_IndirectLoad) {
    // Generics:
    //      T[X: P, Y: Q]
    // 
    // Dep Graph:
    //      p:T[X, Y]               -> X
    //                              -> Y
    // 
    //      p:H                     -> p:T[p:A, p:B]
    //                              -> p:T[p:C, p:D]
    //                              -> p:T[p:C, p:T[p:A, p:B]]

    // NOTE: Don't call ymCtx_Load for any item before the initial p:T[~] loads, so that
    //       it's recursive loading of others is properly tested.
    SETUP_ALL(ctx);
    SETUP_PARCELDEF(p_def);

    auto Int_index = setup_struct(p_def, "Int", {});

    auto P_index = setup_protocol(p_def, "P", {});
    auto P_m_index = ymParcelDef_AddMethodReq(p_def, P_index, "m", "p:Int");
    ymParcelDef_AddParam(p_def, P_m_index, "x", "p:Int");

    auto Q_index = setup_protocol(p_def, "Q", {});
    auto Q_m_index = ymParcelDef_AddMethodReq(p_def, Q_index, "m", "p:Int");

    auto H_index = setup_struct(p_def, "H", { "p:T[p:A, p:B]", "p:T[p:C, p:D]", "p:T[p:C, p:T[p:A, p:B]]" });

    auto T_index = setup_struct(p_def, "T", {}, { { "X", "p:P" }, { "Y", "p:Q" } });
    auto T_m_index = ymParcelDef_AddMethod(p_def, T_index, "m", "p:Int", ymInertCallBhvrFn, nullptr);

    auto A_index = setup_struct(p_def, "A", {});
    auto A_m_index = ymParcelDef_AddMethod(p_def, A_index, "m", "p:Int", ymInertCallBhvrFn, nullptr);
    ymParcelDef_AddParam(p_def, A_m_index, "x", "p:Int");

    auto B_index = setup_struct(p_def, "B", {});
    auto B_m_index = ymParcelDef_AddMethod(p_def, B_index, "m", "p:Int", ymInertCallBhvrFn, nullptr);

    auto C_index = setup_struct(p_def, "C", {});
    auto C_m_index = ymParcelDef_AddMethod(p_def, C_index, "m", "p:Int", ymInertCallBhvrFn, nullptr);
    ymParcelDef_AddParam(p_def, C_m_index, "x", "p:Int");

    auto D_index = setup_struct(p_def, "D", {});
    auto D_m_index = ymParcelDef_AddMethod(p_def, D_index, "m", "p:Int", ymInertCallBhvrFn, nullptr);

    ymDm_BindParcelDef(dm, "p", p_def);

    auto H = load(ctx, "p:H"); // The recursive load under test.
    auto T_C_T_A_B = load(ctx, "p:T[p:C, p:T[p:A, p:B]]");
    auto T_A_B = load(ctx, "p:T[p:A, p:B]");
    auto T_C_D = load(ctx, "p:T[p:C, p:D]");
    auto A = load(ctx, "p:A");
    auto B = load(ctx, "p:B");
    auto C = load(ctx, "p:C");
    auto D = load(ctx, "p:D");

    test_struct(H, "p:H", { T_A_B, T_C_D, T_C_T_A_B });
    test_struct(T_A_B, "p:T[p:A, p:B]", {}, { A, B });
    test_struct(T_C_D, "p:T[p:C, p:D]", {}, { C, D });
    test_struct(T_C_T_A_B, "p:T[p:C, p:T[p:A, p:B]]", {}, { C, T_A_B });
}

TEST(Loading, ItemParams_RecursiveConstraint_ForGenericProtocol) {
    // Generics:
    //      P[T: Self]                              (Test via $Self.)
    //      Q[T: Q[T]]                              (Test via explicit.)
    // 
    // Dep Graph:
    //      p:P[T]                  -> T
    //      p:Q[T]                  -> T
    // 
    //      p:P[p:A]                -> p:P[p:A]     (back ref)
    //                              -> p:A
    //      p:Q[p:A]                -> p:Q[p:A]     (back ref)
    //                              -> p:A

    // NOTE: Don't call ymCtx_Load for any item before the initial p:P[p:A] loads, so that
    //       it's recursive loading of others is properly tested.
    SETUP_ALL(ctx);
    SETUP_PARCELDEF(p_def);

    auto None_index = setup_struct(p_def, "None", {});

    // A conforms to P and Q.
    auto A_index = setup_struct(p_def, "A", {});
    ymParcelDef_AddMethod(p_def, A_index, "m", "p:None", ymInertCallBhvrFn, nullptr);

    // B doesn't conform to P and Q.
    auto B_index = setup_struct(p_def, "B", {});

    auto P_index = setup_protocol(p_def, "P", {}, { { "T", "$Self" } });
    ymParcelDef_AddMethodReq(p_def, P_index, "m", "p:None");

    auto Q_index = setup_protocol(p_def, "Q", {}, { { "T", "p:Q[$T]" } });
    ymParcelDef_AddMethodReq(p_def, Q_index, "m", "p:None");

    ymDm_BindParcelDef(dm, "p", p_def);

    auto P_A = load(ctx, "p:P[p:A]"); // The recursive load under test.
    auto Q_A = load(ctx, "p:Q[p:A]"); // The recursive load under test.
    auto A = load(ctx, "p:A");
    auto B = load(ctx, "p:B");

    test_protocol(P_A, "p:P[p:A]", {}, { A });
    test_protocol(Q_A, "p:Q[p:A]", {}, { A });

    EXPECT_EQ(ymItem_Converts(B, P_A, true), YM_FALSE);
    EXPECT_EQ(ymItem_Converts(B, Q_A, true), YM_FALSE);
}

TEST(Loading, ItemParams_RecursiveConstraint_ForGenericNonProtocol_Illegal) {
    // Generics:
    //      A[T: Self]                              (Test via $Self.)
    //      B[T: B[T]]                              (Test via explicit.)
    // 
    // Dep Graph:
    //      p:A[T]                  -> T
    //      p:B[T]                  -> T
    // 
    //      p:A[p:C]                -> p:A[p:C]     (back ref)
    //                              -> p:C
    //      p:B[p:C]                -> p:B[p:C]     (back ref)
    //                              -> p:C
    SETUP_ALL(ctx);
    SETUP_PARCELDEF(p_def);

    auto A_index = setup_struct(p_def, "A", {}, { { "T", "$Self" } });
    auto B_index = setup_struct(p_def, "B", {}, { { "T", "p:B[$T]" } });
    auto C_index = setup_struct(p_def, "C", {});

    ymDm_BindParcelDef(dm, "p", p_def);

    EXPECT_EQ(ymCtx_Load(ctx, "p:A[p:C]"), nullptr);
    EXPECT_EQ(err[YmErrCode_NonProtocolItem], 1);
    
    err.reset();

    EXPECT_EQ(ymCtx_Load(ctx, "p:B[p:C]"), nullptr);
    EXPECT_EQ(err[YmErrCode_NonProtocolItem], 1);
}

TEST(Loading, MemberAccess) {
    SETUP_ALL(ctx);
    SETUP_PARCELDEF(p_def);

    setup_protocol(p_def, "Any", {});

    auto A_index = setup_struct(p_def, "A", {}, {});
    setup_method(p_def, A_index, "m", "p:A", {});
    auto A_m_ref = ymParcelDef_AddRef(p_def, A_index, "p:A::m");
    auto B_A_m_ref = ymParcelDef_AddRef(p_def, A_index, "p:B[p:A]::m");

    auto B_index = setup_struct(p_def, "B", {}, { { "T", "p:Any" } });
    setup_method(p_def, B_index, "m", "p:A", {});

    ymDm_BindParcelDef(dm, "p", p_def);

    auto A = load(ctx, "p:A");
    auto A_m = load(ctx, "p:A::m");
    auto B_A_m = load(ctx, "p:B[p:A]::m");

    EXPECT_EQ(ymItem_Ref(A, A_m_ref), A_m);
    EXPECT_EQ(ymItem_Ref(A, B_A_m_ref), B_A_m);
}

TEST(Loading, MemberAccess_SelfAndItemParamRefs) {
    SETUP_ALL(ctx);
    SETUP_PARCELDEF(p_def);

    setup_protocol(p_def, "Any", {});

    auto A_index = setup_struct(p_def, "A", {}, { { "T", "p:Any" } });
    setup_method(p_def, A_index, "m1", "p:B", {});
    auto A_m1_ref = ymParcelDef_AddRef(p_def, A_index, "$Self::m1");
    auto B_m2_ref = ymParcelDef_AddRef(p_def, A_index, "$T::m2");

    auto B_index = setup_struct(p_def, "B", {});
    setup_method(p_def, B_index, "m2", "p:B", {});

    ymDm_BindParcelDef(dm, "p", p_def);

    auto A_B = load(ctx, "p:A[p:B]");
    auto A_B_m1 = load(ctx, "p:A[p:B]::m1");
    auto B = load(ctx, "p:B");
    auto B_m2 = load(ctx, "p:B::m2");

    EXPECT_EQ(ymItem_Ref(A_B, A_m1_ref), A_B_m1);
    EXPECT_EQ(ymItem_Ref(A_B, B_m2_ref), B_m2);
}

// TODO: What about situation where callsig load fails due to attempt to use callsig w/
//       a GENERIC type W/OUT item arguments? Our tests don't cover things like that.

TEST(Loading, CallSigs_DirectLoad) {
    SETUP_ALL(ctx);
    SETUP_PARCELDEF(p_def);

    setup_protocol(p_def, "Any", {});
    setup_struct(p_def, "A", {});
    setup_struct(p_def, "B", {});
    
    auto f_ind = setup_fn(p_def, "f", "p:A", {});
    ymParcelDef_AddParam(p_def, f_ind, "a", "p:A");
    
    auto g_ind = setup_fn(p_def, "g", "$U", {}, { { "T", "p:Any" }, { "U", "p:Any" } });
    ymParcelDef_AddParam(p_def, g_ind, "a", "$T");

    ymDm_BindParcelDef(dm, "p", p_def);

    EXPECT_EQ(load(ctx, "p:f(p:A) -> p:A"), load(ctx, "p:f"));
    EXPECT_EQ(load(ctx, "p:g[p:A, p:A](p:A) -> p:A"), load(ctx, "p:g[p:A, p:A]"));
    EXPECT_EQ(load(ctx, "p:g[p:B, p:A](p:B) -> p:A"), load(ctx, "p:g[p:B, p:A]"));
    EXPECT_EQ(load(ctx, "p:g[p:A, p:B](p:A) -> p:B"), load(ctx, "p:g[p:A, p:B]"));

    err.reset();

    // NOTE: The ONLY thing preventing below loads from succeeding is the fact that
    //       they're callsigs, nothing else.

    EXPECT_EQ(ymCtx_Load(ctx, "p:f(p:B) -> p:A"), nullptr);
    EXPECT_EQ(ymCtx_Load(ctx, "p:f(p:A) -> p:B"), nullptr);
    EXPECT_EQ(ymCtx_Load(ctx, "p:f(p:A, p:A) -> p:A"), nullptr);
    EXPECT_EQ(ymCtx_Load(ctx, "p:f() -> p:A"), nullptr);
    EXPECT_EQ(err[YmErrCode_ItemNotFound], 4);

    err.reset();

    EXPECT_EQ(ymCtx_Load(ctx, "p:g[p:A, p:A](p:B) -> p:A"), nullptr);
    EXPECT_EQ(ymCtx_Load(ctx, "p:g[p:A, p:A](p:A) -> p:B"), nullptr);
    EXPECT_EQ(ymCtx_Load(ctx, "p:g[p:A, p:A](p:A, p:A) -> p:A"), nullptr);
    EXPECT_EQ(ymCtx_Load(ctx, "p:g[p:A, p:A]() -> p:A"), nullptr);
    EXPECT_EQ(err[YmErrCode_ItemNotFound], 4);

    err.reset();

    EXPECT_EQ(ymCtx_Load(ctx, "p:g[p:B, p:A](p:A) -> p:A"), nullptr);
    EXPECT_EQ(ymCtx_Load(ctx, "p:g[p:B, p:A](p:B) -> p:B"), nullptr);
    EXPECT_EQ(ymCtx_Load(ctx, "p:g[p:B, p:A](p:B, p:B) -> p:A"), nullptr);
    EXPECT_EQ(ymCtx_Load(ctx, "p:g[p:B, p:A]() -> p:A"), nullptr);
    EXPECT_EQ(err[YmErrCode_ItemNotFound], 4);

    err.reset();

    EXPECT_EQ(ymCtx_Load(ctx, "p:g[p:A, p:B](p:B) -> p:B"), nullptr);
    EXPECT_EQ(ymCtx_Load(ctx, "p:g[p:A, p:B](p:A) -> p:A"), nullptr);
    EXPECT_EQ(ymCtx_Load(ctx, "p:g[p:A, p:B](p:A, p:A) -> p:B"), nullptr);
    EXPECT_EQ(ymCtx_Load(ctx, "p:g[p:A, p:B]() -> p:B"), nullptr);
    EXPECT_EQ(err[YmErrCode_ItemNotFound], 4);
}

TEST(Loading, CallSigs_IndirectLoad) {
    SETUP_ALL(ctx);
    SETUP_PARCELDEF(p_def);

    setup_protocol(p_def, "Any", {});
    setup_struct(p_def, "A", {});
    setup_struct(p_def, "B", {});

    auto f_ind = setup_fn(p_def, "f", "p:A", {});
    ymParcelDef_AddParam(p_def, f_ind, "a", "p:A");

    auto g_ind = setup_fn(p_def, "g", "$U", {}, { { "T", "p:Any" }, { "U", "p:Any" } });
    ymParcelDef_AddParam(p_def, g_ind, "a", "$T");

    auto SUCC_ind = setup_struct(p_def, "SUCC", {});
    auto SUCC_ref1 = ymParcelDef_AddRef(p_def, SUCC_ind, "p:f(p:A) -> p:A");
    auto SUCC_ref2 = ymParcelDef_AddRef(p_def, SUCC_ind, "p:g[p:A, p:A](p:A) -> p:A");
    auto SUCC_ref3 = ymParcelDef_AddRef(p_def, SUCC_ind, "p:g[p:B, p:A](p:B) -> p:A");
    auto SUCC_ref4 = ymParcelDef_AddRef(p_def, SUCC_ind, "p:g[p:A, p:B](p:A) -> p:B");

    // TODO: Currently, as shown below, our failure tests for indirect loads via callsig only
    //       really tests on case, and *presumes* that the more thorough failure testing of
    //       direct loads via callsig holds for indirect ones.
    //
    //       In the future we may want to improve this about our unit tests.

    auto FAIL_ind = setup_struct(p_def, "FAIL", { "p:f(p:B) -> p:A" });

    ymDm_BindParcelDef(dm, "p", p_def);

    auto SUCC = load(ctx, "p:SUCC");
    EXPECT_EQ(ymItem_Ref(SUCC, SUCC_ref1), load(ctx, "p:f"));
    EXPECT_EQ(ymItem_Ref(SUCC, SUCC_ref2), load(ctx, "p:g[p:A, p:A]"));
    EXPECT_EQ(ymItem_Ref(SUCC, SUCC_ref3), load(ctx, "p:g[p:B, p:A]"));
    EXPECT_EQ(ymItem_Ref(SUCC, SUCC_ref4), load(ctx, "p:g[p:A, p:B]"));

    EXPECT_FALSE(ymCtx_Load(ctx, "p:FAIL"));
    EXPECT_EQ(err[YmErrCode_ItemNotFound], 1);
}

TEST(Loading, Fail_IllegalSpecifier_DirectLoad) {
    for (const auto& fullname : illegalFullnames) {
        SETUP_ALL(ctx);
        EXPECT_EQ(ymCtx_Load(ctx, fullname.c_str()), nullptr)
            << "fullname == \"" << fullname << "\"";
        EXPECT_EQ(err[YmErrCode_IllegalSpecifier], 1);
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

    setup_struct(p_def, "A", { "q:A" });
    
    ymDm_BindParcelDef(dm, "p", p_def);
    ymDm_BindParcelDef(dm, "q", q_def);

    EXPECT_EQ(ymCtx_Load(ctx, "p:A"), nullptr);
    EXPECT_EQ(err[YmErrCode_ItemNotFound], 1);
}

TEST(Loading, Fail_ConcreteItem_ArgPackOnConcreteType) {
    SETUP_ALL(ctx);
    SETUP_PARCELDEF(p_def);

    setup_struct(p_def, "Int", {});
    setup_protocol(p_def, "Any", {});

    setup_struct(p_def, "A", {});

    ymDm_BindParcelDef(dm, "p", p_def);

    EXPECT_EQ(ymCtx_Load(ctx, "p:A[p:Int]"), nullptr);
    EXPECT_EQ(err[YmErrCode_ConcreteItem], 1);
}

TEST(Loading, Fail_GenericItem_ArgPackMissing) {
    SETUP_ALL(ctx);
    SETUP_PARCELDEF(p_def);

    setup_protocol(p_def, "Any", {});

    setup_struct(p_def, "A", {}, { { "X", "p:Any" } });

    ymDm_BindParcelDef(dm, "p", p_def);

    EXPECT_EQ(ymCtx_Load(ctx, "p:A"), nullptr);
    EXPECT_EQ(err[YmErrCode_GenericItem], 1);
}

TEST(Loading, Fail_GenericItem_ArgPackMissing_AndAttemptedMemberAccess) {
    SETUP_ALL(ctx);
    SETUP_PARCELDEF(p_def);

    setup_struct(p_def, "Int", {});
    setup_protocol(p_def, "Any", {});

    auto A_index = setup_struct(p_def, "A", {}, { { "X", "p:Any" } });
    setup_method(p_def, A_index, "m", "p:Int", {});

    ymDm_BindParcelDef(dm, "p", p_def);

    EXPECT_EQ(ymCtx_Load(ctx, "p:A::m"), nullptr);
    EXPECT_EQ(err[YmErrCode_GenericItem], 1);
}

TEST(Loading, Fail_ItemArgsError_ArgDoesntConformToConstraint) {
    // Generics:
    //      T[X: P]
    // 
    // Dep Graph:
    //      p:T[X]      -> X
    // 
    //      p:T[p:A]    -> p:A      (X == p:A; but p:A doesn't conform to p:P.)

    // NOTE: Don't call ymCtx_Load for any item before the initial p:T[~] load, so that
    //       it's recursive loading of others is properly tested.
    SETUP_ALL(ctx);
    SETUP_PARCELDEF(p_def);

    auto Int_index = setup_struct(p_def, "Int", {});

    auto P_index = setup_protocol(p_def, "P", {});
    auto P_m_index = ymParcelDef_AddMethodReq(p_def, P_index, "m", "p:Int");
    ymParcelDef_AddParam(p_def, P_m_index, "x", "p:Int");

    setup_struct(p_def, "T", {}, { { "X", "p:P" } });

    auto A_index = setup_struct(p_def, "A", {});
    auto A_m_index = ymParcelDef_AddMethod(p_def, A_index, "m", "p:Int", ymInertCallBhvrFn, nullptr);
    //ymParcelDef_AddParam(p_def, A_m_index, "x", "p:Int");

    ymDm_BindParcelDef(dm, "p", p_def);

    EXPECT_EQ(ymCtx_Load(ctx, "p:T[p:A]"), nullptr);
    EXPECT_EQ(err[YmErrCode_ItemArgsError], 1);
}

TEST(Loading, Fail_ItemArgsError_TooManyArgs) {
    SETUP_ALL(ctx);
    SETUP_PARCELDEF(p_def);

    setup_protocol(p_def, "Any", {});
    setup_struct(p_def, "Int", {});

    setup_struct(p_def, "A", {}, { { "X", "p:Any" } });

    ymDm_BindParcelDef(dm, "p", p_def);

    EXPECT_EQ(ymCtx_Load(ctx, "p:A[p:Int, p:Int]"), nullptr);
    EXPECT_EQ(err[YmErrCode_ItemArgsError], 1);
}

TEST(Loading, Fail_ItemArgsError_TooFewArgs) {
    SETUP_ALL(ctx);
    SETUP_PARCELDEF(p_def);

    setup_protocol(p_def, "Any", {});
    setup_struct(p_def, "Int", {});

    setup_struct(p_def, "A", {}, { { "X", "p:Any" }, { "Y", "p:Any" } });

    ymDm_BindParcelDef(dm, "p", p_def);

    EXPECT_EQ(ymCtx_Load(ctx, "p:A[p:Int]"), nullptr);
    EXPECT_EQ(err[YmErrCode_ItemArgsError], 1);
}

TEST(Loading, Fail_ItemArgsError_EmptyArgPack) {
    SETUP_ALL(ctx);
    SETUP_PARCELDEF(p_def);

    setup_protocol(p_def, "Any", {});
    setup_struct(p_def, "Int", {});

    setup_struct(p_def, "A", {}, { { "X", "p:Any" } });

    ymDm_BindParcelDef(dm, "p", p_def);

    EXPECT_EQ(ymCtx_Load(ctx, "p:A[]"), nullptr);
    EXPECT_EQ(err[YmErrCode_ItemArgsError], 1);
}

TEST(Loading, Fail_ItemNotFound_SelfRefUnavailableForDirectLoads) {
    SETUP_ALL(ctx);
    SETUP_PARCELDEF(p_def);

    ymDm_BindParcelDef(dm, "p", p_def);

    EXPECT_EQ(ymCtx_Load(ctx, "$Self"), nullptr);
    EXPECT_EQ(err[YmErrCode_ItemNotFound], 1);
}

TEST(Loading, Fail_ItemNotFound_ItemArgRefsUnavailableForDirectLoads) {
    SETUP_ALL(ctx);
    SETUP_PARCELDEF(p_def);

    ymDm_BindParcelDef(dm, "p", p_def);

    EXPECT_EQ(ymCtx_Load(ctx, "$X"), nullptr);
    EXPECT_EQ(err[YmErrCode_ItemNotFound], 1);
}

TEST(Loading, Fail_ParcelNotFound_HereUnavailableForDirectLoads) {
    SETUP_ALL(ctx);
    SETUP_PARCELDEF(p_def);

    auto Int_index = setup_struct(p_def, "Int", {});

    ymDm_BindParcelDef(dm, "p", p_def);

    EXPECT_EQ(ymCtx_Load(ctx, "%here:Int"), nullptr);
    EXPECT_EQ(err[YmErrCode_ParcelNotFound], 1);
}

TEST(Loading, Fail_NonProtocolItem_GenericTypeConstraintTypeIsNotAProtocol) {
    SETUP_ALL(ctx);
    SETUP_PARCELDEF(p_def);

    auto Int_index = setup_struct(p_def, "Int", {});

    auto A_index = setup_struct(p_def, "A", {}, { { "T", "p:Int" } });

    ymDm_BindParcelDef(dm, "p", p_def);

    EXPECT_EQ(ymCtx_Load(ctx, "p:A[p:Int]"), nullptr);
    EXPECT_EQ(err[YmErrCode_NonProtocolItem], 1);
}

