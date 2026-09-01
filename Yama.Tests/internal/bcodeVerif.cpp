

#include <gtest/gtest.h>
#include <yama/yama.h>
#include <yama++/general.h>
#include <yama++/print.h>
#include <yama++/Domain.h>
#include <yama++/Context.h>
#include <yama++/ParcelDef.h>

#include "../utils/utils.h"

#include <yama/../internal/bcode.h>
#include <yama/../internal/BCodeVerifier.h>
#include <yama/../internal/YmParcelDef.h>


using namespace ym;


constexpr auto bPush = _ym::bPush;
constexpr auto bDiscard = _ym::bDiscard;


class BCodeVerif : public ::testing::Test {
public:
	ErrCounter err;
	ym::ParcelDef pdef;
	_ym::BCodeWriter bcode;


	BCodeVerif() :
		pdef() {
	}


	std::optional<YmUInt8> pullRefConst(std::string localname, std::string symbol) {
		if (auto info = pdef.get()->info->type(localname)) {
			auto result = info->consts.pullRef(_ym::Spec::type(symbol), YM_MAX_UINT8);
			EXPECT_TRUE(result) << std::format("-- pullRefConst({}, {})\n", localname, symbol);
			if (result) {
				return (YmUInt8)*result;
			}
		}
		return std::nullopt;
	}
	template<typename T>
	std::optional<YmUInt8> pullValConst(std::string localname, const T& v) {
		if (auto info = pdef.get()->info->type(localname)) {
			auto result = info->consts.pullVal(v, YM_MAX_UINT8);
			EXPECT_TRUE(result) << std::format("-- pullValConst({}, {})\n", localname, ym::fmt(v));
			if (result) {
				return (YmUInt8)*result;
			}
		}
		return std::nullopt;
	}

	void addFn(
		std::string name,
		std::string returnType,
		std::vector<std::string> paramTypes) {
		std::vector<std::pair<std::string, std::string>> params{};
		for (const auto& paramType : paramTypes) {
			params.push_back(std::make_pair(std::format("arg{}", params.size()), paramType));
		}
		EXPECT_TRUE(pdef.addFn(name, returnType, params, {}, [](YmCtx*, YmType*, void*) {}))
			<< std::format("-- addFn({}, {}, ~)", name, returnType);
	}

	void passes(std::string fullname, const _ym::BCode& code) {
		EXPECT_TRUE(_test(fullname, code));
	}
	void passes(std::string fullname) {
		EXPECT_TRUE(_test(fullname));
	}
	void fails(std::string fullname, const _ym::BCode& code) {
		EXPECT_FALSE(_test(fullname, code));
	}
	void fails(std::string fullname) {
		EXPECT_FALSE(_test(fullname));
	}


protected:
	void SetUp() override {
		err.setupCallbackForThisThread();
	}
	void TearDown() override {
	}


private:
	bool _test(std::string fullname, const _ym::BCode& code) {
		ym::Domain dm;
		ym::Context ctx(dm);
		dm.bind("p", pdef);
		auto f = ctx.load(fullname).value();
		return _ym::BCodeVerifier(*ctx.get()).verify(*f.get(), code);
	}
	bool _test(std::string fullname) {
		auto code = bcode.done();
		if (!code) {
			ADD_FAILURE() << "bcode was invalid!";
		}
		return _test(std::move(fullname), ym::deref(code));
	}
};


TEST_F(BCodeVerif, TolerateDeadCode) {
	addFn("f", "yama:None", {});

	bcode
		.addNoop()
		.addNoop()
		.addJump(0)
		// *** Dead Code
		.addNoop()
		.addNoop()
		.addNoop()
		// ***
		.addLabel(0)
		.addNoop()
		.addPutNone(bPush)
		.addRet();

	passes("p:f");
}

TEST_F(BCodeVerif, TolerateAllControlPathsBeingCyclicalAndThusNoExitpoints) {
	addFn("f", "yama:None", {});

	bcode
		.addLabel(0)
		.addNoop()
		.addNoop()
		.addNoop()
		.addJump(0);

	passes("p:f");
}

TEST_F(BCodeVerif, NoBranch) {
	addFn("f", "yama:Float", {});
	auto b0 = pullValConst<YmBool>("f", true).value();
	auto f0 = pullValConst<YmFloat>("f", 3.14159).value();

	bcode
		.addPutConst(b0, bPush)
		.addPutConst(f0, bPush)
		.addRet();

	passes("p:f");
}

TEST_F(BCodeVerif, Branch_MultipleExitpoints) {
	addFn("f", "yama:Float", {});
	auto b0 = pullValConst<YmBool>("f", true).value();
	auto f0 = pullValConst<YmFloat>("f", 3.14159).value();
	auto f1 = pullValConst<YmFloat>("f", 0.05).value();

	bcode
		.addPutConst(b0, bPush)
		.addJumpFalse(0)
		// True Path
		.addPutConst(f0, bPush)
		.addRet()
		// False Path
		.addLabel(0)
		.addPutConst(f1, bPush)
		.addRet();

	passes("p:f");
}

TEST_F(BCodeVerif, Branch_ControlPathsMerge) {
	addFn("f", "yama:Float", {});
	auto b0 = pullValConst<YmBool>("f", true).value();
	auto f0 = pullValConst<YmFloat>("f", 3.14159).value();
	auto f1 = pullValConst<YmFloat>("f", 0.05).value();

	bcode
		.addPutConst(b0, bPush)
		.addJumpFalse(0)
		// True Path
		.addPutConst(f0, bPush)
		.addJump(1)
		// False Path
		.addLabel(0)
		.addPutConst(f1, bPush)
		// Merged Path
		.addLabel(1)
		.addRet();

	passes("p:f");
}

TEST_F(BCodeVerif, Branch_CyclicalControlPath) {
	addFn("f", "yama:Float", {});
	auto b0 = pullValConst<YmBool>("f", true).value();
	auto f0 = pullValConst<YmFloat>("f", 3.14159).value();

	bcode
		// Loop Path
		.addLabel(0)
		.addPutConst(f0, bPush) // L(0) is Float to return.
		.addPutConst(b0, bPush) // L(1) is branch cond.
		.addJumpTrue(1) // Consumes L(1), branch to exit.
		.addPop(1) // Pop L(0) before looping.
		.addJump(0) // Branch to loop.
		// Exit Path
		.addLabel(1)
		.addRet(); // L(0) must be Float.

	passes("p:f");
}

