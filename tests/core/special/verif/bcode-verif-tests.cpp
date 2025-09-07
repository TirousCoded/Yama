

#include <gtest/gtest.h>

#include <optional>

#include <yama/core/general.h>
#include <yama/core/res.h>
#include <yama/core/module.h>
#include <yama/core/verifier.h>


using namespace yama::string_literals;


// IMPORTANT:
//      Our policy is gonna be to only testing a single error per unit test, not testing
//      multiple different errors arising in one unit test.
//
//      The reason is so that the impl is free to forgo work checking for other errors
//      after one is found, nor any complexity about certain errors having to be grouped
//      together w/ others.
//
//      One error may correspond to multiple dsignal raises.


class BCodeVerifTests : public testing::Test {
public:
    std::shared_ptr<yama::dsignal_debug> dbg;
    yama::bc::code bcode;
    yama::const_table consts;
    void should_pass(const yama::callsig& callsig, size_t max_locals);
    void should_fail(const yama::callsig& callsig, size_t max_locals);
protected:
    void SetUp() override final {
        dbg = std::make_shared<yama::dsignal_debug>(std::make_shared<yama::stderr_debug>());
    }

    void TearDown() override final {
        //
    }
private:
    bool _test(const yama::callsig& callsig, size_t max_locals);
};

void BCodeVerifTests::should_pass(const yama::callsig& callsig, size_t max_locals) {
    EXPECT_TRUE(_test(callsig, max_locals));
}
void BCodeVerifTests::should_fail(const yama::callsig& callsig, size_t max_locals) {
    EXPECT_FALSE(_test(callsig, max_locals));
}
bool BCodeVerifTests::_test(const yama::callsig& callsig, size_t max_locals) {
    // Reset dbg dsignal counts so they just reflect outcome of this round.
    dbg->reset();
    yama::println("{}", bcode.fmt_disassembly());
    yama::module m{};
    EXPECT_TRUE(m.add_function("f"_str, consts, callsig, max_locals, yama::bcode_call_fn));
    EXPECT_TRUE(m.bind_bcode("f"_str, bcode));
    const bool success = yama::verifier(dbg).verify(
        m,
        // Map names 'yama' and 'abc' to arbitrary parcel IDs.
        yama::parcel_metadata{ "self"_str, { "yama"_str, "abc"_str } },
        "abc"_str);
    // Cleanup for next should_[pass|fail] call.
    bcode = yama::bc::code{};
    consts = yama::const_table{};
    return success;
}


TEST_F(BCodeVerifTests, TolerateDeadCode) {
    bcode
        // block #1
        .add_noop()
        .add_noop()
        .add_jump(3)
        // block #2 (dead code)
        .add_noop()
        .add_noop()
        .add_noop()
        // block #3
        .add_noop()
        .add_put_none(yama::newtop)
        .add_ret(0);
    consts
        .add_primitive_type("yama:None"_str);
    
    should_pass(yama::make_callsig({}, 0), 1);
}

TEST_F(BCodeVerifTests, TolerateAllControlPathsBeingCyclicalAndThusNoExitpoints) {
    bcode
        // block #1
        .add_noop()
        .add_noop()
        .add_noop()
        .add_jump(-4);
    consts
        .add_primitive_type("yama:None"_str);
    
    should_pass(yama::make_callsig({}, 0), 1);
}

TEST_F(BCodeVerifTests, RegisterReinitsOverwriteOneAnother) {
    bcode
        // block #1
        .add_put_none(yama::newtop)
        .add_put_const(0, 1, true) // reinits R(0) (to Int -4)
        .add_put_const(0, 2, true) // reinits R(0) (to Char 'y')
        .add_put_const(0, 3, true) // reinits R(0) (to Float 0.05)
        .add_ret(0);
    consts
        .add_primitive_type("yama:Float"_str)
        .add_int(-4)
        .add_char(U'y')
        .add_float(0.05);
    
    should_pass(yama::make_callsig({}, 0), 1);
}

// IMPORTANT: These [No]Branch[_#] tests must be sure to also test that register
//            coherence allows for the reasonable changing of register types, and
//            that this extends to merging control paths.

TEST_F(BCodeVerifTests, NoBranch) {
    bcode
        // block #1
        .add_put_none(yama::newtop) // inits R(0) (to None)
        .add_noop()
        .add_put_const(yama::newtop, 1) // inits R(1) (to Float 3.14159)
        .add_noop()
        .add_ret(1);
    consts
        .add_primitive_type("yama:Float"_str)
        .add_float(3.14159);
    
    should_pass(yama::make_callsig({}, 0), 2);
}

TEST_F(BCodeVerifTests, Branch_MultipleExitpoints) {
    bcode
        // block #1
        .add_put_const(yama::newtop, 3) // inits R(0) (to Bool true)
        .add_jump_true(1, 2)
        // block #2 (exitpoint)
        .add_put_const(yama::newtop, 1) // inits R(0) (to Float 3.14159)
        .add_ret(0)
        // block #3 (exitpoint)
        .add_put_const(yama::newtop, 2) // inits R(0) (to Float 0.05)
        .add_ret(0);
    consts
        .add_primitive_type("yama:Float"_str)
        .add_float(3.14159)
        .add_float(0.05)
        .add_bool(true);
    
    should_pass(yama::make_callsig({}, 0), 1);
}

TEST_F(BCodeVerifTests, Branch_ControlPathsMerge) {
    bcode
        // block #1
        .add_put_const(yama::newtop, 3) // inits R(0) (to Bool true)
        .add_jump_true(1, 2)
        // block #2
        .add_put_const(yama::newtop, 1) // inits R(0) (to Float 3.14159)
        .add_jump(1) // merges into block #4
        // block #3
        .add_put_const(yama::newtop, 2) // inits R(0) (to Float 0.05)
        // fallthrough merges into block #4
        // block #4
        .add_ret(0); // R(0) is merge of the two above Float reinits
    consts
        .add_primitive_type("yama:Float"_str)
        .add_float(3.14159)
        .add_float(0.05)
        .add_bool(true);
    
    should_pass(yama::make_callsig({}, 0), 2);
}

TEST_F(BCodeVerifTests, Branch_CyclicalControlPath) {
    bcode
        // block #1
        .add_put_const(yama::newtop, 1) // inits R(0) (to Int 10)
        // block #2
        .add_put_const(0, 2) // no reinits R(0), so R(0) MUST be an Int already
        .add_put_const(0, 3, true) // reinits R(0) (to Float 0.05)
        .add_put_const(yama::newtop, 4) // inits R(1) (to Bool true)
        .add_jump_true(1, 2) // jump to block #4, or fallthrough to block #3
        // block #3
        .add_put_const(0, 5, true) // reinits R(0) (to Int 100)
        .add_jump(-6) // jump to block #2 (forming control path cycle)
        // block #4
        .add_ret(0); // R(0) must be a Float by this point
    consts
        .add_primitive_type("yama:Float"_str)
        .add_int(10)
        .add_int(-4)
        .add_float(0.05)
        .add_bool(true)
        .add_int(100);
    
    should_pass(yama::make_callsig({}, 0), 2);
}

TEST_F(BCodeVerifTests, Branch_FallthroughDueToLastInstrOfBlockNotBeingBranchOrExitpoint) {
    // due to basic blocks being partitioned at each branch destination, it's possible for a basic block
    // to end w/ an instruction that is not a branch/exitpoint instruction

    // in this scenario, the expected behaviour is to fallthrough to the next block, however I encountered
    // an issue in the initial impl where our system didn't realize that it was supposed to do this, and
    // quietly just *didn't*, which left me worried that this is something our tests didn't cover

    // to this end, this tests that symbolic execution proceeds across these fallthroughs, doing so by checking
    // for an error in a later block that likely wouldn't be detected if this fallthrough never occurs

    bcode
        // block #1
        .add_noop()
        // fallthrough to block #2
        // block #2
        .add_noop() // jump destination
        .add_put_arg(yama::newtop, 100, true) // <- error!
        .add_jump(-3);
    consts
        .add_primitive_type("yama:None"_str);
    
    should_fail(yama::make_callsig({}, 0), 1);
}

TEST_F(BCodeVerifTests, Fail_BinaryIsEmpty) {
    bcode;
    consts
        .add_primitive_type("yama:Float"_str);
    
    should_fail(yama::make_callsig({}, 0), 1);

    EXPECT_EQ(dbg->count(yama::dsignal::verif_binary_is_empty), 1);
}

TEST_F(BCodeVerifTests, Fail_FinalBlockFallthroughToOutOfBoundsInstrs) {
    bcode
        // block #1
        .add_put_const(yama::newtop, 1) // inits R(0) (to Bool true)
        // block #2
        .add_noop() // jump destination (so R(0) is always Bool)
        .add_noop()
        // we're using jump_if to test more nuanced scenario than just a simple fallthrough due to no branch or exitpoint
        .add_jump_true(1, -3); // illegal fallthrough to out-of-bounds instruction
    consts
        .add_primitive_type("yama:Bool"_str)
        .add_bool(true);
    
    should_fail(yama::make_callsig({}, 0), 1);

    EXPECT_EQ(dbg->count(yama::dsignal::verif_fallthrough_puts_PC_out_of_bounds), 1);
}

static_assert(yama::bc::opcodes == 15);

// TODO: is there a reason to have both #_RA_IsNewtopButPushingOverflows and
//       #_PushingMustNotOverflow tests? or is this unneeded duplication?

TEST_F(BCodeVerifTests, Noop) {
    bcode
        // block #1
        .add_noop()
        .add_noop()
        .add_noop()
        .add_put_none(yama::newtop)
        .add_ret(0);
    consts
        .add_primitive_type("yama:None"_str);
    
    should_pass(yama::make_callsig({}, 0), 1);
}

TEST_F(BCodeVerifTests, Pop) {
    bcode
        // block #1
        .add_put_none(yama::newtop) // inits R(0) (to None)
        .add_pop(1) // pops R(0)
        .add_put_const(yama::newtop, 1) // inits R(0) (to Int 31)
        .add_ret(0);
    consts
        .add_primitive_type("yama:Int"_str)
        .add_int(31);
    
    should_pass(yama::make_callsig({}, 0), 1);
}

TEST_F(BCodeVerifTests, Pop_Zero) {
    bcode
        // block #1
        .add_put_const(yama::newtop, 1) // inits R(0) (to Int 31)
        .add_pop(0) // pops *nothing*
        .add_ret(0);
    consts
        .add_primitive_type("yama:Int"_str)
        .add_int(31);
    
    should_pass(yama::make_callsig({}, 0), 1);
}

TEST_F(BCodeVerifTests, Pop_Many) {
    bcode
        // block #1
        .add_put_none(yama::newtop) // inits R(0) (to None)
        .add_put_none(yama::newtop) // inits R(1) (to None)
        .add_put_none(yama::newtop) // inits R(2) (to None)
        .add_put_none(yama::newtop) // inits R(3) (to None)
        .add_put_none(yama::newtop) // inits R(4) (to None)
        .add_pop(5) // pops R(0), R(1), ..., R(4)
        .add_put_const(yama::newtop, 1) // inits R(0) (to Int 31)
        .add_ret(0);
    consts
        .add_primitive_type("yama:Int"_str)
        .add_int(31);
    
    should_pass(yama::make_callsig({}, 0), 5);
}

TEST_F(BCodeVerifTests, Pop_MoreThanAreOnStack) {
    bcode
        // block #1
        .add_put_none(yama::newtop) // inits R(0) (to None)
        .add_pop(100) // pops R(0) (and more if there were any)
        .add_put_const(yama::newtop, 1) // inits R(0) (to Int 31)
        .add_ret(0);
    consts
        .add_primitive_type("yama:Int"_str)
        .add_int(31);
    
    should_pass(yama::make_callsig({}, 0), 1);
}

