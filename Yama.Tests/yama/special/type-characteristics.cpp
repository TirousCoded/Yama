

#include <gtest/gtest.h>
#include <taul/strings.h>
#include <yama/yama.h>
#include <yama++/general.h>
#include <yama++/print.h>

#include "../../utils/utils.h"


using namespace ym;


namespace {
	struct Extracts {
		std::optional<YmInt> i = std::nullopt;
		std::optional<YmUInt> ui = std::nullopt;
		std::optional<YmFloat> f = std::nullopt;
		std::optional<YmBool> b = std::nullopt;
		std::optional<YmRune> r = std::nullopt;
		YmType* t = nullptr;
	};
	void testExtracts(
		YmObj* obj,
		Extracts extracts) {
		ASSERT_TRUE(obj);
		{
			YmBool succ{};
			EXPECT_EQ(ymObj_ToInt(obj, &succ), extracts.i.value_or(0));
			EXPECT_EQ(succ, extracts.i ? YM_TRUE : YM_FALSE);
		}
		{
			YmBool succ{};
			EXPECT_EQ(ymObj_ToUInt(obj, &succ), extracts.ui.value_or(0));
			EXPECT_EQ(succ, extracts.ui ? YM_TRUE : YM_FALSE);
		}
		{
			YmBool succ{};
			EXPECT_DOUBLE_EQ(ymObj_ToFloat(obj, &succ), extracts.f.value_or(0.0));
			EXPECT_EQ(succ, extracts.f ? YM_TRUE : YM_FALSE);
		}
		{
			YmBool succ{};
			EXPECT_EQ(ymObj_ToBool(obj, &succ), extracts.b.value_or(YM_FALSE));
			EXPECT_EQ(succ, extracts.b ? YM_TRUE : YM_FALSE);
		}
		{
			YmBool succ{};
			EXPECT_EQ(ymObj_ToRune(obj, &succ), extracts.r.value_or(U'\0'));
			EXPECT_EQ(succ, extracts.r ? YM_TRUE : YM_FALSE);
		}
		{
			YmBool succ{};
			EXPECT_EQ(ymObj_ToType(obj, &succ), extracts.t);
			EXPECT_EQ(succ, extracts.t ? YM_TRUE : YM_FALSE);
		}
	}
	struct Unit {
		bool valid = false; // Set to true iff ctor succeeds.

		YmDm* dm;
		YmCtx* ctx;
		ErrCounter& err;
		YmType* type = nullptr, *objType = nullptr;

		// TODO: How do we go about testing fns like ymCtx_[Get|Set]Property?

		Unit(
			YmDm* dm,
			YmCtx* ctx,
			ErrCounter& err,
			const std::string& fullname,
			// This is for when the type of objects of this type aren't this type itself (eg. fn types.)
			// std::nullopt if not instantiatable.
			const std::optional<std::string>& objectTypeFln,
			YmKind kind,
			YmMembers members,
			YmTypeParams typeParams,
			std::function<void(Safe<YmType> type)> testDefaultInit,
			std::function<void(Safe<YmType> type)> testExplicitInit,
			// Test calling w/ ymCtx_Call.
			std::function<void(Safe<YmType> type)> testCall) :
			dm(dm),
			ctx(ctx),
			err(err) {
			_setup(
				dm, ctx, err,
				fullname, objectTypeFln,
				kind, members, typeParams,
				testDefaultInit,
				testExplicitInit,
				testCall);
		}

		void object(
			std::function<YmObj* (Safe<YmType> type)> objectMaker,
			Extracts extracts,
			std::function<void(Safe<YmObj> result)> otherChecks = [](Safe<YmObj>) {}) {
			if (!valid) return;

			ym::println("example object");

			ymCtx_PopAll(ctx);
			ASSERT_EQ(ymCtx_Locals(ctx), 0);
			err.reset();

			SETUP_OBJ(object, objectMaker(Safe(type)));
			ASSERT_EQ(ymObj_Type(object), objType)
				<< "expected type: " << ymType_Fullname(objType)
				<< "\nactual type  : " << ymType_Fullname(ymObj_Type(object));

			testExtracts(object, extracts);
			otherChecks(Safe(object));
		}

		void conversion(
			const std::string& targetFln,
			bool coercion,
			std::function<YmObj* (Safe<YmType> type)> objectMaker,
			Extracts extracts,
			std::function<void(Safe<YmObj> result)> otherChecks = [](Safe<YmObj>) {}) {
			if (!valid) return;

			if (!coercion) {
				ym::println("conversion -> {}", targetFln);
			}
			else {
				ym::println("coercion -> {}", targetFln);
			}

			ASSERT_TRUE(type);
			ASSERT_TRUE(objType);

			auto target = ymCtx_Load(ctx, targetFln.c_str());
			ASSERT_TRUE(target);

			ASSERT_EQ(ymType_Converts(objType, target, YM_FALSE), YM_TRUE);
			ASSERT_EQ(ymType_Converts(objType, target, YM_TRUE), coercion ? YM_TRUE : YM_FALSE);

			ymCtx_PopAll(ctx);
			ASSERT_EQ(ymCtx_Locals(ctx), 0);
			err.reset();

			SETUP_OBJ(object, objectMaker(Safe(type)));
			ASSERT_EQ(ymObj_Type(object), objType)
				<< "expected type: " << ymType_Fullname(objType)
				<< "\nactual type  : " << ymType_Fullname(ymObj_Type(object));

			ASSERT_EQ(ymCtx_Put(ctx, YM_NEWTOP, object, YM_BORROW), YM_TRUE);
			ASSERT_EQ(ymCtx_Convert(ctx, target, YM_NEWTOP), YM_TRUE);

			ASSERT_EQ(ymCtx_Locals(ctx), 1);
			if (auto top = ymCtx_Local(ctx, 0, YM_BORROW)) {
				ASSERT_EQ(ymObj_Type(top), target)
					<< "top object type: " << ymType_Fullname(ymObj_Type(top));
				testExtracts(top, extracts);
				otherChecks(Safe(top));
			}
			else ADD_FAILURE();
		}
		void nonconversions(const std::vector<std::string>& targetFlns) {
			if (!valid) return;

			ASSERT_TRUE(objType);

			for (const auto& targetFln : targetFlns) {
				auto target = ymCtx_Load(ctx, targetFln.c_str());
				ASSERT_TRUE(target);

				ASSERT_EQ(ymType_Converts(objType, target, YM_FALSE), YM_FALSE);
				ASSERT_EQ(ymType_Converts(objType, target, YM_TRUE), YM_FALSE);
			}
		}


	private:
		void _setup(
			YmDm* dm,
			YmCtx* ctx,
			ErrCounter& err,
			const std::string& fullname,
			const std::optional<std::string>& objectTypeFln,
			YmKind kind,
			YmMembers members,
			YmTypeParams typeParams,
			std::function<void(Safe<YmType> type)> testDefaultInit,
			std::function<void(Safe<YmType> type)> testExplicitInit,
			std::function<void(Safe<YmType> type)> testCall) {
			ym::println("testing {}", fullname);

			type = ymCtx_Load(ctx, fullname.c_str());
			ASSERT_TRUE(type);

			if (objectTypeFln) {
				auto fln = objectTypeFln.value();
				objType = ymCtx_Load(ctx, fln.c_str());
				ASSERT_TRUE(objType);
			}

			ASSERT_STREQ(ymType_Fullname(type), fullname.c_str());
			ASSERT_EQ(ymType_Kind(type), kind)
				<< "ymType_Kind(t) == " << ymKind_Fmt(ymType_Kind(type))
				<< "\nkind == " << ymKind_Fmt(kind);
			ASSERT_EQ(ymType_Members(type), members);
			ASSERT_EQ(ymType_TypeParams(type), typeParams);

			ymCtx_PopAll(ctx);
			ASSERT_EQ(ymCtx_Locals(ctx), 0);
			err.reset();
			testDefaultInit(Safe(type));

			ymCtx_PopAll(ctx);
			ASSERT_EQ(ymCtx_Locals(ctx), 0);
			err.reset();
			testExplicitInit(Safe(type));

			ymCtx_PopAll(ctx);
			ASSERT_EQ(ymCtx_Locals(ctx), 0);
			err.reset();
			testCall(Safe(type));

			// Now we can declare this Test object valid.
			valid = true;
		}
	};
}

TEST(TypeCharacteristics, YamaNone) {
	SETUP_ALL(ctx);
	Unit unit(
		dm, ctx, err,
		"yama:None",
		"yama:None",
		YmKind_Struct,
		0,
		0,
		[&ctx](Safe<YmType> type) {
			EXPECT_EQ(ymCtx_DefaultInit(ctx, YM_NEWTOP, type), YM_TRUE);
			auto result = ymCtx_Local(ctx, 0, YM_BORROW);
			ASSERT_TRUE(result);
			EXPECT_EQ(ymObj_Type(result), type);
			testExtracts(result, Extracts{});
		},
		[&ctx](Safe<YmType> type) {
			EXPECT_EQ(ymCtx_ExplicitInit(ctx, YM_NEWTOP, type, ""), YM_TRUE);
			auto result = ymCtx_Local(ctx, 0, YM_BORROW);
			ASSERT_TRUE(result);
			EXPECT_EQ(ymObj_Type(result), type);
			testExtracts(result, Extracts{});
		},
		[&ctx, &err](Safe<YmType> type) {
			EXPECT_EQ(ymCtx_Call(ctx, type, 0, "", YM_DISCARD), YM_FALSE);
			EXPECT_GE(err[YmErrCode_NonCallableType], 1);
		});
	auto mkObject =
		[&ctx](Safe<YmType> type) -> YmObj* {
		return ymCtx_NewNone(ctx);
		};
	unit.object(mkObject, Extracts{});
	unit.conversion("yama:None", true, mkObject, Extracts{});
	unit.conversion("yama:Any", true, mkObject, Extracts{});

	unit.nonconversions({
		"yama:Int",
		"yama:UInt",
		"yama:Float",
		"yama:Bool",
		"yama:Rune",
		"yama:Type",
		});
}

