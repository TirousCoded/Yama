

#include <gtest/gtest.h>
#include <taul/strings.h>
#include <yama/yama.h>
#include <yama++/general.h>
#include <yama++/print.h>

#include "../../utils/utils.h"


using namespace ym;


namespace {
	class ProtocolValues : public testing::Test {
	public:
		ErrCounter err;
		Scoped<YmDm> dm;
		Scoped<YmCtx> ctx;


		ProtocolValues() :
			dm(makeScoped<YmDm>()),
			ctx(makeScoped<YmCtx>(dm.get())) {
		}


	protected:
		void SetUp() override {
			err.setupCallbackForThisThread();
		}
		void TearDown() override {
			//
		}
	};
}

TEST_F(ProtocolValues, BoxingAndUnboxing) {
	SETUP_PARCELDEF(p_def);

	auto P_ind = ymParcelDef_AddProtocol(p_def, "P");
	ymParcelDef_AddMethodReq(p_def, P_ind, "m", "yama:Int");

	auto Q_ind = ymParcelDef_AddProtocol(p_def, "Q");
	ymParcelDef_AddTypeParam(p_def, Q_ind, "T", "yama:Any");
	ymParcelDef_AddMethodReq(p_def, Q_ind, "m", "$T");

	// A conforms to P and Q[Int].
	auto A_ind = ymParcelDef_AddStruct(p_def, "A");
	ymParcelDef_AddMethod(p_def, A_ind, "m", "yama:Int", ymInertCallBhvrFn, nullptr);

	// B conforms to P and Q[Int].
	auto B_ind = ymParcelDef_AddStruct(p_def, "B");
	ymParcelDef_AddMethod(p_def, B_ind, "m", "yama:Int", ymInertCallBhvrFn, nullptr);

	// C conforms to Q[Float] (ie. but not P or Q[Int].)
	auto C_ind = ymParcelDef_AddStruct(p_def, "C");
	ymParcelDef_AddMethod(p_def, C_ind, "m", "yama:Float", ymInertCallBhvrFn, nullptr);

	// D *might* conform to P, and always conforms to Q[$T].
	auto D_ind = ymParcelDef_AddStruct(p_def, "D");
	ymParcelDef_AddTypeParam(p_def, D_ind, "T", "yama:Any");
	ymParcelDef_AddMethod(p_def, D_ind, "m", "$T", ymInertCallBhvrFn, nullptr);

	ymDm_BindParcelDef(dm.get(), "p", p_def);

	auto success = [this](std::string T_fln, std::string Proto_fln) {
		ymCtx_PopAll(ctx.get());
		err.reset();
		ym::println("-- {} <-> {}", T_fln, Proto_fln);

		auto T = ymCtx_Load(ctx.get(), T_fln.c_str());
		auto Proto = ymCtx_Load(ctx.get(), Proto_fln.c_str());
		ASSERT_TRUE(T);
		ASSERT_TRUE(Proto);

		// Push T value, then sample unboxed0.
		ASSERT_EQ(ymCtx_PutDefault(ctx.get(), YM_NEWTOP, T), YM_TRUE);
		SETUP_OBJ(unboxed0, ymCtx_Local(ctx.get(), 0, YM_TAKE));

		// Convert T value to Proto value (ie. boxing), then sample boxed.
		ASSERT_EQ(ymCtx_Convert(ctx.get(), Proto, YM_NEWTOP), YM_TRUE);
		SETUP_OBJ(boxed, ymCtx_Local(ctx.get(), 0, YM_TAKE));
		EXPECT_EQ(ymObj_Type(boxed), Proto);

		// Convert Proto value to T value (ie. unboxing), then sample unboxed1.
		ASSERT_EQ(ymCtx_Convert(ctx.get(), T, YM_NEWTOP), YM_TRUE);
		SETUP_OBJ(unboxed1, ymCtx_Local(ctx.get(), 0, YM_TAKE));

		// unboxed0 and unboxed1 should be the same instance of T.
		EXPECT_EQ(unboxed0, unboxed1);

		// Ensure unboxing can occur multiple times.
		ASSERT_EQ(ymCtx_Put(ctx.get(), 0, boxed, YM_BORROW), YM_TRUE);
		ASSERT_EQ(ymCtx_Convert(ctx.get(), T, YM_NEWTOP), YM_TRUE);
		SETUP_OBJ(unboxed2, ymCtx_Local(ctx.get(), 0, YM_TAKE));
		EXPECT_EQ(unboxed0, unboxed2);
		EXPECT_EQ(unboxed1, unboxed2);
		};
	auto failure = [this](std::string T_fln, std::string Proto_fln) {
		ymCtx_PopAll(ctx.get());
		err.reset();
		ym::println("-- {} <-> {} (fails)", T_fln, Proto_fln);

		auto T = ymCtx_Load(ctx.get(), T_fln.c_str());
		auto Proto = ymCtx_Load(ctx.get(), Proto_fln.c_str());
		ASSERT_TRUE(T);
		ASSERT_TRUE(Proto);

		// T can't be boxed as Proto.
		ASSERT_EQ(ymCtx_PutDefault(ctx.get(), YM_NEWTOP, T), YM_TRUE);
		ASSERT_EQ(ymCtx_Convert(ctx.get(), Proto, YM_NEWTOP), YM_FALSE);
		EXPECT_GE(err[YmErrCode_IllegalConversion], 1);
		};

	success("p:A", "p:P");
	success("p:B", "p:P");
	failure("p:C", "p:P");
	success("p:D[yama:Int]", "p:P");
	failure("p:D[yama:Float]", "p:P");
	failure("p:D[yama:Bool]", "p:P");

	success("p:A", "p:Q[yama:Int]");
	success("p:B", "p:Q[yama:Int]");
	failure("p:C", "p:Q[yama:Int]");
	success("p:D[yama:Int]", "p:Q[yama:Int]");
	failure("p:D[yama:Float]", "p:Q[yama:Int]");
	failure("p:D[yama:Bool]", "p:Q[yama:Int]");

	failure("p:A", "p:Q[yama:Float]");
	failure("p:B", "p:Q[yama:Float]");
	success("p:C", "p:Q[yama:Float]");
	failure("p:D[yama:Int]", "p:Q[yama:Float]");
	success("p:D[yama:Float]", "p:Q[yama:Float]");
	failure("p:D[yama:Bool]", "p:Q[yama:Float]");
}

