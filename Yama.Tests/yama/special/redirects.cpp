

#include <gtest/gtest.h>
#include <taul/strings.h>
#include <yama/yama.h>

#include "../../utils/utils.h"


TEST(Redirects, IndirectLoad) {
	SETUP_ALL(ctx);
	SETUP_PARCELDEF(p_def);
	ymParcelDef_AddFn(p_def, "f", "alt:Int", ymInertCallBhvrFn, nullptr);
	ymDm_BindParcelDef(dm, "p", p_def);
	ASSERT_TRUE(ymDm_AddRedirect(dm, "p", "alt", "yama"));
	YmItem* Int = load(ctx, "yama:Int");
	YmItem* f = load(ctx, "p:f");
	EXPECT_EQ(ymItem_ReturnType(f), Int);
}

TEST(Redirects, DirectImport_Fails) {
	SETUP_ALL(ctx);
	SETUP_PARCELDEF(p_def);
	ymParcelDef_AddFn(p_def, "f", "alt:Int", ymInertCallBhvrFn, nullptr);
	ymDm_BindParcelDef(dm, "p", p_def);
	ASSERT_TRUE(ymDm_AddRedirect(dm, "p", "alt", "yama"));
	EXPECT_FALSE(ymCtx_Import(ctx, "alt"));
	EXPECT_EQ(err[YmErrCode_ParcelNotFound], 1);
}

TEST(Redirects, DirectLoad_Fails) {
	SETUP_ALL(ctx);
	SETUP_PARCELDEF(p_def);
	ymParcelDef_AddFn(p_def, "f", "alt:Int", ymInertCallBhvrFn, nullptr);
	ymDm_BindParcelDef(dm, "p", p_def);
	ASSERT_TRUE(ymDm_AddRedirect(dm, "p", "alt", "yama"));
	EXPECT_FALSE(ymCtx_Load(ctx, "alt:Int"));
	EXPECT_EQ(err[YmErrCode_ParcelNotFound], 1);
}

TEST(Redirects, ParcelDefBindings) {
	SETUP_ALL(ctx);
	SETUP_PARCELDEF(p_def);
	ymParcelDef_AddFn(p_def, "f", "alt:A", ymInertCallBhvrFn, nullptr);
	SETUP_PARCELDEF(q_def);
	ymParcelDef_AddStruct(q_def, "A");
	ymDm_BindParcelDef(dm, "p", p_def);
	ymDm_BindParcelDef(dm, "q", q_def);
	ASSERT_TRUE(ymDm_AddRedirect(dm, "p", "alt", "q"));
	YmItem* f = load(ctx, "p:f");
	YmItem* q_A = load(ctx, "q:A");
	EXPECT_EQ(ymItem_ReturnType(f), q_A);
}

TEST(Redirects, SubjectPrefixPathsCoverAllPathsContainingThem) {
	SETUP_ALL(ctx);
	SETUP_PARCELDEF(p_a_def);
	ymParcelDef_AddFn(p_a_def, "f", "alt:A", ymInertCallBhvrFn, nullptr);
	SETUP_PARCELDEF(p_b_def);
	ymParcelDef_AddFn(p_b_def, "f", "alt:A", ymInertCallBhvrFn, nullptr);
	SETUP_PARCELDEF(p_c_def);
	ymParcelDef_AddFn(p_c_def, "f", "alt:A", ymInertCallBhvrFn, nullptr);
	SETUP_PARCELDEF(q_def);
	ymParcelDef_AddStruct(q_def, "A");
	ymDm_BindParcelDef(dm, "p/a", p_a_def);
	ymDm_BindParcelDef(dm, "p/b", p_b_def);
	ymDm_BindParcelDef(dm, "p/c", p_c_def);
	ymDm_BindParcelDef(dm, "q", q_def);
	ASSERT_TRUE(ymDm_AddRedirect(dm, "p", "alt", "q"));
	YmItem* p_a_f = load(ctx, "p/a:f");
	YmItem* p_b_f = load(ctx, "p/b:f");
	YmItem* p_c_f = load(ctx, "p/c:f");
	YmItem* q_A = load(ctx, "q:A");
	EXPECT_EQ(ymItem_ReturnType(p_a_f), q_A);
	EXPECT_EQ(ymItem_ReturnType(p_b_f), q_A);
	EXPECT_EQ(ymItem_ReturnType(p_c_f), q_A);
}

