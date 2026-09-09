

#include <gtest/gtest.h>

#include <yama/../internal/bcode.h>
#include <yama/../internal/BCodeExec.h>
#include <yama/../internal/ConstTableInfo.h>
#include <yama/../internal/YmCtx.h>
#include <yama/../internal/YmParcelDef.h>
#include <yama++/Context.h>
#include <yama++/ParcelDef.h>

#include "../utils/SideFxHistory.h"
#include "../utils/utils.h"


using namespace ym;


constexpr auto bPush = _ym::bPush;
constexpr auto bDiscard = _ym::bDiscard;


class BCodeExec : public ::testing::Test {
public:
	ErrCounter err;
	Domain dm;
	Context ctx;
	ParcelDef pdef;


	BCodeExec() :
		ctx(dm) {
		err.setupCallbackForThisThread();
	}

	static size_t plus_calls, storedVar_inits, computedVar_gets, computedVar_sets, S_computed_gets, S_computed_sets;
	static YmInt computedVar_last_val, S_computed_last_val;
	void resetCounters() noexcept {
		plus_calls = 0;
		storedVar_inits = 0;
		computedVar_gets = 0;
		computedVar_sets = 0;
		S_computed_gets = 0;
		S_computed_sets = 0;
		computedVar_last_val = 0;
		S_computed_last_val = 0;
	}

	void setup(
		_ym::ConstTableInfo consts,
		_ym::BCodeWriter& bcode,
		std::string returnType,
		std::vector<std::string> paramTypes) {
		pdef.addFn("plus", "yama:Int",
			{ { "a", "yama:Int" }, { "b", "yama:Int" } },
			{},
			[](YmCtx* ctx, YmType* t, void*) {
				plus_calls++;
				auto a = ymObj_ToInt(ymCtx_Arg(ctx, 0, YM_BORROW), nullptr);
				auto b = ymObj_ToInt(ymCtx_Arg(ctx, 1, YM_BORROW), nullptr);
				ymCtx_RetObj(ctx, ymCtx_NewInt(ctx, a + b), YM_TAKE);
			});
		pdef.addFn("lessThan", "yama:Bool",
			{ { "a", "yama:Int" }, { "b", "yama:Int" } },
			{},
			[](YmCtx* ctx, YmType* t, void*) {
				auto a = ymObj_ToInt(ymCtx_Arg(ctx, 0, YM_BORROW), nullptr);
				auto b = ymObj_ToInt(ymCtx_Arg(ctx, 1, YM_BORROW), nullptr);
				ymCtx_RetObj(ctx, ymCtx_NewBool(ctx, a < b), YM_TAKE);
			});
		pdef.addFn("not", "yama:Bool",
			{ { "a", "yama:Bool" } },
			{},
			[](YmCtx* ctx, YmType* t, void*) {
				auto a = ymObj_ToBool(ymCtx_Arg(ctx, 0, YM_BORROW), nullptr);
				ymCtx_RetObj(ctx, ymCtx_NewBool(ctx, !bool(a)), YM_TAKE);
			});
		pdef.addStoredVar("storedVar", "yama:Int",
			[](YmCtx* ctx, YmType* t, void*) {
				storedVar_inits++;
				ymCtx_RetObj(ctx, ymCtx_NewInt(ctx, 5), YM_TAKE);
			});
		pdef.addComputedVar("computedVar", "yama:Int",
			[](YmCtx* ctx, YmType* t, void*) {
				computedVar_gets++;
				ymCtx_RetObj(ctx, ymCtx_NewInt(ctx, 15), YM_TAKE);
			},
			[](YmCtx* ctx, YmType* t, void*) {
				computedVar_sets++;
				computedVar_last_val = Context(Safe(ctx)).arg(0).value().toInt().value();
				ymCtx_RetObj(ctx, ymCtx_NewNone(ctx), YM_TAKE);
			});
		pdef.addStruct("S");
		pdef.addStoredProperty("S", "stored", "yama:Int");
		pdef.addComputedProperty("S", "computed", "yama:Int",
			[](YmCtx* ctx, YmType* t, void*) {
				S_computed_gets++;
				ymCtx_RetObj(ctx, ymCtx_NewInt(ctx, 25), YM_TAKE);
			},
			[](YmCtx* ctx, YmType* t, void*) {
				S_computed_sets++;
				S_computed_last_val = Context(Safe(ctx)).arg(1).value().toInt().value();
				ymCtx_RetObj(ctx, ymCtx_NewNone(ctx), YM_TAKE);
			});
		pdef.get()->addFn("f", returnType, _ym::CallBhvrCallbackInfo(_ym::bcodeExecCallBhvr), std::move(consts));
		size_t i = 0;
		for (const auto& paramType : paramTypes) {
			(void)pdef.get()->addParam("f", std::format("arg{}", i), paramType).value();
			i++;
		}
		pdef.get()->bindBCode("f", bcode.done().value());
		dm.bind("p", pdef);
	}