TEST_F(BCodeVerifTests, PutNone) {
    bcode
        // block #1
        .add_put_const(yama::newtop, 1) // helper
        .add_call(1, yama::newtop) // call helper, putting None into R(0)
        .add_put_none(0) // no reinit, so R(0) MUST be None already
        .add_ret(0);
    consts
        .add_primitive_type("yama:None"_str)
        // quick-n'-dirty fn to let us push None w/out put_none
        .add_function_type("abc:helper"_str, yama::make_callsig({}, 0));
    
    should_pass(yama::make_callsig({}, 0), 1);
}

TEST_F(BCodeVerifTests, PutNone_Reinit) {
    bcode
        // block #1
        .add_put_const(yama::newtop, 1) // inits R(0) (to Int 10)
        .add_put_none(0, true) // reinits R(0) (to None) via index
        .add_ret(0);
    consts
        .add_primitive_type("yama:None"_str)
        .add_int(10);
    
    should_pass(yama::make_callsig({}, 0), 1);
}

TEST_F(BCodeVerifTests, PutNone_Newtop) {
    bcode
        // block #1
        .add_put_none(yama::newtop) // inits R(0) (to None) via newtop
        .add_ret(0);
    consts
        .add_primitive_type("yama:None"_str)
        .add_int(10);
    
    should_pass(yama::make_callsig({}, 0), 1);
}

TEST_F(BCodeVerifTests, PutNone_Newtop_MayBeMarkedReinit) {
    bcode
        // block #1
        .add_put_none(yama::newtop, true) // inits R(0) (to None) via newtop
        .add_ret(0);
    consts
        .add_primitive_type("yama:None"_str)
        .add_int(10);
    
    should_pass(yama::make_callsig({}, 0), 1);
}

TEST_F(BCodeVerifTests, PutNone_Fail_RA_OutOfBounds) {
    bcode
        // block #1
        .add_put_none(yama::newtop) // inits R(0) (to None)
        // push then pop R(1) to better test impl robustness
        .add_put_none(yama::newtop) // inits R(1) (to None)
        .add_pop(1) // pops R(1)
        .add_put_none(1) // R(1) out-of-bounds
        .add_ret(0);
    consts
        .add_primitive_type("yama:None"_str);
    
    should_fail(yama::make_callsig({}, 0), 2);

    EXPECT_EQ(dbg->count(yama::dsignal::verif_RA_out_of_bounds), 1);
}

TEST_F(BCodeVerifTests, PutNone_Fail_RA_IsNewtopButPushingOverflows) {
    bcode
        // block #1
        .add_put_none(yama::newtop) // inits R(0) (to None)
        .add_put_none(yama::newtop) // overflow!
        .add_ret(0);
    consts
        .add_primitive_type("yama:None"_str);
    
    should_fail(yama::make_callsig({}, 0), 1);

    EXPECT_EQ(dbg->count(yama::dsignal::verif_pushing_overflows), 1);
}

TEST_F(BCodeVerifTests, PutNone_Fail_RA_WrongType_AndNotReinit) {
    bcode
        // block #1
        .add_put_const(yama::newtop, 1) // inits R(0) (to Int 10)
        .add_put_none(0) // R(0) not type None
        .add_ret(0);
    consts
        .add_primitive_type("yama:None"_str)
        .add_int(10);
    
    should_fail(yama::make_callsig({}, 0), 1);

    EXPECT_EQ(dbg->count(yama::dsignal::verif_RA_wrong_type), 1);
}

TEST_F(BCodeVerifTests, PutNone_Fail_PushingMustNotOverflow) {
    bcode
        // block #1
        .add_put_none(yama::newtop) // R(0) not type None
        .add_put_none(yama::newtop) // overflow!
        .add_ret(0);
    consts
        .add_primitive_type("yama:None"_str)
        .add_int(10);
    
    should_fail(yama::make_callsig({}, 0), 1);

    EXPECT_EQ(dbg->count(yama::dsignal::verif_pushing_overflows), 1);
}

TEST_F(BCodeVerifTests, PutConst) {
    bcode
        // block #1
        .add_put_const(yama::newtop, 1) // inits R(0) (to Int 10)
        .add_put_const(0, 2) // no reinit, so R(0) MUST be Int already
        .add_ret(0);
    consts
        .add_primitive_type("yama:Int"_str)
        .add_int(10)
        .add_int(-4);
    
    should_pass(yama::make_callsig({}, 0), 1);
}

TEST_F(BCodeVerifTests, PutConst_Reinit) {
    bcode
        // block #1
        .add_put_none(yama::newtop) // inits R(0) (to None)
        .add_put_const(0, 1, true) // reinits R(0) (to Int 10) via index
        .add_ret(0);
    consts
        .add_primitive_type("yama:Int"_str)
        .add_int(10);
    
    should_pass(yama::make_callsig({}, 0), 1);
}

TEST_F(BCodeVerifTests, PutConst_Newtop) {
    bcode
        // block #1
        .add_put_const(yama::newtop, 1) // inits R(0) (to Int 10) via newtop
        .add_ret(0);
    consts
        .add_primitive_type("yama:Int"_str)
        .add_int(10);
    
    should_pass(yama::make_callsig({}, 0), 1);
}

TEST_F(BCodeVerifTests, PutConst_Newtop_MayBeMarkedReinit) {
    bcode
        // block #1
        .add_put_const(yama::newtop, 1, true) // inits R(0) (to Int 10) via newtop
        .add_ret(0);
    consts
        .add_primitive_type("yama:Int"_str)
        .add_int(10);
    
    should_pass(yama::make_callsig({}, 0), 1);
}

TEST_F(BCodeVerifTests, PutConst_Fail_RA_OutOfBounds) {
    bcode
        // block #1
        .add_put_const(yama::newtop, 1) // inits R(0) (to Int 10) <- return value
        // push then pop R(1) to better test impl robustness
        .add_put_none(yama::newtop) // inits R(1) (to None)
        .add_pop(1) // pops R(1)
        .add_put_const(1, 1, true) // R(1) out-of-bounds
        .add_ret(0);
    consts
        .add_primitive_type("yama:None"_str)
        .add_int(10);
    
    should_fail(yama::make_callsig({}, 0), 2);

    EXPECT_EQ(dbg->count(yama::dsignal::verif_RA_out_of_bounds), 1);
}

TEST_F(BCodeVerifTests, PutConst_Fail_RA_IsNewtopButPushingOverflows) {
    bcode
        // block #1
        .add_put_const(yama::newtop, 1) // inits R(0) (to Int 10) <- return value
        .add_put_const(yama::newtop, 1) // overflow!
        .add_ret(0);
    consts
        .add_primitive_type("yama:Int"_str)
        .add_int(10);
    
    should_fail(yama::make_callsig({}, 0), 1);

    EXPECT_EQ(dbg->count(yama::dsignal::verif_pushing_overflows), 1);
}

TEST_F(BCodeVerifTests, PutConst_Fail_KoB_OutOfBounds) {
    bcode
        // block #1
        .add_put_const(yama::newtop, 1) // inits R(0) (to Int 10)
        .add_put_const(0, 2) // Ko(2) out-of-bounds
        .add_ret(0);
    consts
        .add_primitive_type("yama:Int"_str)
        .add_int(10);
    
    should_fail(yama::make_callsig({}, 0), 1);

    EXPECT_EQ(dbg->count(yama::dsignal::verif_KoB_out_of_bounds), 1);
}

TEST_F(BCodeVerifTests, PutConst_Fail_KoB_NotAnObjectConst) {
    bcode
        // block #1
        .add_put_const(yama::newtop, 1) // inits R(0) (to Int 10)
        .add_put_const(0, 0) // Ko(0) not an object constant
        .add_ret(0);
    consts
        .add_primitive_type("yama:Int"_str)
        .add_int(10);
    
    should_fail(yama::make_callsig({}, 0), 1);

    EXPECT_EQ(dbg->count(yama::dsignal::verif_KoB_not_object_const), 1);
}

TEST_F(BCodeVerifTests, PutConst_Fail_RA_And_KoB_TypesDiffer) {
    bcode
        // block #1
        .add_put_const(yama::newtop, 1) // inits R(0) (to Int 10)
        .add_put_const(0, 2) // Ko(2) not type Int
        .add_ret(0);
    consts
        .add_primitive_type("yama:Int"_str)
        .add_int(10)
        .add_char(U'y');
    
    should_fail(yama::make_callsig({}, 0), 1);

    EXPECT_EQ(dbg->count(yama::dsignal::verif_RA_and_KoB_types_differ), 1);
}

TEST_F(BCodeVerifTests, PutConst_Fail_PushingMustNotOverflow) {
    bcode
        // block #1
        .add_put_const(yama::newtop, 1) // inits R(0) (to Int 10)
        .add_put_const(yama::newtop, 1) // overflow!
        .add_ret(0);
    consts
        .add_primitive_type("yama:Int"_str)
        .add_int(10)
        .add_char(U'y');
    
    should_fail(yama::make_callsig({}, 0), 1);

    EXPECT_EQ(dbg->count(yama::dsignal::verif_pushing_overflows), 1);
}

TEST_F(BCodeVerifTests, PutTypeConst) {
    bcode
        // block #1
        .add_put_type_const(yama::newtop, 1) // inits R(0) (to Type yama:Bool)
        .add_put_type_const(0, 2) // no reinit, so R(0) MUST be Type already
        .add_ret(0);
    consts
        .add_primitive_type("yama:Type"_str)
        .add_primitive_type("yama:Bool"_str)
        .add_primitive_type("yama:Char"_str);
    
    should_pass(yama::make_callsig({}, 0), 1);
}

TEST_F(BCodeVerifTests, PutTypeConst_Reinit) {
    bcode
        // block #1
        .add_put_none(yama::newtop) // inits R(0) (to None)
        .add_put_type_const(0, 1, true) // reinits R(0) (to Type yama:Bool) via index
        .add_ret(0);
    consts
        .add_primitive_type("yama:Type"_str)
        .add_primitive_type("yama:Bool"_str);
    
    should_pass(yama::make_callsig({}, 0), 1);
}

TEST_F(BCodeVerifTests, PutTypeConst_Newtop) {
    bcode
        // block #1
        .add_put_type_const(yama::newtop, 1) // inits R(0) (to Type yama:Bool) via newtop
        .add_ret(0);
    consts
        .add_primitive_type("yama:Type"_str)
        .add_primitive_type("yama:Bool"_str);
    
    should_pass(yama::make_callsig({}, 0), 1);
}

TEST_F(BCodeVerifTests, PutTypeConst_Newtop_MayBeMarkedReinit) {
    bcode
        // block #1
        .add_put_type_const(yama::newtop, 1, true) // inits R(0) (to Type yama:Bool) via newtop
        .add_ret(0);
    consts
        .add_primitive_type("yama:Type"_str)
        .add_primitive_type("yama:Bool"_str);
    
    should_pass(yama::make_callsig({}, 0), 1);
}

TEST_F(BCodeVerifTests, PutTypeConst_Fail_RA_OutOfBounds) {
    bcode
        // block #1
        .add_put_const(yama::newtop, 1) // inits R(0) (to Int 10) <- return value
        // push then pop R(1) to better test impl robustness
        .add_put_none(yama::newtop) // inits R(1) (to None)
        .add_pop(1) // pops R(1)
        .add_put_type_const(1, 2, true) // R(1) out-of-bounds
        .add_ret(0);
    consts
        .add_primitive_type("yama:None"_str)
        .add_int(10)
        .add_primitive_type("yama:Bool"_str);
    
    should_fail(yama::make_callsig({}, 0), 2);

    EXPECT_EQ(dbg->count(yama::dsignal::verif_RA_out_of_bounds), 1);
}

TEST_F(BCodeVerifTests, PutTypeConst_Fail_RA_IsNewtopButPushingOverflows) {
    bcode
        // block #1
        .add_put_const(yama::newtop, 1) // inits R(0) (to Int 10) <- return value
        .add_put_type_const(yama::newtop, 2) // overflow!
        .add_ret(0);
    consts
        .add_primitive_type("yama:Int"_str)
        .add_int(10)
        .add_primitive_type("yama:Bool"_str);
    
    should_fail(yama::make_callsig({}, 0), 1);

    EXPECT_EQ(dbg->count(yama::dsignal::verif_pushing_overflows), 1);
}