TEST(TypeCharacteristics, YamaInt) {
	SETUP_ALL(ctx);
	Unit unit(
		dm, ctx, err,
		"yama:Int",
		"yama:Int",
		YmKind_Struct,
		0,
		0,
		[&ctx](Safe<YmType> type) {
			EXPECT_EQ(ymCtx_DefaultInit(ctx, YM_NEWTOP, type), YM_TRUE);
			auto result = ymCtx_Local(ctx, 0, YM_BORROW);
			ASSERT_TRUE(result);
			EXPECT_EQ(ymObj_Type(result), type);
			testExtracts(result, Extracts{ .i = 0 });
		},
		[&ctx](Safe<YmType> type) {
			EXPECT_EQ(ymCtx_ExplicitInit(ctx, YM_NEWTOP, type, ""), YM_TRUE);
			auto result = ymCtx_Local(ctx, 0, YM_BORROW);
			ASSERT_TRUE(result);
			EXPECT_EQ(ymObj_Type(result), type);
			testExtracts(result, Extracts{ .i = 0 });
		},
		[&ctx, &err](Safe<YmType> type) {
			EXPECT_EQ(ymCtx_Call(ctx, type, 0, "", YM_DISCARD), YM_FALSE);
			EXPECT_GE(err[YmErrCode_NonCallableType], 1);
		});
	auto mkObject =
		[&ctx](Safe<YmType> type) -> YmObj* {
		return ymCtx_NewInt(ctx, -50);
		};
	unit.object(mkObject, Extracts{ .i = -50 });
	unit.conversion("yama:Int", true, mkObject, Extracts{ .i = -50 });
	unit.conversion("yama:Any", true, mkObject, Extracts{});

	unit.conversion("yama:None", false, mkObject, Extracts{});

	unit.conversion("yama:UInt", false,
		[&ctx](Safe<YmType>) -> YmObj* { return ymCtx_NewInt(ctx, 0); },
		Extracts{ .ui = 0 });
	unit.conversion("yama:UInt", false,
		[&ctx](Safe<YmType>) -> YmObj* { return ymCtx_NewInt(ctx, 1); },
		Extracts{ .ui = 1 });
	unit.conversion("yama:UInt", false,
		[&ctx](Safe<YmType>) -> YmObj* { return ymCtx_NewInt(ctx, -1); },
		Extracts{ .ui = YM_MAX_UINT64 });
	unit.conversion("yama:UInt", false,
		[&ctx](Safe<YmType>) -> YmObj* { return ymCtx_NewInt(ctx, -2); },
		Extracts{ .ui = YM_MAX_UINT64 - 1 });
	unit.conversion("yama:UInt", false,
		[&ctx](Safe<YmType>) -> YmObj* { return ymCtx_NewInt(ctx, YM_MAX_INT64); },
		Extracts{ .ui = YmUInt(YM_MAX_INT64) });
	unit.conversion("yama:UInt", false,
		[&ctx](Safe<YmType>) -> YmObj* { return ymCtx_NewInt(ctx, YM_MIN_INT64); },
		// Conversion of YM_MIN_INT64 to uint64 performs the subtraction we want.
		Extracts{ .ui = YmUInt(YM_MIN_INT64) });

	unit.conversion("yama:Float", false,
		[&ctx](Safe<YmType>) -> YmObj* { return ymCtx_NewInt(ctx, 0); },
		Extracts{ .f = 0 });
	unit.conversion("yama:Float", false,
		[&ctx](Safe<YmType>) -> YmObj* { return ymCtx_NewInt(ctx, 1); },
		Extracts{ .f = 1 });
	unit.conversion("yama:Float", false,
		[&ctx](Safe<YmType>) -> YmObj* { return ymCtx_NewInt(ctx, -1); },
		Extracts{ .f = -1 });
	unit.conversion("yama:Float", false,
		[&ctx](Safe<YmType>) -> YmObj* { return ymCtx_NewInt(ctx, 1'000'000); },
		Extracts{ .f = 1'000'000 });
	unit.conversion("yama:Float", false,
		[&ctx](Safe<YmType>) -> YmObj* { return ymCtx_NewInt(ctx, -1'000'000); },
		Extracts{ .f = -1'000'000 });

	unit.conversion("yama:Rune", false,
		[&ctx](Safe<YmType>) -> YmObj* { return ymCtx_NewInt(ctx, 0); },
		Extracts{ .r = U'\0' });
	unit.conversion("yama:Rune", false,
		[&ctx](Safe<YmType>) -> YmObj* { return ymCtx_NewInt(ctx, YmInt(U'a')); },
		Extracts{ .r = U'a' });
	unit.conversion("yama:Rune", false,
		[&ctx](Safe<YmType>) -> YmObj* { return ymCtx_NewInt(ctx, YmInt(U'1')); },
		Extracts{ .r = U'1' });
	unit.conversion("yama:Rune", false,
		[&ctx](Safe<YmType>) -> YmObj* { return ymCtx_NewInt(ctx, YmInt(U'1')); },
		Extracts{ .r = U'1' });
	unit.conversion("yama:Rune", false,
		[&ctx](Safe<YmType>) -> YmObj* { return ymCtx_NewInt(ctx, YmInt(0x10ffff)); },
		Extracts{ .r = YmRune(0x10ffff) });
	// Special Overflow
	unit.conversion("yama:Rune", false,
		[&ctx](Safe<YmType>) -> YmObj* { return ymCtx_NewInt(ctx, YmInt(0x110000)); },
		Extracts{ .r = YmRune(0) });
	// Special Overflow
	unit.conversion("yama:Rune", false,
		[&ctx](Safe<YmType>) -> YmObj* { return ymCtx_NewInt(ctx, YmInt(0x110001)); },
		Extracts{ .r = YmRune(1) });
	// NOTE: YmUInt32(-1) % 0x110000 == 0xffff (ie. NOT 0x10ffff)
	// Special Underflow
	unit.conversion("yama:Rune", false,
		[&ctx](Safe<YmType>) -> YmObj* { return ymCtx_NewInt(ctx, YmInt(-1)); },
		Extracts{ .r = YmRune(0xffff) });
	// NOTE: YmUInt32(-1) % 0x110000 == 0xffff (ie. NOT 0x10ffff)
	// Special Underflow
	unit.conversion("yama:Rune", false,
		[&ctx](Safe<YmType>) -> YmObj* { return ymCtx_NewInt(ctx, YmInt(-2)); },
		Extracts{ .r = YmRune(0xfffe) });

	unit.nonconversions({
		"yama:Bool",
		"yama:Type",
		});
}

TEST(TypeCharacteristics, YamaUInt) {
	SETUP_ALL(ctx);
	Unit unit(
		dm, ctx, err,
		"yama:UInt",
		"yama:UInt",
		YmKind_Struct,
		0,
		0,
		[&ctx](Safe<YmType> type) {
			EXPECT_EQ(ymCtx_DefaultInit(ctx, YM_NEWTOP, type), YM_TRUE);
			auto result = ymCtx_Local(ctx, 0, YM_BORROW);
			ASSERT_TRUE(result);
			EXPECT_EQ(ymObj_Type(result), type);
			testExtracts(result, Extracts{ .ui = 0 });
		},
		[&ctx](Safe<YmType> type) {
			EXPECT_EQ(ymCtx_ExplicitInit(ctx, YM_NEWTOP, type, ""), YM_TRUE);
			auto result = ymCtx_Local(ctx, 0, YM_BORROW);
			ASSERT_TRUE(result);
			EXPECT_EQ(ymObj_Type(result), type);
			testExtracts(result, Extracts{ .ui = 0 });
		},
		[&ctx, &err](Safe<YmType> type) {
			EXPECT_EQ(ymCtx_Call(ctx, type, 0, "", YM_DISCARD), YM_FALSE);
			EXPECT_GE(err[YmErrCode_NonCallableType], 1);
		});
	auto mkObject =
		[&ctx](Safe<YmType> type) -> YmObj* {
		return ymCtx_NewUInt(ctx, 50);
		};
	unit.object(mkObject, Extracts{ .ui = 50 });
	unit.conversion("yama:UInt", true, mkObject, Extracts{ .ui = 50 });
	unit.conversion("yama:Any", true, mkObject, Extracts{});

	unit.conversion("yama:None", false, mkObject, Extracts{});

	unit.conversion("yama:Int", false,
		[&ctx](Safe<YmType>) -> YmObj* { return ymCtx_NewUInt(ctx, 0); },
		Extracts{ .i = 0 });
	unit.conversion("yama:Int", false,
		[&ctx](Safe<YmType>) -> YmObj* { return ymCtx_NewUInt(ctx, 1); },
		Extracts{ .i = 1 });
	unit.conversion("yama:Int", false,
		[&ctx](Safe<YmType>) -> YmObj* { return ymCtx_NewUInt(ctx, YM_MAX_INT64); },
		Extracts{ .i = YM_MAX_INT64 });
	unit.conversion("yama:Int", false,
		[&ctx](Safe<YmType>) -> YmObj* { return ymCtx_NewUInt(ctx, YmUInt(YM_MAX_INT64) + 1); },
		Extracts{ .i = YM_MIN_INT64 });
	unit.conversion("yama:Int", false,
		[&ctx](Safe<YmType>) -> YmObj* { return ymCtx_NewUInt(ctx, YM_MAX_UINT64); },
		Extracts{ .i = -1 });

	unit.conversion("yama:Float", false,
		[&ctx](Safe<YmType>) -> YmObj* { return ymCtx_NewUInt(ctx, 0); },
		Extracts{ .f = 0 });
	unit.conversion("yama:Float", false,
		[&ctx](Safe<YmType>) -> YmObj* { return ymCtx_NewUInt(ctx, 1); },
		Extracts{ .f = 1 });
	unit.conversion("yama:Float", false,
		[&ctx](Safe<YmType>) -> YmObj* { return ymCtx_NewUInt(ctx, 1'000'000); },
		Extracts{ .f = 1'000'000 });

	unit.conversion("yama:Rune", false,
		[&ctx](Safe<YmType>) -> YmObj* { return ymCtx_NewUInt(ctx, 0); },
		Extracts{ .r = U'\0' });
	unit.conversion("yama:Rune", false,
		[&ctx](Safe<YmType>) -> YmObj* { return ymCtx_NewUInt(ctx, YmUInt(U'a')); },
		Extracts{ .r = U'a' });
	unit.conversion("yama:Rune", false,
		[&ctx](Safe<YmType>) -> YmObj* { return ymCtx_NewUInt(ctx, YmUInt(U'1')); },
		Extracts{ .r = U'1' });
	unit.conversion("yama:Rune", false,
		[&ctx](Safe<YmType>) -> YmObj* { return ymCtx_NewUInt(ctx, YmUInt(U'1')); },
		Extracts{ .r = U'1' });
	unit.conversion("yama:Rune", false,
		[&ctx](Safe<YmType>) -> YmObj* { return ymCtx_NewUInt(ctx, YmUInt(0x10ffff)); },
		Extracts{ .r = YmRune(0x10ffff) });
	// Special Overflow
	unit.conversion("yama:Rune", false,
		[&ctx](Safe<YmType>) -> YmObj* { return ymCtx_NewUInt(ctx, YmUInt(0x110000)); },
		Extracts{ .r = YmRune(0) });
	// Special Overflow
	unit.conversion("yama:Rune", false,
		[&ctx](Safe<YmType>) -> YmObj* { return ymCtx_NewUInt(ctx, YmUInt(0x110001)); },
		Extracts{ .r = YmRune(1) });

	unit.nonconversions({
		"yama:Bool",
		"yama:Type",
		});
}