	bool expectLocal(YmLocal where, const Val& expects) {
		if (auto local = ymCtx_Local(ctx.get(), where, YM_BORROW)) {
			auto v = toVal(local);
			if (!compare(v, expects)) {
				ADD_FAILURE() << "BCodeExec::expectLocal (" << where << ", ~): " << fmt(v) << " != " << fmt(expects);
				return false;
			}
			return true;
		}
		ADD_FAILURE() << "BCodeExec::expectLocal (" << where << ", ~): **OUT-OF-BOUNDS** != " << fmt(expects);
		return false;
	}
	bool expectSideFx(const SideFxHistory& expects) {
		return sidefx.expectEq(expects);
	}


protected:
	void SetUp() {
		resetCounters();
		setupSideFxParcel(dm.get());
		clearSideFx();
	}
	void TearDown() {
	}
};

size_t BCodeExec::plus_calls = 0;
size_t BCodeExec::storedVar_inits = 0;
size_t BCodeExec::computedVar_gets = 0;
size_t BCodeExec::computedVar_sets = 0;
size_t BCodeExec::S_computed_gets = 0;
size_t BCodeExec::S_computed_sets = 0;
YmInt BCodeExec::computedVar_last_val = 0;
YmInt BCodeExec::S_computed_last_val = 0;


static_assert(_ym::Opcodes == 17);

TEST_F(BCodeExec, Noop) {
	_ym::ConstTableInfo consts{};
	_ym::BCodeWriter bcode{};
	auto i0 = (YmUInt8)consts.pullVal<YmInt>(10).value();
	bcode
		.addPutConst(i0, bPush)
		.addNoop()
		.addNoop()
		.addNoop()
		.addRet();
	setup(consts, bcode, "yama:Int", {});

	ASSERT_TRUE(ctx.call(ctx.load("p:f"), 0));

	expectLocal(0, YmInt(10));
}

TEST_F(BCodeExec, Pop) {
	_ym::ConstTableInfo consts{};
	_ym::BCodeWriter bcode{};
	auto i0 = (YmUInt8)consts.pullVal<YmInt>(10).value();
	bcode
		// Bury 10 in two nones which must be popped to
		// return correct type.
		.addPutConst(i0, bPush)
		.addPutNone(bPush)
		.addPutNone(bPush)
		.addPop(2)
		.addRet();
	setup(consts, bcode, "yama:Int", {});

	ASSERT_TRUE(ctx.call(ctx.load("p:f"), 0));

	expectLocal(0, YmInt(10));
}

TEST_F(BCodeExec, Pop_Zero) {
	_ym::ConstTableInfo consts{};
	_ym::BCodeWriter bcode{};
	auto i0 = (YmUInt8)consts.pullVal<YmInt>(10).value();
	bcode
		// 10 shouldn't get popped from stack.
		.addPutConst(i0, bPush)
		.addPop(0)
		.addRet();
	setup(consts, bcode, "yama:Int", {});

	ASSERT_TRUE(ctx.call(ctx.load("p:f"), 0));

	expectLocal(0, YmInt(10));
}

TEST_F(BCodeExec, Pop_MoreThanAreOnStack) {
	_ym::ConstTableInfo consts{};
	_ym::BCodeWriter bcode{};
	auto i0 = (YmUInt8)consts.pullVal<YmInt>(10).value();
	bcode
		.addPutNone(bPush)
		.addPutNone(bPush)
		.addPutNone(bPush)
		.addPutNone(bPush)
		.addPop(15)
		// Push none, then overwrite local 0 w/ 10, then return
		// top local.
		// If pop popped ALL objs, then our pushed none should be
		// the ONLY thing on stack, and so overwriting it w/ 10
		// and returning should return 10, as there should only
		// be one local, and so our 10 should be at top of stack.
		.addPutNone(bPush)
		.addPutConst(i0, 0)
		.addRet();
	setup(consts, bcode, "yama:Int", {});

	ASSERT_TRUE(ctx.call(ctx.load("p:f"), 0));

	expectLocal(0, YmInt(10));
}

TEST_F(BCodeExec, PutNone) {
	_ym::ConstTableInfo consts{};
	_ym::BCodeWriter bcode{};
	auto i0 = (YmUInt8)consts.pullVal<YmInt>(10).value();
	bcode
		.addPutConst(i0, bPush)
		.addPutNone(0)
		.addRet();
	setup(consts, bcode, "yama:None", {});

	ASSERT_TRUE(ctx.call(ctx.load("p:f"), 0));

	expectLocal(0, NoneVal{});
}