TEST_F(BCodeVerifTests, PutTypeConst_Fail_RA_WrongType_AndNotReinit) {
    bcode
        // block #1
        .add_put_const(yama::newtop, 1) // inits R(0) (to Int 10)
        .add_put_type_const(0, 2) // R(0) not type yama:Type
        .add_ret(0);
    consts
        .add_primitive_type("yama:Type"_str)
        .add_int(10)
        .add_primitive_type("yama:Bool"_str);
    
    should_fail(yama::make_callsig({}, 0), 1);

    EXPECT_EQ(dbg->count(yama::dsignal::verif_RA_wrong_type), 1);
}

TEST_F(BCodeVerifTests, PutTypeConst_Fail_KtB_OutOfBounds) {
    bcode
        // block #1
        .add_put_type_const(yama::newtop, 1) // inits R(0) (to Type yama:Bool)
        .add_put_type_const(0, 2) // Kt(2) out-of-bounds
        .add_ret(0);
    consts
        .add_primitive_type("yama:Type"_str)
        .add_primitive_type("yama:Bool"_str);
    
    should_fail(yama::make_callsig({}, 0), 1);

    EXPECT_EQ(dbg->count(yama::dsignal::verif_KtB_out_of_bounds), 1);
}

TEST_F(BCodeVerifTests, PutTypeConst_Fail_KtB_NotATypeConst) {
    bcode
        // block #1
        .add_put_type_const(yama::newtop, 0) // inits R(0) (to Type yama:Type)
        .add_put_type_const(0, 1) // Kt(1) not a type constant
        .add_ret(0);
    consts
        .add_primitive_type("yama:Type"_str)
        .add_int(10);
    
    should_fail(yama::make_callsig({}, 0), 1);

    EXPECT_EQ(dbg->count(yama::dsignal::verif_KtB_not_type_const), 1);
}

TEST_F(BCodeVerifTests, PutTypeConst_Fail_PushingMustNotOverflow) {
    bcode
        // block #1
        .add_put_type_const(yama::newtop, 1) // inits R(0) (to Type yama:Bool)
        .add_put_type_const(yama::newtop, 1) // overflow!
        .add_ret(0);
    consts
        .add_primitive_type("yama:Type"_str)
        .add_primitive_type("yama:Bool"_str);
    
    should_fail(yama::make_callsig({}, 0), 1);

    EXPECT_EQ(dbg->count(yama::dsignal::verif_pushing_overflows), 1);
}

TEST_F(BCodeVerifTests, PutArg) {
    bcode
        // block #1
        .add_put_const(yama::newtop, 2) // inits R(0) (to f object)
        .add_put_const(yama::newtop, 3) // inits R(1) (to Bool true)
        .add_put_const(yama::newtop, 4) // inits R(2) (to Float 0.05)
        .add_put_arg(0, 0) // load callobj f arg into R(0), w/out reinit
        .add_put_arg(1, 1) // load Bool arg into R(1), w/out reinit
        .add_put_arg(2, 2) // load Float arg into R(2), w/out reinit
        .add_ret(2);
    consts
        .add_primitive_type("yama:Bool"_str)
        .add_primitive_type("yama:Float"_str)
        .add_function_type("abc:f"_str, yama::make_callsig({ 0, 1 }, 1))
        .add_bool(true)
        .add_float(0.05);
    
    should_pass(yama::make_callsig({ 0, 1 }, 1), 3);
}

TEST_F(BCodeVerifTests, PutArg_Reinit) {
    bcode
        // block #1
        .add_put_none(yama::newtop) // inits R(0) (to None)
        .add_put_none(yama::newtop) // inits R(1) (to None)
        .add_put_none(yama::newtop) // inits R(2) (to None)
        .add_put_arg(0, 0, true) // load callobj f arg into R(0), w/ reinit
        .add_put_arg(1, 1, true) // load Bool arg into R(1), w/ reinit
        .add_put_arg(2, 2, true) // load Float arg into R(2), w/ reinit
        .add_put_const(0, 2) // no reinit, so R(0) MUST be f already
        .add_put_const(1, 3) // no reinit, so R(1) MUST be Bool already
        .add_put_const(2, 4) // no reinit, so R(2) MUST be Float already
        .add_ret(2);
    consts
        .add_primitive_type("yama:Bool"_str)
        .add_primitive_type("yama:Float"_str)
        .add_function_type("abc:f"_str, yama::make_callsig({ 0, 1 }, 1))
        .add_bool(true)
        .add_float(0.05);
    
    should_pass(yama::make_callsig({ 0, 1 }, 1), 3);
}

TEST_F(BCodeVerifTests, PutArg_Newtop) {
    bcode
        // block #1
        .add_put_arg(yama::newtop, 0) // load callobj f arg into R(0), w/ reinit
        .add_put_arg(yama::newtop, 1) // load Bool arg into R(1), w/ reinit
        .add_put_arg(yama::newtop, 2) // load Float arg into R(2), w/ reinit
        .add_put_const(0, 2) // no reinit, so R(0) MUST be f already
        .add_put_const(1, 3) // no reinit, so R(1) MUST be Bool already
        .add_put_const(2, 4) // no reinit, so R(2) MUST be Float already
        .add_ret(2);
    consts
        .add_primitive_type("yama:Bool"_str)
        .add_primitive_type("yama:Float"_str)
        .add_function_type("abc:f"_str, yama::make_callsig({ 0, 1 }, 1))
        .add_bool(true)
        .add_float(0.05);
    
    should_pass(yama::make_callsig({ 0, 1 }, 1), 3);
}

TEST_F(BCodeVerifTests, PutArg_Newtop_MayBeMarkedReinit) {
    bcode
        // block #1
        .add_put_arg(yama::newtop, 0, true) // load callobj f arg into R(0), w/ reinit
        .add_put_arg(yama::newtop, 1, true) // load Bool arg into R(1), w/ reinit
        .add_put_arg(yama::newtop, 2, true) // load Float arg into R(2), w/ reinit
        .add_put_const(0, 2) // no reinit, so R(0) MUST be f already
        .add_put_const(1, 3) // no reinit, so R(1) MUST be Bool already
        .add_put_const(2, 4) // no reinit, so R(2) MUST be Float already
        .add_ret(2);
    consts
        .add_primitive_type("yama:Bool"_str)
        .add_primitive_type("yama:Float"_str)
        .add_function_type("abc:f"_str, yama::make_callsig({ 0, 1 }, 1))
        .add_bool(true)
        .add_float(0.05);
    
    should_pass(yama::make_callsig({ 0, 1 }, 1), 3);
}

TEST_F(BCodeVerifTests, PutArg_Fail_RA_OutOfBounds) {
    bcode
        // block #1
        .add_put_const(yama::newtop, 2) // inits R(0) (to Float 0.05) <- return value
        // push then pop R(1) to better test impl robustness
        .add_put_none(yama::newtop) // inits R(1) (to None)
        .add_pop(1) // pops R(1)
        .add_put_arg(1, 0, true) // R(1) out-of-bounds
        .add_ret(0);
    consts
        .add_primitive_type("yama:Bool"_str)
        .add_primitive_type("yama:Float"_str)
        .add_float(0.05);
    
    should_fail(yama::make_callsig({ 0, 1 }, 1), 2);

    EXPECT_EQ(dbg->count(yama::dsignal::verif_RA_out_of_bounds), 1);
}

TEST_F(BCodeVerifTests, PutArg_Fail_RA_IsNewtopButPushingOverflows) {
    bcode
        // block #1
        .add_put_const(yama::newtop, 2) // inits R(0) (to Float 0.05) <- return value
        .add_put_arg(yama::newtop, 0) // overflow!
        .add_ret(0);
    consts
        .add_primitive_type("yama:Bool"_str)
        .add_primitive_type("yama:Float"_str)
        .add_float(0.05);
    
    should_fail(yama::make_callsig({ 0, 1 }, 1), 1);

    EXPECT_EQ(dbg->count(yama::dsignal::verif_pushing_overflows), 1);
}

TEST_F(BCodeVerifTests, PutArg_Fail_ArgB_OutOfBounds) {
    bcode
        // block #1
        .add_put_const(yama::newtop, 2) // inits R(0) (to Float 0.05)
        .add_put_arg(0, 3) // Arg(3) out-of-bounds
        .add_ret(0);
    consts
        .add_primitive_type("yama:Bool"_str)
        .add_primitive_type("yama:Float"_str)
        .add_float(0.05);
    
    should_fail(yama::make_callsig({ 0, 1 }, 1), 1);

    EXPECT_EQ(dbg->count(yama::dsignal::verif_ArgB_out_of_bounds), 1);
}

TEST_F(BCodeVerifTests, PutArg_Fail_RA_And_ArgB_TypesDiffer) {
    bcode
        // block #1
        .add_put_const(yama::newtop, 2) // inits R(0) (to Float 0.05)
        .add_put_arg(0, 1) // Arg(1) not type Float
        .add_ret(0);
    consts
        .add_primitive_type("yama:Bool"_str)
        .add_primitive_type("yama:Float"_str)
        .add_float(0.05);
    
    should_fail(yama::make_callsig({ 0, 1 }, 1), 1);

    EXPECT_EQ(dbg->count(yama::dsignal::verif_RA_and_ArgB_types_differ), 1);
}

TEST_F(BCodeVerifTests, PutArg_Fail_PushingMustNotOverflow) {
    bcode
        // block #1
        .add_put_arg(yama::newtop, 1)
        .add_put_arg(yama::newtop, 1) // overflow!
        .add_ret(0);
    consts
        .add_primitive_type("yama:Bool"_str)
        .add_primitive_type("yama:Float"_str)
        .add_float(0.05);
    
    should_fail(yama::make_callsig({ 0, 1 }, 1), 1);

    EXPECT_EQ(dbg->count(yama::dsignal::verif_pushing_overflows), 1);
}

TEST_F(BCodeVerifTests, Copy) {
    // I once had an issue where copy instr verif was mistakenly treating R(A) as though it
    // had call instr semantics, so I rewrote this test to catch this error
    bcode
        // block #1
        .add_put_none(yama::newtop) // inits R(0) (to None)
        .add_put_const(yama::newtop, 2) // inits R(1) (to Float 3.14159)
        .add_put_none(yama::newtop) // inits R(2) (to None)
        .add_put_const(yama::newtop, 1) // inits R(3) (to Float 0.05)
        .add_copy(3, 1) // no reinit, so R(1) MUST be Float already
        .add_ret(1);
    consts
        .add_primitive_type("yama:Float"_str)
        .add_float(0.05)
        .add_float(3.14159);
    
    should_pass(yama::make_callsig({}, 0), 4);
}

TEST_F(BCodeVerifTests, Copy_Reinit) {
    bcode
        // block #1
        .add_put_const(yama::newtop, 1) // inits R(0) (to Float 0.05)
        .add_put_const(yama::newtop, 2) // inits R(1) (to Int 10)
        .add_copy(0, 1, true) // reinits R(1) (to Float)
        .add_ret(1);
    consts
        .add_primitive_type("yama:Float"_str)
        .add_float(0.05)
        .add_int(10);
    
    should_pass(yama::make_callsig({}, 0), 2);
}

TEST_F(BCodeVerifTests, Copy_Newtop) {
    bcode
        // block #1
        .add_put_const(yama::newtop, 1) // inits R(0) (to Float 0.05)
        .add_copy(0, yama::newtop) // inits R(1) (to Float)
        .add_ret(1);
    consts
        .add_primitive_type("yama:Float"_str)
        .add_float(0.05)
        .add_int(10);
    
    should_pass(yama::make_callsig({}, 0), 2);
}

TEST_F(BCodeVerifTests, Copy_Newtop_MayBeMarkedReinit) {
    bcode
        // block #1
        .add_put_const(yama::newtop, 1) // inits R(0) (to Float 0.05)
        .add_copy(0, yama::newtop, true) // inits R(1) (to Float)
        .add_ret(1);
    consts
        .add_primitive_type("yama:Float"_str)
        .add_float(0.05)
        .add_int(10);
    
    should_pass(yama::make_callsig({}, 0), 2);
}

