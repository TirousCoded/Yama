

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
			std::function<void(Safe<YmType> type)> testPutDefault,
			// Test calling w/ ymCtx_Call.
			std::function<void(Safe<YmType> type)> testCall) :
			dm(dm),
			ctx(ctx),
			err(err) {
			_setup(
				dm, ctx, err,
				fullname, objectTypeFln,
				kind, members, typeParams,
				testPutDefault, testCall);
		}

		void object(
			std::function<YmObj* (Safe<YmType> type)> objectMaker,
			Extracts extracts,
			std::function<void(Safe<YmObj> result)> otherChecks = [](Safe<YmObj>) {}) {
			if (!valid) return;

			ym::println("example object");

			ymCtx_PopN(ctx, ymCtx_Locals(ctx));
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

			ymCtx_PopN(ctx, ymCtx_Locals(ctx));
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
			std::function<void(Safe<YmType> type)> testPutDefault,
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

			ymCtx_PopN(ctx, ymCtx_Locals(ctx));
			ASSERT_EQ(ymCtx_Locals(ctx), 0);
			err.reset();
			testPutDefault(Safe(type));

			ymCtx_PopN(ctx, ymCtx_Locals(ctx));
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
			EXPECT_EQ(ymCtx_PutDefault(ctx, YM_NEWTOP, type), YM_TRUE);
			auto result = ymCtx_Local(ctx, 0, YM_BORROW);
			ASSERT_TRUE(result);
			EXPECT_EQ(ymObj_Type(result), type);
			testExtracts(result, Extracts{});
		},
		[&ctx, &err](Safe<YmType> type) {
			EXPECT_EQ(ymCtx_Call(ctx, type, 0, YM_DISCARD), YM_FALSE);
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
			EXPECT_EQ(ymCtx_PutDefault(ctx, YM_NEWTOP, type), YM_TRUE);
			auto result = ymCtx_Local(ctx, 0, YM_BORROW);
			ASSERT_TRUE(result);
			EXPECT_EQ(ymObj_Type(result), type);
			testExtracts(result, Extracts{ .i = 0 });
		},
		[&ctx, &err](Safe<YmType> type) {
			EXPECT_EQ(ymCtx_Call(ctx, type, 0, YM_DISCARD), YM_FALSE);
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
			EXPECT_EQ(ymCtx_PutDefault(ctx, YM_NEWTOP, type), YM_TRUE);
			auto result = ymCtx_Local(ctx, 0, YM_BORROW);
			ASSERT_TRUE(result);
			EXPECT_EQ(ymObj_Type(result), type);
			testExtracts(result, Extracts{ .ui = 0 });
		},
		[&ctx, &err](Safe<YmType> type) {
			EXPECT_EQ(ymCtx_Call(ctx, type, 0, YM_DISCARD), YM_FALSE);
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
			EXPECT_EQ(ymCtx_PutDefault(ctx, YM_NEWTOP, type), YM_TRUE);
			auto result = ymCtx_Local(ctx, 0, YM_BORROW);
			ASSERT_TRUE(result);
			EXPECT_EQ(ymObj_Type(result), type);
			testExtracts(result, Extracts{ .f = 0 });
		},
		[&ctx, &err](Safe<YmType> type) {
			EXPECT_EQ(ymCtx_Call(ctx, type, 0, YM_DISCARD), YM_FALSE);
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
			EXPECT_EQ(ymCtx_PutDefault(ctx, YM_NEWTOP, type), YM_TRUE);
			auto result = ymCtx_Local(ctx, 0, YM_BORROW);
			ASSERT_TRUE(result);
			EXPECT_EQ(ymObj_Type(result), type);
			testExtracts(result, Extracts{ .b = YM_FALSE });
		},
		[&ctx, &err](Safe<YmType> type) {
			EXPECT_EQ(ymCtx_Call(ctx, type, 0, YM_DISCARD), YM_FALSE);
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
			EXPECT_EQ(ymCtx_PutDefault(ctx, YM_NEWTOP, type), YM_TRUE);
			auto result = ymCtx_Local(ctx, 0, YM_BORROW);
			ASSERT_TRUE(result);
			EXPECT_EQ(ymObj_Type(result), type);
			testExtracts(result, Extracts{ .r = U'\0'});
		},
		[&ctx, &err](Safe<YmType> type) {
			EXPECT_EQ(ymCtx_Call(ctx, type, 0, YM_DISCARD), YM_FALSE);
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
			EXPECT_EQ(ymCtx_PutDefault(ctx, YM_NEWTOP, type), YM_TRUE);
			auto result = ymCtx_Local(ctx, 0, YM_BORROW);
			ASSERT_TRUE(result);
			EXPECT_EQ(ymObj_Type(result), type);
			testExtracts(result, Extracts{ .t = ymCtx_LdNone(ctx) });
		},
		[&ctx, &err](Safe<YmType> type) {
			EXPECT_EQ(ymCtx_Call(ctx, type, 0, YM_DISCARD), YM_FALSE);
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
		[&ctx](Safe<YmType> type) {
			ADD_FAILURE(); // TODO
		},
		[&ctx, &err](Safe<YmType> type) {
			EXPECT_EQ(ymCtx_Call(ctx, type, 0, YM_DISCARD), YM_FALSE);
			EXPECT_GE(err[YmErrCode_NonCallableType], 1);
		});
	ADD_FAILURE(); // TODO: Write 'object' and 'conversion' calls when we have a way to conv to Any.
}