TEST_F(ProtocolValues, CannotUnboxWithWrongType_EvenIfUnboxingTypeConformsToProtocol) {
	SETUP_PARCELDEF(p_def);

	auto P_ind = ymParcelDef_AddProtocol(p_def, "P");
	ymParcelDef_AddMethodReq(p_def, P_ind, "m", "yama:Int");

	// A conforms to P.
	auto A_ind = ymParcelDef_AddStruct(p_def, "A");
	ymParcelDef_AddMethod(p_def, A_ind, "m", "yama:Int", ymInertCallBhvrFn, nullptr);

	// B conforms to P.
	auto B_ind = ymParcelDef_AddStruct(p_def, "B");
	ymParcelDef_AddMethod(p_def, B_ind, "m", "yama:Int", ymInertCallBhvrFn, nullptr);

	ymDm_BindParcelDef(dm.get(), "p", p_def);
	auto P = ymCtx_Load(ctx.get(), "p:P");
	auto A = ymCtx_Load(ctx.get(), "p:A");
	auto B = ymCtx_Load(ctx.get(), "p:B");
	ASSERT_TRUE(P);
	ASSERT_TRUE(A);
	ASSERT_TRUE(B);

	ASSERT_EQ(ymCtx_PutDefault(ctx.get(), YM_NEWTOP, A), YM_TRUE);
	ASSERT_EQ(ymCtx_Convert(ctx.get(), P, YM_NEWTOP), YM_TRUE); // Box A -> P.
	ASSERT_EQ(ymCtx_Convert(ctx.get(), B, YM_NEWTOP), YM_FALSE); // Unbox P -> B (should fail.)
	EXPECT_GE(err[YmErrCode_IllegalConversion], 1);
}