TEST_F(BCodeVerifTests, Copy_Fail_RA_OutOfBounds) {
    bcode
        // block #1
        .add_put_const(yama::newtop, 1) // inits R(0) (to Float 0.05)
        .add_put_const(yama::newtop, 2) // inits R(1) (to Float 3.14159)
        // push then pop R(2) to better test impl robustness
        .add_put_none(yama::newtop) // inits R(2) (to None)
        .add_pop(1) // pops R(2)
        .add_copy(2, 1, true) // R(2) out-of-bounds
        .add_ret(1);
    consts
        .add_primitive_type("yama:Float"_str)
        .add_float(0.05)
        .add_float(3.14159);
    
    should_fail(yama::make_callsig({}, 0), 3);

    EXPECT_EQ(dbg->count(yama::dsignal::verif_RA_out_of_bounds), 1);
}

TEST_F(BCodeVerifTests, Copy_Fail_RB_OutOfBounds) {
    bcode
        // block #1
        .add_put_const(yama::newtop, 1) // inits R(0) (to Float 0.05)
        .add_put_const(yama::newtop, 2) // inits R(1) (to Float 3.14159)
        // push then pop R(2) to better test impl robustness
        .add_put_none(yama::newtop) // inits R(2) (to None)
        .add_pop(2) // pops R(2)
        .add_copy(0, 2, true) // R(2) out-of-bounds
        .add_ret(1);
    consts
        .add_primitive_type("yama:Float"_str)
        .add_float(0.05)
        .add_float(3.14159);
    
    should_fail(yama::make_callsig({}, 0), 3);

    EXPECT_EQ(dbg->count(yama::dsignal::verif_RB_out_of_bounds), 1);
}

TEST_F(BCodeVerifTests, Copy_Fail_RB_IsNewtopButPushingOverflows) {
    bcode
        // block #1
        .add_put_const(yama::newtop, 1) // inits R(0) (to Float 0.05)
        .add_copy(0, yama::newtop) // overflow!
        .add_ret(0);
    consts
        .add_primitive_type("yama:Float"_str)
        .add_float(0.05)
        .add_float(3.14159);
    
    should_fail(yama::make_callsig({}, 0), 1);

    EXPECT_EQ(dbg->count(yama::dsignal::verif_pushing_overflows), 1);
}

TEST_F(BCodeVerifTests, Copy_Fail_RA_And_RB_TypesDiffer) {
    bcode
        // block #1
        .add_put_const(yama::newtop, 1) // inits R(0) (to Int 10)
        .add_put_const(yama::newtop, 2) // inits R(1) (to Float 0.05)
        .add_copy(0, 1) // R(1) not type Int
        .add_ret(1);
    consts
        .add_primitive_type("yama:Float"_str)
        .add_int(10)
        .add_float(0.05);
    
    should_fail(yama::make_callsig({}, 0), 2);

    EXPECT_EQ(dbg->count(yama::dsignal::verif_RA_and_RB_types_differ), 1);
}

TEST_F(BCodeVerifTests, Copy_Fail_PushingMustNotOverflow) {
    bcode
        // block #1
        .add_put_const(yama::newtop, 1) // inits R(0) (to Int 10)
        .add_copy(0, yama::newtop) // overflow!
        .add_ret(1);
    consts
        .add_primitive_type("yama:Float"_str)
        .add_int(10)
        .add_float(0.05);
    
    should_fail(yama::make_callsig({}, 0), 1);

    EXPECT_EQ(dbg->count(yama::dsignal::verif_pushing_overflows), 1);
}

TEST_F(BCodeVerifTests, DefaultInit) {
    bcode
        // block #1
        .add_put_const(yama::newtop, 1) // inits R(0) (to Float 10.0)
        .add_default_init(0, 0) // no reinit, so R(0) MUST be Float already
        .add_ret(0);
    consts
        .add_primitive_type("yama:Float"_str)
        .add_float(10.0);
    
    should_pass(yama::make_callsig({}, 0), 4);
}

TEST_F(BCodeVerifTests, DefaultInit_Reinit) {
    bcode
        // block #1
        .add_put_const(yama::newtop, 1) // inits R(0) (to Int 10)
        .add_default_init(0, 0, true) // reinits R(0) (to Float 0.0)
        .add_ret(0);
    consts
        .add_primitive_type("yama:Float"_str)
        .add_int(10);
    
    should_pass(yama::make_callsig({}, 0), 4);
}

TEST_F(BCodeVerifTests, DefaultInit_Newtop) {
    bcode
        // block #1
        .add_default_init(yama::newtop, 0) // inits R(0) (to Float 0.0)
        .add_ret(0);
    consts
        .add_primitive_type("yama:Float"_str);
    
    should_pass(yama::make_callsig({}, 0), 4);
}

TEST_F(BCodeVerifTests, DefaultInit_Newtop_MayBeMarkedReinit) {
    bcode
        // block #1
        .add_default_init(yama::newtop, 0, true) // inits R(0) (to Float 0.0)
        .add_ret(0);
    consts
        .add_primitive_type("yama:Float"_str);
    
    should_pass(yama::make_callsig({}, 0), 4);
}

TEST_F(BCodeVerifTests, DefaultInit_Fail_RA_OutOfBounds) {
    bcode
        // block #1
        .add_put_const(yama::newtop, 1) // inits R(0) (to Float 0.05)
        // push then pop R(1) to better test impl robustness
        .add_put_none(yama::newtop) // inits R(1) (to None)
        .add_pop(1) // pops R(1)
        .add_default_init(1, 0, true) // R(1) out-of-bounds
        .add_ret(1);
    consts
        .add_primitive_type("yama:Float"_str)
        .add_float(0.05);
    
    should_fail(yama::make_callsig({}, 0), 3);

    EXPECT_EQ(dbg->count(yama::dsignal::verif_RA_out_of_bounds), 1);
}

TEST_F(BCodeVerifTests, DefaultInit_Fail_KtB_OutOfBounds) {
    bcode
        // block #1
        .add_put_const(yama::newtop, 1) // inits R(0) (to Float 10.0)
        .add_default_init(0, 2) // Kt(2) out-of-bounds
        .add_ret(0);
    consts
        .add_primitive_type("yama:Float"_str)
        .add_float(10.0);
    
    should_fail(yama::make_callsig({}, 0), 1);

    EXPECT_EQ(dbg->count(yama::dsignal::verif_KtB_out_of_bounds), 1);
}

TEST_F(BCodeVerifTests, DefaultInit_Fail_KtB_NotATypeConst) {
    bcode
        // block #1
        .add_put_const(yama::newtop, 1) // inits R(0) (to Float 10.0)
        .add_default_init(0, 1) // Kt(1) not a type constant
        .add_ret(0);
    consts
        .add_primitive_type("yama:Float"_str)
        .add_float(10.0);
    
    should_fail(yama::make_callsig({}, 0), 1);

    EXPECT_EQ(dbg->count(yama::dsignal::verif_KtB_not_type_const), 1);
}

TEST_F(BCodeVerifTests, DefaultInit_Fail_RA_And_KtB_TypesDiffer) {
    bcode
        // block #1
        .add_put_const(yama::newtop, 1) // inits R(0) (to Int 10)
        .add_default_init(0, 0) // Kt(0) not type Int
        .add_ret(0);
    consts
        .add_primitive_type("yama:Float"_str)
        .add_int(10);
    
    should_fail(yama::make_callsig({}, 0), 1);

    EXPECT_EQ(dbg->count(yama::dsignal::verif_RA_and_KtB_types_differ), 1);
}

TEST_F(BCodeVerifTests, DefaultInit_Fail_PushingMustNotOverflow) {
    bcode
        // block #1
        .add_put_const(yama::newtop, 1) // inits R(0) (to Float 10.0)
        .add_default_init(yama::newtop, 0) // overflow!
        .add_ret(0);
    consts
        .add_primitive_type("yama:Float"_str)
        .add_float(10.0);
    
    should_fail(yama::make_callsig({}, 0), 1);

    EXPECT_EQ(dbg->count(yama::dsignal::verif_pushing_overflows), 1);
}

TEST_F(BCodeVerifTests, Conv) {
    bcode
        // block #1
        .add_put_const(yama::newtop, 1) // Inits R(0) (to Float 10.0).
        .add_put_const(yama::newtop, 2) // Inits R(1) (to Int 0).
        .add_conv(0, 1, 0) // No reinit, so R(1) MUST be Int already.
        .add_ret(1);
    consts
        .add_primitive_type("yama:Int"_str)
        .add_float(10.0)
        .add_int(0);

    should_pass(yama::make_callsig({}, 0), 2);
}

TEST_F(BCodeVerifTests, Conv_Reinit) {
    bcode
        // block #1
        .add_put_const(yama::newtop, 1) // Inits R(0) (to Float 10.0).
        .add_put_none(yama::newtop) // Inits R(1) (to None).
        .add_conv(0, 1, 0, true) // Reinit R(1) (to Int 10).
        .add_ret(1);
    consts
        .add_primitive_type("yama:Int"_str)
        .add_float(10.0);

    should_pass(yama::make_callsig({}, 0), 2);
}

TEST_F(BCodeVerifTests, Conv_Newtop) {
    bcode
        // block #1
        .add_put_const(yama::newtop, 1) // Inits R(0) (to Float 10.0).
        .add_conv(0, yama::newtop, 0) // Init R(1) (to Int 10).
        .add_ret(1);
    consts
        .add_primitive_type("yama:Int"_str)
        .add_float(10.0);

    should_pass(yama::make_callsig({}, 0), 2);
}

TEST_F(BCodeVerifTests, Conv_Newtop_MayBeMarkedReinit) {
    bcode
        // block #1
        .add_put_const(yama::newtop, 1) // Inits R(0) (to Float 10.0).
        .add_conv(0, yama::newtop, 0, true) // Init R(1) (to Int 10).
        .add_ret(1);
    consts
        .add_primitive_type("yama:Int"_str)
        .add_float(10.0);

    should_pass(yama::make_callsig({}, 0), 2);
}

TEST_F(BCodeVerifTests, Conv_Fail_RA_OutOfBounds) {
    bcode
        // block #1
        .add_put_none(yama::newtop) // Inits R(0) (to None).
        // Push then pop R(1) to better test impl robustness.
        .add_put_const(yama::newtop, 1) // Inits R(1) (to Float 10.0).
        .add_pop(1) // pops R(1)
        .add_conv(1, 0, 0, true) // R(1) out-of-bounds.
        .add_ret(1);
    consts
        .add_primitive_type("yama:Int"_str)
        .add_float(10.0);

    should_fail(yama::make_callsig({}, 0), 2);

    EXPECT_EQ(dbg->count(yama::dsignal::verif_RA_out_of_bounds), 1);
}

TEST_F(BCodeVerifTests, Conv_Fail_RB_OutOfBounds) {
    bcode
        // block #1
        .add_put_const(yama::newtop, 1) // Inits R(0) (to Float 10.0).
        // Push then pop R(1) to better test impl robustness.
        .add_put_none(yama::newtop) // Inits R(1) (to None).
        .add_pop(1) // pops R(1)
        .add_conv(0, 1, 0, true) // R(1) out-of-bounds.
        .add_ret(1);
    consts
        .add_primitive_type("yama:Int"_str)
        .add_float(10.0);

    should_fail(yama::make_callsig({}, 0), 2);

    EXPECT_EQ(dbg->count(yama::dsignal::verif_RB_out_of_bounds), 1);
}

TEST_F(BCodeVerifTests, Conv_Fail_KtC_OutOfBounds) {
    bcode
        // block #1
        .add_put_const(yama::newtop, 1) // Inits R(0) (to Float 10.0).
        .add_conv(0, yama::newtop, 2, true) // Kt(2) out-of-bounds.
        .add_ret(1);
    consts
        .add_primitive_type("yama:Int"_str)
        .add_float(10.0);

    should_fail(yama::make_callsig({}, 0), 2);

    EXPECT_EQ(dbg->count(yama::dsignal::verif_KtC_out_of_bounds), 1);
}