TEST(TypeCharacteristics, Structs) {
	SETUP_ALL(ctx);
	SETUP_PARCELDEF(p_def);
	ASSERT_NE(ymParcelDef_AddStruct(p_def, "EmptyStruct"), YM_NO_TYPE_INDEX);
	ASSERT_EQ(ymDm_BindParcelDef(dm, "p", p_def), YM_TRUE);
	Unit unit(
		dm, ctx, err,
		"p:EmptyStruct",
		"p:EmptyStruct",
		YmKind_Struct,
		0,
		0,
		[&ctx](Safe<YmType> type) {
			EXPECT_EQ(ymCtx_PutDefault(ctx, YM_NEWTOP, type), YM_TRUE);
			auto result = ymCtx_Local(ctx, 0, YM_BORROW);
			ASSERT_TRUE(result);
			EXPECT_EQ(ymObj_Type(result), type);
			testExtracts(result, Extracts{});
		},
		[&ctx, &err](Safe<YmType> type) {
			EXPECT_EQ(ymCtx_Call(ctx, type, 0, YM_DISCARD), YM_FALSE);
			EXPECT_GE(err[YmErrCode_NonCallableType], 1);
		});
	auto mkObject =
		[&ctx](Safe<YmType> type) -> YmObj* {
		return
			ymCtx_PutDefault(ctx, YM_NEWTOP, type) == YM_TRUE
			? ymCtx_Pop(ctx)
			: nullptr;
		};
	unit.object(mkObject, Extracts{});
	unit.conversion("p:EmptyStruct", true, mkObject, Extracts{});
	unit.conversion("yama:Any", true, mkObject, Extracts{});

	unit.conversion("yama:None", false, mkObject, Extracts{});
}

TEST(TypeCharacteristics, GenericStructs) {
	SETUP_ALL(ctx);
	SETUP_PARCELDEF(p_def);
	auto emptyStruct_ind = ymParcelDef_AddStruct(p_def, "EmptyStruct");
	ASSERT_NE(emptyStruct_ind, YM_NO_TYPE_INDEX);
	ASSERT_NE(ymParcelDef_AddTypeParam(p_def, emptyStruct_ind, "T", "yama:Any"), YM_NO_TYPE_PARAM_INDEX);
	ASSERT_EQ(ymDm_BindParcelDef(dm, "p", p_def), YM_TRUE);
	{
		Unit unit(
			dm, ctx, err,
			"p:EmptyStruct[yama:Int]",
			"p:EmptyStruct[yama:Int]",
			YmKind_Struct,
			0,
			1,
			[&ctx](Safe<YmType> type) {
				EXPECT_EQ(ymCtx_PutDefault(ctx, YM_NEWTOP, type), YM_TRUE);
				auto result = ymCtx_Local(ctx, 0, YM_BORROW);
				ASSERT_TRUE(result);
				EXPECT_EQ(ymObj_Type(result), type);
				testExtracts(result, Extracts{});
			},
			[&ctx, &err](Safe<YmType> type) {
				EXPECT_EQ(ymCtx_Call(ctx, type, 0, YM_DISCARD), YM_FALSE);
				EXPECT_GE(err[YmErrCode_NonCallableType], 1);
			});
		auto mkObject =
			[&ctx](Safe<YmType> type) -> YmObj* {
			return
				ymCtx_PutDefault(ctx, YM_NEWTOP, type) == YM_TRUE
				? ymCtx_Pop(ctx)
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
			0,
			1,
			[&ctx](Safe<YmType> type) {
				EXPECT_EQ(ymCtx_PutDefault(ctx, YM_NEWTOP, type), YM_TRUE);
				auto result = ymCtx_Local(ctx, 0, YM_BORROW);
				ASSERT_TRUE(result);
				EXPECT_EQ(ymObj_Type(result), type);
				testExtracts(result, Extracts{});
			},
			[&ctx, &err](Safe<YmType> type) {
				EXPECT_EQ(ymCtx_Call(ctx, type, 0, YM_DISCARD), YM_FALSE);
				EXPECT_GE(err[YmErrCode_NonCallableType], 1);
			});
		auto mkObject =
			[&ctx](Safe<YmType> type) -> YmObj* {
			return
				ymCtx_PutDefault(ctx, YM_NEWTOP, type) == YM_TRUE
				? ymCtx_Pop(ctx)
				: nullptr;
			};
		unit.object(mkObject, Extracts{});
		unit.conversion("p:EmptyStruct[p:EmptyStruct[yama:Int]]", true, mkObject, Extracts{});
		unit.conversion("yama:Any", true, mkObject, Extracts{});

		unit.conversion("yama:None", false, mkObject, Extracts{});
	}
}