TEST_F(BCodeVerif, Branch_FallthroughDueToLastInstrOfBlockNotBeingBranchOrExitpoint) {
	// Basic blocks being partitioned at each branch destination means it's possible for them
	// to end w/ an instr that is not a branch/exitpoint instr.

	// In this scenario, the expected behaviour is to fallthrough to the next block. However, I
	// encountered an issue in our old impl where our system didn't realize that it was supposed
	// to do this, and quietly just *didn't*. This left me worried that this is something our
	// tests didn't cover.

	// To this end, this tests that symbolic execution proceeds across these fallthroughs, doing
	// so by checking for an error in a later block that likely wouldn't be detected if this
	// fallthrough never occurs.

	addFn("f", "yama:Float", {});
	auto b0 = pullValConst<YmBool>("f", true).value();
	auto f0 = pullValConst<YmFloat>("f", 3.14159).value();

	bcode
		// Block #1
		.addNoop()
		// Fallthrough to block #2.
		// Block #2
		.addLabel(0)
		.addNoop() // Jump Destination
		.addPutArg(100, bPush) // Error!
		.addPop(1)
		.addJump(0);

	fails("p:f");
}

TEST_F(BCodeVerif, Branch_IncoherenceDueToTypeMismatch) {
	addFn("f", "yama:Float", {});
	auto b0 = pullValConst<YmBool>("f", true).value();
	auto f0 = pullValConst<YmFloat>("f", 3.14159).value();
	auto i0 = pullValConst<YmInt>("f", -41).value();

	bcode
		// Block #1
		.addPutConst(b0, bPush) // L(0) is Bool.
		.addJumpFalse(0) // Branch to #2 or #3.
		// Block #2
		.addPutConst(f0, bPush) // L(0) is Float.
		.addJump(1) // Branch to #4
		// Block #3
		.addLabel(0)
		.addPutConst(i0, bPush) // L(0) is Int.
		.addJump(1) // Branch to #4
		// Block #4
		.addLabel(1)
		// Above branches to #4 result in coherence violation due
		// to one incoming branch has L(0) be Float, but the other
		// has it as Int.
		.addRet();

	fails("p:f");
}

TEST_F(BCodeVerif, Branch_IncoherenceDueToStkHeightMismatch) {
	addFn("f", "yama:Float", {});
	auto b0 = pullValConst<YmBool>("f", true).value();
	auto f0 = pullValConst<YmFloat>("f", 3.14159).value();
	auto i0 = pullValConst<YmInt>("f", 3).value();

	bcode
		// Block #1
		.addPutConst(b0, bPush) // L(0) is Bool.
		.addJumpTrue(0) // Branch to #2 or #3.
		// Block #2
		.addPutConst(f0, bPush) // L(0) is Float.
		// Block #3
		.addLabel(0)
		// Below is ambiguous as to whether it's affecting
		// L(0) or L(1) due to coherence violation.
		.addPutConst(i0, bPush)
		.addRet();

	fails("p:f");
}

TEST_F(BCodeVerif, BinaryIsEmpty) {
	addFn("f", "yama:None", {});

	fails("p:f");
}

TEST_F(BCodeVerif, FinalBlockFallthroughToOutOfBoundsInstrs) {
	addFn("f", "yama:None", {});
	auto b0 = pullValConst<YmBool>("f", true).value();

	bcode
		.addLabel(0)
		.addPutConst(b0, bPush)
		// This does branch to legal block, but ALSO falls through to
		// out-of-bounds instr!
		.addJumpTrue(0);

	fails("p:f");
}

static_assert(_ym::Opcodes == 17);

TEST_F(BCodeVerif, Noop) {
	addFn("f", "yama:None", {});

	bcode
		.addNoop()
		.addNoop()
		.addNoop()
		.addPutNone(bPush)
		.addRet();

	passes("p:f");
}

TEST_F(BCodeVerif, Pop) {
	addFn("f", "yama:Float", {});
	auto f0 = pullValConst<YmFloat>("f", 0.05).value();

	bcode
		.addPutConst(f0, bPush)
		.addPutNone(bPush)
		.addPutNone(bPush)
		.addPutNone(bPush)
		.addPop(3) // Pop 3 None(s) so can return our Float.
		.addRet();

	passes("p:f");
}

TEST_F(BCodeVerif, Pop_Zero) {
	addFn("f", "yama:Float", {});
	auto f0 = pullValConst<YmFloat>("f", 0.05).value();

	bcode
		.addPutConst(f0, bPush)
		.addPop(0)
		.addRet();

	passes("p:f");
}

TEST_F(BCodeVerif, Pop_MoreThanAreOnStack) {
	addFn("f", "yama:None", {});

	bcode
		// Stack should be empty to branch here.
		.addLabel(0)
		.addPutNone(bPush)
		.addPutNone(bPush)
		.addPutNone(bPush)
		.addPutNone(bPush)
		.addPop(15)
		.addJump(0);

	passes("p:f");
}

TEST_F(BCodeVerif, PutNone) {
	addFn("f", "yama:None", {});
	auto f0 = pullValConst<YmFloat>("f", 0.05).value();

	bcode
		.addPutConst(f0, bPush) // L(0) is Float.
		.addPutNone(0) // L(0) is None.
		.addRet();

	passes("p:f");
}

TEST_F(BCodeVerif, PutNone_Push) {
	addFn("f", "yama:None", {});

	bcode
		.addPutNone(bPush) // L(0) is None.
		.addRet();

	passes("p:f");
}

TEST_F(BCodeVerif, PutNone_Discard) {
	addFn("f", "yama:Float", {});
	auto f0 = pullValConst<YmFloat>("f", 0.05).value();

	bcode
		.addPutConst(f0, bPush) // L(0) is Float.
		.addPutNone(bDiscard) // Discards.
		.addRet();

	passes("p:f");
}

TEST_F(BCodeVerif, PutNone_LwA_OutOfBounds) {
	addFn("f", "yama:Float", {});

	bcode
		.addPutNone(100)
		.addRet();

	fails("p:f");
}

TEST_F(BCodeVerif, PutConst) {
	addFn("f", "yama:Float", {});
	auto f0 = pullValConst<YmFloat>("f", 0.05).value();

	bcode
		.addPutNone(bPush) // L(0) is None.
		.addPutConst(f0, 0) // L(0) is Float.
		.addRet();

	passes("p:f");
}

TEST_F(BCodeVerif, PutConst_Push) {
	addFn("f", "yama:Float", {});
	auto f0 = pullValConst<YmFloat>("f", 0.05).value();

	bcode
		.addPutConst(f0, bPush) // L(0) is Float.
		.addRet();

	passes("p:f");
}