TEST_F(ProtocolValues, ConversionBetweenProtocolTypes) {
	SETUP_PARCELDEF(p_def);

	auto P1_ind = ymParcelDef_AddProtocol(p_def, "P1");
	ymParcelDef_AddMethodReq(p_def, P1_ind, "m", "yama:Int");

	auto P2_ind = ymParcelDef_AddProtocol(p_def, "P2");
	ymParcelDef_AddMethodReq(p_def, P2_ind, "n", "yama:Int");

	auto Pg1_ind = ymParcelDef_AddProtocol(p_def, "Pg1");
	ymParcelDef_AddTypeParam(p_def, Pg1_ind, "T", "yama:Any");
	ymParcelDef_AddMethodReq(p_def, Pg1_ind, "m", "$T");

	auto Pg2_ind = ymParcelDef_AddProtocol(p_def, "Pg2");
	ymParcelDef_AddTypeParam(p_def, Pg2_ind, "T", "yama:Any");
	ymParcelDef_AddMethodReq(p_def, Pg2_ind, "n", "$T");

	// A conforms to P1, P2, Pg1[Int], and Pg2[Int].
	auto A_ind = ymParcelDef_AddStruct(p_def, "A");
	ymParcelDef_AddMethod(p_def, A_ind, "m", "yama:Int", ymInertCallBhvrFn, nullptr);
	ymParcelDef_AddMethod(p_def, A_ind, "n", "yama:Int", ymInertCallBhvrFn, nullptr);

	// B conforms to P1 and Pg1[Int], but not P2 nor Pg2[Int].
	auto B_ind = ymParcelDef_AddStruct(p_def, "B");
	ymParcelDef_AddMethod(p_def, B_ind, "m", "yama:Int", ymInertCallBhvrFn, nullptr);

	// Ag conforms to Pg1[$T] and Pg2[$T], and *maybe* P1 and P2.
	auto Ag_ind = ymParcelDef_AddStruct(p_def, "Ag");
	ymParcelDef_AddTypeParam(p_def, Ag_ind, "T", "yama:Any");
	ymParcelDef_AddMethod(p_def, Ag_ind, "m", "$T", ymInertCallBhvrFn, nullptr);
	ymParcelDef_AddMethod(p_def, Ag_ind, "n", "$T", ymInertCallBhvrFn, nullptr);

	// Bg conforms to Pg1[$T] and *maybe* P1, but *never* Pg2 nor P2.
	auto Bg_ind = ymParcelDef_AddStruct(p_def, "Bg");
	ymParcelDef_AddTypeParam(p_def, Bg_ind, "T", "yama:Any");
	ymParcelDef_AddMethod(p_def, Bg_ind, "m", "$T", ymInertCallBhvrFn, nullptr);

	ymDm_BindParcelDef(dm.get(), "p", p_def);

	auto success = [this](std::string T_fln, std::string Proto1_fln, std::string Proto2_fln) {
		ymCtx_PopAll(ctx.get());
		err.reset();
		ym::println("-- {} -> {} -> {} -> {}", T_fln, Proto1_fln, Proto2_fln, T_fln);

		auto T = ymCtx_Load(ctx.get(), T_fln.c_str());
		auto Proto1 = ymCtx_Load(ctx.get(), Proto1_fln.c_str());
		auto Proto2 = ymCtx_Load(ctx.get(), Proto2_fln.c_str());
		ASSERT_TRUE(T);
		ASSERT_TRUE(Proto1);
		ASSERT_TRUE(Proto2);

		ASSERT_EQ(ymCtx_PutDefault(ctx.get(), YM_NEWTOP, T), YM_TRUE);
		SETUP_OBJ(unboxed0, ymCtx_Local(ctx.get(), 0, YM_TAKE));
		ASSERT_EQ(ymCtx_Convert(ctx.get(), Proto1, YM_NEWTOP), YM_TRUE); // Box
		ASSERT_EQ(ymCtx_Convert(ctx.get(), Proto2, YM_NEWTOP), YM_TRUE);
		ASSERT_EQ(ymCtx_Convert(ctx.get(), T, YM_NEWTOP), YM_TRUE); // Unbox
		SETUP_OBJ(unboxed1, ymCtx_Local(ctx.get(), 0, YM_TAKE));

		// unboxed0 and unboxed1 should be the same instance of T.
		EXPECT_EQ(unboxed0, unboxed1);
		};
	auto failure = [this](std::string T_fln, std::string Proto1_fln, std::string Proto2_fln) {
		ymCtx_PopAll(ctx.get());
		err.reset();
		ym::println("-- {} -> {} -> {} -> {} (fails)", T_fln, Proto1_fln, Proto2_fln, T_fln);

		auto T = ymCtx_Load(ctx.get(), T_fln.c_str());
		auto Proto1 = ymCtx_Load(ctx.get(), Proto1_fln.c_str());
		auto Proto2 = ymCtx_Load(ctx.get(), Proto2_fln.c_str());
		ASSERT_TRUE(T);
		ASSERT_TRUE(Proto1);
		ASSERT_TRUE(Proto2);

		// T can't be boxed as Proto.
		ASSERT_EQ(ymCtx_PutDefault(ctx.get(), YM_NEWTOP, T), YM_TRUE);
		ASSERT_EQ(ymCtx_Convert(ctx.get(), Proto1, YM_NEWTOP), YM_TRUE);
		ASSERT_EQ(ymCtx_Convert(ctx.get(), Proto2, YM_NEWTOP), YM_FALSE);
		EXPECT_GE(err[YmErrCode_IllegalConversion], 1);
		};

	success("p:A", "p:P1", "p:P2");
	failure("p:B", "p:P1", "p:P2");
	success("p:Ag[yama:Int]", "p:P1", "p:P2");
	failure("p:Ag[yama:Float]", "p:Pg1[yama:Float]", "p:P2");
	failure("p:Ag[yama:Bool]", "p:Pg1[yama:Bool]", "p:P2");
	failure("p:Bg[yama:Int]", "p:P1", "p:P2");
	failure("p:Bg[yama:Float]", "p:Pg1[yama:Float]", "p:P2");
	failure("p:Bg[yama:Bool]", "p:Pg1[yama:Bool]", "p:P2");

	success("p:A", "p:P1", "p:Pg2[yama:Int]");
	failure("p:B", "p:P1", "p:Pg2[yama:Int]");
	success("p:Ag[yama:Int]", "p:Pg1[yama:Int]", "p:Pg2[yama:Int]");
	failure("p:Ag[yama:Float]", "p:Pg1[yama:Float]", "p:Pg2[yama:Int]");
	failure("p:Ag[yama:Bool]", "p:Pg1[yama:Bool]", "p:Pg2[yama:Int]");
	failure("p:Bg[yama:Int]", "p:Pg1[yama:Int]", "p:Pg2[yama:Int]");
	failure("p:Bg[yama:Float]", "p:Pg1[yama:Float]", "p:Pg2[yama:Int]");
	failure("p:Bg[yama:Bool]", "p:Pg1[yama:Bool]", "p:Pg2[yama:Int]");

	failure("p:A", "p:P1", "p:Pg2[yama:Float]");
	failure("p:B", "p:P1", "p:Pg2[yama:Float]");
	failure("p:Ag[yama:Int]", "p:Pg1[yama:Int]", "p:Pg2[yama:Float]");
	success("p:Ag[yama:Float]", "p:Pg1[yama:Float]", "p:Pg2[yama:Float]");
	failure("p:Ag[yama:Bool]", "p:Pg1[yama:Bool]", "p:Pg2[yama:Float]");
	failure("p:Bg[yama:Int]", "p:Pg1[yama:Int]", "p:Pg2[yama:Float]");
	failure("p:Bg[yama:Float]", "p:Pg1[yama:Float]", "p:Pg2[yama:Float]");
	failure("p:Bg[yama:Bool]", "p:Pg1[yama:Bool]", "p:Pg2[yama:Float]");
}