TEST(TypeCharacteristics, YamaFloat) {
	SETUP_ALL(ctx);
	Unit unit(
		dm, ctx, err,
		"yama:Float",
		"yama:Float",
		YmKind_Struct,
		0,
		0,
		[&ctx](Safe<YmType> type) {
			EXPECT_EQ(ymCtx_DefaultInit(ctx, YM_NEWTOP, type), YM_TRUE);
			auto result = ymCtx_Local(ctx, 0, YM_BORROW);
			ASSERT_TRUE(result);
			EXPECT_EQ(ymObj_Type(result), type);
			testExtracts(result, Extracts{ .f = 0 });
		},
		[&ctx](Safe<YmType> type) {
			EXPECT_EQ(ymCtx_ExplicitInit(ctx, YM_NEWTOP, type, ""), YM_TRUE);
			auto result = ymCtx_Local(ctx, 0, YM_BORROW);
			ASSERT_TRUE(result);
			EXPECT_EQ(ymObj_Type(result), type);
			testExtracts(result, Extracts{ .f = 0 });
		},
		[&ctx, &err](Safe<YmType> type) {
			EXPECT_EQ(ymCtx_Call(ctx, type, 0, "", YM_DISCARD), YM_FALSE);
			EXPECT_GE(err[YmErrCode_NonCallableType], 1);
		});
	auto mkObject =
		[&ctx](Safe<YmType> type) -> YmObj* {
		return ymCtx_NewFloat(ctx, 3.14159);
		};
	unit.object(mkObject, Extracts{ .f = 3.14159 });
	unit.conversion("yama:Float", true, mkObject, Extracts{ .f = 3.14159 });
	unit.conversion("yama:Any", true, mkObject, Extracts{});

	unit.conversion("yama:None", false, mkObject, Extracts{});

	unit.conversion("yama:Int", false,
		[&ctx](Safe<YmType>) -> YmObj* { return ymCtx_NewFloat(ctx, 0); },
		Extracts{ .i = 0 });
	unit.conversion("yama:Int", false,
		[&ctx](Safe<YmType>) -> YmObj* { return ymCtx_NewFloat(ctx, 1); },
		Extracts{ .i = 1 });
	unit.conversion("yama:Int", false,
		[&ctx](Safe<YmType>) -> YmObj* { return ymCtx_NewFloat(ctx, -1); },
		Extracts{ .i = -1 });
	unit.conversion("yama:Int", false,
		[&ctx](Safe<YmType>) -> YmObj* { return ymCtx_NewFloat(ctx, 0.55); },
		Extracts{ .i = 0 });
	unit.conversion("yama:Int", false,
		[&ctx](Safe<YmType>) -> YmObj* { return ymCtx_NewFloat(ctx, 1.55); },
		Extracts{ .i = 1 });
	unit.conversion("yama:Int", false,
		[&ctx](Safe<YmType>) -> YmObj* { return ymCtx_NewFloat(ctx, -1.55); },
		Extracts{ .i = -1 });
	unit.conversion("yama:Int", false,
		[&ctx](Safe<YmType>) -> YmObj* { return ymCtx_NewFloat(ctx, 1'000'000); },
		Extracts{ .i = 1'000'000 });
	unit.conversion("yama:Int", false,
		[&ctx](Safe<YmType>) -> YmObj* { return ymCtx_NewFloat(ctx, -1'000'000); },
		Extracts{ .i = -1'000'000 });

	unit.conversion("yama:UInt", false,
		[&ctx](Safe<YmType>) -> YmObj* { return ymCtx_NewFloat(ctx, 0); },
		Extracts{ .ui = 0 });
	unit.conversion("yama:UInt", false,
		[&ctx](Safe<YmType>) -> YmObj* { return ymCtx_NewFloat(ctx, 1); },
		Extracts{ .ui = 1 });
	unit.conversion("yama:UInt", false,
		[&ctx](Safe<YmType>) -> YmObj* { return ymCtx_NewFloat(ctx, -1); },
		Extracts{ .ui = (YmUInt)-1.0 });
	unit.conversion("yama:UInt", false,
		[&ctx](Safe<YmType>) -> YmObj* { return ymCtx_NewFloat(ctx, 0.55); },
		Extracts{ .ui = 0 });
	unit.conversion("yama:UInt", false,
		[&ctx](Safe<YmType>) -> YmObj* { return ymCtx_NewFloat(ctx, 1.55); },
		Extracts{ .ui = 1 });
	unit.conversion("yama:UInt", false,
		[&ctx](Safe<YmType>) -> YmObj* { return ymCtx_NewFloat(ctx, -1.55); },
		Extracts{ .ui = (YmUInt)-1.0 });
	unit.conversion("yama:UInt", false,
		[&ctx](Safe<YmType>) -> YmObj* { return ymCtx_NewFloat(ctx, 1'000'000); },
		Extracts{ .ui = (YmUInt)1'000'000.0 });
	unit.conversion("yama:UInt", false,
		[&ctx](Safe<YmType>) -> YmObj* { return ymCtx_NewFloat(ctx, -1'000'000); },
		Extracts{ .ui = (YmUInt)-1'000'000.0 });

	unit.conversion("yama:Rune", false,
		[&ctx](Safe<YmType>) -> YmObj* { return ymCtx_NewFloat(ctx, 0); },
		Extracts{ .r = YmRune(0) });
	unit.conversion("yama:Rune", false,
		[&ctx](Safe<YmType>) -> YmObj* { return ymCtx_NewFloat(ctx, 1); },
		Extracts{ .r = YmRune(1) });
	unit.conversion("yama:Rune", false,
		[&ctx](Safe<YmType>) -> YmObj* { return ymCtx_NewFloat(ctx, YmFloat(U'a')); },
		Extracts{ .r = U'a' });
	unit.conversion("yama:Rune", false,
		[&ctx](Safe<YmType>) -> YmObj* { return ymCtx_NewFloat(ctx, YmFloat(U'1')); },
		Extracts{ .r = U'1' });
	unit.conversion("yama:Rune", false,
		[&ctx](Safe<YmType>) -> YmObj* { return ymCtx_NewFloat(ctx, YmFloat(0x10ffff)); },
		Extracts{ .r = YmRune(0x10ffff) });
	unit.conversion("yama:Rune", false,
		[&ctx](Safe<YmType>) -> YmObj* { return ymCtx_NewFloat(ctx, YmFloat(0x110000)); },
		Extracts{ .r = YmRune(0) });
	unit.conversion("yama:Rune", false,
		[&ctx](Safe<YmType>) -> YmObj* { return ymCtx_NewFloat(ctx, YmFloat(0x110001)); },
		Extracts{ .r = YmRune(1) });
	// NOTE: YmUInt(-1) % 0x110000 == 0xffff (ie. NOT 0x10ffff)
	unit.conversion("yama:Rune", false,
		[&ctx](Safe<YmType>) -> YmObj* { return ymCtx_NewFloat(ctx, -1.0); },
		Extracts{ .r = YmRune(0xffff) });
	unit.conversion("yama:Rune", false,
		[&ctx](Safe<YmType>) -> YmObj* { return ymCtx_NewFloat(ctx, 0.55); },
		Extracts{ .r = YmRune(0) });
	unit.conversion("yama:Rune", false,
		[&ctx](Safe<YmType>) -> YmObj* { return ymCtx_NewFloat(ctx, 1.55); },
		Extracts{ .r = YmRune(1) });
	// NOTE: YmUInt(-1) % 0x110000 == 0xffff (ie. NOT 0x10ffff)
	unit.conversion("yama:Rune", false,
		[&ctx](Safe<YmType>) -> YmObj* { return ymCtx_NewFloat(ctx, -1.55); },
		Extracts{ .r = YmRune(0xffff) });

	unit.nonconversions({
		"yama:Bool",
		"yama:Type",
		});
}