TEST_F(BCodeVerif, PutConst_Discard) {
	addFn("f", "yama:None", {});
	auto f0 = pullValConst<YmFloat>("f", 0.05).value();

	bcode
		.addPutNone(bPush) // L(0) is None.
		.addPutConst(f0, bDiscard) // Discards.
		.addRet();

	passes("p:f");
}

TEST_F(BCodeVerif, PutConst_KoA_OutOfBounds) {
	addFn("f", "yama:Float", {});

	bcode
		.addPutConst(100, bPush)
		.addRet();

	fails("p:f");
}

TEST_F(BCodeVerif, PutConst_KoA_NonObjConst) {
	// TODO
}

TEST_F(BCodeVerif, PutConst_LwB_OutOfBounds) {
	addFn("f", "yama:Float", {});
	auto f0 = pullValConst<YmFloat>("f", 0.05).value();

	bcode
		.addPutConst(f0, 0)
		.addRet();

	fails("p:f");
}

TEST_F(BCodeVerif, PutArg) {
	addFn("f", "yama:Float", { "yama:Float" });

	bcode
		.addPutNone(bPush) // L(0) is None.
		.addPutArg(0, 0) // L(0) is Float.
		.addRet();

	passes("p:f");
}

TEST_F(BCodeVerif, PutArg_Push) {
	addFn("f", "yama:Float", { "yama:Float" });

	bcode
		.addPutArg(0, bPush) // L(0) is Float.
		.addRet();

	passes("p:f");
}

TEST_F(BCodeVerif, PutArg_Discard) {
	addFn("f", "yama:None", { "yama:Float" });

	bcode
		.addPutNone(bPush) // L(0) is None.
		.addPutArg(0, bDiscard) // Discards.
		.addRet();

	passes("p:f");
}

TEST_F(BCodeVerif, PutArg_ArgA_OutOfBounds) {
	addFn("f", "yama:Float", { "yama:Float" });

	bcode
		.addPutArg(100, bPush)
		.addRet();

	fails("p:f");
}

TEST_F(BCodeVerif, PutArg_LwB_OutOfBounds) {
	addFn("f", "yama:Float", { "yama:Float" });

	bcode
		.addPutArg(0, 0)
		.addRet();

	fails("p:f");
}

TEST_F(BCodeVerif, Copy) {
	addFn("f", "yama:Float", {});
	auto f0 = pullValConst<YmFloat>("f", 0.05).value();

	bcode
		.addPutConst(f0, bPush) // L(0) is Float.
		.addPutNone(bPush) // L(1) is None.
		.addCopy(0, 1) // L(1) is Float.
		.addRet();

	passes("p:f");
}

TEST_F(BCodeVerif, Copy_Push) {
	addFn("f", "yama:Float", {});
	auto f0 = pullValConst<YmFloat>("f", 0.05).value();

	bcode
		.addPutConst(f0, bPush) // L(0) is Float.
		.addPutNone(bPush) // L(1) is None.
		.addCopy(0, bPush) // L(2) is Float.
		.addRet();

	passes("p:f");
}

TEST_F(BCodeVerif, Copy_Discard) {
	addFn("f", "yama:None", {});
	auto f0 = pullValConst<YmFloat>("f", 0.05).value();

	bcode
		.addPutConst(f0, bPush) // L(0) is Float.
		.addPutNone(bPush) // L(1) is None.
		.addCopy(0, bDiscard) // Discards.
		.addRet();

	passes("p:f");
}

TEST_F(BCodeVerif, Copy_LA_OutOfBounds) {
	addFn("f", "yama:Float", {});

	bcode
		.addCopy(100, bPush)
		.addRet();

	fails("p:f");
}

TEST_F(BCodeVerif, Copy_LwB_OutOfBounds) {
	addFn("f", "yama:Float", {});
	auto f0 = pullValConst<YmFloat>("f", 0.05).value();

	bcode
		.addPutConst(f0, bPush) // L(0) is Float.
		.addCopy(0, 1)
		.addRet();

	fails("p:f");
}

TEST_F(BCodeVerif, DefaultInit) {
	addFn("f", "yama:Float", {});
	auto t0 = pullRefConst("f", "yama:Float").value();

	bcode
		.addPutNone(bPush)
		.addDefaultInit(t0, 0)
		.addRet();

	passes("p:f");
}

TEST_F(BCodeVerif, DefaultInit_Push) {
	addFn("f", "yama:Float", {});
	auto t0 = pullRefConst("f", "yama:Float").value();

	bcode
		.addDefaultInit(t0, bPush)
		.addRet();

	passes("p:f");
}

TEST_F(BCodeVerif, DefaultInit_Discard) {
	addFn("f", "yama:None", {});
	auto t0 = pullRefConst("f", "yama:Float").value();

	bcode
		.addPutNone(bPush)
		.addDefaultInit(t0, bDiscard)
		.addRet();

	passes("p:f");
}

TEST_F(BCodeVerif, DefaultInit_KtA_OutOfBounds) {
	addFn("f", "yama:Float", {});

	bcode
		.addDefaultInit(100, bPush)
		.addRet();

	fails("p:f");
}

TEST_F(BCodeVerif, DefaultInit_KtA_NonTypeConst) {
	addFn("f", "yama:Float", {});
	auto f0 = pullValConst<YmFloat>("f", 0.05).value();

	bcode
		.addDefaultInit(f0, bPush)
		.addRet();

	fails("p:f");
}

TEST_F(BCodeVerif, DefaultInit_KtA_NoDefaultValue) {
	addFn("f", "yama:Float", {});
	addFn("g", "yama:None", {});
	auto t0 = pullRefConst("f", "p:g").value();

	bcode
		.addDefaultInit(t0, bPush)
		.addRet();

	fails("p:f");
}

TEST_F(BCodeVerif, DefaultInit_LwB_OutOfBounds) {
	addFn("f", "yama:Float", {});
	auto t0 = pullRefConst("f", "yama:Float").value();

	bcode
		.addDefaultInit(t0, 0)
		.addRet();

	fails("p:f");
}

// TODO: Also remember to add stored property tests for getProp/setProp
//		 when we add structInit.

TEST_F(BCodeVerif, StructInit) {
	// TODO
}