TEST_F(BCodeVerifTests, Conv_Fail_KtC_NotATypeConst) {
    bcode
        // block #1
        .add_put_const(yama::newtop, 1) // Inits R(0) (to Float 10.0).
        .add_conv(0, yama::newtop, 1, true) // Kt(1) not a type constant.
        .add_ret(1);
    consts
        .add_primitive_type("yama:Int"_str)
        .add_float(10.0);

    should_fail(yama::make_callsig({}, 0), 2);

    EXPECT_EQ(dbg->count(yama::dsignal::verif_KtC_not_type_const), 1);
}

TEST_F(BCodeVerifTests, Conv_Fail_RB_And_KtC_TypesDiffer) {
    bcode
        // block #1
        .add_put_const(yama::newtop, 1) // Inits R(0) (to Float 10.0).
        .add_put_const(yama::newtop, 2) // Inits R(1) (to Bool false).
        .add_conv(0, 1, 0) // Kt(0) not type Int.
        .add_ret(0);
    consts
        .add_primitive_type("yama:Int"_str)
        .add_float(10.0)
        .add_bool(false);

    should_fail(yama::make_callsig({}, 0), 2);

    EXPECT_EQ(dbg->count(yama::dsignal::verif_RB_and_KtC_types_differ), 1);
}

TEST_F(BCodeVerifTests, Conv_Fail_PushingMustNotOverflow) {
    bcode
        // block #1
        .add_put_const(yama::newtop, 1) // Inits R(0) (to Float 10.0).
        .add_conv(0, yama::newtop, 0) // Overflow!
        .add_ret(0);
    consts
        .add_primitive_type("yama:Int"_str)
        .add_float(10.0);

    should_fail(yama::make_callsig({}, 0), 1);

    EXPECT_EQ(dbg->count(yama::dsignal::verif_pushing_overflows), 1);
}

static_assert(yama::kinds == 4); // one BCode_Call_# test for each callable kind

TEST_F(BCodeVerifTests, Call_Function) {
    bcode
        // block #1
        .add_put_const(yama::newtop, 7) // inits R(0) to type of return value
        .add_put_const(yama::newtop, 3) // inits R(1) to type of call object
        .add_put_const(yama::newtop, 4) // inits R(2) to type of arg #1
        .add_put_const(yama::newtop, 5) // inits R(3) to type of arg #2
        .add_put_const(yama::newtop, 6) // inits R(4) to type of arg #3
        .add_call(4, 0) // no reinit, so R(0) MUST be Char already
        .add_ret(0);
    consts
        .add_primitive_type("yama:Int"_str)
        .add_primitive_type("yama:Float"_str)
        .add_primitive_type("yama:Char"_str)
        .add_function_type("abc:g"_str, yama::make_callsig({ 0, 1, 0 }, 2)) // fn(Int, Float, Int) -> Char
        .add_int(10)
        .add_float(0.05)
        .add_int(-4)
        .add_char(U'y');
    
    should_pass(yama::make_callsig({}, 2), 5);
}

TEST_F(BCodeVerifTests, Call_Method) {
    bcode
        // block #1
        .add_put_const(yama::newtop, 7) // inits R(0) to type of return value
        .add_put_const(yama::newtop, 3) // inits R(1) to type of call object
        .add_put_const(yama::newtop, 4) // inits R(2) to type of arg #1
        .add_put_const(yama::newtop, 5) // inits R(3) to type of arg #2
        .add_put_const(yama::newtop, 6) // inits R(4) to type of arg #3
        .add_call(4, 0) // no reinit, so R(0) MUST be Char already
        .add_ret(0);
    consts
        .add_primitive_type("yama:Int"_str)
        .add_primitive_type("yama:Float"_str)
        .add_primitive_type("yama:Char"_str)
        .add_method_type("abc:Something::g"_str, yama::make_callsig({ 0, 1, 0 }, 2)) // fn(Int, Float, Int) -> Char
        .add_int(10)
        .add_float(0.05)
        .add_int(-4)
        .add_char(U'y');
    
    should_pass(yama::make_callsig({}, 2), 5);
}

TEST_F(BCodeVerifTests, Call_NoReinit) {
    bcode
        // block #1
        .add_put_const(yama::newtop, 7) // inits R(0) to type of return value
        .add_put_const(yama::newtop, 3) // inits R(1) to type of call object
        .add_put_const(yama::newtop, 4) // inits R(2) to type of arg #1
        .add_put_const(yama::newtop, 5) // inits R(3) to type of arg #2
        .add_put_const(yama::newtop, 6) // inits R(4) to type of arg #3
        .add_call(4, 0) // no reinit, so R(0) MUST be Char already
        .add_ret(0);
    consts
        .add_primitive_type("yama:Int"_str)
        .add_primitive_type("yama:Float"_str)
        .add_primitive_type("yama:Char"_str)
        .add_function_type("abc:g"_str, yama::make_callsig({ 0, 1, 0 }, 2)) // fn(Int, Float, Int) -> Char
        .add_int(10)
        .add_float(0.05)
        .add_int(-4)
        .add_char(U'y');
    
    should_pass(yama::make_callsig({}, 2), 5);
}

TEST_F(BCodeVerifTests, Call_Reinit) {
    bcode
        // block #1
        .add_put_none(yama::newtop) // inits R(0) (to None)
        .add_put_const(yama::newtop, 3) // inits R(1) to type of call object
        .add_put_const(yama::newtop, 4) // inits R(2) to type of arg #1
        .add_put_const(yama::newtop, 5) // inits R(3) to type of arg #2
        .add_put_const(yama::newtop, 6) // inits R(4) to type of arg #3
        .add_call(4, 0, true) // reinits R(0) to return type Char
        .add_ret(0);
    consts
        .add_primitive_type("yama:Int"_str)
        .add_primitive_type("yama:Float"_str)
        .add_primitive_type("yama:Char"_str)
        .add_function_type("abc:g"_str, yama::make_callsig({ 0, 1, 0 }, 2)) // fn(Int, Float, Int) -> Char
        .add_int(10)
        .add_float(0.05)
        .add_int(-4)
        .add_char(U'y');
    
    should_pass(yama::make_callsig({}, 2), 5);
}

TEST_F(BCodeVerifTests, Call_Newtop) {
    bcode
        // block #1
        .add_put_const(yama::newtop, 3) // inits R(0) to type of call object
        .add_put_const(yama::newtop, 4) // inits R(1) to type of arg #1
        .add_put_const(yama::newtop, 5) // inits R(2) to type of arg #2
        .add_put_const(yama::newtop, 6) // inits R(3) to type of arg #3
        .add_call(4, yama::newtop) // inits R(0) to return type Char
        .add_ret(0);
    consts
        .add_primitive_type("yama:Int"_str)
        .add_primitive_type("yama:Float"_str)
        .add_primitive_type("yama:Char"_str)
        .add_function_type("abc:g"_str, yama::make_callsig({ 0, 1, 0 }, 2)) // fn(Int, Float, Int) -> Char
        .add_int(10)
        .add_float(0.05)
        .add_int(-4)
        .add_char(U'y');
    
    should_pass(yama::make_callsig({}, 2), 5);
}

TEST_F(BCodeVerifTests, Call_Newtop_MayBeMarkedReinit) {
    bcode
        // block #1
        .add_put_const(yama::newtop, 3) // inits R(0) to type of call object
        .add_put_const(yama::newtop, 4) // inits R(1) to type of arg #1
        .add_put_const(yama::newtop, 5) // inits R(2) to type of arg #2
        .add_put_const(yama::newtop, 6) // inits R(3) to type of arg #3
        .add_call(4, yama::newtop, true) // inits R(0) to return type Char
        .add_ret(0);
    consts
        .add_primitive_type("yama:Int"_str)
        .add_primitive_type("yama:Float"_str)
        .add_primitive_type("yama:Char"_str)
        .add_function_type("abc:g"_str, yama::make_callsig({ 0, 1, 0 }, 2)) // fn(Int, Float, Int) -> Char
        .add_int(10)
        .add_float(0.05)
        .add_int(-4)
        .add_char(U'y');
    
    should_pass(yama::make_callsig({}, 2), 5);
}

TEST_F(BCodeVerifTests, Call_Fail_ArgRs_OutOfBounds) {
    bcode
        // block #1
        .add_put_const(yama::newtop, 3) // inits R(0) to type of call object
        .add_put_const(yama::newtop, 4) // inits R(1) to type of arg #1
        // push then pop R(2) and R(3) to better test impl robustness
        .add_put_const(yama::newtop, 5) // inits R(2) to type of arg #2
        .add_put_const(yama::newtop, 6) // inits R(3) to type of arg #3
        .add_pop(2) // pops R(2) and R(3)
        .add_call(4, yama::newtop) // [R(0), R(3)] out-of-bounds
        .add_ret(0);
    consts
        .add_primitive_type("yama:Int"_str)
        .add_primitive_type("yama:Float"_str)
        .add_primitive_type("yama:Char"_str)
        .add_function_type("abc:g"_str, yama::make_callsig({ 0, 1, 0 }, 2)) // fn(Int, Float, Int) -> Char
        .add_int(10)
        .add_float(0.05)
        .add_int(-4)
        .add_char(U'y');
    
    should_fail(yama::make_callsig({}, 2), 4);

    EXPECT_EQ(dbg->count(yama::dsignal::verif_ArgRs_out_of_bounds), 1);
}

TEST_F(BCodeVerifTests, Call_Fail_ArgRs_ZeroObjects) {
    bcode
        // block #1
        .add_put_none(yama::newtop) // inits R(0) to type of return value
        // NOTE: R(top-A) being an otherwise valid call object ensures error couldn't be about R(top-A) not being one
        .add_put_const(yama::newtop, 1) // inits R(1) to type of call object (though this will go unused)
        .add_call(0, 0) // A == 0 means no call object specified
        .add_ret(0);
    consts
        .add_primitive_type("yama:None"_str)
        .add_function_type("abc:g"_str, yama::make_callsig({}, 0)); // fn() -> None
    
    should_fail(yama::make_callsig({}, 0), 5);

    EXPECT_EQ(dbg->count(yama::dsignal::verif_ArgRs_zero_objects), 1);
}

TEST_F(BCodeVerifTests, Call_Fail_ArgRs_IllegalCallObject) {
    bcode
        // block #1
        .add_put_const(yama::newtop, 7) // inits R(0) to type of return value
        .add_put_const(yama::newtop, 3) // inits R(1) to type of call object
        .add_put_const(yama::newtop, 4) // inits R(2) to type of arg #1
        .add_put_const(yama::newtop, 5) // inits R(3) to type of arg #2
        .add_put_const(yama::newtop, 6) // inits R(4) to type of arg #3
        .add_call(4, 0) // R(1) cannot be used as call object
        .add_ret(0);
    consts
        .add_primitive_type("yama:Int"_str)
        .add_primitive_type("yama:Float"_str)
        .add_primitive_type("yama:Char"_str)
        .add_int(50) // <- cannot be used as call object
        .add_int(10)
        .add_float(0.05)
        .add_int(-4)
        .add_char(U'y');
    
    should_fail(yama::make_callsig({}, 2), 5);

    EXPECT_EQ(dbg->count(yama::dsignal::verif_ArgRs_illegal_callobj), 1);
}