TEST_F(BCodeExec, PutNone_Push) {
	_ym::ConstTableInfo consts{};
	_ym::BCodeWriter bcode{};
	auto i0 = (YmUInt8)consts.pullVal<YmInt>(10).value();
	bcode
		.addPutConst(i0, bPush)
		.addPutNone(bPush)
		.addRet();
	setup(consts, bcode, "yama:None", {});

	ASSERT_TRUE(ctx.call(ctx.load("p:f"), 0));

	expectLocal(0, NoneVal{});
}

TEST_F(BCodeExec, PutNone_Discard) {
	_ym::ConstTableInfo consts{};
	_ym::BCodeWriter bcode{};
	auto i0 = (YmUInt8)consts.pullVal<YmInt>(10).value();
	bcode
		.addPutConst(i0, bPush)
		.addPutNone(bDiscard)
		.addRet();
	setup(consts, bcode, "yama:Int", {});

	ASSERT_TRUE(ctx.call(ctx.load("p:f"), 0));

	expectLocal(0, YmInt(10));
}

TEST_F(BCodeExec, PutConst) {
	_ym::ConstTableInfo consts{};
	_ym::BCodeWriter bcode{};
	auto i0 = (YmUInt8)consts.pullVal<YmInt>(10).value();
	bcode
		.addPutNone(bPush)
		.addPutConst(i0, 0)
		.addRet();
	setup(consts, bcode, "yama:Int", {});

	ASSERT_TRUE(ctx.call(ctx.load("p:f"), 0));

	expectLocal(0, YmInt(10));
}

TEST_F(BCodeExec, PutConst_Push) {
	_ym::ConstTableInfo consts{};
	_ym::BCodeWriter bcode{};
	auto i0 = (YmUInt8)consts.pullVal<YmInt>(10).value();
	bcode
		.addPutConst(i0, bPush)
		.addRet();
	setup(consts, bcode, "yama:Int", {});

	ASSERT_TRUE(ctx.call(ctx.load("p:f"), 0));

	expectLocal(0, YmInt(10));
}

TEST_F(BCodeExec, PutConst_Discard) {
	_ym::ConstTableInfo consts{};
	_ym::BCodeWriter bcode{};
	auto i0 = (YmUInt8)consts.pullVal<YmInt>(10).value();
	bcode
		.addPutNone(bPush)
		.addPutConst(i0, bDiscard)
		.addRet();
	setup(consts, bcode, "yama:None", {});

	ASSERT_TRUE(ctx.call(ctx.load("p:f"), 0));

	expectLocal(0, NoneVal{});
}

TEST_F(BCodeExec, PutArg) {
	_ym::ConstTableInfo consts{};
	_ym::BCodeWriter bcode{};
	bcode
		.addPutNone(bPush)
		.addPutArg(0, 0)
		.addRet();
	setup(consts, bcode, "yama:Int", { "yama:Int" });

	ASSERT_TRUE(ctx.pushInt(101));
	ASSERT_TRUE(ctx.call(ctx.load("p:f"), 1));

	expectLocal(0, YmInt(101));
}

TEST_F(BCodeExec, PutArg_Push) {
	_ym::ConstTableInfo consts{};
	_ym::BCodeWriter bcode{};
	bcode
		.addPutArg(0, bPush)
		.addRet();
	setup(consts, bcode, "yama:Int", { "yama:Int" });

	ASSERT_TRUE(ctx.pushInt(101));
	ASSERT_TRUE(ctx.call(ctx.load("p:f"), 1));

	expectLocal(0, YmInt(101));
}

TEST_F(BCodeExec, PutArg_Discard) {
	_ym::ConstTableInfo consts{};
	_ym::BCodeWriter bcode{};
	bcode
		.addPutNone(bPush)
		.addPutArg(0, bDiscard)
		.addRet();
	setup(consts, bcode, "yama:None", { "yama:Int" });

	ASSERT_TRUE(ctx.pushInt(101));
	ASSERT_TRUE(ctx.call(ctx.load("p:f"), 1));

	expectLocal(0, NoneVal{});
}

TEST_F(BCodeExec, Copy) {
	_ym::ConstTableInfo consts{};
	_ym::BCodeWriter bcode{};
	auto i0 = (YmUInt8)consts.pullVal<YmInt>(10).value();
	bcode
		.addPutNone(bPush)
		.addPutConst(i0, bPush)
		.addPutNone(bPush)
		.addPutNone(bPush)
		.addCopy(1, 3)
		.addRet();
	setup(consts, bcode, "yama:Int", {});

	ASSERT_TRUE(ctx.call(ctx.load("p:f"), 0));

	expectLocal(0, YmInt(10));
}