TEST_F(BCodeVerif, PCall) {
	addFn("f", "yama:Float", {});
	addFn("g", "yama:Float", { "yama:Int", "yama:Rune" });
	auto i0 = pullValConst<YmInt>("f", 5).value();
	auto r0 = pullValConst<YmRune>("f", 'j').value();
	auto t0 = pullRefConst("f", "p:g").value();

	bcode
		.addPutNone(bPush) // L(0) is None.
		.addPutConst(i0, bPush) // L(1) is Int.
		.addPutConst(r0, bPush) // L(2) is Rune.
		.addPCall(t0, 0) // L(0) is Float.
		.addRet();

	passes("p:f");
}

TEST_F(BCodeVerif, PCall_Push) {
	addFn("f", "yama:Float", {});
	addFn("g", "yama:Float", { "yama:Int", "yama:Rune" });
	auto i0 = pullValConst<YmInt>("f", 5).value();
	auto r0 = pullValConst<YmRune>("f", 'j').value();
	auto t0 = pullRefConst("f", "p:g").value();

	bcode
		.addPutConst(i0, bPush) // L(0) is Int.
		.addPutConst(r0, bPush) // L(1) is Rune.
		.addPCall(t0, bPush) // L(0) is Float.
		.addRet();

	passes("p:f");
}

TEST_F(BCodeVerif, PCall_Discard) {
	addFn("f", "yama:None", {});
	addFn("g", "yama:Float", { "yama:Int", "yama:Rune" });
	auto i0 = pullValConst<YmInt>("f", 5).value();
	auto r0 = pullValConst<YmRune>("f", 'j').value();
	auto t0 = pullRefConst("f", "p:g").value();

	bcode
		.addPutNone(bPush) // L(0) is None.
		.addPutConst(i0, bPush) // L(1) is Int.
		.addPutConst(r0, bPush) // L(2) is Rune.
		.addPCall(t0, bDiscard) // Discards.
		.addRet();

	passes("p:f");
}

TEST_F(BCodeVerif, PCall_KtA_OutOfBounds) {
	addFn("f", "yama:Float", {});
	addFn("g", "yama:Float", { "yama:Int", "yama:Rune" });
	auto i0 = pullValConst<YmInt>("f", 5).value();
	auto r0 = pullValConst<YmRune>("f", 'j').value();
	auto t0 = pullRefConst("f", "p:g").value();

	bcode
		.addPutConst(i0, bPush) // L(0) is Int.
		.addPutConst(r0, bPush) // L(1) is Rune.
		.addPCall(100, bPush)
		.addRet();

	fails("p:f");
}

TEST_F(BCodeVerif, PCall_KtA_NonTypeConst) {
	addFn("f", "yama:Float", {});
	addFn("g", "yama:Float", { "yama:Int", "yama:Rune" });
	auto i0 = pullValConst<YmInt>("f", 5).value();
	auto r0 = pullValConst<YmRune>("f", 'j').value();

	bcode
		.addPutConst(i0, bPush) // L(0) is Int.
		.addPutConst(r0, bPush) // L(1) is Rune.
		.addPCall(r0, bPush) // r0 isn't type.
		.addRet();

	fails("p:f");
}

TEST_F(BCodeVerif, PCall_KtA_NonCallableType) {
	addFn("f", "yama:Float", {});
	addFn("g", "yama:Float", { "yama:Int", "yama:Rune" });
	auto i0 = pullValConst<YmInt>("f", 5).value();
	auto r0 = pullValConst<YmRune>("f", 'j').value();
	auto t0 = pullRefConst("f", "yama:Bool").value(); // Bool isn't callable.

	bcode
		.addPutConst(i0, bPush) // L(0) is Int.
		.addPutConst(r0, bPush) // L(1) is Rune.
		.addPCall(t0, bPush)
		.addRet();

	fails("p:f");
}

TEST_F(BCodeVerif, PCall_LwB_OutOfBounds) {
	addFn("f", "yama:Float", {});
	addFn("g", "yama:Float", { "yama:Int", "yama:Rune" });
	auto i0 = pullValConst<YmInt>("f", 5).value();
	auto r0 = pullValConst<YmRune>("f", 'j').value();
	auto t0 = pullRefConst("f", "p:g").value();

	bcode
		.addPutConst(i0, bPush) // L(0) is Int.
		.addPutConst(r0, bPush) // L(1) is Rune.
		.addPCall(t0, 100)
		.addRet();

	fails("p:f");
}

TEST_F(BCodeVerif, PCall_Args_TooFew) {
	addFn("f", "yama:Float", {});
	addFn("g", "yama:Float", { "yama:Int", "yama:Rune" });
	auto i0 = pullValConst<YmInt>("f", 5).value();
	auto r0 = pullValConst<YmRune>("f", 'j').value();
	auto t0 = pullRefConst("f", "p:g").value();

	bcode
		.addPutConst(i0, bPush) // L(0) is Int.
		.addPCall(t0, bPush)
		.addRet();

	fails("p:f");
}

TEST_F(BCodeVerif, PCall_Args_WrongTypes) {
	addFn("f", "yama:Float", {});
	addFn("g", "yama:Float", { "yama:Int", "yama:Rune" });
	auto i0 = pullValConst<YmInt>("f", 5).value();
	auto r0 = pullValConst<YmRune>("f", 'j').value();
	auto t0 = pullRefConst("f", "p:g").value();

	bcode
		.addPutConst(i0, bPush) // L(0) is Int.
		.addPutConst(i0, bPush) // L(1) is Int. (Wrong Type)
		.addPCall(t0, bPush)
		.addRet();

	fails("p:f");
}

TEST_F(BCodeVerif, PNCall) {
	// TODO
}

TEST_F(BCodeVerif, Ret) {
	addFn("f", "yama:Float", {});
	auto f0 = pullValConst<YmFloat>("f", 0.05).value();

	bcode
		.addPutConst(f0, bPush) // L(0) is Float.
		.addRet();

	passes("p:f");
}

TEST_F(BCodeVerif, Ret_Value_Missing) {
	addFn("f", "yama:Float", {});

	bcode
		.addRet();

	fails("p:f");
}

TEST_F(BCodeVerif, Ret_Value_WrongType) {
	addFn("f", "yama:Float", {});

	bcode
		.addPutNone(bPush) // Error
		.addRet();

	fails("p:f");
}

TEST_F(BCodeVerif, GetVar_Stored) {
	addFn("f", "yama:Float", {});
	pdef.addReadOnlyStoredVar("V", "yama:Float", [](YmCtx*, YmType*, void*) {});
	auto t0 = pullRefConst("f", "p:V").value();

	bcode
		.addPutNone(bPush) // L(0) is None.
		.addGetVar(t0, 0) // L(0) is Float.
		.addRet();

	passes("p:f");
}

