

#include <gtest/gtest.h>
#include <taul/strings.h>
#include <yama/yama.h>
#include <yama++/general.h>

#include "../../utils/utils.h"


TEST(StandardLibrary, YamaParcel) {
	SETUP_ALL(ctx);
	auto p = import(ctx, "yama");
	EXPECT_STREQ(ymParcel_Path(p), "yama");
}

static void basics(YmCtx* ctx, YmType* t, const std::string& fln, YmKind k, YmMembers members, YmTypeParams typeParams) {
	EXPECT_STREQ(ymType_Fullname(t), fln.c_str());
	EXPECT_EQ(ymType_Kind(t), k);
	EXPECT_EQ(ymType_Members(t), members);
	EXPECT_EQ(ymType_TypeParams(t), typeParams);
}

static void conformsTo(YmCtx* ctx, YmType* t, std::vector<std::string> protocols) {
	for (auto& protocol : protocols) {
		EXPECT_EQ(ymType_Converts(t, load(ctx, protocol), YM_TRUE), YM_TRUE) << "protocol==" << protocol;
	}
}

// obj gets cleaned up automatically.
static void extracts(
	YmObj* obj,
	std::optional<YmInt> i,
	std::optional<YmUInt> ui,
	std::optional<YmFloat> f,
	std::optional<YmBool> b,
	std::optional<YmRune> r,
	YmType* t) {
	ASSERT_TRUE(obj);
	{
		YmBool succ{};
		EXPECT_EQ(ymObj_ToInt(obj, &succ), i.value_or(0));
		EXPECT_EQ(succ, i ? YM_TRUE : YM_FALSE);
	}
	{
		YmBool succ{};
		EXPECT_EQ(ymObj_ToUInt(obj, &succ), ui.value_or(0));
		EXPECT_EQ(succ, ui ? YM_TRUE : YM_FALSE);
	}
	{
		YmBool succ{};
		EXPECT_DOUBLE_EQ(ymObj_ToFloat(obj, &succ), f.value_or(0.0));
		EXPECT_EQ(succ, f ? YM_TRUE : YM_FALSE);
	}
	{
		YmBool succ{};
		EXPECT_EQ(ymObj_ToBool(obj, &succ), b.value_or(YM_FALSE));
		EXPECT_EQ(succ, b ? YM_TRUE : YM_FALSE);
	}
	{
		YmBool succ{};
		EXPECT_EQ(ymObj_ToRune(obj, &succ), r.value_or(U'\0'));
		EXPECT_EQ(succ, r ? YM_TRUE : YM_FALSE);
	}
	{
		YmBool succ{};
		EXPECT_EQ(ymObj_ToType(obj, &succ), t);
		EXPECT_EQ(succ, t ? YM_TRUE : YM_FALSE);
	}
	ymObj_Release(obj);
}

TEST(StandardLibrary, None) {
	SETUP_ALL(ctx);
	std::string fln = "yama:None";
	auto t = load(ctx, fln);
	basics(ctx, t, fln, YmKind_Struct, 0, 0);
	conformsTo(ctx, t, {
		"yama:Any",
		});
	extracts(
		ymCtx_NewNone(ctx),
		std::nullopt,
		std::nullopt,
		std::nullopt,
		std::nullopt,
		std::nullopt,
		nullptr);
}

TEST(StandardLibrary, Int) {
	SETUP_ALL(ctx);
	std::string fln = "yama:Int";
	auto t = load(ctx, fln);
	basics(ctx, t, fln, YmKind_Struct, 0, 0);
	conformsTo(ctx, t, {
		"yama:Any",
		});
	extracts(
		ymCtx_NewInt(ctx, -50),
		-50,
		std::nullopt,
		std::nullopt,
		std::nullopt,
		std::nullopt,
		nullptr);
}

TEST(StandardLibrary, UInt) {
	SETUP_ALL(ctx);
	std::string fln = "yama:UInt";
	auto t = load(ctx, fln);
	basics(ctx, t, fln, YmKind_Struct, 0, 0);
	conformsTo(ctx, t, {
		"yama:Any",
		});
	extracts(
		ymCtx_NewUInt(ctx, 50),
		std::nullopt,
		50,
		std::nullopt,
		std::nullopt,
		std::nullopt,
		nullptr);
}

TEST(StandardLibrary, Float) {
	SETUP_ALL(ctx);
	std::string fln = "yama:Float";
	auto t = load(ctx, fln);
	basics(ctx, t, fln, YmKind_Struct, 0, 0);
	conformsTo(ctx, t, {
		"yama:Any",
		});
	extracts(
		ymCtx_NewFloat(ctx, 3.14159),
		std::nullopt,
		std::nullopt,
		3.14159,
		std::nullopt,
		std::nullopt,
		nullptr);
}

TEST(StandardLibrary, Bool) {
	SETUP_ALL(ctx);
	std::string fln = "yama:Bool";
	auto t = load(ctx, fln);
	basics(ctx, t, fln, YmKind_Struct, 0, 0);
	conformsTo(ctx, t, {
		"yama:Any",
		});
	extracts(
		ymCtx_NewBool(ctx, YM_TRUE),
		std::nullopt,
		std::nullopt,
		std::nullopt,
		YM_TRUE,
		std::nullopt,
		nullptr);
}

TEST(StandardLibrary, Rune) {
	SETUP_ALL(ctx);
	std::string fln = "yama:Rune";
	auto t = load(ctx, fln);
	basics(ctx, t, fln, YmKind_Struct, 0, 0);
	conformsTo(ctx, t, {
		"yama:Any",
		});
	extracts(
		ymCtx_NewRune(ctx, U'&'),
		std::nullopt,
		std::nullopt,
		std::nullopt,
		std::nullopt,
		U'&',
		nullptr);
}

TEST(StandardLibrary, Type) {
	SETUP_ALL(ctx);
	std::string fln = "yama:Type";
	auto t = load(ctx, fln);
	basics(ctx, t, fln, YmKind_Struct, 0, 0);
	conformsTo(ctx, t, {
		"yama:Any",
		});
	extracts(
		ymCtx_NewType(ctx, t),
		std::nullopt,
		std::nullopt,
		std::nullopt,
		std::nullopt,
		std::nullopt,
		t);
}

TEST(StandardLibrary, Any) {
	SETUP_ALL(ctx);
	std::string fln = "yama:Any";
	auto t = load(ctx, fln);
	basics(ctx, t, fln, YmKind_Protocol, 0, 0);
	conformsTo(ctx, t, {
		"yama:Any",
		});
	// TODO: Can't test w/ 'extracts' here yet.
}