TEST_F(BCodeExec, Copy_Push) {
	_ym::ConstTableInfo consts{};
	_ym::BCodeWriter bcode{};
	auto i0 = (YmUInt8)consts.pullVal<YmInt>(10).value();
	bcode
		.addPutNone(bPush)
		.addPutConst(i0, bPush)
		.addPutNone(bPush)
		.addPutNone(bPush)
		.addCopy(1, bPush)
		.addRet();
	setup(consts, bcode, "yama:Int", {});

	ASSERT_TRUE(ctx.call(ctx.load("p:f"), 0));

	expectLocal(0, YmInt(10));
}

TEST_F(BCodeExec, Copy_Discard) {
	_ym::ConstTableInfo consts{};
	_ym::BCodeWriter bcode{};
	auto i0 = (YmUInt8)consts.pullVal<YmInt>(10).value();
	bcode
		.addPutNone(bPush)
		.addPutConst(i0, bPush)
		.addPutNone(bPush)
		.addPutNone(bPush)
		.addCopy(1, bDiscard)
		.addRet();
	setup(consts, bcode, "yama:None", {});

	ASSERT_TRUE(ctx.call(ctx.load("p:f"), 0));

	expectLocal(0, NoneVal{});
}

TEST_F(BCodeExec, DefaultInit) {
	_ym::ConstTableInfo consts{};
	_ym::BCodeWriter bcode{};
	auto t0 = (YmUInt8)consts.pullRef("yama:Int").value();
	bcode
		.addPutNone(bPush)
		.addDefaultInit(t0, 0)
		.addRet();
	setup(consts, bcode, "yama:Int", {});

	ASSERT_TRUE(ctx.call(ctx.load("p:f"), 0));

	expectLocal(0, YmInt(0));
}

TEST_F(BCodeExec, DefaultInit_Push) {
	_ym::ConstTableInfo consts{};
	_ym::BCodeWriter bcode{};
	auto t0 = (YmUInt8)consts.pullRef("yama:Int").value();
	bcode
		.addDefaultInit(t0, bPush)
		.addRet();
	setup(consts, bcode, "yama:Int", {});

	ASSERT_TRUE(ctx.call(ctx.load("p:f"), 0));

	expectLocal(0, YmInt(0));
}

TEST_F(BCodeExec, DefaultInit_Discard) {
	_ym::ConstTableInfo consts{};
	_ym::BCodeWriter bcode{};
	auto t0 = (YmUInt8)consts.pullRef("yama:Int").value();
	bcode
		.addPutNone(bPush)
		.addDefaultInit(t0, bDiscard)
		.addRet();
	setup(consts, bcode, "yama:None", {});

	ASSERT_TRUE(ctx.call(ctx.load("p:f"), 0));

	expectLocal(0, NoneVal{});
}

TEST_F(BCodeExec, StructInit) {
	static_assert(_ym::Opcodes == 17); // TODO
}

TEST_F(BCodeExec, PCall) {
	_ym::ConstTableInfo consts{};
	_ym::BCodeWriter bcode{};
	auto t0 = (YmUInt8)consts.pullRef("p:plus").value();
	auto i0 = (YmUInt8)consts.pullVal<YmInt>(10).value();
	auto i1 = (YmUInt8)consts.pullVal<YmInt>(3).value();
	bcode
		.addPutNone(bPush)
		.addPutConst(i0, bPush)
		.addPutConst(i1, bPush)
		.addPCall(t0, 0)
		.addRet();
	setup(consts, bcode, "yama:Int", {});

	ASSERT_TRUE(ctx.call(ctx.load("p:f"), 0));

	expectLocal(0, YmInt(13));
	ASSERT_EQ(plus_calls, 1);
}

TEST_F(BCodeExec, PCall_Push) {
	_ym::ConstTableInfo consts{};
	_ym::BCodeWriter bcode{};
	auto t0 = (YmUInt8)consts.pullRef("p:plus").value();
	auto i0 = (YmUInt8)consts.pullVal<YmInt>(10).value();
	auto i1 = (YmUInt8)consts.pullVal<YmInt>(3).value();
	bcode
		.addPutConst(i0, bPush)
		.addPutConst(i1, bPush)
		.addPCall(t0, bPush)
		.addRet();
	setup(consts, bcode, "yama:Int", {});

	ASSERT_TRUE(ctx.call(ctx.load("p:f"), 0));

	expectLocal(0, YmInt(13));
	ASSERT_EQ(plus_calls, 1);
}

TEST_F(BCodeExec, PCall_Discard) {
	_ym::ConstTableInfo consts{};
	_ym::BCodeWriter bcode{};
	auto t0 = (YmUInt8)consts.pullRef("p:plus").value();
	auto i0 = (YmUInt8)consts.pullVal<YmInt>(10).value();
	auto i1 = (YmUInt8)consts.pullVal<YmInt>(3).value();
	bcode
		.addPutNone(bPush)
		.addPutConst(i0, bPush)
		.addPutConst(i1, bPush)
		.addPCall(t0, bDiscard)
		.addRet();
	setup(consts, bcode, "yama:None", {});

	ASSERT_TRUE(ctx.call(ctx.load("p:f"), 0));

	expectLocal(0, NoneVal{});
	ASSERT_EQ(plus_calls, 1);
}