TEST_F(BCodeVerifTests, Call_Fail_ArgRs_IllegalCallObject_AttemptedCallObjectTypeIsNotInConstTable) {
    // IMPORTANT: this test exists as the verifier relies on const table for type info
    //            about things like attempted call object types, and so impl must be
    //            able to handle if this type info is missing
    // TODO: this testing need was found from a bug in our impl where it would
    //       crash if it couldn't find a const table entry for attempted call object
    //       type, w/ me feeling their may be other similar problems in our impl
    //       which our unit tests aren't catching
    bcode
        // block #1
        .add_put_const(yama::newtop, 7) // inits R(0) to type of return value
        .add_put_const(yama::newtop, 3) // inits R(1) to type of call object
        .add_put_const(yama::newtop, 4) // inits R(2) to type of arg #1
        .add_put_const(yama::newtop, 5) // inits R(3) to type of arg #2
        .add_put_const(yama::newtop, 6) // inits R(4) to type of arg #3
        .add_call(4, 0) // R(1) cannot be used as call object
        .add_ret(0);
    consts
        .add_primitive_type("yama:Type"_str) // <- no type yama:Int in this const table
        .add_primitive_type("yama:Float"_str)
        .add_primitive_type("yama:Char"_str)
        .add_int(50) // <- cannot be used as call object + yama:Int is not in const table
        .add_int(10)
        .add_float(0.05)
        .add_int(-4)
        .add_char(U'y');
    
    should_fail(yama::make_callsig({}, 2), 5);

    EXPECT_EQ(dbg->count(yama::dsignal::verif_ArgRs_illegal_callobj), 1);
}

TEST_F(BCodeVerifTests, Call_Fail_ParamArgRs_TooMany) {
    bcode
        // block #1
        .add_put_const(yama::newtop, 7) // inits R(0) to type of return value
        .add_put_const(yama::newtop, 3) // inits R(1) to type of call object
        .add_put_const(yama::newtop, 4) // inits R(2) to type of arg #1
        .add_put_const(yama::newtop, 5) // inits R(3) to type of arg #2
        .add_put_const(yama::newtop, 6) // inits R(4) to type of arg #3
        .add_put_const(yama::newtop, 6) // inits R(5) to type of arg #4
        .add_call(5, 0) // 5 is too many args for call to g
        .add_ret(0);
    consts
        .add_primitive_type("yama:Int"_str)
        .add_primitive_type("yama:Float"_str)
        .add_primitive_type("yama:Char"_str)
        .add_function_type("abc:g"_str, yama::make_callsig({ 0, 1, 0 }, 2)) // fn(Int, Float, Int) -> Char
        .add_int(10)
        .add_float(0.05)
        .add_int(-4)
        .add_char(U'y');
    
    should_fail(yama::make_callsig({}, 2), 6);

    EXPECT_EQ(dbg->count(yama::dsignal::verif_ParamArgRs_wrong_number), 1);
}

TEST_F(BCodeVerifTests, Call_Fail_ParamArgRs_TooFew) {
    bcode
        // block #1
        .add_put_const(yama::newtop, 7) // inits R(0) to type of return value
        .add_put_const(yama::newtop, 3) // inits R(1) to type of call object
        .add_put_const(yama::newtop, 4) // inits R(2) to type of arg #1
        .add_put_const(yama::newtop, 5) // inits R(3) to type of arg #2
        .add_call(3, 0) // 3 is too few args for call to g
        .add_ret(0);
    consts
        .add_primitive_type("yama:Int"_str)
        .add_primitive_type("yama:Float"_str)
        .add_primitive_type("yama:Char"_str)
        .add_function_type("abc:g"_str, yama::make_callsig({ 0, 1, 0 }, 2)) // fn(Int, Float, Int) -> Char
        .add_int(10)
        .add_float(0.05)
        .add_int(-4)
        .add_char(U'y');
    
    should_fail(yama::make_callsig({}, 2), 4);

    EXPECT_EQ(dbg->count(yama::dsignal::verif_ParamArgRs_wrong_number), 1);
}

TEST_F(BCodeVerifTests, Call_Fail_ParamArgRs_WrongTypes) {
    bcode
        // block #1
        .add_put_const(yama::newtop, 7) // inits R(0) to type of return value
        .add_put_const(yama::newtop, 3) // inits R(1) to type of call object
        .add_put_const(yama::newtop, 4) // inits R(2) to type of arg #1
        .add_put_const(yama::newtop, 5) // inits R(3) to type of arg #2
        .add_put_const(yama::newtop, 6) // inits R(4) to type of arg #3
        .add_call(4, 0) // arg #2 is UInt, but a Float was expected
        .add_ret(0);
    consts
        .add_primitive_type("yama:Int"_str)
        .add_primitive_type("yama:Float"_str)
        .add_primitive_type("yama:Char"_str)
        .add_function_type("abc:g"_str, yama::make_callsig({ 0, 1, 0 }, 2)) // fn(Int, Float, Int) -> Char
        .add_int(10)
        .add_uint(1000) // <- wrong type for arg #2 for call to g
        .add_int(-4)
        .add_char(U'y');
    
    should_fail(yama::make_callsig({}, 2), 5);

    EXPECT_EQ(dbg->count(yama::dsignal::verif_ParamArgRs_wrong_types), 1);
}

TEST_F(BCodeVerifTests, Call_Fail_RB_OutOfBounds_AfterTheCall) {
    bcode
        // block #1
        .add_put_const(yama::newtop, 3) // inits R(0) to type of call object
        .add_put_const(yama::newtop, 4) // inits R(1) to type of arg #1
        .add_put_const(yama::newtop, 5) // inits R(2) to type of arg #2
        .add_put_const(yama::newtop, 6) // inits R(3) to type of arg #3
        .add_call(4, 0, true) // R(0) out-of-bounds after the call
        .add_ret(0);
    consts
        .add_primitive_type("yama:Int"_str)
        .add_primitive_type("yama:Float"_str)
        .add_primitive_type("yama:Char"_str)
        .add_function_type("abc:g"_str, yama::make_callsig({ 0, 1, 0 }, 2)) // fn(Int, Float, Int) -> Char
        .add_int(10)
        .add_float(0.05)
        .add_int(-4)
        .add_char(U'y');
    
    should_fail(yama::make_callsig({}, 2), 6);

    EXPECT_EQ(dbg->count(yama::dsignal::verif_RB_out_of_bounds), 1);
}

TEST_F(BCodeVerifTests, Call_Fail_RB_WrongType) {
    bcode
        // block #1
        .add_put_const(yama::newtop, 7) // inits R(0) to NOT type of return value
        .add_put_const(yama::newtop, 3) // inits R(1) to type of call object
        .add_put_const(yama::newtop, 4) // inits R(2) to type of arg #1
        .add_put_const(yama::newtop, 5) // inits R(3) to type of arg #2
        .add_put_const(yama::newtop, 6) // inits R(4) to type of arg #3
        .add_call(4, 0) // R(0) not type Char
        .add_ret(0);
    consts
        .add_primitive_type("yama:Int"_str)
        .add_primitive_type("yama:Float"_str)
        .add_primitive_type("yama:Char"_str)
        .add_function_type("abc:g"_str, yama::make_callsig({ 0, 1, 0 }, 2)) // fn(Int, Float, Int) -> Char
        .add_int(10)
        .add_float(0.05)
        .add_int(-4)
        .add_uint(1000); // <- return type of g is Char, not UInt
    
    should_fail(yama::make_callsig({}, 2), 5);

    EXPECT_EQ(dbg->count(yama::dsignal::verif_RB_wrong_type), 1);
}

static_assert(yama::kinds == 4); // one BCode_CallNR_# test for each callable kind

TEST_F(BCodeVerifTests, CallNR_Function) {
    bcode
        // block #1
        .add_put_const(yama::newtop, 3) // inits R(0) to type of call object
        .add_put_const(yama::newtop, 4) // inits R(1) to type of arg #1
        .add_put_const(yama::newtop, 5) // inits R(2) to type of arg #2
        .add_put_const(yama::newtop, 6) // inits R(3) to type of arg #3
        .add_call_nr(4)
        .add_put_none(yama::newtop)
        .add_ret(0); // return the None object from R(0)
    consts
        .add_primitive_type("yama:Int"_str)
        .add_primitive_type("yama:Float"_str)
        .add_primitive_type("yama:Char"_str)
        .add_function_type("abc:g"_str, yama::make_callsig({ 0, 1, 0 }, 7)) // fn(Int, Float, Int) -> None
        .add_int(10)
        .add_float(0.05)
        .add_int(-4)
        .add_primitive_type("yama:None"_str);
    
    should_pass(yama::make_callsig({}, 7), 4);
}

TEST_F(BCodeVerifTests, CallNR_Method) {
    bcode
        // block #1
        .add_put_const(yama::newtop, 3) // inits R(0) to type of call object
        .add_put_const(yama::newtop, 4) // inits R(1) to type of arg #1
        .add_put_const(yama::newtop, 5) // inits R(2) to type of arg #2
        .add_put_const(yama::newtop, 6) // inits R(3) to type of arg #3
        .add_call_nr(4)
        .add_put_none(yama::newtop)
        .add_ret(0); // return the None object from R(0)
    consts
        .add_primitive_type("yama:Int"_str)
        .add_primitive_type("yama:Float"_str)
        .add_primitive_type("yama:Char"_str)
        .add_method_type("abc:Something::g"_str, yama::make_callsig({ 0, 1, 0 }, 7)) // fn(Int, Float, Int) -> None
        .add_int(10)
        .add_float(0.05)
        .add_int(-4)
        .add_primitive_type("yama:None"_str);
    
    should_pass(yama::make_callsig({}, 7), 4);
}

TEST_F(BCodeVerifTests, CallNR_Fail_ArgRs_OutOfBounds) {
    bcode
        // block #1
        .add_put_const(yama::newtop, 3) // inits R(0) to type of call object
        .add_put_const(yama::newtop, 4) // inits R(1) to type of arg #1
        // push then pop R(2) and R(3) to better test impl robustness
        .add_put_const(yama::newtop, 5) // inits R(2) to type of arg #2
        .add_put_const(yama::newtop, 6) // inits R(3) to type of arg #3
        .add_pop(2) // pops R(2) and R(3)
        .add_call_nr(4) // [R(0), R(3)] out-of-bounds
        .add_put_none(0, true)
        .add_ret(0);
    consts
        .add_primitive_type("yama:Int"_str)
        .add_primitive_type("yama:Float"_str)
        .add_primitive_type("yama:Char"_str)
        .add_function_type("abc:g"_str, yama::make_callsig({ 0, 1, 0 }, 2)) // fn(Int, Float, Int) -> Char
        .add_int(10)
        .add_float(0.05)
        .add_int(-4)
        .add_char(U'y');
    
    should_fail(yama::make_callsig({}, 2), 4);

    EXPECT_EQ(dbg->count(yama::dsignal::verif_ArgRs_out_of_bounds), 1);
}

TEST_F(BCodeVerifTests, CallNR_Fail_ArgRs_ZeroObjects) {
    bcode
        // block #1
        // NOTE: R(top-A) being a valid call object ensures error couldn't be about R(top-A) not being one
        .add_put_const(yama::newtop, 1) // inits R(0) to type of call object (though this will go unused)
        .add_call_nr(0) // A == 0 means no call object specified
        .add_put_none(0, true)
        .add_ret(0);
    consts
        .add_primitive_type("yama:None"_str)
        .add_function_type("abc:g"_str, yama::make_callsig({}, 0)); // fn() -> None
    
    should_fail(yama::make_callsig({}, 0), 5);

    EXPECT_EQ(dbg->count(yama::dsignal::verif_ArgRs_zero_objects), 1);
}

TEST_F(BCodeVerifTests, CallNR_Fail_ArgRs_IllegalCallObject) {
    bcode
        // block #1
        .add_put_const(yama::newtop, 3) // inits R(0) to type of call object
        .add_put_const(yama::newtop, 4) // inits R(1) to type of arg #1
        .add_put_const(yama::newtop, 5) // inits R(2) to type of arg #2
        .add_put_const(yama::newtop, 6) // inits R(3) to type of arg #3
        .add_call_nr(4) // R(0) cannot be used as call object
        .add_put_none(0, true)
        .add_ret(0);
    consts
        .add_primitive_type("yama:Int"_str)
        .add_primitive_type("yama:Float"_str)
        .add_primitive_type("yama:Char"_str)
        .add_int(50) // <- cannot be used as call object
        .add_int(10)
        .add_float(0.05)
        .add_int(-4)
        .add_char(U'y');
    
    should_fail(yama::make_callsig({}, 2), 5);

    EXPECT_EQ(dbg->count(yama::dsignal::verif_ArgRs_illegal_callobj), 1);
}