TEST_F(BCodeVerif, GetVar_Computed) {
	addFn("f", "yama:Float", {});
	pdef.addReadOnlyComputedVar("V", "yama:Float", [](YmCtx*, YmType*, void*) {});
	auto t0 = pullRefConst("f", "p:V").value();

	bcode
		.addPutNone(bPush) // L(0) is None.
		.addGetVar(t0, 0) // L(0) is Float.
		.addRet();

	passes("p:f");
}

TEST_F(BCodeVerif, GetVar_Push) {
	addFn("f", "yama:Float", {});
	pdef.addReadOnlyComputedVar("V", "yama:Float", [](YmCtx*, YmType*, void*) {});
	auto t0 = pullRefConst("f", "p:V").value();

	bcode
		.addGetVar(t0, bPush) // L(0) is Float.
		.addRet();

	passes("p:f");
}

TEST_F(BCodeVerif, GetVar_Discard) {
	addFn("f", "yama:None", {});
	pdef.addReadOnlyComputedVar("V", "yama:Float", [](YmCtx*, YmType*, void*) {});
	auto t0 = pullRefConst("f", "p:V").value();

	bcode
		.addPutNone(bPush) // L(0) is None.
		.addGetVar(t0, bDiscard) // Discards.
		.addRet();

	passes("p:f");
}

TEST_F(BCodeVerif, GetVar_KtA_OutOfBounds) {
	addFn("f", "yama:Float", {});

	bcode
		.addGetVar(100, bPush)
		.addRet();

	fails("p:f");
}

TEST_F(BCodeVerif, GetVar_KtA_NonTypeConst) {
	addFn("f", "yama:Float", {});
	auto f0 = pullValConst<YmFloat>("f", 0.05).value();

	bcode
		.addGetVar(f0, bPush)
		.addRet();

	fails("p:f");
}

TEST_F(BCodeVerif, GetVar_KtA_NonVarType) {
	addFn("f", "yama:Float", {});
	auto t0 = pullRefConst("f", "yama:Int").value();

	bcode
		.addGetVar(t0, bPush)
		.addRet();

	fails("p:f");
}

TEST_F(BCodeVerif, GetVar_LwB_OutOfBounds) {
	addFn("f", "yama:Float", {});
	pdef.addReadOnlyComputedVar("V", "yama:Float", [](YmCtx*, YmType*, void*) {});
	auto t0 = pullRefConst("f", "p:V").value();

	bcode
		.addGetVar(t0, 0)
		.addRet();

	fails("p:f");
}

TEST_F(BCodeVerif, SetVar_Stored) {
	addFn("f", "yama:None", {});
	pdef.addStoredVar("V", "yama:Float", [](YmCtx*, YmType*, void*) {});
	auto t0 = pullRefConst("f", "p:V").value();
	auto f0 = pullValConst<YmFloat>("f", 0.05).value();

	bcode
		.addPutConst(f0, bPush)
		.addSetVar(t0)
		.addPutNone(bPush)
		.addRet();

	passes("p:f");
}

TEST_F(BCodeVerif, SetVar_Computed) {
	addFn("f", "yama:None", {});
	pdef.addComputedVar("V", "yama:Float", [](YmCtx*, YmType*, void*) {}, [](YmCtx*, YmType*, void*) {});
	auto t0 = pullRefConst("f", "p:V").value();
	auto f0 = pullValConst<YmFloat>("f", 0.05).value();

	bcode
		.addPutConst(f0, bPush)
		.addSetVar(t0)
		.addPutNone(bPush)
		.addRet();

	passes("p:f");
}

TEST_F(BCodeVerif, SetVar_KtA_OutOfBounds) {
	addFn("f", "yama:None", {});
	auto f0 = pullValConst<YmFloat>("f", 0.05).value();

	bcode
		.addPutConst(f0, bPush)
		.addSetVar(100)
		.addPutNone(bPush)
		.addRet();

	fails("p:f");
}

TEST_F(BCodeVerif, SetVar_KtA_NonTypeConst) {
	addFn("f", "yama:None", {});
	auto f0 = pullValConst<YmFloat>("f", 0.05).value();

	bcode
		.addPutConst(f0, bPush)
		.addSetVar(f0)
		.addPutNone(bPush)
		.addRet();

	fails("p:f");
}

TEST_F(BCodeVerif, SetVar_KtA_NonVarType) {
	addFn("f", "yama:None", {});
	pdef.addComputedVar("V", "yama:Float", [](YmCtx*, YmType*, void*) {}, [](YmCtx*, YmType*, void*) {});
	auto t0 = pullRefConst("f", "yama:Int").value(); // Error!
	auto f0 = pullValConst<YmFloat>("f", 0.05).value();

	bcode
		.addPutConst(f0, bPush)
		.addSetVar(t0)
		.addPutNone(bPush)
		.addRet();

	fails("p:f");
}

TEST_F(BCodeVerif, SetVar_KtA_ReadOnlyVarType) {
	addFn("f", "yama:None", {});
	pdef.addReadOnlyComputedVar("V", "yama:Float", [](YmCtx*, YmType*, void*) {});
	auto t0 = pullRefConst("f", "p:V").value();
	auto f0 = pullValConst<YmFloat>("f", 0.05).value();

	bcode
		.addPutConst(f0, bPush)
		.addSetVar(t0)
		.addPutNone(bPush)
		.addRet();

	fails("p:f");
}

TEST_F(BCodeVerif, SetVar_Value_Missing) {
	addFn("f", "yama:None", {});
	pdef.addComputedVar("V", "yama:Float", [](YmCtx*, YmType*, void*) {}, [](YmCtx*, YmType*, void*) {});
	auto t0 = pullRefConst("f", "p:V").value();

	bcode
		.addSetVar(t0)
		.addPutNone(bPush)
		.addRet();

	fails("p:f");
}

TEST_F(BCodeVerif, SetVar_Value_WrongType) {
	addFn("f", "yama:None", {});
	pdef.addComputedVar("V", "yama:Float", [](YmCtx*, YmType*, void*) {}, [](YmCtx*, YmType*, void*) {});
	auto t0 = pullRefConst("f", "p:V").value();
	auto i0 = pullValConst<YmInt>("f", 5).value();

	bcode
		.addPutConst(i0, bPush) // Error
		.addSetVar(t0)
		.addPutNone(bPush)
		.addRet();

	fails("p:f");
}