TEST(TypeCharacteristics, YamaBool) {
	SETUP_ALL(ctx);
	Unit unit(
		dm, ctx, err,
		"yama:Bool",
		"yama:Bool",
		YmKind_Struct,
		0,
		0,
		[&ctx](Safe<YmType> type) {
			EXPECT_EQ(ymCtx_DefaultInit(ctx, YM_NEWTOP, type), YM_TRUE);
			auto result = ymCtx_Local(ctx, 0, YM_BORROW);
			ASSERT_TRUE(result);
			EXPECT_EQ(ymObj_Type(result), type);
			testExtracts(result, Extracts{ .b = YM_FALSE });
		},
		[&ctx](Safe<YmType> type) {
			EXPECT_EQ(ymCtx_ExplicitInit(ctx, YM_NEWTOP, type, ""), YM_TRUE);
			auto result = ymCtx_Local(ctx, 0, YM_BORROW);
			ASSERT_TRUE(result);
			EXPECT_EQ(ymObj_Type(result), type);
			testExtracts(result, Extracts{ .b = YM_FALSE });
		},
		[&ctx, &err](Safe<YmType> type) {
			EXPECT_EQ(ymCtx_Call(ctx, type, 0, "", YM_DISCARD), YM_FALSE);
			EXPECT_GE(err[YmErrCode_NonCallableType], 1);
		});
	auto mkObject =
		[&ctx](Safe<YmType> type) -> YmObj* {
		return ymCtx_NewBool(ctx, YM_TRUE);
		};
	unit.object(mkObject, Extracts{ .b = YM_TRUE });
	unit.conversion("yama:Bool", true, mkObject, Extracts{ .b = YM_TRUE });
	unit.conversion("yama:Any", true, mkObject, Extracts{});

	unit.conversion("yama:None", false, mkObject, Extracts{});

	unit.conversion("yama:Int", false,
		[&ctx](Safe<YmType>) -> YmObj* { return ymCtx_NewBool(ctx, YM_TRUE); },
		Extracts{ .i = 1 });
	unit.conversion("yama:Int", false,
		[&ctx](Safe<YmType>) -> YmObj* { return ymCtx_NewBool(ctx, YM_FALSE); },
		Extracts{ .i = 0 });

	unit.conversion("yama:UInt", false,
		[&ctx](Safe<YmType>) -> YmObj* { return ymCtx_NewBool(ctx, YM_TRUE); },
		Extracts{ .ui = 1 });
	unit.conversion("yama:UInt", false,
		[&ctx](Safe<YmType>) -> YmObj* { return ymCtx_NewBool(ctx, YM_FALSE); },
		Extracts{ .ui = 0 });

	unit.conversion("yama:Float", false,
		[&ctx](Safe<YmType>) -> YmObj* { return ymCtx_NewBool(ctx, YM_TRUE); },
		Extracts{ .f = 1 });
	unit.conversion("yama:Float", false,
		[&ctx](Safe<YmType>) -> YmObj* { return ymCtx_NewBool(ctx, YM_FALSE); },
		Extracts{ .f = 0 });

	unit.nonconversions({
		"yama:Rune",
		"yama:Type",
		});
}

TEST(TypeCharacteristics, YamaRune) {
	SETUP_ALL(ctx);
	Unit unit(
		dm, ctx, err,
		"yama:Rune",
		"yama:Rune",
		YmKind_Struct,
		0,
		0,
		[&ctx](Safe<YmType> type) {
			EXPECT_EQ(ymCtx_DefaultInit(ctx, YM_NEWTOP, type), YM_TRUE);
			auto result = ymCtx_Local(ctx, 0, YM_BORROW);
			ASSERT_TRUE(result);
			EXPECT_EQ(ymObj_Type(result), type);
			testExtracts(result, Extracts{ .r = U'\0'});
		},
		[&ctx](Safe<YmType> type) {
			EXPECT_EQ(ymCtx_ExplicitInit(ctx, YM_NEWTOP, type, ""), YM_TRUE);
			auto result = ymCtx_Local(ctx, 0, YM_BORROW);
			ASSERT_TRUE(result);
			EXPECT_EQ(ymObj_Type(result), type);
			testExtracts(result, Extracts{ .r = U'\0'});
		},
		[&ctx, &err](Safe<YmType> type) {
			EXPECT_EQ(ymCtx_Call(ctx, type, 0, "", YM_DISCARD), YM_FALSE);
			EXPECT_GE(err[YmErrCode_NonCallableType], 1);
		});
	auto mkObject =
		[&ctx](Safe<YmType> type) -> YmObj* {
		return ymCtx_NewRune(ctx, U'&');
		};
	unit.object(mkObject, Extracts{ .r = U'&' });

	// Test that value > 0x10ffff wraps around to 0x0.
	unit.object(
		[&ctx](Safe<YmType> type) -> YmObj* {
			return ymCtx_NewRune(ctx, 0x110000); // Should wrap around to 0.
		},
		Extracts{ .r = YmRune(0) });

	unit.conversion("yama:Rune", true, mkObject, Extracts{ .r = U'&' });
	unit.conversion("yama:Any", true, mkObject, Extracts{});

	unit.conversion("yama:None", false, mkObject, Extracts{});
	
	unit.conversion("yama:Int", false,
		[&ctx](Safe<YmType>) -> YmObj* { return ymCtx_NewRune(ctx, YmRune(0)); },
		Extracts{ .i = 0 });
	unit.conversion("yama:Int", false,
		[&ctx](Safe<YmType>) -> YmObj* { return ymCtx_NewRune(ctx, U'a'); },
		Extracts{ .i = YmInt(U'a') });
	unit.conversion("yama:Int", false,
		[&ctx](Safe<YmType>) -> YmObj* { return ymCtx_NewRune(ctx, U'1'); },
		Extracts{ .i = YmInt(U'1') });
	unit.conversion("yama:Int", false,
		[&ctx](Safe<YmType>) -> YmObj* { return ymCtx_NewRune(ctx, YmRune(0x10ffff)); },
		Extracts{ .i = YmInt(0x10ffff) });
	unit.conversion("yama:Int", false,
		[&ctx](Safe<YmType>) -> YmObj* { return ymCtx_NewRune(ctx, YmRune(0x110000)); },
		Extracts{ .i = 0 });
	unit.conversion("yama:Int", false,
		[&ctx](Safe<YmType>) -> YmObj* { return ymCtx_NewRune(ctx, YmRune(0x110001)); },
		Extracts{ .i = 1 });
	
	unit.conversion("yama:UInt", false,
		[&ctx](Safe<YmType>) -> YmObj* { return ymCtx_NewRune(ctx, YmRune(0)); },
		Extracts{ .ui = 0 });
	unit.conversion("yama:UInt", false,
		[&ctx](Safe<YmType>) -> YmObj* { return ymCtx_NewRune(ctx, U'a'); },
		Extracts{ .ui = YmInt(U'a') });
	unit.conversion("yama:UInt", false,
		[&ctx](Safe<YmType>) -> YmObj* { return ymCtx_NewRune(ctx, U'1'); },
		Extracts{ .ui = YmInt(U'1') });
	unit.conversion("yama:UInt", false,
		[&ctx](Safe<YmType>) -> YmObj* { return ymCtx_NewRune(ctx, YmRune(0x10ffff)); },
		Extracts{ .ui = YmInt(0x10ffff) });
	unit.conversion("yama:UInt", false,
		[&ctx](Safe<YmType>) -> YmObj* { return ymCtx_NewRune(ctx, YmRune(0x110000)); },
		Extracts{ .ui = 0 });
	unit.conversion("yama:UInt", false,
		[&ctx](Safe<YmType>) -> YmObj* { return ymCtx_NewRune(ctx, YmRune(0x110001)); },
		Extracts{ .ui = 1 });

	unit.nonconversions({
		"yama:Float",
		"yama:Bool",
		"yama:Type",
		});
}

TEST(TypeCharacteristics, YamaType) {
	SETUP_ALL(ctx);
	Unit unit(
		dm, ctx, err,
		"yama:Type",
		"yama:Type",
		YmKind_Struct,
		0,
		0,
		[&ctx](Safe<YmType> type) {
			EXPECT_EQ(ymCtx_DefaultInit(ctx, YM_NEWTOP, type), YM_TRUE);
			auto result = ymCtx_Local(ctx, 0, YM_BORROW);
			ASSERT_TRUE(result);
			EXPECT_EQ(ymObj_Type(result), type);
			testExtracts(result, Extracts{ .t = ymCtx_LdNone(ctx) });
		},
		[&ctx](Safe<YmType> type) {
			EXPECT_EQ(ymCtx_ExplicitInit(ctx, YM_NEWTOP, type, ""), YM_TRUE);
			auto result = ymCtx_Local(ctx, 0, YM_BORROW);
			ASSERT_TRUE(result);
			EXPECT_EQ(ymObj_Type(result), type);
			testExtracts(result, Extracts{ .t = ymCtx_LdNone(ctx) });
		},
		[&ctx, &err](Safe<YmType> type) {
			EXPECT_EQ(ymCtx_Call(ctx, type, 0, "", YM_DISCARD), YM_FALSE);
			EXPECT_GE(err[YmErrCode_NonCallableType], 1);
		});
	auto mkObject =
		[&ctx](Safe<YmType> type) -> YmObj* {
		return ymCtx_NewType(ctx, ymCtx_LdFloat(ctx));
		};
	unit.object(mkObject, Extracts{ .t = ymCtx_LdFloat(ctx) });
	unit.conversion("yama:Type", true, mkObject, Extracts{ .t = ymCtx_LdFloat(ctx) });
	unit.conversion("yama:Any", true, mkObject, Extracts{});

	unit.conversion("yama:None", false, mkObject, Extracts{});

	unit.nonconversions({
		"yama:Int",
		"yama:UInt",
		"yama:Float",
		"yama:Bool",
		"yama:Rune",
		});
}

TEST(TypeCharacteristics, YamaAny) {
	SETUP_ALL(ctx);
	Unit unit(
		dm, ctx, err,
		"yama:Any",
		"yama:Any",
		YmKind_Protocol,
		0,
		0,
		[&ctx, &err](Safe<YmType> type) {
			EXPECT_EQ(ymCtx_DefaultInit(ctx, YM_DISCARD, type), YM_FALSE);
			EXPECT_GE(err[YmErrCode_NoDefaultValue], 1);
		},
		[&ctx, &err](Safe<YmType> type) {
			EXPECT_EQ(ymCtx_ExplicitInit(ctx, YM_DISCARD, type, ""), YM_FALSE);
			EXPECT_GE(err[YmErrCode_NonStructType], 1);
		},
		[&ctx, &err](Safe<YmType> type) {
			EXPECT_EQ(ymCtx_Call(ctx, type, 0, "", YM_DISCARD), YM_FALSE);
			EXPECT_GE(err[YmErrCode_NonCallableType], 1);
		});
	// TODO: Maybe add some 'unit.object' and 'unit.conversion' stuff.
}