TEST_F(BCodeExec, PNCall) {
	static_assert(_ym::Opcodes == 17); // TODO
}

TEST_F(BCodeExec, Ret) {
	_ym::ConstTableInfo consts{};
	_ym::BCodeWriter bcode{};
	auto i0 = (YmUInt8)consts.pullVal<YmInt>(10).value();
	bcode
		.addPutConst(i0, bPush)
		.addRet();
	setup(consts, bcode, "yama:Int", {});

	ASSERT_TRUE(ctx.call(ctx.load("p:f"), 0));

	expectLocal(0, YmInt(10));
}

TEST_F(BCodeExec, GetVar_Stored) {
	_ym::ConstTableInfo consts{};
	_ym::BCodeWriter bcode{};
	auto t0 = (YmUInt8)consts.pullRef("p:storedVar").value();
	bcode
		.addPutNone(bPush)
		.addGetVar(t0, 0)
		.addRet();
	setup(consts, bcode, "yama:Int", {});

	ASSERT_TRUE(ctx.call(ctx.load("p:f"), 0));

	expectLocal(0, YmInt(5));
	ASSERT_EQ(storedVar_inits, 1);
}

TEST_F(BCodeExec, GetVar_Computed) {
	_ym::ConstTableInfo consts{};
	_ym::BCodeWriter bcode{};
	auto t0 = (YmUInt8)consts.pullRef("p:computedVar").value();
	bcode
		.addPutNone(bPush)
		.addGetVar(t0, 0)
		.addRet();
	setup(consts, bcode, "yama:Int", {});

	ASSERT_TRUE(ctx.call(ctx.load("p:f"), 0));

	expectLocal(0, YmInt(15));
	ASSERT_EQ(computedVar_gets, 1);
}

TEST_F(BCodeExec, GetVar_Push) {
	_ym::ConstTableInfo consts{};
	_ym::BCodeWriter bcode{};
	auto t0 = (YmUInt8)consts.pullRef("p:storedVar").value();
	bcode
		.addGetVar(t0, bPush)
		.addRet();
	setup(consts, bcode, "yama:Int", {});

	ASSERT_TRUE(ctx.call(ctx.load("p:f"), 0));

	expectLocal(0, YmInt(5));
	ASSERT_EQ(storedVar_inits, 1);
}

TEST_F(BCodeExec, GetVar_Discard) {
	_ym::ConstTableInfo consts{};
	_ym::BCodeWriter bcode{};
	auto t0 = (YmUInt8)consts.pullRef("p:storedVar").value();
	bcode
		.addPutNone(bPush)
		.addGetVar(t0, bDiscard)
		.addRet();
	setup(consts, bcode, "yama:None", {});

	ASSERT_TRUE(ctx.call(ctx.load("p:f"), 0));

	expectLocal(0, NoneVal{});
	ASSERT_EQ(storedVar_inits, 1);
}

TEST_F(BCodeExec, SetVar_Stored) {
	_ym::ConstTableInfo consts{};
	_ym::BCodeWriter bcode{};
	auto t0 = (YmUInt8)consts.pullRef("p:storedVar").value();
	auto i0 = (YmUInt8)consts.pullVal<YmInt>(101).value();
	bcode
		.addPutNone(bPush)
		.addPutConst(i0, bPush)
		.addSetVar(t0)
		.addRet();
	setup(consts, bcode, "yama:None", {});

	ASSERT_TRUE(ctx.call(ctx.load("p:f"), 0));

	expectLocal(0, NoneVal{});
	ASSERT_EQ(storedVar_inits, 1);

	ASSERT_TRUE(ctx.getVar(ctx.load("p:storedVar")));

	expectLocal(1, YmInt(101));
}

TEST_F(BCodeExec, SetVar_Computed) {
	_ym::ConstTableInfo consts{};
	_ym::BCodeWriter bcode{};
	auto t0 = (YmUInt8)consts.pullRef("p:computedVar").value();
	auto i0 = (YmUInt8)consts.pullVal<YmInt>(101).value();
	bcode
		.addPutNone(bPush)
		.addPutConst(i0, bPush)
		.addSetVar(t0)
		.addRet();
	setup(consts, bcode, "yama:None", {});

	ASSERT_TRUE(ctx.call(ctx.load("p:f"), 0));

	expectLocal(0, NoneVal{});
	ASSERT_EQ(computedVar_sets, 1);
	ASSERT_EQ(computedVar_last_val, 101);
}

