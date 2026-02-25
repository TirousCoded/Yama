

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

TEST(StandardLibrary, None) {
	SETUP_ALL(ctx);
	std::string fln = "yama:None";
	auto t = load(ctx, fln);
	basics(ctx, t, fln, YmKind_Struct, 0, 0);
	conformsTo(ctx, t, {
		"yama:Any",
		});
}

TEST(StandardLibrary, Int) {
	SETUP_ALL(ctx);
	std::string fln = "yama:Int";
	auto t = load(ctx, fln);
	basics(ctx, t, fln, YmKind_Struct, 0, 0);
	conformsTo(ctx, t, {
		"yama:Any",
		});
}

TEST(StandardLibrary, UInt) {
	SETUP_ALL(ctx);
	std::string fln = "yama:UInt";
	auto t = load(ctx, fln);
	basics(ctx, t, fln, YmKind_Struct, 0, 0);
	conformsTo(ctx, t, {
		"yama:Any",
		});
}

TEST(StandardLibrary, Float) {
	SETUP_ALL(ctx);
	std::string fln = "yama:Float";
	auto t = load(ctx, fln);
	basics(ctx, t, fln, YmKind_Struct, 0, 0);
	conformsTo(ctx, t, {
		"yama:Any",
		});
}

TEST(StandardLibrary, Bool) {
	SETUP_ALL(ctx);
	std::string fln = "yama:Bool";
	auto t = load(ctx, fln);
	basics(ctx, t, fln, YmKind_Struct, 0, 0);
	conformsTo(ctx, t, {
		"yama:Any",
		});
}

TEST(StandardLibrary, Rune) {
	SETUP_ALL(ctx);
	std::string fln = "yama:Rune";
	auto t = load(ctx, fln);
	basics(ctx, t, fln, YmKind_Struct, 0, 0);
	conformsTo(ctx, t, {
		"yama:Any",
		});
}

TEST(StandardLibrary, Any) {
	SETUP_ALL(ctx);
	std::string fln = "yama:Any";
	auto t = load(ctx, fln);
	basics(ctx, t, fln, YmKind_Protocol, 0, 0);
	conformsTo(ctx, t, {
		"yama:Any",
		});
}