TEST(TypeCharacteristics, Structs) {
	SETUP_ALL(ctx);
	SETUP_PARCELDEF(p_def);
	ASSERT_NE(ymParcelDef_AddStruct(p_def, "EmptyStruct"), YM_NO_TYPE_INDEX);
	ASSERT_NE(ymParcelDef_AddComputedProperty(p_def, "EmptyStruct", "computed", "yama:Int",
		ymInertCallBhvrFn, nullptr, ymInertCallBhvrFn, nullptr), YM_NO_TYPE_INDEX);
	ASSERT_NE(ymParcelDef_AddStruct(p_def, "NonEmptyStruct"), YM_NO_TYPE_INDEX);
	ASSERT_NE(ymParcelDef_AddStoredProperty(p_def, "NonEmptyStruct", "x", "yama:Int"), YM_NO_TYPE_INDEX);
	ASSERT_NE(ymParcelDef_AddStoredProperty(p_def, "NonEmptyStruct", "y", "yama:Int"), YM_NO_TYPE_INDEX);
	ASSERT_NE(ymParcelDef_AddStoredProperty(p_def, "NonEmptyStruct", "z", "yama:Int"), YM_NO_TYPE_INDEX);
	ASSERT_NE(ymParcelDef_AddComputedProperty(p_def, "NonEmptyStruct", "computed", "yama:Int",
		ymInertCallBhvrFn, nullptr, ymInertCallBhvrFn, nullptr), YM_NO_TYPE_INDEX);
	ASSERT_EQ(ymDm_BindParcelDef(dm, "p", p_def), YM_TRUE);
	{
		Unit unit(
			dm, ctx, err,
			"p:EmptyStruct",
			"p:EmptyStruct",
			YmKind_Struct,
			2,
			0,
			[&ctx](Safe<YmType> type) {
				EXPECT_EQ(ymCtx_DefaultInit(ctx, YM_NEWTOP, type), YM_TRUE);
				auto result = ymCtx_Local(ctx, 0, YM_BORROW);
				ASSERT_TRUE(result);
				EXPECT_EQ(ymObj_Type(result), type);
				testExtracts(result, Extracts{});
			},
			[&ctx](Safe<YmType> type) {
				EXPECT_EQ(ymCtx_ExplicitInit(ctx, YM_NEWTOP, type, ""), YM_TRUE);
				auto result = ymCtx_Local(ctx, 0, YM_BORROW);
				ASSERT_TRUE(result);
				EXPECT_EQ(ymObj_Type(result), type);
				testExtracts(result, Extracts{});
			},
			[&ctx, &err](Safe<YmType> type) {
				EXPECT_EQ(ymCtx_Call(ctx, type, 0, "", YM_DISCARD), YM_FALSE);
				EXPECT_GE(err[YmErrCode_NonCallableType], 1);
			});
		auto mkObject =
			[&ctx](Safe<YmType> type) -> YmObj* {
			return
				ymCtx_DefaultInit(ctx, YM_NEWTOP, type) == YM_TRUE
				? ymCtx_Pull(ctx)
				: nullptr;
			};
		unit.object(mkObject, Extracts{});
		unit.conversion("p:EmptyStruct", true, mkObject, Extracts{});
		unit.conversion("yama:Any", true, mkObject, Extracts{});

		unit.conversion("yama:None", false, mkObject, Extracts{});
	}
	{
		Unit unit(
			dm, ctx, err,
			"p:NonEmptyStruct",
			"p:NonEmptyStruct",
			YmKind_Struct,
			8,
			0,
			[&ctx, &err](Safe<YmType> type) {
				EXPECT_EQ(ymCtx_DefaultInit(ctx, YM_DISCARD, type), YM_FALSE);
				EXPECT_GE(err[YmErrCode_NoDefaultValue], 1);
			},
			[&ctx, &err](Safe<YmType> type) {
				ymCtx_PutInt(ctx, YM_NEWTOP, 10);
				ymCtx_PutInt(ctx, YM_NEWTOP, 11);
				ymCtx_PutInt(ctx, YM_NEWTOP, 12);
				EXPECT_EQ(ymCtx_ExplicitInit(ctx, YM_NEWTOP, type, "x,y,z"), YM_TRUE);
				ASSERT_EQ(ymCtx_Locals(ctx), 1);
				auto result = ymCtx_Local(ctx, 0, YM_BORROW);
				ASSERT_TRUE(result);
				EXPECT_EQ(ymObj_Type(result), type);
				testExtracts(result, Extracts{});
				ymCtx_Put(ctx, YM_NEWTOP, result, YM_BORROW);
				ymCtx_GetProperty(ctx, ymType_MemberByName(type, "x"), YM_NEWTOP);
				ymCtx_Put(ctx, YM_NEWTOP, result, YM_BORROW);
				ymCtx_GetProperty(ctx, ymType_MemberByName(type, "y"), YM_NEWTOP);
				ymCtx_Put(ctx, YM_NEWTOP, result, YM_BORROW);
				ymCtx_GetProperty(ctx, ymType_MemberByName(type, "z"), YM_NEWTOP);
				ASSERT_EQ(ymCtx_Locals(ctx), 4);
				ASSERT_EQ(ymObj_ToInt(ymCtx_Local(ctx, 1, YM_BORROW), nullptr), 10);
				ASSERT_EQ(ymObj_ToInt(ymCtx_Local(ctx, 2, YM_BORROW), nullptr), 11);
				ASSERT_EQ(ymObj_ToInt(ymCtx_Local(ctx, 3, YM_BORROW), nullptr), 12);
			},
			[&ctx, &err](Safe<YmType> type) {
				EXPECT_EQ(ymCtx_Call(ctx, type, 0, "", YM_DISCARD), YM_FALSE);
				EXPECT_GE(err[YmErrCode_NonCallableType], 1);
			});
		auto mkObject =
			[&ctx](Safe<YmType> type) -> YmObj* {
			return
				ymCtx_PutInt(ctx, YM_NEWTOP, 3) &&
				ymCtx_PutInt(ctx, YM_NEWTOP, 1) &&
				ymCtx_PutInt(ctx, YM_NEWTOP, 2) &&
				ymCtx_ExplicitInit(ctx, YM_NEWTOP, type, "x,y,z")
				? ymCtx_Pull(ctx)
				: nullptr;
			};
		unit.object(mkObject, Extracts{});
		unit.conversion("p:NonEmptyStruct", true, mkObject, Extracts{});
		unit.conversion("yama:Any", true, mkObject, Extracts{});
		
		unit.conversion("yama:None", false, mkObject, Extracts{});
	}
}

