

#include <gtest/gtest.h>
#include <taul/strings.h>
#include <yama/yama.h>
#include <yama++/print.h>
#include <yama++/Safe.h>

#include "../../utils/ErrCounter.h"
#include "../../utils/utils.h"


TEST(Loading, WorksWithAllConstTypes) {
    static_assert(YmConstType_Num == 6);
    // Dep Graph:
    //      p:A -> Int   -4         (non-ref)
    //          -> UInt  301        (non-ref)
    //          -> Float 3.14159    (non-ref)
    //          -> Bool  true       (non-ref)
    //          -> Rune  'y'        (non-ref)
    //          -> p:B              (ref)

    // NOTE: Don't call ymCtx_Load for any item before the initial p:A load, so that
    //       it's recursive loading of others is properly tested.
    YmInt i = -4;
    YmUInt ui = 301;
    YmFloat f = 3.14159;
    YmBool b = YM_TRUE;
    YmRune r = U'y';
    SETUP_ALL(ctx);
    SETUP_PARCELDEF(p_def);
    YmItemIndex p_A_index = ymParcelDef_FnItem(p_def, "A");
    YmItemIndex p_B_index = ymParcelDef_FnItem(p_def, "B");
    static_assert(YmConstType_Num == 6);
    ymParcelDef_IntConst(p_def, p_A_index, i);
    ymParcelDef_UIntConst(p_def, p_A_index, ui);
    ymParcelDef_FloatConst(p_def, p_A_index, f);
    ymParcelDef_BoolConst(p_def, p_A_index, b);
    ymParcelDef_RuneConst(p_def, p_A_index, r);
    ymParcelDef_RefConst(p_def, p_A_index, "p:B");
    ymDm_BindParcelDef(dm, "p", p_def);
    YmItem* p_A = ymCtx_Load(ctx, "p:A"); // The recursive load under test.
    YmItem* p_B = ymCtx_Load(ctx, "p:B");
    ASSERT_NE(p_A, nullptr);
    ASSERT_NE(p_B, nullptr);

    static_assert(YmConstType_Num == 6);
    ASSERT_EQ(ymItem_Consts(p_A), YmConstType_Num);
    ASSERT_EQ(ymItem_ConstType(p_A, 0), YmConstType_Int);
    ASSERT_EQ(ymItem_ConstType(p_A, 1), YmConstType_UInt);
    ASSERT_EQ(ymItem_ConstType(p_A, 2), YmConstType_Float);
    ASSERT_EQ(ymItem_ConstType(p_A, 3), YmConstType_Bool);
    ASSERT_EQ(ymItem_ConstType(p_A, 4), YmConstType_Rune);
    ASSERT_EQ(ymItem_ConstType(p_A, 5), YmConstType_Ref);
    static_assert(YmConstType_Num == 6);
    EXPECT_EQ(ymItem_IntConst(p_A, 0), i);
    EXPECT_EQ(ymItem_UIntConst(p_A, 1), ui);
    EXPECT_EQ(ymItem_FloatConst(p_A, 2), f);
    EXPECT_EQ(ymItem_BoolConst(p_A, 3), b);
    EXPECT_EQ(ymItem_RuneConst(p_A, 4), r);
    EXPECT_EQ(ymItem_RefConst(p_A, 5), p_B);
}

YmItemIndex setup_item(
    YmParcelDef* def,
    const std::string& localName,
    std::initializer_list<std::string> refconsts) {
    if (!def) {
        ADD_FAILURE();
        return YM_NO_ITEM_INDEX;
    }
    YmItemIndex result = ymParcelDef_FnItem(def, localName.c_str());
    for (const auto& refconst : refconsts) {
        ymParcelDef_RefConst(def, result, refconst.c_str());
    }
    if (result == YM_NO_ITEM_INDEX) {
        ADD_FAILURE();
    }
    return result;
}

void test_item(
    YmItem* item,
    const std::string& fullname,
    YmKind kind,
    std::initializer_list<YmItem*> refconsts) {
    ym::println("-- testing {}", fullname);
    ASSERT_NE(item, nullptr);
    EXPECT_STREQ(ymItem_Fullname(item), fullname.c_str());
    EXPECT_EQ(ymItem_Kind(item), kind);
    ASSERT_EQ(ymItem_Consts(item), refconsts.size());
    YmConst i = 0;
    for (YmItem* refconst : refconsts) {
        ASSERT_EQ(ymItem_ConstType(item, i), YmConstType_Ref) << "i==" << i;
        EXPECT_EQ(ymItem_RefConst(item, i), refconst) << "i==" << i;
        i++;
    }
}