TEST_F(BCodeExec, GetProp_Stored) {
	_ym::ConstTableInfo consts{};
	_ym::BCodeWriter bcode{};
	auto S = (YmUInt8)consts.pullRef("p:S").value();
	auto S_stored = (YmUInt8)consts.pullRef("p:S::stored").value();
	bcode
		.addPutNone(bPush)
		.addPutArg(0, bPush)
		.addGetProp(S_stored, 0)
		.addRet();
	setup(consts, bcode, "yama:Int", { "p:S" });

	ASSERT_TRUE(ctx.pushInt(-101));
	ASSERT_TRUE(ctx.structInit(ctx.load("p:S"), "stored"));

	ASSERT_TRUE(ctx.copy(0));
	ASSERT_TRUE(ctx.call(ctx.load("p:f"), 1));

	expectLocal(1, YmInt(-101));
}

TEST_F(BCodeExec, GetProp_Computed) {
	_ym::ConstTableInfo consts{};
	_ym::BCodeWriter bcode{};
	auto S = (YmUInt8)consts.pullRef("p:S").value();
	auto S_computed = (YmUInt8)consts.pullRef("p:S::computed").value();
	bcode
		.addPutNone(bPush)
		.addPutArg(0, bPush)
		.addGetProp(S_computed, 0)
		.addRet();
	setup(consts, bcode, "yama:Int", { "p:S" });

	ASSERT_TRUE(ctx.pushInt(-101));
	ASSERT_TRUE(ctx.structInit(ctx.load("p:S"), "stored"));

	ASSERT_TRUE(ctx.copy(0));
	ASSERT_TRUE(ctx.call(ctx.load("p:f"), 1));

	expectLocal(1, YmInt(25));
	EXPECT_EQ(S_computed_gets, 1);
}

TEST_F(BCodeExec, GetProp_Push) {
	_ym::ConstTableInfo consts{};
	_ym::BCodeWriter bcode{};
	auto S = (YmUInt8)consts.pullRef("p:S").value();
	auto S_stored = (YmUInt8)consts.pullRef("p:S::stored").value();
	bcode
		.addPutArg(0, bPush)
		.addGetProp(S_stored, bPush)
		.addRet();
	setup(consts, bcode, "yama:Int", { "p:S" });

	ASSERT_TRUE(ctx.pushInt(-101));
	ASSERT_TRUE(ctx.structInit(ctx.load("p:S"), "stored"));

	ASSERT_TRUE(ctx.copy(0));
	ASSERT_TRUE(ctx.call(ctx.load("p:f"), 1));

	expectLocal(1, YmInt(-101));
}

TEST_F(BCodeExec, GetProp_Discard) {
	_ym::ConstTableInfo consts{};
	_ym::BCodeWriter bcode{};
	auto S = (YmUInt8)consts.pullRef("p:S").value();
	auto S_stored = (YmUInt8)consts.pullRef("p:S::stored").value();
	bcode
		.addPutNone(bPush)
		.addPutArg(0, bPush)
		.addGetProp(S_stored, bDiscard)
		.addRet();
	setup(consts, bcode, "yama:None", { "p:S" });

	ASSERT_TRUE(ctx.pushInt(-101));
	ASSERT_TRUE(ctx.structInit(ctx.load("p:S"), "stored"));

	ASSERT_TRUE(ctx.copy(0));
	ASSERT_TRUE(ctx.call(ctx.load("p:f"), 1));

	expectLocal(1, NoneVal{});
}

TEST_F(BCodeExec, SetProp_Stored) {
	_ym::ConstTableInfo consts{};
	_ym::BCodeWriter bcode{};
	auto S = (YmUInt8)consts.pullRef("p:S").value();
	auto S_stored = (YmUInt8)consts.pullRef("p:S::stored").value();
	auto i0 = (YmUInt8)consts.pullVal<YmInt>(10).value();
	bcode
		.addPutNone(bPush)
		.addPutArg(0, bPush)
		.addPutConst(i0, bPush)
		.addSetProp(S_stored)
		.addRet();
	setup(consts, bcode, "yama:None", { "p:S" });

	ASSERT_TRUE(ctx.pushInt(-101));
	ASSERT_TRUE(ctx.structInit(ctx.load("p:S"), "stored"));

	ASSERT_TRUE(ctx.copy(0));
	ASSERT_TRUE(ctx.getProperty(ctx.load("p:S::stored")));

	expectLocal(1, YmInt(-101));

	ASSERT_TRUE(ctx.copy(0));
	ASSERT_TRUE(ctx.call(ctx.load("p:f"), 1));

	expectLocal(2, NoneVal{});

	ASSERT_TRUE(ctx.copy(0));
	ASSERT_TRUE(ctx.getProperty(ctx.load("p:S::stored")));

	expectLocal(3, YmInt(10));
}