TEST_F(BCodeVerif, GetProp_Stored) {
	// TODO
}

TEST_F(BCodeVerif, GetProp_Computed) {
	addFn("f", "yama:Float", {});
	pdef.addStruct("S");
	pdef.addReadOnlyComputedProperty("S", "v", "yama:Float", [](YmCtx*, YmType*, void*) {});
	auto t0 = pullRefConst("f", "p:S").value();
	auto t1 = pullRefConst("f", "p:S::v").value();

	bcode
		.addPutNone(bPush)
		.addDefaultInit(t0, bPush)
		.addGetProp(t1, 0)
		.addRet();

	passes("p:f");
}

TEST_F(BCodeVerif, GetProp_Push) {
	addFn("f", "yama:Float", {});
	pdef.addStruct("S");
	pdef.addReadOnlyComputedProperty("S", "v", "yama:Float", [](YmCtx*, YmType*, void*) {});
	auto t0 = pullRefConst("f", "p:S").value();
	auto t1 = pullRefConst("f", "p:S::v").value();

	bcode
		.addDefaultInit(t0, bPush)
		.addGetProp(t1, bPush)
		.addRet();

	passes("p:f");
}

TEST_F(BCodeVerif, GetProp_Discard) {
	addFn("f", "yama:None", {});
	pdef.addStruct("S");
	pdef.addReadOnlyComputedProperty("S", "v", "yama:Float", [](YmCtx*, YmType*, void*) {});
	auto t0 = pullRefConst("f", "p:S").value();
	auto t1 = pullRefConst("f", "p:S::v").value();

	bcode
		.addPutNone(bPush)
		.addDefaultInit(t0, bPush)
		.addGetProp(t1, bDiscard)
		.addRet();

	passes("p:f");
}

TEST_F(BCodeVerif, GetProp_KtA_OutOfBounds) {
	addFn("f", "yama:Float", {});
	pdef.addStruct("S");
	pdef.addReadOnlyComputedProperty("S", "v", "yama:Float", [](YmCtx*, YmType*, void*) {});
	auto t0 = pullRefConst("f", "p:S").value();
	auto t1 = pullRefConst("f", "p:S::v").value();

	bcode
		.addDefaultInit(t0, bPush)
		.addGetProp(100, bPush)
		.addRet();

	fails("p:f");
}

TEST_F(BCodeVerif, GetProp_KtA_NonTypeConst) {
	addFn("f", "yama:Float", {});
	pdef.addStruct("S");
	pdef.addReadOnlyComputedProperty("S", "v", "yama:Float", [](YmCtx*, YmType*, void*) {});
	auto t0 = pullRefConst("f", "p:S").value();
	auto f0 = pullValConst<YmFloat>("f", 0.05).value();

	bcode
		.addDefaultInit(t0, bPush)
		.addGetProp(f0, bPush)
		.addRet();

	fails("p:f");
}

TEST_F(BCodeVerif, GetProp_KtA_NonPropertyType) {
	addFn("f", "yama:Float", {});
	pdef.addStruct("S");
	pdef.addReadOnlyComputedProperty("S", "v", "yama:Float", [](YmCtx*, YmType*, void*) {});
	auto t0 = pullRefConst("f", "p:S").value();
	auto t1 = pullRefConst("f", "yama:Int").value();

	bcode
		.addDefaultInit(t0, bPush)
		.addGetProp(t1, bPush)
		.addRet();

	fails("p:f");
}

TEST_F(BCodeVerif, GetProp_LwB_OutOfBounds) {
	addFn("f", "yama:Float", {});
	pdef.addStruct("S");
	pdef.addReadOnlyComputedProperty("S", "v", "yama:Float", [](YmCtx*, YmType*, void*) {});
	auto t0 = pullRefConst("f", "p:S").value();
	auto t1 = pullRefConst("f", "p:S::v").value();

	bcode
		.addDefaultInit(t0, bPush)
		.addGetProp(t1, 0)
		.addRet();

	fails("p:f");
}

TEST_F(BCodeVerif, GetProp_Subject_Missing) {
	addFn("f", "yama:Float", {});
	pdef.addStruct("S");
	pdef.addReadOnlyComputedProperty("S", "v", "yama:Float", [](YmCtx*, YmType*, void*) {});
	auto t0 = pullRefConst("f", "p:S").value();
	auto t1 = pullRefConst("f", "p:S::v").value();

	bcode
		.addGetProp(t1, 0)
		.addRet();

	fails("p:f");
}

TEST_F(BCodeVerif, GetProp_Subject_WrongType) {
	addFn("f", "yama:Float", {});
	pdef.addStruct("S");
	pdef.addReadOnlyComputedProperty("S", "v", "yama:Float", [](YmCtx*, YmType*, void*) {});
	auto t0 = pullRefConst("f", "yama:Int").value(); // Error
	auto t1 = pullRefConst("f", "p:S::v").value();

	bcode
		.addDefaultInit(t0, bPush)
		.addGetProp(t1, 0)
		.addRet();

	fails("p:f");
}

TEST_F(BCodeVerif, SetProp_Stored) {
	// TODO
}

TEST_F(BCodeVerif, SetProp_Computed) {
	addFn("f", "yama:None", {});
	pdef.addStruct("S");
	pdef.addComputedProperty("S", "v", "yama:Float", [](YmCtx*, YmType*, void*) {}, [](YmCtx*, YmType*, void*) {});
	auto t0 = pullRefConst("f", "p:S").value();
	auto t1 = pullRefConst("f", "p:S::v").value();
	auto f0 = pullValConst<YmFloat>("f", 0.05).value();

	bcode
		.addDefaultInit(t0, bPush)
		.addPutConst(f0, bPush)
		.addSetProp(t1)
		.addPutNone(bPush)
		.addRet();

	passes("p:f");
}

TEST_F(BCodeVerif, SetProp_KtA_OutOfBounds) {
	addFn("f", "yama:None", {});
	pdef.addStruct("S");
	pdef.addComputedProperty("S", "v", "yama:Float", [](YmCtx*, YmType*, void*) {}, [](YmCtx*, YmType*, void*) {});
	auto t0 = pullRefConst("f", "p:S").value();
	auto t1 = pullRefConst("f", "p:S::v").value();
	auto f0 = pullValConst<YmFloat>("f", 0.05).value();

	bcode
		.addDefaultInit(t0, bPush)
		.addPutConst(f0, bPush)
		.addSetProp(100)
		.addPutNone(bPush)
		.addRet();

	fails("p:f");
}