TEST(TypeCharacteristics, GenericStructs) {
	SETUP_ALL(ctx);
	SETUP_PARCELDEF(p_def);
	ASSERT_NE(ymParcelDef_AddStruct(p_def, "EmptyStruct"), YM_NO_TYPE_INDEX);
	ASSERT_NE(ymParcelDef_AddTypeParam(p_def, "EmptyStruct", "T", "yama:Any"), YM_NO_TYPE_PARAM_INDEX);
	ASSERT_NE(ymParcelDef_AddComputedProperty(p_def, "EmptyStruct", "computed", "yama:Int",
		ymInertCallBhvrFn, nullptr, ymInertCallBhvrFn, nullptr), YM_NO_TYPE_INDEX);
	ASSERT_NE(ymParcelDef_AddStruct(p_def, "NonEmptyStruct"), YM_NO_TYPE_INDEX);
	ASSERT_NE(ymParcelDef_AddTypeParam(p_def, "NonEmptyStruct", "T", "yama:Any"), YM_NO_TYPE_PARAM_INDEX);
	ASSERT_NE(ymParcelDef_AddStoredProperty(p_def, "NonEmptyStruct", "x", "$T"), YM_NO_TYPE_INDEX);
	ASSERT_NE(ymParcelDef_AddStoredProperty(p_def, "NonEmptyStruct", "y", "$T"), YM_NO_TYPE_INDEX);
	ASSERT_NE(ymParcelDef_AddStoredProperty(p_def, "NonEmptyStruct", "z", "$T"), YM_NO_TYPE_INDEX);
	ASSERT_NE(ymParcelDef_AddComputedProperty(p_def, "NonEmptyStruct", "computed", "$T",
		ymInertCallBhvrFn, nullptr, ymInertCallBhvrFn, nullptr), YM_NO_TYPE_INDEX);
	ASSERT_EQ(ymDm_BindParcelDef(dm, "p", p_def), YM_TRUE);
	{
		Unit unit(
			dm, ctx, err,
			"p:EmptyStruct[yama:Int]",
			"p:EmptyStruct[yama:Int]",
			YmKind_Struct,
			2,
			1,
			[&ctx](Safe<YmType> type) {
				EXPECT_EQ(ymCtx_DefaultInit(ctx, YM_NEWTOP, type), YM_TRUE);
				auto result = ymCtx_Local(ctx, 0, YM_BORROW);
				ASSERT_TRUE(result);
				EXPECT_EQ(ymObj_Type(result), type);
				testExtracts(result, Extracts{});
			},
			[&ctx](Safe<YmType> type) {
				EXPECT_EQ(ymCtx_ExplicitInit(ctx, YM_NEWTOP, type, ""), YM_TRUE);
				auto result = ymCtx_Local(ctx, 0, YM_BORROW);
				ASSERT_TRUE(result);
				EXPECT_EQ(ymObj_Type(result), type);
				testExtracts(result, Extracts{});
			},
			[&ctx, &err](Safe<YmType> type) {
				EXPECT_EQ(ymCtx_Call(ctx, type, 0, "", YM_DISCARD), YM_FALSE);
				EXPECT_GE(err[YmErrCode_NonCallableType], 1);
			});
		auto mkObject =
			[&ctx](Safe<YmType> type) -> YmObj* {
			return
				ymCtx_DefaultInit(ctx, YM_NEWTOP, type) == YM_TRUE
				? ymCtx_Pull(ctx)
				: nullptr;
			};
		unit.object(mkObject, Extracts{});
		unit.conversion("p:EmptyStruct[yama:Int]", true, mkObject, Extracts{});
		unit.conversion("yama:Any", true, mkObject, Extracts{});

		unit.conversion("yama:None", false, mkObject, Extracts{});
	}
	{
		Unit unit(
			dm, ctx, err,
			"p:EmptyStruct[p:EmptyStruct[yama:Int]]",
			"p:EmptyStruct[p:EmptyStruct[yama:Int]]",
			YmKind_Struct,
			2,
			1,
			[&ctx](Safe<YmType> type) {
				EXPECT_EQ(ymCtx_DefaultInit(ctx, YM_NEWTOP, type), YM_TRUE);
				auto result = ymCtx_Local(ctx, 0, YM_BORROW);
				ASSERT_TRUE(result);
				EXPECT_EQ(ymObj_Type(result), type);
				testExtracts(result, Extracts{});
			},
			[&ctx](Safe<YmType> type) {
				EXPECT_EQ(ymCtx_ExplicitInit(ctx, YM_NEWTOP, type, ""), YM_TRUE);
				auto result = ymCtx_Local(ctx, 0, YM_BORROW);
				ASSERT_TRUE(result);
				EXPECT_EQ(ymObj_Type(result), type);
				testExtracts(result, Extracts{});
			},
			[&ctx, &err](Safe<YmType> type) {
				EXPECT_EQ(ymCtx_Call(ctx, type, 0, "", YM_DISCARD), YM_FALSE);
				EXPECT_GE(err[YmErrCode_NonCallableType], 1);
			});
		auto mkObject =
			[&ctx](Safe<YmType> type) -> YmObj* {
			return
				ymCtx_DefaultInit(ctx, YM_NEWTOP, type) == YM_TRUE
				? ymCtx_Pull(ctx)
				: nullptr;
			};
		unit.object(mkObject, Extracts{});
		unit.conversion("p:EmptyStruct[p:EmptyStruct[yama:Int]]", true, mkObject, Extracts{});
		unit.conversion("yama:Any", true, mkObject, Extracts{});

		unit.conversion("yama:None", false, mkObject, Extracts{});
	}
	{
		Unit unit(
			dm, ctx, err,
			"p:NonEmptyStruct[yama:Int]",
			"p:NonEmptyStruct[yama:Int]",
			YmKind_Struct,
			8,
			1,
			[&ctx, &err](Safe<YmType> type) {
				EXPECT_EQ(ymCtx_DefaultInit(ctx, YM_DISCARD, type), YM_FALSE);
				EXPECT_GE(err[YmErrCode_NoDefaultValue], 1);
			},
			[&ctx, &err](Safe<YmType> type) {
				ymCtx_PutInt(ctx, YM_NEWTOP, 10);
				ymCtx_PutInt(ctx, YM_NEWTOP, 11);
				ymCtx_PutInt(ctx, YM_NEWTOP, 12);
				EXPECT_EQ(ymCtx_ExplicitInit(ctx, YM_NEWTOP, type, "x,y,z"), YM_TRUE);
				ASSERT_EQ(ymCtx_Locals(ctx), 1);
				auto result = ymCtx_Local(ctx, 0, YM_BORROW);
				ASSERT_TRUE(result);
				EXPECT_EQ(ymObj_Type(result), type);
				testExtracts(result, Extracts{});
				ymCtx_Put(ctx, YM_NEWTOP, result, YM_BORROW);
				ymCtx_GetProperty(ctx, ymType_MemberByName(type, "x"), YM_NEWTOP);
				ymCtx_Put(ctx, YM_NEWTOP, result, YM_BORROW);
				ymCtx_GetProperty(ctx, ymType_MemberByName(type, "y"), YM_NEWTOP);
				ymCtx_Put(ctx, YM_NEWTOP, result, YM_BORROW);
				ymCtx_GetProperty(ctx, ymType_MemberByName(type, "z"), YM_NEWTOP);
				ASSERT_EQ(ymCtx_Locals(ctx), 4);
				ASSERT_EQ(ymObj_ToInt(ymCtx_Local(ctx, 1, YM_BORROW), nullptr), 10);
				ASSERT_EQ(ymObj_ToInt(ymCtx_Local(ctx, 2, YM_BORROW), nullptr), 11);
				ASSERT_EQ(ymObj_ToInt(ymCtx_Local(ctx, 3, YM_BORROW), nullptr), 12);
			},
			[&ctx, &err](Safe<YmType> type) {
				EXPECT_EQ(ymCtx_Call(ctx, type, 0, "", YM_DISCARD), YM_FALSE);
				EXPECT_GE(err[YmErrCode_NonCallableType], 1);
			});
		auto mkObject =
			[&ctx](Safe<YmType> type) -> YmObj* {
			return
				ymCtx_PutInt(ctx, YM_NEWTOP, 3) &&
				ymCtx_PutInt(ctx, YM_NEWTOP, 1) &&
				ymCtx_PutInt(ctx, YM_NEWTOP, 2) &&
				ymCtx_ExplicitInit(ctx, YM_NEWTOP, type, "x,y,z")
				? ymCtx_Pull(ctx)
				: nullptr;
			};
		unit.object(mkObject, Extracts{});
		unit.conversion("p:NonEmptyStruct[yama:Int]", true, mkObject, Extracts{});
		unit.conversion("yama:Any", true, mkObject, Extracts{});

		unit.conversion("yama:None", false, mkObject, Extracts{});
	}
}

TEST(TypeCharacteristics, Protocols) {
	// NOTE: See protocol-values.cpp for test coverage of protocol value behaviour.

	SETUP_ALL(ctx);
	SETUP_PARCELDEF(p_def);
	ymParcelDef_AddProtocol(p_def, "P");
	ymDm_BindParcelDef(dm, "p", p_def);
	Unit unit(
		dm, ctx, err,
		"p:P",
		"p:P",
		YmKind_Protocol,
		0,
		0,
		[&ctx, &err](Safe<YmType> type) {
			EXPECT_EQ(ymCtx_DefaultInit(ctx, YM_DISCARD, type), YM_FALSE);
			EXPECT_GE(err[YmErrCode_NoDefaultValue], 1);
		},
		[&ctx, &err](Safe<YmType> type) {
			EXPECT_EQ(ymCtx_ExplicitInit(ctx, YM_DISCARD, type, ""), YM_FALSE);
			EXPECT_GE(err[YmErrCode_NonStructType], 1);
		},
		[&ctx, &err](Safe<YmType> type) {
			EXPECT_EQ(ymCtx_Call(ctx, type, 0, "", YM_DISCARD), YM_FALSE);
			EXPECT_GE(err[YmErrCode_NonCallableType], 1);
		});
}

TEST(TypeCharacteristics, GenericProtocols) {
	// NOTE: See protocol-values.cpp for test coverage of protocol value behaviour.

	SETUP_ALL(ctx);
	SETUP_PARCELDEF(p_def);
	ymParcelDef_AddProtocol(p_def, "P");
	ymParcelDef_AddTypeParam(p_def, "P", "T", "yama:Any");
	ymDm_BindParcelDef(dm, "p", p_def);
	{
		Unit unit(
			dm, ctx, err,
			"p:P[yama:Int]",
			"p:P[yama:Int]",
			YmKind_Protocol,
			0,
			1,
			[&ctx, &err](Safe<YmType> type) {
				EXPECT_EQ(ymCtx_DefaultInit(ctx, YM_DISCARD, type), YM_FALSE);
				EXPECT_GE(err[YmErrCode_NoDefaultValue], 1);
			},
			[&ctx, &err](Safe<YmType> type) {
				EXPECT_EQ(ymCtx_ExplicitInit(ctx, YM_DISCARD, type, ""), YM_FALSE);
				EXPECT_GE(err[YmErrCode_NonStructType], 1);
			},
			[&ctx, &err](Safe<YmType> type) {
				EXPECT_EQ(ymCtx_Call(ctx, type, 0, "", YM_DISCARD), YM_FALSE);
				EXPECT_GE(err[YmErrCode_NonCallableType], 1);
			});
	}
	{
		Unit unit(
			dm, ctx, err,
			"p:P[yama:Float]",
			"p:P[yama:Float]",
			YmKind_Protocol,
			0,
			1,
			[&ctx, &err](Safe<YmType> type) {
				EXPECT_EQ(ymCtx_DefaultInit(ctx, YM_DISCARD, type), YM_FALSE);
				EXPECT_GE(err[YmErrCode_NoDefaultValue], 1);
			},
			[&ctx, &err](Safe<YmType> type) {
				EXPECT_EQ(ymCtx_ExplicitInit(ctx, YM_DISCARD, type, ""), YM_FALSE);
				EXPECT_GE(err[YmErrCode_NonStructType], 1);
			},
			[&ctx, &err](Safe<YmType> type) {
				EXPECT_EQ(ymCtx_Call(ctx, type, 0, "", YM_DISCARD), YM_FALSE);
				EXPECT_GE(err[YmErrCode_NonCallableType], 1);
			});
	}
}

TEST(TypeCharacteristics, Fns) {
	SETUP_ALL(ctx);
	SETUP_PARCELDEF(p_def);
	ymParcelDef_AddFn(p_def,
		"identity",
		"yama:Int",
		[](YmCtx* ctx, void*) {
			ymCtx_Ret(ctx, ymCtx_Arg(ctx, 0, YM_TAKE), YM_TAKE);
		},
		nullptr);
	ASSERT_NE(ymParcelDef_AddParam(p_def, "identity", "x", "yama:Int"), YM_NO_PARAM_INDEX);
	ASSERT_EQ(ymDm_BindParcelDef(dm, "p", p_def), YM_TRUE);
	Unit unit(
		dm, ctx, err,
		"p:identity",
		std::nullopt,
		YmKind_Fn,
		0,
		0,
		[&ctx, &err](Safe<YmType> type) {
			EXPECT_EQ(ymCtx_DefaultInit(ctx, YM_DISCARD, type), YM_FALSE);
			EXPECT_GE(err[YmErrCode_NoDefaultValue], 1);
		},
		[&ctx, &err](Safe<YmType> type) {
			EXPECT_EQ(ymCtx_ExplicitInit(ctx, YM_DISCARD, type, ""), YM_FALSE);
			EXPECT_GE(err[YmErrCode_NonStructType], 1);
		},
		[&ctx](Safe<YmType> type) {
			ASSERT_EQ(ymCtx_PutInt(ctx, YM_NEWTOP, 101), YM_TRUE);
			ASSERT_EQ(ymCtx_Call(ctx, type, 1, "", YM_NEWTOP), YM_TRUE);
			if (auto top = ymCtx_Local(ctx, 0, YM_BORROW)) {
				EXPECT_EQ(ymObj_Type(top), ymCtx_LdInt(ctx));
				EXPECT_EQ(ymObj_ToInt(top, nullptr), 101);
			}
		});
}