TEST(Loading, NoRefConsts) {
    // Dep Graph:
    //      p:A

    // NOTE: Don't call ymCtx_Load for any item before the initial p:A load, so that
    //       it's recursive loading of others is properly tested.
    SETUP_ALL(ctx);
    SETUP_PARCELDEF(p_def);

    YmItemIndex p_A_index = setup_item(p_def, "A", {});

    ymDm_BindParcelDef(dm, "p", p_def);
    
    YmItem* p_A = ymCtx_Load(ctx, "p:A"); // The recursive load under test.

    test_item(p_A, "p:A", YmKind_Fn, {});
}

TEST(Loading, RefConsts) {
    // Dep Graph:
    //      p:A -> p:B
    //          -> p:C

    // NOTE: Don't call ymCtx_Load for any item before the initial p:A load, so that
    //       it's recursive loading of others is properly tested.
    SETUP_ALL(ctx);
    SETUP_PARCELDEF(p_def);

    YmItemIndex p_A_index = setup_item(p_def, "A", { "p:B", "p:C" });
    YmItemIndex p_B_index = setup_item(p_def, "B", {});
    YmItemIndex p_C_index = setup_item(p_def, "C", {});

    ymDm_BindParcelDef(dm, "p", p_def);
    
    YmItem* p_A = ymCtx_Load(ctx, "p:A"); // The recursive load under test.
    YmItem* p_B = ymCtx_Load(ctx, "p:B");
    YmItem* p_C = ymCtx_Load(ctx, "p:C");

    test_item(p_A, "p:A", YmKind_Fn, { p_B, p_C });
    test_item(p_B, "p:B", YmKind_Fn, {});
    test_item(p_C, "p:C", YmKind_Fn, {});
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

    YmItemIndex p_A_index = setup_item(p_def, "A", { "p:B", "p:C" });
    YmItemIndex p_B_index = setup_item(p_def, "B", { "p:D", "p:E" });
    YmItemIndex p_C_index = setup_item(p_def, "C", { "p:F" });
    YmItemIndex p_D_index = setup_item(p_def, "D", {});
    YmItemIndex p_E_index = setup_item(p_def, "E", {});
    YmItemIndex p_F_index = setup_item(p_def, "F", {});

    ymDm_BindParcelDef(dm, "p", p_def);
    
    YmItem* p_A = ymCtx_Load(ctx, "p:A"); // The recursive load under test.
    YmItem* p_B = ymCtx_Load(ctx, "p:B");
    YmItem* p_C = ymCtx_Load(ctx, "p:C");
    YmItem* p_D = ymCtx_Load(ctx, "p:D");
    YmItem* p_E = ymCtx_Load(ctx, "p:E");
    YmItem* p_F = ymCtx_Load(ctx, "p:F");

    test_item(p_A, "p:A", YmKind_Fn, { p_B, p_C });
    test_item(p_B, "p:B", YmKind_Fn, { p_D, p_E });
    test_item(p_C, "p:C", YmKind_Fn, { p_F });
    test_item(p_D, "p:D", YmKind_Fn, {});
    test_item(p_E, "p:E", YmKind_Fn, {});
    test_item(p_F, "p:F", YmKind_Fn, {});
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

    YmItemIndex p_A_index = setup_item(p_def, "A", { "p:B", "p:C", "p:D" });
    YmItemIndex p_B_index = setup_item(p_def, "B", { "p:D" });
    YmItemIndex p_C_index = setup_item(p_def, "C", { "p:D" });
    YmItemIndex p_D_index = setup_item(p_def, "D", {});

    ymDm_BindParcelDef(dm, "p", p_def);
    
    YmItem* p_A = ymCtx_Load(ctx, "p:A"); // The recursive load under test.
    YmItem* p_B = ymCtx_Load(ctx, "p:B");
    YmItem* p_C = ymCtx_Load(ctx, "p:C");
    YmItem* p_D = ymCtx_Load(ctx, "p:D");

    test_item(p_A, "p:A", YmKind_Fn, { p_B, p_C, p_D });
    test_item(p_B, "p:B", YmKind_Fn, { p_D });
    test_item(p_C, "p:C", YmKind_Fn, { p_D });
    test_item(p_D, "p:D", YmKind_Fn, {});
}