TEST_F(BCodeExec, SetProp_Computed) {
	_ym::ConstTableInfo consts{};
	_ym::BCodeWriter bcode{};
	auto S = (YmUInt8)consts.pullRef("p:S").value();
	auto S_computed = (YmUInt8)consts.pullRef("p:S::computed").value();
	auto i0 = (YmUInt8)consts.pullVal<YmInt>(10).value();
	bcode
		.addPutNone(bPush)
		.addPutArg(0, bPush)
		.addPutConst(i0, bPush)
		.addSetProp(S_computed)
		.addRet();
	setup(consts, bcode, "yama:None", { "p:S" });

	ASSERT_TRUE(ctx.pushInt(-101));
	ASSERT_TRUE(ctx.structInit(ctx.load("p:S"), "stored"));

	ASSERT_TRUE(ctx.copy(0));
	ASSERT_TRUE(ctx.getProperty(ctx.load("p:S::computed")));

	expectLocal(1, YmInt(25));
	EXPECT_EQ(S_computed_gets, 1);
	EXPECT_EQ(S_computed_sets, 0);

	ASSERT_TRUE(ctx.copy(0));
	ASSERT_TRUE(ctx.call(ctx.load("p:f"), 1));

	expectLocal(2, NoneVal{});
	EXPECT_EQ(S_computed_gets, 1);
	EXPECT_EQ(S_computed_sets, 1);
	EXPECT_EQ(S_computed_last_val, 10);

	ASSERT_TRUE(ctx.copy(0));
	ASSERT_TRUE(ctx.getProperty(ctx.load("p:S::computed")));

	expectLocal(3, YmInt(25));
	EXPECT_EQ(S_computed_gets, 2);
	EXPECT_EQ(S_computed_sets, 1);
}

TEST_F(BCodeExec, Conv) {
	_ym::ConstTableInfo consts{};
	_ym::BCodeWriter bcode{};
	auto Int = (YmUInt8)consts.pullRef("yama:Int").value();
	bcode
		.addPutNone(bPush)
		.addPutArg(0, bPush)
		.addConv(Int, 0)
		.addRet();
	setup(consts, bcode, "yama:Int", { "yama:UInt" });

	ASSERT_TRUE(ctx.pushUInt(105));
	ASSERT_TRUE(ctx.call(ctx.load("p:f"), 1));

	expectLocal(0, YmInt(105));
}

TEST_F(BCodeExec, Conv_Push) {
	_ym::ConstTableInfo consts{};
	_ym::BCodeWriter bcode{};
	auto Int = (YmUInt8)consts.pullRef("yama:Int").value();
	bcode
		.addPutArg(0, bPush)
		.addConv(Int, bPush)
		.addRet();
	setup(consts, bcode, "yama:Int", { "yama:UInt" });

	ASSERT_TRUE(ctx.pushUInt(105));
	ASSERT_TRUE(ctx.call(ctx.load("p:f"), 1));

	expectLocal(0, YmInt(105));
}

TEST_F(BCodeExec, Conv_Discard) {
	_ym::ConstTableInfo consts{};
	_ym::BCodeWriter bcode{};
	auto Int = (YmUInt8)consts.pullRef("yama:Int").value();
	bcode
		.addPutNone(bPush)
		.addPutArg(0, bPush)
		.addConv(Int, bDiscard)
		.addRet();
	setup(consts, bcode, "yama:None", { "yama:UInt" });

	ASSERT_TRUE(ctx.pushUInt(105));
	ASSERT_TRUE(ctx.call(ctx.load("p:f"), 1));

	expectLocal(0, NoneVal{});
}

TEST_F(BCodeExec, Jump) {
	_ym::ConstTableInfo consts{};
	_ym::BCodeWriter bcode{};
	auto i0 = (YmUInt8)consts.pullVal<YmInt>(-105).value();
	bcode
		.addPutConst(i0, bPush)
		// Push two None(s), then pop them after the label before
		// returning.
		// This checks that jump doesn't modify the stack.
		.addPutNone(bPush)
		.addPutNone(bPush)
		.addJump(0)
		.addPutNone(bPush)
		.addPutNone(bPush)
		.addPutNone(bPush)
		.addLabel(0)
		.addPop(2)
		.addRet();
	setup(consts, bcode, "yama:Int", {});

	ASSERT_TRUE(ctx.call(ctx.load("p:f"), 0));

	expectLocal(0, YmInt(-105));
}