TEST_F(ProtocolValues, TypeMethodsOfProtocolsCannotBeCalled) {
	// NOTE: Type methods (ie. methods w/out a 'self' parameter) of protocols cannot be called, as
	//		 they lack a way to discern dynamically a boxed type.

	SETUP_PARCELDEF(p_def);

	auto P_ind = ymParcelDef_AddProtocol(p_def, "P");
	ymParcelDef_AddMethodReq(p_def, P_ind, "m", "yama:Int");

	ymDm_BindParcelDef(dm.get(), "p", p_def);
	auto P_m = ymCtx_Load(ctx.get(), "p:P::m");
	ASSERT_TRUE(P_m);

	EXPECT_EQ(ymCtx_Call(ctx.get(), P_m, 0, YM_DISCARD), YM_FALSE);
	EXPECT_GE(err[YmErrCode_NonCallableType], 1);
}

namespace {
	inline YmObj* selfExpected = nullptr;
}

TEST_F(ProtocolValues, ObjectMethodsOfProtocolsCanBeCalled_AndPerformSpecialDynamicDispatchBehaviour) {
	// NOTE: When protocol methods are called, the protocol method itself never appears on the call
	//		 stack, instead forwarding directly to the corresponding method of the boxed type.

	SETUP_PARCELDEF(p_def);

	auto P_ind = ymParcelDef_AddProtocol(p_def, "P");
	auto P_m_ind = ymParcelDef_AddMethodReq(p_def, P_ind, "m", "yama:Int");
	ymParcelDef_AddParam(p_def, P_m_ind, "self", "$Self");
	ymParcelDef_AddParam(p_def, P_m_ind, "x", "yama:Int");

	// A conforms to P.
	auto A_ind = ymParcelDef_AddStruct(p_def, "A");
	auto A_m_ind = ymParcelDef_AddMethod(p_def, A_ind, "m", "yama:Int",
		[](YmCtx* ctx, void*) {
			ASSERT_TRUE(selfExpected);

			// P::m shouldn't appear on the call stack.
			EXPECT_EQ(ymCtx_CallStackHeight(ctx), 2);

			EXPECT_EQ(ymCtx_Args(ctx), 2);

			// Arg #1 (ie. self) is as expected.
			if (auto arg = ymCtx_Arg(ctx, 0, YM_BORROW)) {
				EXPECT_EQ(arg, selfExpected);
				EXPECT_EQ(ymObj_Type(arg), ymObj_Type(selfExpected));
			}
			else ADD_FAILURE();

			// Arg #2 is as expected.
			if (auto arg = ymCtx_Arg(ctx, 1, YM_BORROW)) {
				EXPECT_EQ(ymObj_Type(arg), ymCtx_Ref(ctx, 0));
				// A::m will double the input value.
				ymCtx_Ret(ctx, ymCtx_NewInt(ctx, ymObj_ToInt(arg, nullptr) * 2), YM_TAKE);
			}
			else {
				ADD_FAILURE();
				ymCtx_Ret(ctx, ymCtx_NewInt(ctx, 0), YM_TAKE);
			}
		},
		nullptr);
	ymParcelDef_AddParam(p_def, A_m_ind, "self", "$Self");
	ymParcelDef_AddParam(p_def, A_m_ind, "x", "yama:Int");
	ymParcelDef_AddRef(p_def, A_m_ind, "yama:Int");

	// B conforms to P.
	auto B_ind = ymParcelDef_AddStruct(p_def, "B");
	auto B_m_ind = ymParcelDef_AddMethod(p_def, B_ind, "m", "yama:Int",
		[](YmCtx* ctx, void*) {
			ASSERT_TRUE(selfExpected);

			// P::m shouldn't appear on the call stack.
			EXPECT_EQ(ymCtx_CallStackHeight(ctx), 2);

			EXPECT_EQ(ymCtx_Args(ctx), 2);

			// Arg #1 (ie. self) is as expected.
			if (auto arg = ymCtx_Arg(ctx, 0, YM_BORROW)) {
				EXPECT_EQ(arg, selfExpected);
				EXPECT_EQ(ymObj_Type(arg), ymObj_Type(selfExpected));
			}
			else ADD_FAILURE();

			// Arg #2 is as expected.
			if (auto arg = ymCtx_Arg(ctx, 1, YM_BORROW)) {
				EXPECT_EQ(ymObj_Type(arg), ymCtx_Ref(ctx, 0));
				// B::m will negate the input value.
				ymCtx_Ret(ctx, ymCtx_NewInt(ctx, -ymObj_ToInt(arg, nullptr)), YM_TAKE);
			}
			else {
				ADD_FAILURE();
				ymCtx_Ret(ctx, ymCtx_NewInt(ctx, 0), YM_TAKE);
			}
		},
		nullptr);
	ymParcelDef_AddParam(p_def, B_m_ind, "self", "$Self");
	ymParcelDef_AddParam(p_def, B_m_ind, "x", "yama:Int");
	ymParcelDef_AddRef(p_def, B_m_ind, "yama:Int");

	ymDm_BindParcelDef(dm.get(), "p", p_def);
	auto P_m = ymCtx_Load(ctx.get(), "p:P::m");
	ASSERT_TRUE(P_m);

	auto call_A_m = [this](YmInt x) {
		ymCtx_PopAll(ctx.get());
		err.reset();

		auto P = ymCtx_Load(ctx.get(), "p:P");
		auto P_m = ymCtx_Load(ctx.get(), "p:P::m");
		auto A = ymCtx_Load(ctx.get(), "p:A");
		ASSERT_TRUE(P);
		ASSERT_TRUE(P_m);
		ASSERT_TRUE(A);

		ASSERT_EQ(ymCtx_PutDefault(ctx.get(), YM_NEWTOP, A), YM_TRUE);
		selfExpected = ymCtx_Local(ctx.get(), 0, YM_BORROW); // Should see 'self' as unboxed value.
		ASSERT_EQ(ymCtx_Convert(ctx.get(), P, YM_NEWTOP), YM_TRUE);
		ASSERT_EQ(ymCtx_PutInt(ctx.get(), YM_NEWTOP, x), YM_TRUE);
		ASSERT_EQ(ymCtx_Call(ctx.get(), P_m, 2, YM_NEWTOP), YM_TRUE);

		ASSERT_EQ(ymCtx_Locals(ctx.get()), 1);
		auto result = Safe(ymCtx_Pull(ctx.get()));

		EXPECT_EQ(ymObj_Type(result), ymCtx_LdInt(ctx.get()));
		EXPECT_EQ(ymObj_ToInt(result, nullptr), x * 2); // A::m should double x.

		ym::println("p:P::m([some p:A], {}) => {}", x, ymObj_ToInt(result, nullptr));
		};
	auto call_B_m = [this](YmInt x) {
		ymCtx_PopAll(ctx.get());
		err.reset();

		auto P = ymCtx_Load(ctx.get(), "p:P");
		auto P_m = ymCtx_Load(ctx.get(), "p:P::m");
		auto B = ymCtx_Load(ctx.get(), "p:B");
		ASSERT_TRUE(P);
		ASSERT_TRUE(P_m);
		ASSERT_TRUE(B);

		ASSERT_EQ(ymCtx_PutDefault(ctx.get(), YM_NEWTOP, B), YM_TRUE);
		selfExpected = ymCtx_Local(ctx.get(), 0, YM_BORROW); // Should see 'self' as unboxed value.
		ASSERT_EQ(ymCtx_Convert(ctx.get(), P, YM_NEWTOP), YM_TRUE);
		ASSERT_EQ(ymCtx_PutInt(ctx.get(), YM_NEWTOP, x), YM_TRUE);
		ASSERT_EQ(ymCtx_Call(ctx.get(), P_m, 2, YM_NEWTOP), YM_TRUE);

		ASSERT_EQ(ymCtx_Locals(ctx.get()), 1);
		auto result = Safe(ymCtx_Pull(ctx.get()));

		EXPECT_EQ(ymObj_Type(result), ymCtx_LdInt(ctx.get()));
		EXPECT_EQ(ymObj_ToInt(result, nullptr), -x); // B::m should negate x.

		ym::println("p:P::m([some p:B], {}) => {}", x, ymObj_ToInt(result, nullptr));
		};

	call_A_m(0);
	call_A_m(1);
	call_A_m(2);
	call_A_m(10);
	call_A_m(50);
	call_A_m(100);
	call_A_m(100'000'000);
	call_A_m(-1);
	call_A_m(-2);
	call_A_m(-10);
	call_A_m(-50);
	call_A_m(-100);
	call_A_m(-100'000'000);

	call_B_m(0);
	call_B_m(1);
	call_B_m(2);
	call_B_m(10);
	call_B_m(50);
	call_B_m(100);
	call_B_m(100'000'000);
	call_B_m(-1);
	call_B_m(-2);
	call_B_m(-10);
	call_B_m(-50);
	call_B_m(-100);
	call_B_m(-100'000'000);
}