TEST(TypeCharacteristics, Protocols) {
	ADD_FAILURE();
}

TEST(TypeCharacteristics, GenericProtocols) {
	ADD_FAILURE();
}

TEST(TypeCharacteristics, Fns) {
	SETUP_ALL(ctx);
	SETUP_PARCELDEF(p_def);
	auto identity_ind = ymParcelDef_AddFn(p_def,
		"identity",
		"yama:Int",
		[](YmCtx* ctx, void*) {
			ymCtx_Ret(ctx, ymCtx_Arg(ctx, 0, YM_TAKE), YM_TAKE);
		},
		nullptr);
	ASSERT_NE(identity_ind, YM_NO_TYPE_INDEX);
	ASSERT_NE(ymParcelDef_AddParam(p_def, identity_ind, "x", "yama:Int"), YM_NO_PARAM_INDEX);
	ASSERT_EQ(ymDm_BindParcelDef(dm, "p", p_def), YM_TRUE);
	Unit unit(
		dm, ctx, err,
		"p:identity",
		std::nullopt,
		YmKind_Fn,
		0,
		0,
		[&ctx, &err](Safe<YmType> type) {
			EXPECT_EQ(ymCtx_PutDefault(ctx, YM_DISCARD, type), YM_FALSE);
			EXPECT_GE(err[YmErrCode_NoDefaultValue], 1);
		},
		[&ctx](Safe<YmType> type) {
			ASSERT_EQ(ymCtx_PutInt(ctx, YM_NEWTOP, 101), YM_TRUE);
			ASSERT_EQ(ymCtx_Call(ctx, type, 1, YM_NEWTOP), YM_TRUE);
			if (auto top = ymCtx_Local(ctx, 0, YM_BORROW)) {
				EXPECT_EQ(ymObj_Type(top), ymCtx_LdInt(ctx));
				EXPECT_EQ(ymObj_ToInt(top, nullptr), 101);
			}
		});
}

TEST(TypeCharacteristics, GenericFns) {
	SETUP_ALL(ctx);
	SETUP_PARCELDEF(p_def);
	auto identity_ind = ymParcelDef_AddFn(p_def,
		"identity",
		"$T",
		[](YmCtx* ctx, void*) {
			ymCtx_Ret(ctx, ymCtx_Arg(ctx, 0, YM_TAKE), YM_TAKE);
		},
		nullptr);
	ASSERT_NE(identity_ind, YM_NO_TYPE_INDEX);
	ASSERT_NE(ymParcelDef_AddTypeParam(p_def, identity_ind, "T", "yama:Any"), YM_NO_TYPE_PARAM_INDEX);
	ASSERT_NE(ymParcelDef_AddParam(p_def, identity_ind, "x", "$T"), YM_NO_PARAM_INDEX);
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
				EXPECT_EQ(ymCtx_PutDefault(ctx, YM_DISCARD, type), YM_FALSE);
				EXPECT_GE(err[YmErrCode_NoDefaultValue], 1);
			},
			[&ctx](Safe<YmType> type) {
				ASSERT_EQ(ymCtx_PutInt(ctx, YM_NEWTOP, 101), YM_TRUE);
				ASSERT_EQ(ymCtx_Call(ctx, type, 1, YM_NEWTOP), YM_TRUE);
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
				EXPECT_EQ(ymCtx_PutDefault(ctx, YM_DISCARD, type), YM_FALSE);
				EXPECT_GE(err[YmErrCode_NoDefaultValue], 1);
			},
			[&ctx](Safe<YmType> type) {
				ASSERT_EQ(ymCtx_PutFloat(ctx, YM_NEWTOP, 3.14159), YM_TRUE);
				ASSERT_EQ(ymCtx_Call(ctx, type, 1, YM_NEWTOP), YM_TRUE);
				if (auto top = ymCtx_Local(ctx, 0, YM_BORROW)) {
					EXPECT_EQ(ymObj_Type(top), ymCtx_LdFloat(ctx));
					EXPECT_DOUBLE_EQ(ymObj_ToFloat(top, nullptr), 3.14159);
				}
			});
	}
}