TEST_F(BCodeExec, JumpTrue) {
	_ym::ConstTableInfo consts{};
	_ym::BCodeWriter bcode{};
	auto observeInt = (YmUInt8)consts.pullRef("sidefx:observeInt").value();
	auto plus = (YmUInt8)consts.pullRef("p:plus").value();
	auto lessThan = (YmUInt8)consts.pullRef("p:lessThan").value();
	auto not_ = (YmUInt8)consts.pullRef("p:not").value();
	auto i0 = (YmUInt8)consts.pullVal<YmInt>(0).value();
	auto i1 = (YmUInt8)consts.pullVal<YmInt>(1).value();
	bcode
		// Init counter to 0.
		.addPutConst(i0, bPush)
		// Start of our loop.
		.addLabel(0)
		// Check if counter is not less than arg.
		.addCopy(0, bPush)
		.addPutArg(0, bPush)
		.addPCall(lessThan, bPush)
		.addPCall(not_, bPush)
		// If counter is greater or eq to arg, branch to end.
		// Otherwise, perform iter of incr loop.
		.addJumpTrue(1)
		// Incr counter.
		.addCopy(0, bPush)
		.addPutConst(i1, bPush)
		.addPCall(plus, 0) // Local 0 is our counter.
		// Output sidefx of new counter value.
		.addCopy(0, bPush)
		.addPCall(observeInt, bDiscard)
		// Jump back to loop start.
		.addJump(0)
		// Return final counter value (which should be 0 if
		// arg was negative.)
		.addLabel(1)
		// Our counter should be the only thing on stack.
		.addRet();
	setup(consts, bcode, "yama:Int", { "yama:Int" });

	ASSERT_TRUE(ctx.pushInt(5));
	ASSERT_TRUE(ctx.call(ctx.load("p:f"), 1));

	expectLocal(0, YmInt(5));

	SideFxHistory hist0{};
	hist0.observeInt(1);
	hist0.observeInt(2);
	hist0.observeInt(3);
	hist0.observeInt(4);
	hist0.observeInt(5);
	expectSideFx(hist0);

	clearSideFx();
	ASSERT_TRUE(ctx.pushInt(3));
	ASSERT_TRUE(ctx.call(ctx.load("p:f"), 1));

	expectLocal(1, YmInt(3));

	SideFxHistory hist1{};
	hist1.observeInt(1);
	hist1.observeInt(2);
	hist1.observeInt(3);
	expectSideFx(hist1);

	clearSideFx();
	ASSERT_TRUE(ctx.pushInt(-4));
	ASSERT_TRUE(ctx.call(ctx.load("p:f"), 1));

	expectLocal(2, YmInt(0));

	SideFxHistory hist2{};
	expectSideFx(hist2);
}

TEST_F(BCodeExec, JumpFalse) {
	_ym::ConstTableInfo consts{};
	_ym::BCodeWriter bcode{};
	auto observeInt = (YmUInt8)consts.pullRef("sidefx:observeInt").value();
	auto plus = (YmUInt8)consts.pullRef("p:plus").value();
	auto lessThan = (YmUInt8)consts.pullRef("p:lessThan").value();
	auto i0 = (YmUInt8)consts.pullVal<YmInt>(0).value();
	auto i1 = (YmUInt8)consts.pullVal<YmInt>(1).value();
	bcode
		// Init counter to 0.
		.addPutConst(i0, bPush)
		// Start of our loop.
		.addLabel(0)
		// Check if counter is less than arg.
		.addCopy(0, bPush)
		.addPutArg(0, bPush)
		.addPCall(lessThan, bPush)
		// If counter is less than arg, perform iter of incr loop.
		// Otherwise, branch to end.
		.addJumpFalse(1)
		// Incr counter.
		.addCopy(0, bPush)
		.addPutConst(i1, bPush)
		.addPCall(plus, 0) // Local 0 is our counter.
		// Output sidefx of new counter value.
		.addCopy(0, bPush)
		.addPCall(observeInt, bDiscard)
		// Jump back to loop start.
		.addJump(0)
		// Return final counter value (which should be 0 if
		// arg was negative.)
		.addLabel(1)
		// Our counter should be the only thing on stack.
		.addRet();
	setup(consts, bcode, "yama:Int", { "yama:Int" });

	ASSERT_TRUE(ctx.pushInt(5));
	ASSERT_TRUE(ctx.call(ctx.load("p:f"), 1));

	expectLocal(0, YmInt(5));

	SideFxHistory hist0{};
	hist0.observeInt(1);
	hist0.observeInt(2);
	hist0.observeInt(3);
	hist0.observeInt(4);
	hist0.observeInt(5);
	expectSideFx(hist0);

	clearSideFx();
	ASSERT_TRUE(ctx.pushInt(3));
	ASSERT_TRUE(ctx.call(ctx.load("p:f"), 1));

	expectLocal(1, YmInt(3));

	SideFxHistory hist1{};
	hist1.observeInt(1);
	hist1.observeInt(2);
	hist1.observeInt(3);
	expectSideFx(hist1);

	clearSideFx();
	ASSERT_TRUE(ctx.pushInt(-4));
	ASSERT_TRUE(ctx.call(ctx.load("p:f"), 1));

	expectLocal(2, YmInt(0));

	SideFxHistory hist2{};
	expectSideFx(hist2);
}