TEST_F(BCodeVerifTests, CallNR_Fail_ArgRs_IllegalCallObject_AttemptedCallObjectTypeIsNotInConstTable) {
    // IMPORTANT: this test exists as the verifier relies on const table for type info
    //            about things like attempted call object types, and so impl must be
    //            able to handle if this type info is missing
    // TODO: this testing need was found from a bug in our impl where it would
    //       crash if it couldn't find a const table entry for attempted call object
    //       type, w/ me feeling their may be other similar problems in our impl
    //       which our unit tests aren't catching
    bcode
        // block #1
        .add_put_const(yama::newtop, 3) // inits R(0) to type of call object
        .add_put_const(yama::newtop, 4) // inits R(1) to type of arg #1
        .add_put_const(yama::newtop, 5) // inits R(2) to type of arg #2
        .add_put_const(yama::newtop, 6) // inits R(3) to type of arg #3
        .add_call_nr(4) // R(0) cannot be used as call object
        .add_put_none(0, true)
        .add_ret(0);
    consts
        .add_primitive_type("yama:Type"_str) // <- no type yama:Int in this const table
        .add_primitive_type("yama:Float"_str)
        .add_primitive_type("yama:Char"_str)
        .add_int(50) // <- cannot be used as call object + yama:Int is not in const table
        .add_int(10)
        .add_float(0.05)
        .add_int(-4)
        .add_char(U'y');
    
    should_fail(yama::make_callsig({}, 2), 5);

    EXPECT_EQ(dbg->count(yama::dsignal::verif_ArgRs_illegal_callobj), 1);
}

TEST_F(BCodeVerifTests, CallNR_Fail_ParamArgRs_TooMany) {
    bcode
        // block #1
        .add_put_const(yama::newtop, 3) // inits R(0) to type of call object
        .add_put_const(yama::newtop, 4) // inits R(1) to type of arg #1
        .add_put_const(yama::newtop, 5) // inits R(2) to type of arg #2
        .add_put_const(yama::newtop, 6) // inits R(3) to type of arg #3
        .add_put_const(yama::newtop, 6) // inits R(4) to type of arg #4
        .add_call_nr(5) // 5 is too many args for call to g
        .add_put_none(0, true)
        .add_ret(0);
    consts
        .add_primitive_type("yama:Int"_str)
        .add_primitive_type("yama:Float"_str)
        .add_primitive_type("yama:Char"_str)
        .add_function_type("abc:g"_str, yama::make_callsig({ 0, 1, 0 }, 2)) // fn(Int, Float, Int) -> Char
        .add_int(10)
        .add_float(0.05)
        .add_int(-4)
        .add_char(U'y');
    
    should_fail(yama::make_callsig({}, 2), 6);

    EXPECT_EQ(dbg->count(yama::dsignal::verif_ParamArgRs_wrong_number), 1);
}

TEST_F(BCodeVerifTests, CallNR_Fail_ParamArgRs_TooFew) {
    bcode
        // block #1
        .add_put_const(yama::newtop, 3) // inits R(0) to type of call object
        .add_put_const(yama::newtop, 4) // inits R(1) to type of arg #1
        .add_put_const(yama::newtop, 5) // inits R(2) to type of arg #2
        .add_call_nr(3) // 3 is too few args for call to g
        .add_put_none(0, true)
        .add_ret(0);
    consts
        .add_primitive_type("yama:Int"_str)
        .add_primitive_type("yama:Float"_str)
        .add_primitive_type("yama:Char"_str)
        .add_function_type("abc:g"_str, yama::make_callsig({ 0, 1, 0 }, 2)) // fn(Int, Float, Int) -> Char
        .add_int(10)
        .add_float(0.05)
        .add_int(-4)
        .add_char(U'y');
    
    should_fail(yama::make_callsig({}, 2), 4);

    EXPECT_EQ(dbg->count(yama::dsignal::verif_ParamArgRs_wrong_number), 1);
}

TEST_F(BCodeVerifTests, CallNR_Fail_ParamArgRs_WrongTypes) {
    bcode
        // block #1
        .add_put_const(yama::newtop, 3) // inits R(0) to type of call object
        .add_put_const(yama::newtop, 4) // inits R(1) to type of arg #1
        .add_put_const(yama::newtop, 5) // inits R(2) to type of arg #2
        .add_put_const(yama::newtop, 6) // inits R(3) to type of arg #3
        .add_call_nr(4) // arg #2 is UInt, but a Float was expected
        .add_put_none(0, true)
        .add_ret(0);
    consts
        .add_primitive_type("yama:Int"_str)
        .add_primitive_type("yama:Float"_str)
        .add_primitive_type("yama:Char"_str)
        .add_function_type("abc:g"_str, yama::make_callsig({ 0, 1, 0 }, 2)) // fn(Int, Float, Int) -> Char
        .add_int(10)
        .add_uint(1000) // <- wrong type for arg #2 for call to g
        .add_int(-4)
        .add_char(U'y');
    
    should_fail(yama::make_callsig({}, 2), 5);

    EXPECT_EQ(dbg->count(yama::dsignal::verif_ParamArgRs_wrong_types), 1);
}

TEST_F(BCodeVerifTests, Ret) {
    bcode
        // block #1
        .add_put_const(yama::newtop, 1) // inits R(0) (to Float 0.05)
        .add_ret(0); // return Float
    consts
        .add_primitive_type("yama:Float"_str)
        .add_float(0.05);
    
    should_pass(yama::make_callsig({}, 0), 1);
}

TEST_F(BCodeVerifTests, Ret_Fail_RA_OutOfBounds) {
    bcode
        // block #1
        // push then pop R(0) to better test impl robustness
        .add_put_none(yama::newtop) // inits R(0) (to None)
        .add_pop(1) // pops R(0)
        .add_ret(0); // R(0) out-of-bounds
    consts
        .add_primitive_type("yama:None"_str);
    
    should_fail(yama::make_callsig({}, 0), 1);

    EXPECT_EQ(dbg->count(yama::dsignal::verif_RA_out_of_bounds), 1);
}

TEST_F(BCodeVerifTests, Ret_Fail_RA_WrongType) {
    bcode
        // block #1
        .add_put_const(yama::newtop, 1) // inits R(0) (to Int 10)
        .add_ret(0); // R(0) not type Float
    consts
        .add_primitive_type("yama:Float"_str)
        .add_int(10);
    
    should_fail(yama::make_callsig({}, 0), 1);

    EXPECT_EQ(dbg->count(yama::dsignal::verif_RA_wrong_type), 1);
}

TEST_F(BCodeVerifTests, Jump) {
    bcode
        // block #1
        .add_jump(2) // jump to block #3
        // block #2 (dead code)
        // this dead code block ensures that we actually *jump over* block #2
        .add_put_const(yama::newtop, 1) // inits R(0) (to Int 10)
        .add_ret(0) // <- static verif error if reachable
        // block #3
        .add_put_const(yama::newtop, 2) // inits R(0) (to Float 0.05)
        .add_ret(0);
    consts
        .add_primitive_type("yama:Float"_str)
        .add_int(10)
        .add_float(0.05);
    
    should_pass(yama::make_callsig({}, 0), 1);
}

TEST_F(BCodeVerifTests, Jump_Fail_PutsPCOutOfBounds) {
    bcode
        // block #1
        .add_jump(1); // jump to out-of-bounds instruction index
    consts
        .add_primitive_type("yama:None"_str);
    
    should_fail(yama::make_callsig({}, 0), 1);

    EXPECT_EQ(dbg->count(yama::dsignal::verif_puts_PC_out_of_bounds), 1);
}

TEST_F(BCodeVerifTests, Jump_Fail_ViolatesRegisterCoherence) {
    bcode
        // block #1
        .add_put_const(yama::newtop, 1) // inits R(0) (to Bool true)
        .add_jump_true(0, 2) // jump to block #3, or fallthrough to block #2
        // block #2
        .add_put_const(0, 2, true) // reinits R(0) (to Float 3.14159)
        .add_jump(2) // merge R(0) into block #4 as Float (thus ALL branches to block #4 must have R(0) be Float)
        // block #3
        .add_put_const(0, 3, true) // reinits R(0) (to Int 100)
        .add_jump(0) // merge R(0) into block #4 as Int (violating register coherence w/ above R(0) being Float)
        // block #5
        .add_put_none(0, true) // reinits R(0) (to None)
        .add_ret(0); // return None object
    consts
        .add_primitive_type("yama:None"_str)
        .add_bool(true)
        .add_float(3.14159)
        .add_int(100);
    
    should_fail(yama::make_callsig({}, 0), 1);

    EXPECT_EQ(dbg->count(yama::dsignal::verif_violates_register_coherence), 1);
}

TEST_F(BCodeVerifTests, JumpTrue_PopOne) {
    bcode
        // block #1
        .add_put_none(yama::newtop) // inits R(0) (to None)
        .add_put_const(yama::newtop, 1) // inits R(1) (to Bool true)
        .add_jump_true(1, 1) // jump to block #3, or fallthrough to block #2, either way popping R(0)
        // block #2
        // expect R(0) to still be good
        .add_ret(0)
        // block #3
        // expect R(0) to still be good
        .add_ret(0);
    consts
        .add_primitive_type("yama:None"_str)
        .add_bool(true);
    
    should_pass(yama::make_callsig({}, 0), 2);
}

TEST_F(BCodeVerifTests, JumpTrue_PopZero) {
    bcode
        // block #1
        .add_put_const(yama::newtop, 1) // inits R(0) (to Bool true)
        .add_jump_true(0, 1) // jump to block #3, or fallthrough to block #2, *popping nothing*
        // block #2
        // expect R(0) to still be good
        .add_ret(0)
        // block #3
        // expect R(0) to still be good
        .add_ret(0);
    consts
        .add_primitive_type("yama:Bool"_str)
        .add_bool(true);
    
    should_pass(yama::make_callsig({}, 0), 1);
}

TEST_F(BCodeVerifTests, JumpTrue_PopMany) {
    bcode
        // block #1
        .add_put_const(yama::newtop, 1) // inits R(0) (to Bool true)
        .add_put_const(yama::newtop, 1) // inits R(1) (to Bool true)
        .add_put_const(yama::newtop, 1) // inits R(2) (to Bool true)
        .add_put_const(yama::newtop, 1) // inits R(3) (to Bool true)
        .add_put_const(yama::newtop, 1) // inits R(4) (to Bool true)
        .add_jump_true(5, 2) // jump to block #3, or fallthrough to block #2, either way popping R(0), R(1), ..., R(4)
        // block #2
        // pushing should push None to R(0) as stack should be empty
        .add_put_none(yama::newtop) // inits R(0) (to None)
        .add_ret(0)
        // block #3
        // pushing should push None to R(0) as stack should be empty
        .add_put_none(yama::newtop) // inits R(0) (to None)
        .add_ret(0);
    consts
        .add_primitive_type("yama:None"_str)
        .add_bool(true);
    
    should_pass(yama::make_callsig({}, 0), 5);
}

TEST_F(BCodeVerifTests, JumpTrue_PopMoreThanAreOnStack) {
    bcode
        // block #1
        .add_put_const(yama::newtop, 1) // inits R(0) (to Bool true)
        .add_put_const(yama::newtop, 1) // inits R(1) (to Bool true)
        .add_put_const(yama::newtop, 1) // inits R(2) (to Bool true)
        .add_put_const(yama::newtop, 1) // inits R(3) (to Bool true)
        .add_put_const(yama::newtop, 1) // inits R(4) (to Bool true)
        .add_jump_true(100, 2) // jump to block #3, or fallthrough to block #2, either way popping R(0), R(1), ..., R(4)
        // block #2
        // pushing should push None to R(0) as stack should be empty
        .add_put_none(yama::newtop) // inits R(0) (to None)
        .add_ret(0)
        // block #3
        // pushing should push None to R(0) as stack should be empty
        .add_put_none(yama::newtop) // inits R(0) (to None)
        .add_ret(0);
    consts
        .add_primitive_type("yama:None"_str)
        .add_bool(true);
    
    should_pass(yama::make_callsig({}, 0), 5);
}