TEST_F(BCodeVerif, SetProp_KtA_NonTypeConst) {
	addFn("f", "yama:None", {});
	pdef.addStruct("S");
	pdef.addComputedProperty("S", "v", "yama:Float", [](YmCtx*, YmType*, void*) {}, [](YmCtx*, YmType*, void*) {});
	auto t0 = pullRefConst("f", "p:S").value();
	auto f0 = pullValConst<YmFloat>("f", 0.05).value();

	bcode
		.addDefaultInit(t0, bPush)
		.addPutConst(f0, bPush)
		.addSetProp(f0)
		.addPutNone(bPush)
		.addRet();

	fails("p:f");
}

TEST_F(BCodeVerif, SetProp_KtA_NonPropertyType) {
	addFn("f", "yama:None", {});
	pdef.addStruct("S");
	pdef.addComputedProperty("S", "v", "yama:Float", [](YmCtx*, YmType*, void*) {}, [](YmCtx*, YmType*, void*) {});
	auto t0 = pullRefConst("f", "p:S").value();
	auto t1 = pullRefConst("f", "yama:Int").value();
	auto f0 = pullValConst<YmFloat>("f", 0.05).value();

	bcode
		.addDefaultInit(t0, bPush)
		.addPutConst(f0, bPush)
		.addSetProp(t1)
		.addPutNone(bPush)
		.addRet();

	fails("p:f");
}

TEST_F(BCodeVerif, SetProp_KtA_ReadOnlyProperty) {
	addFn("f", "yama:None", {});
	pdef.addStruct("S");
	pdef.addReadOnlyComputedProperty("S", "v", "yama:Float", [](YmCtx*, YmType*, void*) {});
	auto t0 = pullRefConst("f", "p:S").value();
	auto t1 = pullRefConst("f", "p:S::v").value();
	auto f0 = pullValConst<YmFloat>("f", 0.05).value();

	bcode
		.addDefaultInit(t0, bPush)
		.addPutConst(f0, bPush)
		.addSetProp(t1)
		.addPutNone(bPush)
		.addRet();

	fails("p:f");
}

TEST_F(BCodeVerif, SetProp_SubjectOrValue_Missing) {
	addFn("f", "yama:None", {});
	pdef.addStruct("S");
	pdef.addComputedProperty("S", "v", "yama:Float", [](YmCtx*, YmType*, void*) {}, [](YmCtx*, YmType*, void*) {});
	auto t0 = pullRefConst("f", "p:S").value();
	auto t1 = pullRefConst("f", "p:S::v").value();
	auto f0 = pullValConst<YmFloat>("f", 0.05).value();

	bcode
		.addPutConst(f0, bPush)
		.addSetProp(t1)
		.addPutNone(bPush)
		.addRet();

	fails("p:f");
}

TEST_F(BCodeVerif, SetProp_Subject_WrongType) {
	addFn("f", "yama:None", {});
	pdef.addStruct("S");
	pdef.addComputedProperty("S", "v", "yama:Float", [](YmCtx*, YmType*, void*) {}, [](YmCtx*, YmType*, void*) {});
	auto t0 = pullRefConst("f", "yama:Int").value(); // Error
	auto t1 = pullRefConst("f", "p:S::v").value();
	auto f0 = pullValConst<YmFloat>("f", 0.05).value();

	bcode
		.addDefaultInit(t0, bPush)
		.addPutConst(f0, bPush)
		.addSetProp(t1)
		.addPutNone(bPush)
		.addRet();

	fails("p:f");
}

TEST_F(BCodeVerif, SetProp_Value_WrongType) {
	addFn("f", "yama:None", {});
	pdef.addStruct("S");
	pdef.addComputedProperty("S", "v", "yama:Float", [](YmCtx*, YmType*, void*) {}, [](YmCtx*, YmType*, void*) {});
	auto t0 = pullRefConst("f", "p:S").value();
	auto t1 = pullRefConst("f", "yama:Int").value(); // Error
	auto f0 = pullValConst<YmFloat>("f", 0.05).value();

	bcode
		.addDefaultInit(t0, bPush)
		.addPutConst(f0, bPush)
		.addSetProp(t1)
		.addPutNone(bPush)
		.addRet();

	fails("p:f");
}

TEST_F(BCodeVerif, Conv) {
	addFn("f", "yama:Any", {});
	auto t0 = pullRefConst("f", "yama:Any").value();
	auto f0 = pullValConst<YmFloat>("f", 0.05).value();

	bcode
		.addPutNone(bPush)
		.addPutConst(f0, bPush)
		.addConv(t0, 0)
		.addRet();

	passes("p:f");
}

TEST_F(BCodeVerif, Conv_Push) {
	addFn("f", "yama:Any", {});
	auto t0 = pullRefConst("f", "yama:Any").value();
	auto f0 = pullValConst<YmFloat>("f", 0.05).value();

	bcode
		.addPutConst(f0, bPush)
		.addConv(t0, bPush)
		.addRet();

	passes("p:f");
}

TEST_F(BCodeVerif, Conv_Discard) {
	addFn("f", "yama:None", {});
	auto t0 = pullRefConst("f", "yama:Any").value();
	auto f0 = pullValConst<YmFloat>("f", 0.05).value();

	bcode
		.addPutNone(bPush)
		.addPutConst(f0, bPush)
		.addConv(t0, bDiscard)
		.addRet();

	passes("p:f");
}

TEST_F(BCodeVerif, Conv_KtA_OutOfBounds) {
	addFn("f", "yama:Any", {});
	auto f0 = pullValConst<YmFloat>("f", 0.05).value();

	bcode
		.addPutConst(f0, bPush)
		.addConv(100, bPush)
		.addRet();

	fails("p:f");
}

TEST_F(BCodeVerif, Conv_KtA_NonTypeConst) {
	addFn("f", "yama:Any", {});
	auto f0 = pullValConst<YmFloat>("f", 0.05).value();

	bcode
		.addPutConst(f0, bPush)
		.addConv(f0, bPush)
		.addRet();

	fails("p:f");
}

TEST_F(BCodeVerif, Conv_LwB_OutOfBounds) {
	addFn("f", "yama:Any", {});
	auto t0 = pullRefConst("f", "yama:Any").value();
	auto f0 = pullValConst<YmFloat>("f", 0.05).value();

	bcode
		.addPutConst(f0, bPush)
		.addConv(t0, 100)
		.addRet();

	fails("p:f");
}