TEST(Loading, DepGraphCycle) {
    // Dep Graph:
    //      p:A -> p:B -> p:A       (back ref)
    //          -> p:A              (back ref)

    // NOTE: Don't call ymCtx_Load for any item before the initial p:A load, so that
    //       it's recursive loading of others is properly tested.
    SETUP_ALL(ctx);
    SETUP_PARCELDEF(p_def);

    YmItemIndex p_A_index = setup_item(p_def, "A", { "p:B", "p:A" });
    YmItemIndex p_B_index = setup_item(p_def, "B", { "p:A" });

    ymDm_BindParcelDef(dm, "p", p_def);
    
    YmItem* p_A = ymCtx_Load(ctx, "p:A"); // The recursive load under test.
    YmItem* p_B = ymCtx_Load(ctx, "p:B");

    test_item(p_A, "p:A", YmKind_Fn, { p_B, p_A });
    test_item(p_B, "p:B", YmKind_Fn, { p_A });
}

TEST(Loading, ItemsReferencedFromDifferentParcels) {
    // Dep Graph:
    //      p:A -> q:A -> p:B

    // NOTE: Don't call ymCtx_Load for any item before the initial p:A load, so that
    //       it's recursive loading of others is properly tested.
    SETUP_ALL(ctx);
    SETUP_PARCELDEF(p_def);
    SETUP_PARCELDEF(q_def);

    YmItemIndex p_A_index = setup_item(p_def, "A", { "q:A" });
    YmItemIndex p_B_index = setup_item(p_def, "B", {});
    YmItemIndex q_A_index = setup_item(q_def, "A", { "p:B" });

    ymDm_BindParcelDef(dm, "p", p_def);
    ymDm_BindParcelDef(dm, "q", q_def);
    
    YmItem* p_A = ymCtx_Load(ctx, "p:A"); // The recursive load under test.
    YmItem* p_B = ymCtx_Load(ctx, "p:B");
    YmItem* q_A = ymCtx_Load(ctx, "q:A");

    test_item(p_A, "p:A", YmKind_Fn, { q_A });
    test_item(p_B, "p:B", YmKind_Fn, {});
    test_item(q_A, "q:A", YmKind_Fn, { p_B });
}

TEST(Loading, DirectLoadsAutoImportParcels) {
    // Dep Graph:
    //      p:A                     (We're testing that p gets auto-imported.)

    // NOTE: Don't call ymCtx_Load for any item before the initial p:A load, so that
    //       it's recursive loading of others is properly tested.
    // NOTE: Vary important to not call ymCtx_Import until AFTER our load.
    SETUP_ALL(ctx);
    SETUP_PARCELDEF(p_def);

    YmItemIndex p_A_index = setup_item(p_def, "A", {});

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

    YmItemIndex p_A_index = setup_item(p_def, "A", { "q:A" });
    YmItemIndex q_A_index = setup_item(q_def, "A", {});

    ymDm_BindParcelDef(dm, "p", p_def);
    ymDm_BindParcelDef(dm, "q", q_def);

    YmItem* p_A = ymCtx_Load(ctx, "p:A"); // The recursive load under test.
    YmItem* q_A = ymCtx_Load(ctx, "q:A");

    ASSERT_NE(q_A, nullptr);
    EXPECT_EQ(ymItem_Parcel(q_A), ymCtx_Import(ctx, "q"));
}

TEST(Loading, Fail_IllegalFullname_DirectLoad) {
    for (const auto& fullname : illegalFullnames) {
        SETUP_ALL(ctx);
        EXPECT_EQ(ymCtx_Load(ctx, fullname.c_str()), nullptr)
            << "fullname == \"" << fullname << "\"";
        EXPECT_EQ(err[YmErrCode_IllegalFullname], 1);
    }
}

TEST(Loading, Fail_IllegalFullname_IndirectLoad) {
    // NOTE: ymParcelDef_RefConst will cover checking fullname syntax for indirect loading.
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

    YmItemIndex p_A_index = setup_item(p_def, "A", { "q:A" });

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

    YmItemIndex p_A_index = setup_item(p_def, "A", { "q:A" });
    
    ymDm_BindParcelDef(dm, "p", p_def);
    ymDm_BindParcelDef(dm, "q", q_def);

    EXPECT_EQ(ymCtx_Load(ctx, "p:A"), nullptr);
    EXPECT_EQ(err[YmErrCode_ItemNotFound], 1);
}