TEST_F(BCodeVerifTests, JumpTrue_Fail_RTop_DoesNotExist) {
    bcode
        // block #1
        // push then pop R(0) to better test impl robustness
        .add_put_const(yama::newtop, 1) // inits R(0) (to Bool true)
        .add_pop(1) // pops R(0)
        .add_jump_true(1, 2) // R(top) (aka. R(0)) does not exist
        // block #2
        .add_put_const(yama::newtop, 3) // inits R(0) (to Float 0.05)
        .add_ret(0)
        // block #3
        .add_put_const(yama::newtop, 3) // inits R(0) (to Float 0.05)
        .add_ret(0);
    consts
        .add_primitive_type("yama:Float"_str)
        .add_bool(true)
        .add_int(10)
        .add_float(0.05);
    
    should_fail(yama::make_callsig({}, 0), 1);

    EXPECT_EQ(dbg->count(yama::dsignal::verif_RTop_does_not_exist), 1);
}

TEST_F(BCodeVerifTests, JumpTrue_Fail_RTop_WrongType) {
    bcode
        // block #1
        .add_put_const(yama::newtop, 1) // inits R(0) (to Int -4)
        .add_jump_true(1, 2) // R(0) not type Bool
        // block #2
        .add_put_const(yama::newtop, 3) // inits R(0) (to Float 0.05)
        .add_ret(0)
        // block #3
        .add_put_const(yama::newtop, 3) // inits R(0) (to Float 0.05)
        .add_ret(0);
    consts
        .add_primitive_type("yama:Float"_str)
        .add_int(-4)
        .add_int(10)
        .add_float(0.05);
    
    should_fail(yama::make_callsig({}, 0), 1);

    EXPECT_EQ(dbg->count(yama::dsignal::verif_RTop_wrong_type), 1);
}

TEST_F(BCodeVerifTests, JumpTrue_Fail_PutsPCOutOfBounds) {
    bcode
        // block #1
        .add_put_const(yama::newtop, 1) // inits R(0) (to Bool true)
        .add_jump_true(1, 10'000) // branches to out-of-bounds instruction
        // block #2
        .add_put_const(yama::newtop, 3) // inits R(0) (to Float 0.05)
        .add_ret(0)
        // block #3
        .add_put_const(yama::newtop, 3) // inits R(0) (to Float 0.05)
        .add_ret(0);
    consts
        .add_primitive_type("yama:Float"_str)
        .add_bool(true)
        .add_int(10)
        .add_float(0.05);
    
    should_fail(yama::make_callsig({}, 0), 1);

    EXPECT_EQ(dbg->count(yama::dsignal::verif_puts_PC_out_of_bounds), 1);
}

TEST_F(BCodeVerifTests, JumpTrue_Fail_ViolatesRegisterCoherence) {
    bcode
        // block #1
        .add_put_const(yama::newtop, 1) // inits R(0) (to Bool true)
        .add_jump_true(1, 5) // branch to block #4, or fallthrough to block #2
        // block #2
        .add_put_const(yama::newtop, 2) // inits R(0) (to Float 3.14159)
        .add_put_const(yama::newtop, 1) // inits R(1) (to Bool true)
        // using jump_true instead of jump as the former is the one under test
        .add_jump_true(1, 7) // merge R(0) into block #6 as Float
        // block #3
        // this block is here just to provide a place to fallthrough to
        .add_put_none(0, true)
        .add_ret(0)
        // block #4
        .add_put_const(yama::newtop, 3) // inits R(0) (to Int 100)
        .add_put_const(yama::newtop, 1) // inits R(1) (to Bool true)
        // using jump_true instead of jump as the former is the one under test
        .add_jump_true(1, 2) // merge R(0) into block #6 as Int (violating register coherence w/ above R(0) being Float)
        // block #5
        // this block is here just to provide a place to fallthrough to
        .add_put_none(0, true)
        .add_ret(0)
        // block #6
        // R(0) will be either Float or Int by this point, violating register coherence
        .add_put_none(0, true) // reinits R(0) (to None)
        .add_ret(0); // return None object
    consts
        .add_primitive_type("yama:None"_str)
        .add_bool(true)
        .add_float(3.14159)
        .add_int(100);
    
    should_fail(yama::make_callsig({}, 0), 2);

    EXPECT_EQ(dbg->count(yama::dsignal::verif_violates_register_coherence), 1);
}

TEST_F(BCodeVerifTests, JumpFalse_PopOne) {
    bcode
        // block #1
        .add_put_none(yama::newtop) // inits R(0) (to None)
        .add_put_const(yama::newtop, 1) // inits R(1) (to Bool true)
        .add_jump_false(1, 1) // jump to block #3, or fallthrough to block #2, either way popping R(0)
        // block #2
        // expect R(0) to still be good
        .add_ret(0)
        // block #3
        // expect R(0) to still be good
        .add_ret(0);
    consts
        .add_primitive_type("yama:None"_str)
        .add_bool(true);
    
    should_pass(yama::make_callsig({}, 0), 2);
}

TEST_F(BCodeVerifTests, JumpFalse_PopZero) {
    bcode
        // block #1
        .add_put_const(yama::newtop, 1) // inits R(0) (to Bool true)
        .add_jump_false(0, 1) // jump to block #3, or fallthrough to block #2, *popping nothing*
        // block #2
        // expect R(0) to still be good
        .add_ret(0)
        // block #3
        // expect R(0) to still be good
        .add_ret(0);
    consts
        .add_primitive_type("yama:Bool"_str)
        .add_bool(true);
    
    should_pass(yama::make_callsig({}, 0), 1);
}

TEST_F(BCodeVerifTests, JumpFalse_PopMany) {
    bcode
        // block #1
        .add_put_const(yama::newtop, 1) // inits R(0) (to Bool true)
        .add_put_const(yama::newtop, 1) // inits R(1) (to Bool true)
        .add_put_const(yama::newtop, 1) // inits R(2) (to Bool true)
        .add_put_const(yama::newtop, 1) // inits R(3) (to Bool true)
        .add_put_const(yama::newtop, 1) // inits R(4) (to Bool true)
        .add_jump_false(5, 2) // jump to block #3, or fallthrough to block #2, either way popping R(0), R(1), ..., R(4)
        // block #2
        // pushing should push None to R(0) as stack should be empty
        .add_put_none(yama::newtop) // inits R(0) (to None)
        .add_ret(0)
        // block #3
        // pushing should push None to R(0) as stack should be empty
        .add_put_none(yama::newtop) // inits R(0) (to None)
        .add_ret(0);
    consts
        .add_primitive_type("yama:None"_str)
        .add_bool(true);
    
    should_pass(yama::make_callsig({}, 0), 5);
}

TEST_F(BCodeVerifTests, JumpFalse_PopMoreThanAreOnStack) {
    bcode
        // block #1
        .add_put_const(yama::newtop, 1) // inits R(0) (to Bool true)
        .add_put_const(yama::newtop, 1) // inits R(1) (to Bool true)
        .add_put_const(yama::newtop, 1) // inits R(2) (to Bool true)
        .add_put_const(yama::newtop, 1) // inits R(3) (to Bool true)
        .add_put_const(yama::newtop, 1) // inits R(4) (to Bool true)
        .add_jump_false(100, 2) // jump to block #3, or fallthrough to block #2, either way popping R(0), R(1), ..., R(4)
        // block #2
        // pushing should push None to R(0) as stack should be empty
        .add_put_none(yama::newtop) // inits R(0) (to None)
        .add_ret(0)
        // block #3
        // pushing should push None to R(0) as stack should be empty
        .add_put_none(yama::newtop) // inits R(0) (to None)
        .add_ret(0);
    consts
        .add_primitive_type("yama:None"_str)
        .add_bool(true);
    
    should_pass(yama::make_callsig({}, 0), 5);
}

TEST_F(BCodeVerifTests, JumpFalse_Fail_RTop_DoesNotExist) {
    bcode
        // block #1
        // push then pop R(0) to better test impl robustness
        .add_put_const(yama::newtop, 1) // inits R(0) (to Bool true)
        .add_pop(1) // pops R(0)
        .add_jump_false(1, 2) // R(top) (aka. R(0)) does not exist
        // block #2
        .add_put_const(yama::newtop, 3) // inits R(0) (to Float 0.05)
        .add_ret(0)
        // block #3
        .add_put_const(yama::newtop, 3) // inits R(0) (to Float 0.05)
        .add_ret(0);
    consts
        .add_primitive_type("yama:Float"_str)
        .add_bool(true)
        .add_int(10)
        .add_float(0.05);
    
    should_fail(yama::make_callsig({}, 0), 1);

    EXPECT_EQ(dbg->count(yama::dsignal::verif_RTop_does_not_exist), 1);
}

TEST_F(BCodeVerifTests, JumpFalse_Fail_RTop_WrongType) {
    bcode
        // block #1
        .add_put_const(yama::newtop, 1) // inits R(0) (to Int -4)
        .add_jump_false(1, 2) // R(0) not type Bool
        // block #2
        .add_put_const(yama::newtop, 3) // inits R(0) (to Float 0.05)
        .add_ret(0)
        // block #3
        .add_put_const(yama::newtop, 3) // inits R(0) (to Float 0.05)
        .add_ret(0);
    consts
        .add_primitive_type("yama:Float"_str)
        .add_int(-4)
        .add_int(10)
        .add_float(0.05);
    
    should_fail(yama::make_callsig({}, 0), 1);

    EXPECT_EQ(dbg->count(yama::dsignal::verif_RTop_wrong_type), 1);
}

TEST_F(BCodeVerifTests, JumpFalse_Fail_PutsPCOutOfBounds) {
    bcode
        // block #1
        .add_put_const(yama::newtop, 1) // inits R(0) (to Bool true)
        .add_jump_false(1, 10'000) // branches to out-of-bounds instruction
        // block #2
        .add_put_const(yama::newtop, 3) // inits R(0) (to Float 0.05)
        .add_ret(0)
        // block #3
        .add_put_const(yama::newtop, 3) // inits R(0) (to Float 0.05)
        .add_ret(0);
    consts
        .add_primitive_type("yama:Float"_str)
        .add_bool(true)
        .add_int(10)
        .add_float(0.05);
    
    should_fail(yama::make_callsig({}, 0), 1);

    EXPECT_EQ(dbg->count(yama::dsignal::verif_puts_PC_out_of_bounds), 1);
}

TEST_F(BCodeVerifTests, JumpFalse_Fail_ViolatesRegisterCoherence) {
    bcode
        // block #1
        .add_put_const(yama::newtop, 1) // inits R(0) (to Bool true)
        .add_jump_false(1, 5) // branch to block #4, or fallthrough to block #2
        // block #2
        .add_put_const(yama::newtop, 2) // inits R(0) (to Float 3.14159)
        .add_put_const(yama::newtop, 1) // inits R(1) (to Bool true)
        // using jump_false instead of jump as the former is the one under test
        .add_jump_false(1, 7) // merge R(0) into block #6 as Float
        // block #3
        // this block is here just to provide a place to fallthrough to
        .add_put_none(0, true)
        .add_ret(0)
        // block #4
        .add_put_const(yama::newtop, 3) // inits R(0) (to Int 100)
        .add_put_const(yama::newtop, 1) // inits R(1) (to Bool true)
        // using jump_false instead of jump as the former is the one under test
        .add_jump_false(1, 2) // merge R(0) into block #6 as Int (violating register coherence w/ above R(0) being Float)
        // block #5
        // this block is here just to provide a place to fallthrough to
        .add_put_none(0, true)
        .add_ret(0)
        // block #6
        // R(0) will be either Float or Int by this point, violating register coherence
        .add_put_none(0, true) // reinits R(0) (to None)
        .add_ret(0); // return None object
    consts
        .add_primitive_type("yama:None"_str)
        .add_bool(true)
        .add_float(3.14159)
        .add_int(100);
    
    should_fail(yama::make_callsig({}, 0), 2);

    EXPECT_EQ(dbg->count(yama::dsignal::verif_violates_register_coherence), 1);
}