TEST_F(BCodeVerif, Conv_Top_Missing) {
	addFn("f", "yama:Any", {});
	auto t0 = pullRefConst("f", "yama:Any").value();

	bcode
		.addConv(t0, bPush)
		.addRet();

	fails("p:f");
}

TEST_F(BCodeVerif, Conv_Top_CannotConvertToKtA) {
	addFn("f", "yama:Int", {});
	auto t0 = pullRefConst("f", "yama:Int").value();

	bcode
		.addPutNone(bPush)
		.addConv(t0, bPush) // None -> Int is illegal.
		.addRet();

	fails("p:f");
}

TEST_F(BCodeVerif, Jump) {
	addFn("f", "yama:Float", {});
	auto f0 = pullValConst<YmFloat>("f", 0.05).value();

	bcode
		.addPutConst(f0, bPush)
		.addJump(0)
		.addPutNone(bPush)
		.addLabel(0)
		.addRet();

	passes("p:f");
}

TEST_F(BCodeVerif, Jump_sBx_JumpsOutOfBounds) {
	addFn("f", "yama:Float", {});
	auto f0 = pullValConst<YmFloat>("f", 0.05).value();

	// Manually create a code object, then overwrite branch w/
	// new invalid sBx.
	_ym::BCodeWriter w{};
	auto code = w
		.addLabel(0)
		.addJump(0)
		.addPutConst(f0, bPush)
		.addRet()
		.done().value();
	code[0].sBx = 100;

	fails("p:f", code);
}

TEST_F(BCodeVerif, Jump_CoherenceViolation) {
	addFn("f", "yama:Float", {});
	auto b0 = pullValConst<YmBool>("f", true).value();
	auto f0 = pullValConst<YmFloat>("f", 0.05).value();
	auto i0 = pullValConst<YmInt>("f", 3).value();

	bcode
		// Block #1
		.addPutConst(b0, bPush) // L(0) is Bool.
		.addJumpFalse(0) // Branch to #2 or #3.
		// Block #2
		.addPutConst(f0, bPush) // L(0) is Float.
		.addJump(1) // Branch to #4
		// Block #3
		.addLabel(0)
		.addPutConst(i0, bPush) // L(0) is Int.
		.addJump(1) // Branch to #4
		// Block #4
		.addLabel(1)
		// Above branches to #4 result in coherence violation due
		// to one incoming branch has L(0) be Float, but the other
		// has it as Int.
		.addRet();

	fails("p:f");
}

TEST_F(BCodeVerif, JumpTrue) {
	addFn("f", "yama:Float", {});
	auto b0 = pullValConst<YmBool>("f", true).value();
	auto f0 = pullValConst<YmFloat>("f", 0.05).value();
	auto f1 = pullValConst<YmFloat>("f", 3.14159).value();

	bcode
		.addPutConst(b0, bPush)
		.addJumpTrue(0)
		.addPutConst(f0, bPush)
		.addRet()
		.addLabel(0)
		.addPutConst(f1, bPush)
		.addRet();

	passes("p:f");
}

TEST_F(BCodeVerif, JumpTrue_sBx_JumpsOutOfBounds) {
	addFn("f", "yama:Float", {});
	auto b0 = pullValConst<YmBool>("f", true).value();
	auto f0 = pullValConst<YmFloat>("f", 0.05).value();

	// Manually create a code object, then overwrite branch w/
	// new invalid sBx.
	_ym::BCodeWriter w{};
	auto code = w
		.addLabel(0)
		.addPutConst(b0, bPush)
		.addJumpTrue(0)
		.addPutConst(f0, bPush)
		.addRet()
		.done().value();
	code[1].sBx = 100;

	fails("p:f", code);
}

TEST_F(BCodeVerif, JumpTrue_CoherenceViolation) {
	addFn("f", "yama:Int", {});
	auto b0 = pullValConst<YmBool>("f", true).value();
	auto f0 = pullValConst<YmFloat>("f", 0.05).value();
	auto i0 = pullValConst<YmInt>("f", 3).value();

	bcode
		// Block #1
		.addPutConst(b0, bPush) // L(0) is Bool.
		.addJumpTrue(0) // Branch to #2 or #3.
		// Block #2
		.addPutConst(f0, bPush) // L(0) is Float.
		// Block #3
		.addLabel(0)
		// Below is ambiguous as to whether it's affecting
		// L(0) or L(1) due to coherence violation.
		.addPutConst(i0, bPush)
		.addRet();

	fails("p:f");
}

TEST_F(BCodeVerif, JumpFalse) {
	addFn("f", "yama:Float", {});
	auto b0 = pullValConst<YmBool>("f", true).value();
	auto f0 = pullValConst<YmFloat>("f", 0.05).value();
	auto f1 = pullValConst<YmFloat>("f", 3.14159).value();

	bcode
		.addPutConst(b0, bPush)
		.addJumpFalse(0)
		.addPutConst(f0, bPush)
		.addRet()
		.addLabel(0)
		.addPutConst(f1, bPush)
		.addRet();

	passes("p:f");
}

TEST_F(BCodeVerif, JumpFalse_sBx_JumpsOutOfBounds) {
	addFn("f", "yama:Float", {});
	auto b0 = pullValConst<YmBool>("f", true).value();
	auto f0 = pullValConst<YmFloat>("f", 0.05).value();

	// Manually create a code object, then overwrite branch w/
	// new invalid sBx.
	_ym::BCodeWriter w{};
	auto code = w
		.addLabel(0)
		.addPutConst(b0, bPush)
		.addJumpFalse(0)
		.addPutConst(f0, bPush)
		.addRet()
		.done().value();
	code[1].sBx = 100;

	fails("p:f", code);
}

TEST_F(BCodeVerif, JumpFalse_CoherenceViolation) {
	addFn("f", "yama:Int", {});
	auto b0 = pullValConst<YmBool>("f", true).value();
	auto f0 = pullValConst<YmFloat>("f", 0.05).value();
	auto i0 = pullValConst<YmInt>("f", 3).value();

	bcode
		// Block #1
		.addPutConst(b0, bPush) // L(0) is Bool.
		.addJumpFalse(0) // Branch to #2 or #3.
		// Block #2
		.addPutConst(f0, bPush) // L(0) is Float.
		// Block #3
		.addLabel(0)
		// Below is ambiguous as to whether it's affecting
		// L(0) or L(1) due to coherence violation.
		.addPutConst(i0, bPush)
		.addRet();

	fails("p:f");
}