TEST(TypeCharacteristics, GenericFns) {
	SETUP_ALL(ctx);
	SETUP_PARCELDEF(p_def);
	ymParcelDef_AddFn(p_def,
		"identity",
		"$T",
		[](YmCtx* ctx, void*) {
			ymCtx_Ret(ctx, ymCtx_Arg(ctx, 0, YM_TAKE), YM_TAKE);
		},
		nullptr);
	ASSERT_NE(ymParcelDef_AddTypeParam(p_def, "identity", "T", "yama:Any"), YM_NO_TYPE_PARAM_INDEX);
	ASSERT_NE(ymParcelDef_AddParam(p_def, "identity", "x", "$T"), YM_NO_PARAM_INDEX);
	ASSERT_EQ(ymDm_BindParcelDef(dm, "p", p_def), YM_TRUE);
	{
		Unit unit(
			dm, ctx, err,
			"p:identity[yama:Int]",
			std::nullopt,
			YmKind_Fn,
			0,
			1,
			[&ctx, &err](Safe<YmType> type) {
				EXPECT_EQ(ymCtx_DefaultInit(ctx, YM_DISCARD, type), YM_FALSE);
				EXPECT_GE(err[YmErrCode_NoDefaultValue], 1);
			},
			[&ctx, &err](Safe<YmType> type) {
				EXPECT_EQ(ymCtx_ExplicitInit(ctx, YM_DISCARD, type, ""), YM_FALSE);
				EXPECT_GE(err[YmErrCode_NonStructType], 1);
			},
			[&ctx](Safe<YmType> type) {
				ASSERT_EQ(ymCtx_PutInt(ctx, YM_NEWTOP, 101), YM_TRUE);
				ASSERT_EQ(ymCtx_Call(ctx, type, 1, "", YM_NEWTOP), YM_TRUE);
				if (auto top = ymCtx_Local(ctx, 0, YM_BORROW)) {
					EXPECT_EQ(ymObj_Type(top), ymCtx_LdInt(ctx));
					EXPECT_EQ(ymObj_ToInt(top, nullptr), 101);
				}
			});
	}
	{
		Unit unit(
			dm, ctx, err,
			"p:identity[yama:Float]",
			std::nullopt,
			YmKind_Fn,
			0,
			1,
			[&ctx, &err](Safe<YmType> type) {
				EXPECT_EQ(ymCtx_DefaultInit(ctx, YM_DISCARD, type), YM_FALSE);
				EXPECT_GE(err[YmErrCode_NoDefaultValue], 1);
			},
			[&ctx, &err](Safe<YmType> type) {
				EXPECT_EQ(ymCtx_ExplicitInit(ctx, YM_DISCARD, type, ""), YM_FALSE);
				EXPECT_GE(err[YmErrCode_NonStructType], 1);
			},
			[&ctx](Safe<YmType> type) {
				ASSERT_EQ(ymCtx_PutFloat(ctx, YM_NEWTOP, 3.14159), YM_TRUE);
				ASSERT_EQ(ymCtx_Call(ctx, type, 1, "", YM_NEWTOP), YM_TRUE);
				if (auto top = ymCtx_Local(ctx, 0, YM_BORROW)) {
					EXPECT_EQ(ymObj_Type(top), ymCtx_LdFloat(ctx));
					EXPECT_DOUBLE_EQ(ymObj_ToFloat(top, nullptr), 3.14159);
				}
			});
	}
}

// NOTE: Methods of protocol types, and their special dispatch behaviour, are
//		 tested elsewhere in protocol-values.cpp.

TEST(TypeCharacteristics, Methods) {
	SETUP_ALL(ctx);
	SETUP_PARCELDEF(p_def);
	ASSERT_NE(ymParcelDef_AddStruct(p_def, "A"), YM_NO_TYPE_INDEX);
	ymParcelDef_AddMethod(p_def,
		"A",
		"m",
		"yama:Int",
		[](YmCtx* ctx, void*) {
			ymCtx_Ret(ctx, ymCtx_Arg(ctx, 0, YM_TAKE), YM_TAKE);
		},
		nullptr);
	ASSERT_NE(ymParcelDef_AddParam(p_def, "A::m", "x", "yama:Int"), YM_NO_PARAM_INDEX);
	ASSERT_EQ(ymDm_BindParcelDef(dm, "p", p_def), YM_TRUE);
	Unit unit(
		dm, ctx, err,
		"p:A::m",
		std::nullopt,
		YmKind_Method,
		0,
		0,
		[&ctx, &err](Safe<YmType> type) {
			EXPECT_EQ(ymCtx_DefaultInit(ctx, YM_DISCARD, type), YM_FALSE);
			EXPECT_GE(err[YmErrCode_NoDefaultValue], 1);
		},
		[&ctx, &err](Safe<YmType> type) {
			EXPECT_EQ(ymCtx_ExplicitInit(ctx, YM_DISCARD, type, ""), YM_FALSE);
			EXPECT_GE(err[YmErrCode_NonStructType], 1);
		},
		[&ctx](Safe<YmType> type) {
			ASSERT_EQ(ymCtx_PutInt(ctx, YM_NEWTOP, 101), YM_TRUE);
			ASSERT_EQ(ymCtx_Call(ctx, type, 1, "", YM_NEWTOP), YM_TRUE);
			if (auto top = ymCtx_Local(ctx, 0, YM_BORROW)) {
				EXPECT_EQ(ymObj_Type(top), ymCtx_LdInt(ctx));
				EXPECT_EQ(ymObj_ToInt(top, nullptr), 101);
			}
		});
}

TEST(TypeCharacteristics, GenericTypeMethods) {
	SETUP_ALL(ctx);
	SETUP_PARCELDEF(p_def);
	auto A_ind = ymParcelDef_AddStruct(p_def, "A");
	ASSERT_NE(A_ind, YM_NO_TYPE_INDEX);
	ASSERT_NE(ymParcelDef_AddTypeParam(p_def, "A", "T", "yama:Any"), YM_NO_TYPE_PARAM_INDEX);
	ymParcelDef_AddMethod(p_def,
		"A",
		"m",
		"$T",
		[](YmCtx* ctx, void*) {
			ymCtx_Ret(ctx, ymCtx_Arg(ctx, 0, YM_TAKE), YM_TAKE);
		},
		nullptr);
	ASSERT_NE(ymParcelDef_AddParam(p_def, "A::m", "x", "$T"), YM_NO_PARAM_INDEX);
	ASSERT_EQ(ymDm_BindParcelDef(dm, "p", p_def), YM_TRUE);
	{
		Unit unit(
			dm, ctx, err,
			"p:A[yama:Int]::m",
			std::nullopt,
			YmKind_Method,
			0,
			1,
			[&ctx, &err](Safe<YmType> type) {
				EXPECT_EQ(ymCtx_DefaultInit(ctx, YM_DISCARD, type), YM_FALSE);
				EXPECT_GE(err[YmErrCode_NoDefaultValue], 1);
			},
			[&ctx, &err](Safe<YmType> type) {
				EXPECT_EQ(ymCtx_ExplicitInit(ctx, YM_DISCARD, type, ""), YM_FALSE);
				EXPECT_GE(err[YmErrCode_NonStructType], 1);
			},
			[&ctx](Safe<YmType> type) {
				ASSERT_EQ(ymCtx_PutInt(ctx, YM_NEWTOP, 101), YM_TRUE);
				ASSERT_EQ(ymCtx_Call(ctx, type, 1, "", YM_NEWTOP), YM_TRUE);
				if (auto top = ymCtx_Local(ctx, 0, YM_BORROW)) {
					EXPECT_EQ(ymObj_Type(top), ymCtx_LdInt(ctx));
					EXPECT_EQ(ymObj_ToInt(top, nullptr), 101);
				}
			});
	}
	{
		Unit unit(
			dm, ctx, err,
			"p:A[yama:Float]::m",
			std::nullopt,
			YmKind_Method,
			0,
			1,
			[&ctx, &err](Safe<YmType> type) {
				EXPECT_EQ(ymCtx_DefaultInit(ctx, YM_DISCARD, type), YM_FALSE);
				EXPECT_GE(err[YmErrCode_NoDefaultValue], 1);
			},
			[&ctx, &err](Safe<YmType> type) {
				EXPECT_EQ(ymCtx_ExplicitInit(ctx, YM_DISCARD, type, ""), YM_FALSE);
				EXPECT_GE(err[YmErrCode_NonStructType], 1);
			},
			[&ctx](Safe<YmType> type) {
				ASSERT_EQ(ymCtx_PutFloat(ctx, YM_NEWTOP, 3.14159), YM_TRUE);
				ASSERT_EQ(ymCtx_Call(ctx, type, 1, "", YM_NEWTOP), YM_TRUE);
				if (auto top = ymCtx_Local(ctx, 0, YM_BORROW)) {
					EXPECT_EQ(ymObj_Type(top), ymCtx_LdFloat(ctx));
					EXPECT_DOUBLE_EQ(ymObj_ToFloat(top, nullptr), 3.14159);
				}
			});
	}
}

// TODO: Didn't bother w/ tests w/ generic types w/ properties.