TEST(Redirects, SubjectPrefixPathsShadowRedirectsWithLessSpecificOnes) {
	SETUP_ALL(ctx);
	SETUP_PARCELDEF(p_a_def);
	ymParcelDef_AddFn(p_a_def, "f", "alt:A", ymInertCallBhvrFn, nullptr);
	SETUP_PARCELDEF(p_b_def);
	ymParcelDef_AddFn(p_b_def, "f", "alt:A", ymInertCallBhvrFn, nullptr);
	SETUP_PARCELDEF(p_c_def);
	ymParcelDef_AddFn(p_c_def, "f", "alt:A", ymInertCallBhvrFn, nullptr);
	SETUP_PARCELDEF(q_def);
	ymParcelDef_AddStruct(q_def, "A");
	SETUP_PARCELDEF(instead_def);
	ymParcelDef_AddStruct(instead_def, "A");
	ymDm_BindParcelDef(dm, "p/a", p_a_def);
	ymDm_BindParcelDef(dm, "p/b", p_b_def);
	ymDm_BindParcelDef(dm, "p/c", p_c_def);
	ymDm_BindParcelDef(dm, "q", q_def);
	ymDm_BindParcelDef(dm, "instead", instead_def);
	ASSERT_TRUE(ymDm_AddRedirect(dm, "p", "alt", "q"));
	ASSERT_TRUE(ymDm_AddRedirect(dm, "p/b", "alt", "instead"));
	YmItem* p_a_f = load(ctx, "p/a:f");
	YmItem* p_b_f = load(ctx, "p/b:f");
	YmItem* p_c_f = load(ctx, "p/c:f");
	YmItem* q_A = load(ctx, "q:A");
	YmItem* instead_A = load(ctx, "instead:A");
	EXPECT_EQ(ymItem_ReturnType(p_a_f), q_A);
	EXPECT_EQ(ymItem_ReturnType(p_b_f), instead_A)
		<< ymItem_Fullname(ymItem_ReturnType(p_b_f)) << "\n"
		<< ymItem_Fullname(instead_A);
	EXPECT_EQ(ymItem_ReturnType(p_c_f), q_A);
}

TEST(Redirects, SubjectPrefixPathsSegregateRedirects) {
	SETUP_ALL(ctx);
	SETUP_PARCELDEF(p_a_def);
	ymParcelDef_AddFn(p_a_def, "f", "alt:A", ymInertCallBhvrFn, nullptr);
	SETUP_PARCELDEF(p_b_def);
	ymParcelDef_AddFn(p_b_def, "f", "alt:A", ymInertCallBhvrFn, nullptr);
	SETUP_PARCELDEF(q_a_def);
	ymParcelDef_AddStruct(q_a_def, "A");
	SETUP_PARCELDEF(q_b_def);
	ymParcelDef_AddStruct(q_b_def, "A");
	ymDm_BindParcelDef(dm, "p/a", p_a_def);
	ymDm_BindParcelDef(dm, "p/b", p_b_def);
	ymDm_BindParcelDef(dm, "q/a", q_a_def);
	ymDm_BindParcelDef(dm, "q/b", q_b_def);
	ASSERT_TRUE(ymDm_AddRedirect(dm, "p/a", "alt", "q/a"));
	ASSERT_TRUE(ymDm_AddRedirect(dm, "p/b", "alt", "q/b"));
	YmItem* p_a_f = load(ctx, "p/a:f");
	YmItem* p_b_f = load(ctx, "p/b:f");
	YmItem* q_a_A = load(ctx, "q/a:A");
	YmItem* q_b_A = load(ctx, "q/b:A");
	EXPECT_EQ(ymItem_ReturnType(p_a_f), q_a_A);
	EXPECT_EQ(ymItem_ReturnType(p_b_f), q_b_A);
}

TEST(Redirects, BeforePrefixPathsCoverAllPathsContainingThem) {
	SETUP_ALL(ctx);
	SETUP_PARCELDEF(p_def);
	auto p_f_ind = ymParcelDef_AddFn(p_def, "f", "alt/a:A", ymInertCallBhvrFn, nullptr);
	ymParcelDef_AddParam(p_def, p_f_ind, "b", "alt/b:A");
	ymParcelDef_AddParam(p_def, p_f_ind, "c", "alt/c:A");
	SETUP_PARCELDEF(q_a_def);
	ymParcelDef_AddStruct(q_a_def, "A");
	SETUP_PARCELDEF(q_b_def);
	ymParcelDef_AddStruct(q_b_def, "A");
	SETUP_PARCELDEF(q_c_def);
	ymParcelDef_AddStruct(q_c_def, "A");
	ymDm_BindParcelDef(dm, "p", p_def);
	ymDm_BindParcelDef(dm, "q/a", q_a_def);
	ymDm_BindParcelDef(dm, "q/b", q_b_def);
	ymDm_BindParcelDef(dm, "q/c", q_c_def);
	ASSERT_TRUE(ymDm_AddRedirect(dm, "p", "alt", "q"));
	YmItem* f = load(ctx, "p:f");
	YmItem* q_a_A = load(ctx, "q/a:A");
	YmItem* q_b_A = load(ctx, "q/b:A");
	YmItem* q_c_A = load(ctx, "q/c:A");
	EXPECT_EQ(ymItem_ReturnType(f), q_a_A);
	EXPECT_EQ(ymItem_ParamType(f, 0), q_b_A);
	EXPECT_EQ(ymItem_ParamType(f, 1), q_c_A);
}