// TODO: Below doesn't cover ALL POSSIBLE WAYS ymCtx_Call CAN FAIL, so in the future maybe revise
//		 ymCtx_Call's unit tests to cover these nuances.

TEST_F(ProtocolValues, ObjectMethodsOfProtocolsFailCorrectly_LeavingLocalObjStkAsExpected) {
	SETUP_PARCELDEF(p_def);

	auto P_ind = ymParcelDef_AddProtocol(p_def, "P");
	auto P_m_ind = ymParcelDef_AddMethodReq(p_def, P_ind, "m", "yama:Int");
	ymParcelDef_AddParam(p_def, P_m_ind, "self", "$Self");
	ymParcelDef_AddParam(p_def, P_m_ind, "x", "yama:Int");

	// A conforms to P.
	auto A_ind = ymParcelDef_AddStruct(p_def, "A");
	auto A_m_ind = ymParcelDef_AddMethod(p_def, A_ind, "m", "yama:Int",
		[](YmCtx* ctx, void*) {
			// Fail due to no return value bound.
		},
		nullptr);
	ymParcelDef_AddParam(p_def, A_m_ind, "self", "$Self");
	ymParcelDef_AddParam(p_def, A_m_ind, "x", "yama:Int");
	ymParcelDef_AddRef(p_def, A_m_ind, "yama:Int");

	ymDm_BindParcelDef(dm.get(), "p", p_def);
	auto P = ymCtx_Load(ctx.get(), "p:P");
	auto P_m = ymCtx_Load(ctx.get(), "p:P::m");
	auto A = ymCtx_Load(ctx.get(), "p:A");
	ASSERT_TRUE(P);
	ASSERT_TRUE(P_m);
	ASSERT_TRUE(A);

	ASSERT_EQ(ymCtx_PutDefault(ctx.get(), YM_NEWTOP, A), YM_TRUE);
	ASSERT_EQ(ymCtx_Convert(ctx.get(), P, YM_NEWTOP), YM_TRUE);
	auto obj0 = ymCtx_Local(ctx.get(), 0, YM_BORROW);
	ASSERT_EQ(ymCtx_PutInt(ctx.get(), YM_NEWTOP, -14), YM_TRUE);
	auto obj1 = ymCtx_Local(ctx.get(), 1, YM_BORROW);
	ASSERT_EQ(ymCtx_Call(ctx.get(), P_m, 2, YM_NEWTOP), YM_FALSE);
	EXPECT_GE(err[YmErrCode_CallProcedureError], 1);

	EXPECT_EQ(ymCtx_Locals(ctx.get()), 2);
	EXPECT_EQ(ymCtx_Local(ctx.get(), 0, YM_BORROW), obj0);
	EXPECT_EQ(ymCtx_Local(ctx.get(), 1, YM_BORROW), obj1);
}