TEST(TypeCharacteristics, StoredProperties_IncludingReadOnlyOnesAndAssigners) {
	SETUP_ALL(ctx);
	SETUP_PARCELDEF(p_def);
	ASSERT_NE(ymParcelDef_AddStruct(p_def, "A"), YM_NO_TYPE_INDEX);
	ASSERT_NE(ymParcelDef_AddStoredProperty(p_def, "A", "a", "yama:Int"), YM_NO_TYPE_INDEX);
	ASSERT_NE(ymParcelDef_AddReadOnlyStoredProperty(p_def, "A", "b", "yama:Int"), YM_NO_TYPE_INDEX);
	ASSERT_EQ(ymDm_BindParcelDef(dm, "p", p_def), YM_TRUE);
	{
		Unit unit(
			dm, ctx, err,
			"p:A::a",
			std::nullopt,
			YmKind_Property,
			0,
			0,
			[&ctx, &err](Safe<YmType> type) {
				EXPECT_EQ(ymCtx_DefaultInit(ctx, YM_DISCARD, type), YM_FALSE);
				EXPECT_GE(err[YmErrCode_NoDefaultValue], 1);
			},
			[&ctx, &err](Safe<YmType> type) {
				EXPECT_EQ(ymCtx_ExplicitInit(ctx, YM_DISCARD, type, ""), YM_FALSE);
				EXPECT_GE(err[YmErrCode_NonStructType], 1);
			},
			[&ctx](Safe<YmType> type) {
				ASSERT_EQ(ymCtx_PutInt(ctx, YM_NEWTOP, 10), YM_TRUE);
				ASSERT_EQ(ymCtx_PutInt(ctx, YM_NEWTOP, 11), YM_TRUE);
				EXPECT_EQ(ymCtx_ExplicitInit(ctx, YM_NEWTOP, ymType_Owner(type), "a,b"), YM_TRUE);
				ASSERT_EQ(ymCtx_Call(ctx, type, 1, "", YM_NEWTOP), YM_TRUE);
				if (auto top = ymCtx_Local(ctx, 0, YM_BORROW)) {
					EXPECT_EQ(ymObj_Type(top), ymCtx_LdInt(ctx));
					EXPECT_EQ(ymObj_ToInt(top, nullptr), 10);
				}
			});
	}
	{
		Unit unit(
			dm, ctx, err,
			"p:A::a$assigner",
			std::nullopt,
			YmKind_PropertyAssigner,
			0,
			0,
			[&ctx, &err](Safe<YmType> type) {
				EXPECT_EQ(ymCtx_DefaultInit(ctx, YM_DISCARD, type), YM_FALSE);
				EXPECT_GE(err[YmErrCode_NoDefaultValue], 1);
			},
			[&ctx, &err](Safe<YmType> type) {
				EXPECT_EQ(ymCtx_ExplicitInit(ctx, YM_DISCARD, type, ""), YM_FALSE);
				EXPECT_GE(err[YmErrCode_NonStructType], 1);
			},
			[&ctx](Safe<YmType> type) {
				auto A_a = ymCtx_Load(ctx, "p:A::a");
				ASSERT_TRUE(A_a);
				ASSERT_EQ(ymCtx_PutInt(ctx, YM_NEWTOP, 10), YM_TRUE);
				ASSERT_EQ(ymCtx_PutInt(ctx, YM_NEWTOP, 11), YM_TRUE);
				EXPECT_EQ(ymCtx_ExplicitInit(ctx, YM_NEWTOP, ymType_Owner(type), "a,b"), YM_TRUE);
				ASSERT_EQ(ymCtx_Put(ctx, YM_NEWTOP, ymCtx_Local(ctx, 0, YM_BORROW), YM_BORROW), YM_TRUE);
				ASSERT_EQ(ymCtx_PutInt(ctx, YM_NEWTOP, 22), YM_TRUE);
				ASSERT_EQ(ymCtx_Call(ctx, type, 2, "", YM_DISCARD), YM_TRUE);
				ASSERT_EQ(ymCtx_Put(ctx, YM_NEWTOP, ymCtx_Local(ctx, 0, YM_BORROW), YM_BORROW), YM_TRUE);
				ASSERT_EQ(ymCtx_GetProperty(ctx, A_a, YM_NEWTOP), YM_TRUE);
				if (auto top = ymCtx_Local(ctx, 1, YM_BORROW)) {
					EXPECT_EQ(ymObj_Type(top), ymCtx_LdInt(ctx));
					EXPECT_EQ(ymObj_ToInt(top, nullptr), 22);
				}
			});
	}
	{
		Unit unit(
			dm, ctx, err,
			"p:A::b",
			std::nullopt,
			YmKind_Property,
			0,
			0,
			[&ctx, &err](Safe<YmType> type) {
				EXPECT_EQ(ymCtx_DefaultInit(ctx, YM_DISCARD, type), YM_FALSE);
				EXPECT_GE(err[YmErrCode_NoDefaultValue], 1);
			},
			[&ctx, &err](Safe<YmType> type) {
				EXPECT_EQ(ymCtx_ExplicitInit(ctx, YM_DISCARD, type, ""), YM_FALSE);
				EXPECT_GE(err[YmErrCode_NonStructType], 1);
			},
			[&ctx](Safe<YmType> type) {
				ASSERT_EQ(ymCtx_PutInt(ctx, YM_NEWTOP, 10), YM_TRUE);
				ASSERT_EQ(ymCtx_PutInt(ctx, YM_NEWTOP, 11), YM_TRUE);
				EXPECT_EQ(ymCtx_ExplicitInit(ctx, YM_NEWTOP, ymType_Owner(type), "a,b"), YM_TRUE);
				ASSERT_EQ(ymCtx_Call(ctx, type, 1, "", YM_NEWTOP), YM_TRUE);
				if (auto top = ymCtx_Local(ctx, 1, YM_BORROW)) {
					EXPECT_EQ(ymObj_Type(top), ymCtx_LdInt(ctx));
					EXPECT_EQ(ymObj_ToInt(top, nullptr), 11);
				}
			});
	}
}

static size_t computedPropertyCalls = 0;

TEST(TypeCharacteristics, ComputedProperties_IncludingReadOnlyOnesAndAssigners) {
	SETUP_ALL(ctx);
	SETUP_PARCELDEF(p_def);
	ASSERT_NE(ymParcelDef_AddStruct(p_def, "A"), YM_NO_TYPE_INDEX);
	ASSERT_NE(ymParcelDef_AddComputedProperty(p_def, "A", "a", "yama:Int",
		[](YmCtx* ctx, void*) {
			computedPropertyCalls++;
			ymCtx_Ret(ctx, ymCtx_NewInt(ctx, 101), YM_TAKE);
		},
		nullptr,
		[](YmCtx* ctx, void*) {
			computedPropertyCalls++;
			ymCtx_Ret(ctx, ymCtx_NewNone(ctx), YM_TAKE);
			if (auto top = ymCtx_Arg(ctx, 1, YM_BORROW)) {
				EXPECT_EQ(ymObj_Type(top), ymCtx_LdInt(ctx));
				EXPECT_EQ(ymObj_ToInt(top, nullptr), 22);
			}
		},
		nullptr),
		YM_NO_TYPE_INDEX);
	ASSERT_NE(ymParcelDef_AddReadOnlyComputedProperty(p_def, "A", "b", "yama:Int",
		[](YmCtx* ctx, void*) {
			computedPropertyCalls++;
			ymCtx_Ret(ctx, ymCtx_NewInt(ctx, 202), YM_TAKE);
		},
		nullptr),
		YM_NO_TYPE_INDEX);
	ASSERT_EQ(ymDm_BindParcelDef(dm, "p", p_def), YM_TRUE);
	{
		Unit unit(
			dm, ctx, err,
			"p:A::a",
			std::nullopt,
			YmKind_Property,
			0,
			0,
			[&ctx, &err](Safe<YmType> type) {
				EXPECT_EQ(ymCtx_DefaultInit(ctx, YM_DISCARD, type), YM_FALSE);
				EXPECT_GE(err[YmErrCode_NoDefaultValue], 1);
			},
			[&ctx, &err](Safe<YmType> type) {
				EXPECT_EQ(ymCtx_ExplicitInit(ctx, YM_DISCARD, type, ""), YM_FALSE);
				EXPECT_GE(err[YmErrCode_NonStructType], 1);
			},
			[&ctx](Safe<YmType> type) {
				EXPECT_EQ(ymCtx_ExplicitInit(ctx, YM_NEWTOP, ymType_Owner(type), ""), YM_TRUE);
				computedPropertyCalls = 0;
				ASSERT_EQ(ymCtx_Call(ctx, type, 1, "", YM_NEWTOP), YM_TRUE);
				EXPECT_EQ(computedPropertyCalls, 1);
				if (auto top = ymCtx_Local(ctx, 0, YM_BORROW)) {
					EXPECT_EQ(ymObj_Type(top), ymCtx_LdInt(ctx));
					EXPECT_EQ(ymObj_ToInt(top, nullptr), 101);
				}
			});
	}
	{
		Unit unit(
			dm, ctx, err,
			"p:A::a$assigner",
			std::nullopt,
			YmKind_PropertyAssigner,
			0,
			0,
			[&ctx, &err](Safe<YmType> type) {
				EXPECT_EQ(ymCtx_DefaultInit(ctx, YM_DISCARD, type), YM_FALSE);
				EXPECT_GE(err[YmErrCode_NoDefaultValue], 1);
			},
			[&ctx, &err](Safe<YmType> type) {
				EXPECT_EQ(ymCtx_ExplicitInit(ctx, YM_DISCARD, type, ""), YM_FALSE);
				EXPECT_GE(err[YmErrCode_NonStructType], 1);
			},
			[&ctx](Safe<YmType> type) {
				EXPECT_EQ(ymCtx_ExplicitInit(ctx, YM_NEWTOP, ymType_Owner(type), ""), YM_TRUE);
				ASSERT_EQ(ymCtx_Put(ctx, YM_NEWTOP, ymCtx_Local(ctx, 0, YM_BORROW), YM_BORROW), YM_TRUE);
				ASSERT_EQ(ymCtx_PutInt(ctx, YM_NEWTOP, 22), YM_TRUE);
				computedPropertyCalls = 0;
				ASSERT_EQ(ymCtx_Call(ctx, type, 2, "", YM_DISCARD), YM_TRUE);
				EXPECT_EQ(computedPropertyCalls, 1);
			});
	}
	{
		Unit unit(
			dm, ctx, err,
			"p:A::b",
			std::nullopt,
			YmKind_Property,
			0,
			0,
			[&ctx, &err](Safe<YmType> type) {
				EXPECT_EQ(ymCtx_DefaultInit(ctx, YM_DISCARD, type), YM_FALSE);
				EXPECT_GE(err[YmErrCode_NoDefaultValue], 1);
			},
			[&ctx, &err](Safe<YmType> type) {
				EXPECT_EQ(ymCtx_ExplicitInit(ctx, YM_DISCARD, type, ""), YM_FALSE);
				EXPECT_GE(err[YmErrCode_NonStructType], 1);
			},
			[&ctx](Safe<YmType> type) {
				EXPECT_EQ(ymCtx_ExplicitInit(ctx, YM_NEWTOP, ymType_Owner(type), ""), YM_TRUE);
				computedPropertyCalls = 0;
				ASSERT_EQ(ymCtx_Call(ctx, type, 1, "", YM_NEWTOP), YM_TRUE);
				EXPECT_EQ(computedPropertyCalls, 1);
				if (auto top = ymCtx_Local(ctx, 1, YM_BORROW)) {
					EXPECT_EQ(ymObj_Type(top), ymCtx_LdInt(ctx));
					EXPECT_EQ(ymObj_ToInt(top, nullptr), 202);
				}
			});
	}
}