TEST(TypeCharacteristics, Methods) {
	SETUP_ALL(ctx);
	SETUP_PARCELDEF(p_def);
	auto A_ind = ymParcelDef_AddStruct(p_def, "A");
	ASSERT_NE(A_ind, YM_NO_TYPE_INDEX);
	auto A_m_ind = ymParcelDef_AddMethod(p_def,
		A_ind,
		"m",
		"yama:Int",
		[](YmCtx* ctx, void*) {
			ymCtx_Ret(ctx, ymCtx_Arg(ctx, 0, YM_TAKE), YM_TAKE);
		},
		nullptr);
	ASSERT_NE(A_m_ind, YM_NO_TYPE_INDEX);
	ASSERT_NE(ymParcelDef_AddParam(p_def, A_m_ind, "x", "yama:Int"), YM_NO_PARAM_INDEX);
	ASSERT_EQ(ymDm_BindParcelDef(dm, "p", p_def), YM_TRUE);
	Unit unit(
		dm, ctx, err,
		"p:A::m",
		std::nullopt,
		YmKind_Method,
		0,
		0,
		[&ctx, &err](Safe<YmType> type) {
			EXPECT_EQ(ymCtx_PutDefault(ctx, YM_DISCARD, type), YM_FALSE);
			EXPECT_GE(err[YmErrCode_NoDefaultValue], 1);
		},
		[&ctx](Safe<YmType> type) {
			ASSERT_EQ(ymCtx_PutInt(ctx, YM_NEWTOP, 101), YM_TRUE);
			ASSERT_EQ(ymCtx_Call(ctx, type, 1, YM_NEWTOP), YM_TRUE);
			if (auto top = ymCtx_Local(ctx, 0, YM_BORROW)) {
				EXPECT_EQ(ymObj_Type(top), ymCtx_LdInt(ctx));
				EXPECT_EQ(ymObj_ToInt(top, nullptr), 101);
			}
		});
}

TEST(TypeCharacteristics, MethodsOfGenericTypes) {
	SETUP_ALL(ctx);
	SETUP_PARCELDEF(p_def);
	auto A_ind = ymParcelDef_AddStruct(p_def, "A");
	ASSERT_NE(A_ind, YM_NO_TYPE_INDEX);
	ASSERT_NE(ymParcelDef_AddTypeParam(p_def, A_ind, "T", "yama:Any"), YM_NO_TYPE_PARAM_INDEX);
	auto A_m_ind = ymParcelDef_AddMethod(p_def,
		A_ind,
		"m",
		"$T",
		[](YmCtx* ctx, void*) {
			ymCtx_Ret(ctx, ymCtx_Arg(ctx, 0, YM_TAKE), YM_TAKE);
		},
		nullptr);
	ASSERT_NE(ymParcelDef_AddParam(p_def, A_m_ind, "x", "$T"), YM_NO_PARAM_INDEX);
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
				EXPECT_EQ(ymCtx_PutDefault(ctx, YM_DISCARD, type), YM_FALSE);
				EXPECT_GE(err[YmErrCode_NoDefaultValue], 1);
			},
			[&ctx](Safe<YmType> type) {
				ASSERT_EQ(ymCtx_PutInt(ctx, YM_NEWTOP, 101), YM_TRUE);
				ASSERT_EQ(ymCtx_Call(ctx, type, 1, YM_NEWTOP), YM_TRUE);
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
				EXPECT_EQ(ymCtx_PutDefault(ctx, YM_DISCARD, type), YM_FALSE);
				EXPECT_GE(err[YmErrCode_NoDefaultValue], 1);
			},
			[&ctx](Safe<YmType> type) {
				ASSERT_EQ(ymCtx_PutFloat(ctx, YM_NEWTOP, 3.14159), YM_TRUE);
				ASSERT_EQ(ymCtx_Call(ctx, type, 1, YM_NEWTOP), YM_TRUE);
				if (auto top = ymCtx_Local(ctx, 0, YM_BORROW)) {
					EXPECT_EQ(ymObj_Type(top), ymCtx_LdFloat(ctx));
					EXPECT_DOUBLE_EQ(ymObj_ToFloat(top, nullptr), 3.14159);
				}
			});
	}
}

TEST(TypeCharacteristics, MethodsOfProtocolTypes) {
	ADD_FAILURE();
}

TEST(TypeCharacteristics, MethodsOfGenericProtocolTypes) {
	ADD_FAILURE();
}