TEST(Redirects, BeforePrefixPathsShadowRedirectsWithLessSpecificOnes) {
	SETUP_ALL(ctx);
	SETUP_PARCELDEF(p_def);
	auto p_f_ind = ymParcelDef_AddFn(p_def, "f", "alt/a:A", ymInertCallBhvrFn, nullptr);
	ymParcelDef_AddParam(p_def, p_f_ind, "b", "alt/b:A");
	ymParcelDef_AddParam(p_def, p_f_ind, "c", "alt/c:A");
	SETUP_PARCELDEF(q_a_def);
	ymParcelDef_AddStruct(q_a_def, "A");
	SETUP_PARCELDEF(q_b_def);
	ymParcelDef_AddStruct(q_b_def, "A");
	SETUP_PARCELDEF(q_c_def);
	ymParcelDef_AddStruct(q_c_def, "A");
	SETUP_PARCELDEF(instead_def);
	ymParcelDef_AddStruct(instead_def, "A");
	ymDm_BindParcelDef(dm, "p", p_def);
	ymDm_BindParcelDef(dm, "q/a", q_a_def);
	ymDm_BindParcelDef(dm, "q/b", q_b_def);
	ymDm_BindParcelDef(dm, "q/c", q_c_def);
	ymDm_BindParcelDef(dm, "instead", instead_def);
	ASSERT_TRUE(ymDm_AddRedirect(dm, "p", "alt", "q"));
	ASSERT_TRUE(ymDm_AddRedirect(dm, "p", "alt/b", "instead"));
	YmItem* f = load(ctx, "p:f");
	YmItem* q_a_A = load(ctx, "q/a:A");
	YmItem* q_b_A = load(ctx, "q/b:A");
	YmItem* q_c_A = load(ctx, "q/c:A");
	YmItem* instead_A = load(ctx, "instead:A");
	EXPECT_EQ(ymItem_ReturnType(f), q_a_A);
	EXPECT_EQ(ymItem_ParamType(f, 0), instead_A)
		<< ymItem_Fullname(ymItem_ParamType(f, 0)) << "\n"
		<< ymItem_Fullname(instead_A);
	EXPECT_EQ(ymItem_ParamType(f, 1), q_c_A);
}

TEST(Redirects, WorksInItemArgsAndCallSuffParamAndReturnTypes) {
	SETUP_ALL(ctx);
	SETUP_PARCELDEF(p_def);
	ymParcelDef_AddFn(p_def, "f", "alt1:g[alt2:Int](alt2:Int) -> alt2:Int", ymInertCallBhvrFn, nullptr);
	SETUP_PARCELDEF(q_def);
	auto q_g_ind = ymParcelDef_AddFn(q_def, "g", "$T", ymInertCallBhvrFn, nullptr);
	ymParcelDef_AddItemParam(q_def, q_g_ind, "T", "yama:Any");
	ymParcelDef_AddParam(q_def, q_g_ind, "a", "$T");
	ymDm_BindParcelDef(dm, "p", p_def);
	ymDm_BindParcelDef(dm, "q", q_def);
	ASSERT_TRUE(ymDm_AddRedirect(dm, "p", "alt1", "q"));
	ASSERT_TRUE(ymDm_AddRedirect(dm, "p", "alt2", "yama"));
	YmItem* p_f = load(ctx, "p:f");
	YmItem* q_g_yama_Int = load(ctx, "q:g[yama:Int](yama:Int) -> yama:Int");
	EXPECT_EQ(ymItem_ReturnType(p_f), q_g_yama_Int);
}

TEST(Redirects, OriginalPathCanStillBeUsedIfItIsntShadowedByAnotherRedirect) {
	SETUP_ALL(ctx);
	SETUP_PARCELDEF(p_def);
	auto p_f_ind = ymParcelDef_AddFn(p_def, "f", "yama:Int", ymInertCallBhvrFn, nullptr);
	ymParcelDef_AddParam(p_def, p_f_ind, "a", "alt:Int");
	ymDm_BindParcelDef(dm, "p", p_def);
	ASSERT_TRUE(ymDm_AddRedirect(dm, "p", "alt", "yama"));
	YmItem* Int = load(ctx, "yama:Int");
	YmItem* p_f = load(ctx, "p:f");
	EXPECT_EQ(ymItem_ReturnType(p_f), Int);
	EXPECT_EQ(ymItem_ParamType(p_f, 0), Int);
}

TEST(Redirects, RedirectsCanSubstituteInInvalidPaths) {
	SETUP_ALL(ctx);
	SETUP_PARCELDEF(p_def);
	ymParcelDef_AddFn(p_def, "f", "missing0:A", ymInertCallBhvrFn, nullptr);
	ymDm_BindParcelDef(dm, "p", p_def);
	ASSERT_TRUE(ymDm_AddRedirect(dm, "p", "missing0", "missing1"));
	EXPECT_FALSE(ymCtx_Load(ctx, "p:f"));
	EXPECT_EQ(err[YmErrCode_ParcelNotFound], 1);
}

