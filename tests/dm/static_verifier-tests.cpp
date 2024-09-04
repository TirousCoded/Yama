

#include <gtest/gtest.h>

#include <yama/core/general.h>
#include <yama/core/type_info.h>
#include <yama/dm/static_verifier.h>
#include <yama/debug-impls/stderr_debug.h>


using namespace yama::string_literals;


class StaticVerifierTests : public testing::Test {
public:

    std::unique_ptr<yama::dm::static_verifier> verif;


protected:

    void SetUp() override final {
        verif = std::make_unique<yama::dm::static_verifier>(std::make_shared<yama::stderr_debug>());
    }

    void TearDown() override final {
        //
    }
};


TEST_F(StaticVerifierTests, Verify) {
    // this is just a general test of successful usage of static_verifier

    const auto a_consts =
        yama::const_table_info()
        .add_primitive_type("b"_str)
        .add_function_type("c"_str, yama::make_callsig_info({ 0 }, 1)) // ie. 'fn(b) -> c'
        .add_primitive_type("d"_str);
    yama::type_info a{
        .fullname = "a"_str,
        .consts = a_consts,
        .info = yama::function_info{
            .callsig = yama::make_callsig_info({ 0, 1, 2 }, 0),
            .call_fn = yama::noop_call_fn,
            .locals = 4,
        },
    };

    EXPECT_TRUE(verif->verify(a));
}

TEST_F(StaticVerifierTests, Verify_Fail_CallSigConstIndexOutOfBounds_ParamType_ForTypeItself) {
    const auto a_consts =
        yama::const_table_info()
        .add_primitive_type("b"_str);
    yama::type_info a{
        .fullname = "a"_str,
        .consts = a_consts,
        .info = yama::function_info{
            // illegal out-of-bounds constant index (for param type of a)
            .callsig = yama::make_callsig_info({ 1 }, 0),
            .call_fn = yama::noop_call_fn,
            .locals = 4,
        },
    };

    EXPECT_FALSE(verif->verify(a));
}

TEST_F(StaticVerifierTests, Verify_Fail_CallSigConstIndexOutOfBounds_ParamType_ForLinkSymbol) {
    const auto a_consts =
        yama::const_table_info()
        // illegal out-of-bounds constant index (for param type of b)
        .add_function_type("b"_str, yama::make_callsig_info({ 2 }, 1))
        .add_primitive_type("c"_str);
    yama::type_info a{
        .fullname = "a"_str,
        .consts = a_consts,
        .info = yama::primitive_info{
            .ptype = yama::ptype::bool0,
        },
    };

    EXPECT_FALSE(verif->verify(a));
}

TEST_F(StaticVerifierTests, Verify_Fail_CallSigConstIndexOutOfBounds_ReturnType_ForTypeItself) {
    const auto a_consts =
        yama::const_table_info()
        .add_primitive_type("b"_str);
    yama::type_info a{
        .fullname = "a"_str,
        .consts = a_consts,
        .info = yama::function_info{
            // illegal out-of-bounds constant index (for return type of a)
            .callsig = yama::make_callsig_info({}, 1),
            .call_fn = yama::noop_call_fn,
            .locals = 4,
        },
    };

    EXPECT_FALSE(verif->verify(a));
}

TEST_F(StaticVerifierTests, Verify_Fail_CallSigConstIndexOutOfBounds_ReturnType_ForLinkSymbol) {
    const auto a_consts =
        yama::const_table_info()
        // illegal out-of-bounds constant index (for return type of b)
        .add_function_type("b"_str, yama::make_callsig_info({}, 2))
        .add_primitive_type("c"_str);
    yama::type_info a{
        .fullname = "a"_str,
        .consts = a_consts,
        .info = yama::primitive_info{
            .ptype = yama::ptype::bool0,
        },
    };

    EXPECT_FALSE(verif->verify(a));
}

TEST_F(StaticVerifierTests, Verify_Fail_CallSigConstNotATypeConst_ParamType_ForTypeItself) {
    const auto a_consts =
        yama::const_table_info()
        .add_primitive_type("b"_str)
        .add_int(10);
    yama::type_info a{
        .fullname = "a"_str,
        .consts = a_consts,
        .info = yama::function_info{
            // illegal use of non-type constant index (for param type of a)
            .callsig = yama::make_callsig_info({ 1 }, 0),
            .call_fn = yama::noop_call_fn,
            .locals = 4,
        },
    };

    EXPECT_FALSE(verif->verify(a));
}

TEST_F(StaticVerifierTests, Verify_Fail_CallSigConstNotATypeConst_ParamType_ForLinkSymbol) {
    const auto a_consts =
        yama::const_table_info()
        // illegal use of non-type constant index (for param type of b)
        .add_function_type("b"_str, yama::make_callsig_info({ 2 }, 1))
        .add_primitive_type("c"_str)
        .add_int(10);
    yama::type_info a{
        .fullname = "a"_str,
        .consts = a_consts,
        .info = yama::primitive_info{
            .ptype = yama::ptype::bool0,
        },
    };

    EXPECT_FALSE(verif->verify(a));
}

TEST_F(StaticVerifierTests, Verify_Fail_CallSigConstNotATypeConst_ReturnType_ForTypeItself) {
    const auto a_consts =
        yama::const_table_info()
        .add_primitive_type("b"_str)
        .add_int(10);
    yama::type_info a{
        .fullname = "a"_str,
        .consts = a_consts,
        .info = yama::function_info{
            // illegal use of non-type constant index (for return type of a)
            .callsig = yama::make_callsig_info({}, 1),
            .call_fn = yama::noop_call_fn,
            .locals = 4,
        },
    };

    EXPECT_FALSE(verif->verify(a));
}

TEST_F(StaticVerifierTests, Verify_Fail_CallSigConstNotATypeConst_ReturnType_ForLinkSymbol) {
    const auto a_consts =
        yama::const_table_info()
        // illegal use of non-type constant index (for return type of b)
        .add_function_type("b"_str, yama::make_callsig_info({}, 2))
        .add_primitive_type("c"_str)
        .add_int(10);
    yama::type_info a{
        .fullname = "a"_str,
        .consts = a_consts,
        .info = yama::primitive_info{
            .ptype = yama::ptype::bool0,
        },
    };

    EXPECT_FALSE(verif->verify(a));
}

TEST_F(StaticVerifierTests, Verify_BCode_DisregardBCodeIfCallFnIsNotBCodeCallFn) {
    const auto f_bcode =
        yama::bc::code(); // otherwise illegal empty bcode
    std::cerr << f_bcode.fmt_disassembly() << "\n";
    const auto f_consts =
        yama::const_table_info()
        .add_primitive_type("None"_str);
    yama::type_info f{
        .fullname = "f"_str,
        .consts = f_consts,
        .info = yama::function_info{
            .callsig = yama::make_callsig_info({}, 0),
            .call_fn = yama::noop_call_fn, // <- NOT bcode_call_fn
            .locals = 1,
            .bcode = f_bcode,
        },
    };

    EXPECT_TRUE(verif->verify(f));
}

TEST_F(StaticVerifierTests, Verify_BCode_TolerateDeadCode) {
    const auto f_bcode =
        yama::bc::code()
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
        .add_ret(0);
    std::cerr << f_bcode.fmt_disassembly() << "\n";
    const auto f_consts =
        yama::const_table_info()
        .add_primitive_type("None"_str);
    yama::type_info f{
        .fullname = "f"_str,
        .consts = f_consts,
        .info = yama::function_info{
            .callsig = yama::make_callsig_info({}, 0),
            .call_fn = yama::bcode_call_fn,
            .locals = 1,
            .bcode = f_bcode,
        },
    };

    EXPECT_TRUE(verif->verify(f));
}

TEST_F(StaticVerifierTests, Verify_BCode_TolerateAllControlPathsBeingCyclicalAndThusNoExitpoints) {
    const auto f_bcode =
        yama::bc::code()
        // block #1
        .add_noop()
        .add_noop()
        .add_noop()
        .add_jump(-4);
    std::cerr << f_bcode.fmt_disassembly() << "\n";
    const auto f_consts =
        yama::const_table_info()
        .add_primitive_type("None"_str);
    yama::type_info f{
        .fullname = "f"_str,
        .consts = f_consts,
        .info = yama::function_info{
            .callsig = yama::make_callsig_info({}, 0),
            .call_fn = yama::bcode_call_fn,
            .locals = 1,
            .bcode = f_bcode,
        },
    };

    EXPECT_TRUE(verif->verify(f));
}

TEST_F(StaticVerifierTests, Verify_BCode_RegistersAreInitToNoneObjectAtEntrypoint) {
    const auto f_bcode =
        yama::bc::code()
        // block #1
        // no reinit means types MUST be None
        .add_load_none(0)
        .add_load_none(1)
        .add_load_none(2)
        .add_load_none(3)
        .add_load_none(4)
        .add_ret(0);
    std::cerr << f_bcode.fmt_disassembly() << "\n";
    const auto f_consts =
        yama::const_table_info()
        .add_primitive_type("None"_str);
    yama::type_info f{
        .fullname = "f"_str,
        .consts = f_consts,
        .info = yama::function_info{
            .callsig = yama::make_callsig_info({}, 0),
            .call_fn = yama::bcode_call_fn,
            .locals = 5,
            .bcode = f_bcode,
        },
    };

    EXPECT_TRUE(verif->verify(f));
}

TEST_F(StaticVerifierTests, Verify_BCode_RegistersReinitsOverwriteOneAnother) {
    const auto f_bcode =
        yama::bc::code()
        // block #1
        .add_load_const(0, 1, true) // reinit R(0) (to Int -4)
        .add_load_const(0, 2, true) // reinit R(0) (to Char 'y')
        .add_load_const(0, 3, true) // reinit R(0) (to Float 0.05)
        .add_ret(0);
    std::cerr << f_bcode.fmt_disassembly() << "\n";
    const auto f_consts =
        yama::const_table_info()
        .add_primitive_type("Float"_str)
        .add_int(-4)
        .add_char(U'y')
        .add_float(0.05);
    yama::type_info f{
        .fullname = "f"_str,
        .consts = f_consts,
        .info = yama::function_info{
            .callsig = yama::make_callsig_info({}, 0),
            .call_fn = yama::bcode_call_fn,
            .locals = 1,
            .bcode = f_bcode,
        },
    };

    EXPECT_TRUE(verif->verify(f));
}

// IMPORTANT: these Verify_BCode_[No]Branch[_#] tests must be sure to also test that
//            register coherence allows for the reasonable changing of register types,
//            and that this extends to merging control paths

TEST_F(StaticVerifierTests, Verify_BCode_NoBranch) {
    const auto f_bcode =
        yama::bc::code()
        // block #1
        .add_noop()
        .add_load_const(1, 1, true) // reinit R(1)
        .add_noop()
        .add_ret(1);
    std::cerr << f_bcode.fmt_disassembly() << "\n";
    const auto f_consts =
        yama::const_table_info()
        .add_primitive_type("Float"_str)
        .add_float(3.14159);
    yama::type_info f{
        .fullname = "f"_str,
        .consts = f_consts,
        .info = yama::function_info{
            .callsig = yama::make_callsig_info({}, 0),
            .call_fn = yama::bcode_call_fn,
            .locals = 2,
            .bcode = f_bcode,
        },
    };

    EXPECT_TRUE(verif->verify(f));
}

TEST_F(StaticVerifierTests, Verify_BCode_Branch_MultipleExitpoints) {
    const auto f_bcode =
        yama::bc::code()
        // block #1
        .add_load_const(0, 3, true) // reinit R(0) (to Bool true)
        .add_jump_if(0, 2)
        // block #2 (exitpoint)
        .add_load_const(1, 1, true) // reinit R(1) (to Float 3.14159)
        .add_ret(1)
        // block #3 (exitpoint)
        .add_load_const(1, 2, true) // reinit R(1) (to Float 0.05)
        .add_ret(1);
    std::cerr << f_bcode.fmt_disassembly() << "\n";
    const auto f_consts =
        yama::const_table_info()
        .add_primitive_type("Float"_str)
        .add_float(3.14159)
        .add_float(0.05)
        .add_bool(true);
    yama::type_info f{
        .fullname = "f"_str,
        .consts = f_consts,
        .info = yama::function_info{
            .callsig = yama::make_callsig_info({}, 0),
            .call_fn = yama::bcode_call_fn,
            .locals = 2,
            .bcode = f_bcode,
        },
    };

    EXPECT_TRUE(verif->verify(f));
}

TEST_F(StaticVerifierTests, Verify_BCode_Branch_ControlPathsMerge) {
    const auto f_bcode =
        yama::bc::code()
        // block #1
        .add_load_const(0, 3, true) // reinit R(0) (to Bool true)
        .add_jump_if(0, 2)
        // block #2
        .add_load_const(1, 1, true) // reinit R(1) (to Float 3.14159)
        .add_jump(1) // merges into block #4
        // block #3
        .add_load_const(1, 2, true) // reinit R(1) (to Float 0.05)
        // fallthrough merges into block #4
        // block #4
        .add_ret(1); // R(1) is merge of the two above Float reinits
    std::cerr << f_bcode.fmt_disassembly() << "\n";
    const auto f_consts =
        yama::const_table_info()
        .add_primitive_type("Float"_str)
        .add_float(3.14159)
        .add_float(0.05)
        .add_bool(true);
    yama::type_info f{
        .fullname = "f"_str,
        .consts = f_consts,
        .info = yama::function_info{
            .callsig = yama::make_callsig_info({}, 0),
            .call_fn = yama::bcode_call_fn,
            .locals = 2,
            .bcode = f_bcode,
        },
    };

    EXPECT_TRUE(verif->verify(f));
}

TEST_F(StaticVerifierTests, Verify_BCode_Branch_CyclicalControlPath) {
    const auto f_bcode =
        yama::bc::code()
        // block #1
        .add_load_const(0, 1, true) // reinit R(0) (to Int 10)
        // block #2
        .add_load_const(0, 2) // no reinit R(0), so R(0) MUST be an Int already
        .add_load_const(0, 3, true) // reinit R(0) (to Float 0.05)
        .add_load_const(1, 4, true) // reinit R(1) (to Bool true)
        .add_jump_if(1, 3) // jump to block #4, or fallthrough to block #3
        // block #3
        .add_load_const(0, 5, true) // reinit R(0) (to Int 100)
        // notice: block #2 is entered w/ R(1) being None, NOT Bool, so we gotta reinit R(1)
        .add_load_none(1, true) // reinit R(1) (to None)
        .add_jump(-7) // jump to block #2 (forming control path cycle)
        // block #4
        .add_ret(0); // R(0) must be a Float by this point
    std::cerr << f_bcode.fmt_disassembly() << "\n";
    const auto f_consts =
        yama::const_table_info()
        .add_primitive_type("Float"_str)
        .add_int(10)
        .add_int(-4)
        .add_float(0.05)
        .add_bool(true)
        .add_int(100);
    yama::type_info f{
        .fullname = "f"_str,
        .consts = f_consts,
        .info = yama::function_info{
            .callsig = yama::make_callsig_info({}, 0),
            .call_fn = yama::bcode_call_fn,
            .locals = 2,
            .bcode = f_bcode,
        },
    };

    EXPECT_TRUE(verif->verify(f));
}

TEST_F(StaticVerifierTests, Verify_BCode_Branch_FallthroughDueToLastInstrOfBlockNotBeingBranchOrExitpoint) {
    // due to basic blocks being partitioned at each branch destination, it's possible for a basic block
    // to end w/ an instruction that is not a branch/exitpoint instruction

    // in this scenario, the expected behaviour is to fallthrough to the next block, however I encountered
    // an issue in the initial impl where our system didn't realize that it was supposed to do this, and
    // quietly just *didn't*, which left me worried that this is something our tests didn't cover

    // to this end, this tests that symbolic execution proceeds across these fallthroughs, doing so by checking
    // for an error in a later block that likely wouldn't be detected if this fallthrough never occurs

    const auto f_bcode =
        yama::bc::code()
        // block #1
        .add_noop()
        // fallthrough to block #2
        // block #2
        .add_noop() // jump destination
        .add_load_arg(0, 100, true) // <- error!
        .add_jump(-3);
    std::cerr << f_bcode.fmt_disassembly() << "\n";
    const auto f_consts =
        yama::const_table_info()
        .add_primitive_type("None"_str);
    yama::type_info f{
        .fullname = "f"_str,
        .consts = f_consts,
        .info = yama::function_info{
            .callsig = yama::make_callsig_info({}, 0),
            .call_fn = yama::bcode_call_fn,
            .locals = 1,
            .bcode = f_bcode,
        },
    };

    EXPECT_FALSE(verif->verify(f));
}

TEST_F(StaticVerifierTests, Verify_BCode_Fail_BinaryIsEmpty) {
    const auto f_bcode =
        yama::bc::code();
    std::cerr << f_bcode.fmt_disassembly() << "\n";
    const auto f_consts =
        yama::const_table_info()
        .add_primitive_type("Float"_str);
    yama::type_info f{
        .fullname = "f"_str,
        .consts = f_consts,
        .info = yama::function_info{
            .callsig = yama::make_callsig_info({}, 0),
            .call_fn = yama::bcode_call_fn,
            .locals = 1,
            .bcode = f_bcode,
        },
    };

    EXPECT_FALSE(verif->verify(f));
}

TEST_F(StaticVerifierTests, Verify_BCode_Fail_FinalBlockFallthroughToOutOfBoundsInstrs) {
    const auto f_bcode =
        yama::bc::code()
        // block #1
        .add_load_const(0, 1, true) // reinit R(A) (to Bool true)
        // block #2
        .add_noop() // jump destination (so R(A) is always Bool)
        .add_noop()
        // we're using jump_if to test more nuanced scenario than just a simple fallthrough due to no branch or exitpoint
        .add_jump_if(0, -3); // illegal fallthrough to out-of-bounds instruction
    std::cerr << f_bcode.fmt_disassembly() << "\n";
    const auto f_consts =
        yama::const_table_info()
        .add_primitive_type("Bool"_str)
        .add_bool(true);
    yama::type_info f{
        .fullname = "f"_str,
        .consts = f_consts,
        .info = yama::function_info{
            .callsig = yama::make_callsig_info({}, 0),
            .call_fn = yama::bcode_call_fn,
            .locals = 1,
            .bcode = f_bcode,
        },
    };

    EXPECT_FALSE(verif->verify(f));
}

static_assert(yama::bc::opcodes == 10);

TEST_F(StaticVerifierTests, Verify_BCode_Noop) {
    const auto f_bcode =
        yama::bc::code()
        // block #1
        .add_noop()
        .add_noop()
        .add_noop()
        .add_ret(0);
    std::cerr << f_bcode.fmt_disassembly() << "\n";
    const auto f_consts =
        yama::const_table_info()
        .add_primitive_type("None"_str);
    yama::type_info f{
        .fullname = "f"_str,
        .consts = f_consts,
        .info = yama::function_info{
            .callsig = yama::make_callsig_info({}, 0),
            .call_fn = yama::bcode_call_fn,
            .locals = 1,
            .bcode = f_bcode,
        },
    };

    EXPECT_TRUE(verif->verify(f));
}

TEST_F(StaticVerifierTests, Verify_BCode_LoadNone) {
    const auto f_bcode =
        yama::bc::code()
        // block #1
        .add_load_none(0) // no reinit, so R(0) MUST be None already
        .add_ret(0);
    std::cerr << f_bcode.fmt_disassembly() << "\n";
    const auto f_consts =
        yama::const_table_info()
        .add_primitive_type("None"_str);
    yama::type_info f{
        .fullname = "f"_str,
        .consts = f_consts,
        .info = yama::function_info{
            .callsig = yama::make_callsig_info({}, 0),
            .call_fn = yama::bcode_call_fn,
            .locals = 1,
            .bcode = f_bcode,
        },
    };

    EXPECT_TRUE(verif->verify(f));
}

TEST_F(StaticVerifierTests, Verify_BCode_LoadNone_Reinit) {
    const auto f_bcode =
        yama::bc::code()
        // block #1
        .add_load_const(0, 1, true) // reinit R(0) (to Int 10)
        .add_load_none(0, true) // reinit R(0) (to None)
        .add_ret(0);
    std::cerr << f_bcode.fmt_disassembly() << "\n";
    const auto f_consts =
        yama::const_table_info()
        .add_primitive_type("None"_str)
        .add_int(10);
    yama::type_info f{
        .fullname = "f"_str,
        .consts = f_consts,
        .info = yama::function_info{
            .callsig = yama::make_callsig_info({}, 0),
            .call_fn = yama::bcode_call_fn,
            .locals = 1,
            .bcode = f_bcode,
        },
    };

    EXPECT_TRUE(verif->verify(f));
}

TEST_F(StaticVerifierTests, Verify_BCode_LoadNone_Fail_RA_OutOfBounds) {
    const auto f_bcode =
        yama::bc::code()
        // block #1
        .add_load_none(1) // R(1) out-of-bounds
        .add_ret(0);
    std::cerr << f_bcode.fmt_disassembly() << "\n";
    const auto f_consts =
        yama::const_table_info()
        .add_primitive_type("None"_str);
    yama::type_info f{
        .fullname = "f"_str,
        .consts = f_consts,
        .info = yama::function_info{
            .callsig = yama::make_callsig_info({}, 0),
            .call_fn = yama::bcode_call_fn,
            .locals = 1,
            .bcode = f_bcode,
        },
    };

    EXPECT_FALSE(verif->verify(f));
}

TEST_F(StaticVerifierTests, Verify_BCode_LoadNone_Fail_RA_NotOfTypeNone_AndNotReinit) {
    const auto f_bcode =
        yama::bc::code()
        // block #1
        .add_load_const(0, 1, true) // reinit R(0) (to Int 10)
        .add_load_none(0) // R(0) not type None
        .add_ret(0);
    std::cerr << f_bcode.fmt_disassembly() << "\n";
    const auto f_consts =
        yama::const_table_info()
        .add_primitive_type("None"_str)
        .add_int(10);
    yama::type_info f{
        .fullname = "f"_str,
        .consts = f_consts,
        .info = yama::function_info{
            .callsig = yama::make_callsig_info({}, 0),
            .call_fn = yama::bcode_call_fn,
            .locals = 1,
            .bcode = f_bcode,
        },
    };

    EXPECT_FALSE(verif->verify(f));
}

TEST_F(StaticVerifierTests, Verify_BCode_LoadConst) {
    const auto f_bcode =
        yama::bc::code()
        // block #1
        .add_load_const(0, 1, true) // reinit R(0) (to Int 10)
        .add_load_const(0, 2) // no reinit, so R(0) MUST be Int already
        .add_ret(0);
    std::cerr << f_bcode.fmt_disassembly() << "\n";
    const auto f_consts =
        yama::const_table_info()
        .add_primitive_type("Int"_str)
        .add_int(10)
        .add_int(-4);
    yama::type_info f{
        .fullname = "f"_str,
        .consts = f_consts,
        .info = yama::function_info{
            .callsig = yama::make_callsig_info({}, 0),
            .call_fn = yama::bcode_call_fn,
            .locals = 1,
            .bcode = f_bcode,
        },
    };

    EXPECT_TRUE(verif->verify(f));
}

TEST_F(StaticVerifierTests, Verify_BCode_LoadConst_Reinit) {
    const auto f_bcode =
        yama::bc::code()
        // block #1
        .add_load_const(0, 1, true) // reinit R(0) (to Int 10)
        .add_ret(0);
    std::cerr << f_bcode.fmt_disassembly() << "\n";
    const auto f_consts =
        yama::const_table_info()
        .add_primitive_type("Int"_str)
        .add_int(10);
    yama::type_info f{
        .fullname = "f"_str,
        .consts = f_consts,
        .info = yama::function_info{
            .callsig = yama::make_callsig_info({}, 0),
            .call_fn = yama::bcode_call_fn,
            .locals = 1,
            .bcode = f_bcode,
        },
    };

    EXPECT_TRUE(verif->verify(f));
}

TEST_F(StaticVerifierTests, Verify_BCode_LoadConst_Fail_RA_OutOfBounds) {
    const auto f_bcode =
        yama::bc::code()
        // block #1
        .add_load_const(0, 1, true) // reinit R(0) (to Int 10)
        .add_load_const(1, 1) // R(1) out-of-bounds
        .add_ret(0);
    std::cerr << f_bcode.fmt_disassembly() << "\n";
    const auto f_consts =
        yama::const_table_info()
        .add_primitive_type("None"_str)
        .add_int(10);
    yama::type_info f{
        .fullname = "f"_str,
        .consts = f_consts,
        .info = yama::function_info{
            .callsig = yama::make_callsig_info({}, 0),
            .call_fn = yama::bcode_call_fn,
            .locals = 1,
            .bcode = f_bcode,
        },
    };

    EXPECT_FALSE(verif->verify(f));
}

TEST_F(StaticVerifierTests, Verify_BCode_LoadConst_Fail_KoB_OutOfBounds) {
    const auto f_bcode =
        yama::bc::code()
        // block #1
        .add_load_const(0, 1, true) // reinit R(0) (to Int 10)
        .add_load_const(0, 2) // Ko(2) out-of-bounds
        .add_ret(0);
    std::cerr << f_bcode.fmt_disassembly() << "\n";
    const auto f_consts =
        yama::const_table_info()
        .add_primitive_type("Int"_str)
        .add_int(10);
    yama::type_info f{
        .fullname = "f"_str,
        .consts = f_consts,
        .info = yama::function_info{
            .callsig = yama::make_callsig_info({}, 0),
            .call_fn = yama::bcode_call_fn,
            .locals = 1,
            .bcode = f_bcode,
        },
    };

    EXPECT_FALSE(verif->verify(f));
}

TEST_F(StaticVerifierTests, Verify_BCode_LoadConst_Fail_KoB_NotAnObjectConst) {
    const auto f_bcode =
        yama::bc::code()
        // block #1
        .add_load_const(0, 1, true) // reinit R(0) (to Int 10)
        .add_load_const(0, 0) // Ko(0) not an object constant
        .add_ret(0);
    std::cerr << f_bcode.fmt_disassembly() << "\n";
    const auto f_consts =
        yama::const_table_info()
        .add_primitive_type("Int"_str)
        .add_int(10);
    yama::type_info f{
        .fullname = "f"_str,
        .consts = f_consts,
        .info = yama::function_info{
            .callsig = yama::make_callsig_info({}, 0),
            .call_fn = yama::bcode_call_fn,
            .locals = 1,
            .bcode = f_bcode,
        },
    };

    EXPECT_FALSE(verif->verify(f));
}

TEST_F(StaticVerifierTests, Verify_BCode_LoadConst_Fail_RA_And_KoB_TypesDiffer) {
    const auto f_bcode =
        yama::bc::code()
        // block #1
        .add_load_const(0, 1, true) // reinit R(0) (to Int 10)
        .add_load_const(0, 2) // Ko(2) not type Int
        .add_ret(0);
    std::cerr << f_bcode.fmt_disassembly() << "\n";
    const auto f_consts =
        yama::const_table_info()
        .add_primitive_type("Int"_str)
        .add_int(10)
        .add_char(U'y');
    yama::type_info f{
        .fullname = "f"_str,
        .consts = f_consts,
        .info = yama::function_info{
            .callsig = yama::make_callsig_info({}, 0),
            .call_fn = yama::bcode_call_fn,
            .locals = 1,
            .bcode = f_bcode,
        },
    };

    EXPECT_FALSE(verif->verify(f));
}

TEST_F(StaticVerifierTests, Verify_BCode_LoadArg) {
    const auto f_bcode =
        yama::bc::code()
        // block #1
        .add_load_const(0, 2, true) // reinit R(0) (to Bool true)
        .add_load_arg(0, 0) // load Bool arg into R(0), w/out reinit
        .add_load_const(1, 3, true) // reinit R(1) (to Float 0.05)
        .add_load_arg(1, 1) // load Float arg into R(1), w/out reinit
        .add_ret(1);
    std::cerr << f_bcode.fmt_disassembly() << "\n";
    const auto f_consts =
        yama::const_table_info()
        .add_primitive_type("Bool"_str)
        .add_primitive_type("Float"_str)
        .add_bool(true)
        .add_float(0.05);
    yama::type_info f{
        .fullname = "f"_str,
        .consts = f_consts,
        .info = yama::function_info{
            .callsig = yama::make_callsig_info({ 0, 1 }, 1),
            .call_fn = yama::bcode_call_fn,
            .locals = 2,
            .bcode = f_bcode,
        },
    };

    EXPECT_TRUE(verif->verify(f));
}

TEST_F(StaticVerifierTests, Verify_BCode_LoadArg_Reinit) {
    const auto f_bcode =
        yama::bc::code()
        // block #1
        .add_load_arg(0, 0, true) // load Bool arg into R(0), w/ reinit
        .add_load_const(0, 2) // no reinit, so R(0) MUST be Bool already
        .add_load_arg(1, 1, true) // load Float arg into R(1), w/ reinit
        .add_load_const(1, 3) // no reinit, so R(1) MUST be Float already
        .add_ret(1);
    std::cerr << f_bcode.fmt_disassembly() << "\n";
    const auto f_consts =
        yama::const_table_info()
        .add_primitive_type("Bool"_str)
        .add_primitive_type("Float"_str)
        .add_bool(true)
        .add_float(0.05);
    yama::type_info f{
        .fullname = "f"_str,
        .consts = f_consts,
        .info = yama::function_info{
            .callsig = yama::make_callsig_info({ 0, 1 }, 1),
            .call_fn = yama::bcode_call_fn,
            .locals = 2,
            .bcode = f_bcode,
        },
    };

    EXPECT_TRUE(verif->verify(f));
}

TEST_F(StaticVerifierTests, Verify_BCode_LoadArg_Fail_RA_OutOfBounds) {
    const auto f_bcode =
        yama::bc::code()
        // block #1
        .add_load_const(0, 2, true) // reinit R(0) (to Float 0.05)
        .add_load_arg(1, 0) // R(1) out-of-bounds
        .add_ret(0);
    std::cerr << f_bcode.fmt_disassembly() << "\n";
    const auto f_consts =
        yama::const_table_info()
        .add_primitive_type("Bool"_str)
        .add_primitive_type("Float"_str)
        .add_float(0.05);
    yama::type_info f{
        .fullname = "f"_str,
        .consts = f_consts,
        .info = yama::function_info{
            .callsig = yama::make_callsig_info({ 0, 1 }, 1),
            .call_fn = yama::bcode_call_fn,
            .locals = 1,
            .bcode = f_bcode,
        },
    };

    EXPECT_FALSE(verif->verify(f));
}

TEST_F(StaticVerifierTests, Verify_BCode_LoadArg_Fail_ArgB_OutOfBounds) {
    const auto f_bcode =
        yama::bc::code()
        // block #1
        .add_load_const(0, 2, true) // reinit R(0) (to Float 0.05)
        .add_load_arg(0, 2) // Arg(2) out-of-bounds
        .add_ret(0);
    std::cerr << f_bcode.fmt_disassembly() << "\n";
    const auto f_consts =
        yama::const_table_info()
        .add_primitive_type("Bool"_str)
        .add_primitive_type("Float"_str)
        .add_float(0.05);
    yama::type_info f{
        .fullname = "f"_str,
        .consts = f_consts,
        .info = yama::function_info{
            .callsig = yama::make_callsig_info({ 0, 1 }, 1),
            .call_fn = yama::bcode_call_fn,
            .locals = 1,
            .bcode = f_bcode,
        },
    };

    EXPECT_FALSE(verif->verify(f));
}

TEST_F(StaticVerifierTests, Verify_BCode_LoadArg_Fail_RA_And_ArgB_TypesDiffer) {
    const auto f_bcode =
        yama::bc::code()
        // block #1
        .add_load_const(0, 2, true) // reinit R(0) (to Float 0.05)
        .add_load_arg(0, 0) // Arg(0) not type Float
        .add_ret(0);
    std::cerr << f_bcode.fmt_disassembly() << "\n";
    const auto f_consts =
        yama::const_table_info()
        .add_primitive_type("Bool"_str)
        .add_primitive_type("Float"_str)
        .add_float(0.05);
    yama::type_info f{
        .fullname = "f"_str,
        .consts = f_consts,
        .info = yama::function_info{
            .callsig = yama::make_callsig_info({ 0, 1 }, 1),
            .call_fn = yama::bcode_call_fn,
            .locals = 1,
            .bcode = f_bcode,
        },
    };

    EXPECT_FALSE(verif->verify(f));
}

TEST_F(StaticVerifierTests, Verify_BCode_Copy) {
    const auto f_bcode =
        yama::bc::code()
        // block #1
        .add_load_const(0, 1, true) // reinit R(0) (to Float 0.05)
        .add_load_const(1, 2, true) // reinit R(1) (to Float 3.14159)
        .add_copy(0, 1) // no reinit, so R(1) MUST be Float already
        .add_ret(1);
    std::cerr << f_bcode.fmt_disassembly() << "\n";
    const auto f_consts =
        yama::const_table_info()
        .add_primitive_type("Float"_str)
        .add_float(0.05)
        .add_float(3.14159);
    yama::type_info f{
        .fullname = "f"_str,
        .consts = f_consts,
        .info = yama::function_info{
            .callsig = yama::make_callsig_info({}, 0),
            .call_fn = yama::bcode_call_fn,
            .locals = 2,
            .bcode = f_bcode,
        },
    };

    EXPECT_TRUE(verif->verify(f));
}

TEST_F(StaticVerifierTests, Verify_BCode_Copy_Reinit) {
    const auto f_bcode =
        yama::bc::code()
        // block #1
        .add_load_const(0, 1, true) // reinit R(0) (to Float 0.05)
        .add_load_const(1, 2, true) // reinit R(1) (to Int 10)
        .add_copy(0, 1, true) // reinit R(1) (to Float)
        .add_ret(1);
    std::cerr << f_bcode.fmt_disassembly() << "\n";
    const auto f_consts =
        yama::const_table_info()
        .add_primitive_type("Float"_str)
        .add_float(0.05)
        .add_int(10);
    yama::type_info f{
        .fullname = "f"_str,
        .consts = f_consts,
        .info = yama::function_info{
            .callsig = yama::make_callsig_info({}, 0),
            .call_fn = yama::bcode_call_fn,
            .locals = 2,
            .bcode = f_bcode,
        },
    };

    EXPECT_TRUE(verif->verify(f));
}

TEST_F(StaticVerifierTests, Verify_BCode_Copy_Fail_RA_OutOfBounds) {
    const auto f_bcode =
        yama::bc::code()
        // block #1
        .add_load_const(0, 1, true) // reinit R(0) (to Float 0.05)
        .add_load_const(1, 2, true) // reinit R(1) (to Float 3.14159)
        .add_copy(2, 1) // R(2) out-of-bounds
        .add_ret(1);
    std::cerr << f_bcode.fmt_disassembly() << "\n";
    const auto f_consts =
        yama::const_table_info()
        .add_primitive_type("Float"_str)
        .add_float(0.05)
        .add_float(3.14159);
    yama::type_info f{
        .fullname = "f"_str,
        .consts = f_consts,
        .info = yama::function_info{
            .callsig = yama::make_callsig_info({}, 0),
            .call_fn = yama::bcode_call_fn,
            .locals = 2,
            .bcode = f_bcode,
        },
    };

    EXPECT_FALSE(verif->verify(f));
}

TEST_F(StaticVerifierTests, Verify_BCode_Copy_Fail_RB_OutOfBounds) {
    const auto f_bcode =
        yama::bc::code()
        // block #1
        .add_load_const(0, 1, true) // reinit R(0) (to Float 0.05)
        .add_load_const(1, 2, true) // reinit R(1) (to Float 3.14159)
        .add_copy(0, 2) // R(2) out-of-bounds
        .add_ret(1);
    std::cerr << f_bcode.fmt_disassembly() << "\n";
    const auto f_consts =
        yama::const_table_info()
        .add_primitive_type("Float"_str)
        .add_float(0.05)
        .add_float(3.14159);
    yama::type_info f{
        .fullname = "f"_str,
        .consts = f_consts,
        .info = yama::function_info{
            .callsig = yama::make_callsig_info({}, 0),
            .call_fn = yama::bcode_call_fn,
            .locals = 2,
            .bcode = f_bcode,
        },
    };

    EXPECT_FALSE(verif->verify(f));
}

TEST_F(StaticVerifierTests, Verify_BCode_Copy_Fail_RA_And_RB_TypesDiffer) {
    const auto f_bcode =
        yama::bc::code()
        // block #1
        .add_load_const(0, 1, true) // reinit R(0) (to Int 10)
        .add_load_const(1, 2, true) // reinit R(1) (to Float 0.05)
        .add_copy(0, 1) // R(1) not type Int
        .add_ret(1);
    std::cerr << f_bcode.fmt_disassembly() << "\n";
    const auto f_consts =
        yama::const_table_info()
        .add_primitive_type("Float"_str)
        .add_int(10)
        .add_float(0.05);
    yama::type_info f{
        .fullname = "f"_str,
        .consts = f_consts,
        .info = yama::function_info{
            .callsig = yama::make_callsig_info({}, 0),
            .call_fn = yama::bcode_call_fn,
            .locals = 2,
            .bcode = f_bcode,
        },
    };

    EXPECT_FALSE(verif->verify(f));
}

TEST_F(StaticVerifierTests, Verify_BCode_Call) {
    const auto f_bcode =
        yama::bc::code()
        // block #1
        .add_load_const(0, 3, true) // reinit R(0) to type of call object
        .add_load_const(1, 4, true) // reinit R(1) to type of arg #1
        .add_load_const(2, 5, true) // reinit R(2) to type of arg #2
        .add_load_const(3, 6, true) // reinit R(3) to type of arg #3
        .add_load_const(4, 7, true) // reinit R(4) to type of return value
        .add_call(0, 4, 4)
        .add_ret(4);
    std::cerr << f_bcode.fmt_disassembly() << "\n";
    const auto f_consts =
        yama::const_table_info()
        .add_primitive_type("Int"_str)
        .add_primitive_type("Float"_str)
        .add_primitive_type("Char"_str)
        .add_function_type("g"_str, yama::make_callsig_info({ 0, 1, 0 }, 2)) // fn(Int, Float, Int) -> Char
        .add_int(10)
        .add_float(0.05)
        .add_int(-4)
        .add_char(U'y');
    yama::type_info f{
        .fullname = "f"_str,
        .consts = f_consts,
        .info = yama::function_info{
            .callsig = yama::make_callsig_info({}, 2),
            .call_fn = yama::bcode_call_fn,
            .locals = 5,
            .bcode = f_bcode,
        },
    };

    EXPECT_TRUE(verif->verify(f));
}

TEST_F(StaticVerifierTests, Verify_BCode_Call_Reinit) {
    const auto f_bcode =
        yama::bc::code()
        // block #1
        .add_load_const(0, 3, true) // reinit R(0) to type of call object
        .add_load_const(1, 4, true) // reinit R(1) to type of arg #1
        .add_load_const(2, 5, true) // reinit R(2) to type of arg #2
        .add_load_const(3, 6, true) // reinit R(3) to type of arg #3
        .add_call(0, 4, 4, true) // reinits R(4) to return type Char
        .add_ret(4);
    std::cerr << f_bcode.fmt_disassembly() << "\n";
    const auto f_consts =
        yama::const_table_info()
        .add_primitive_type("Int"_str)
        .add_primitive_type("Float"_str)
        .add_primitive_type("Char"_str)
        .add_function_type("g"_str, yama::make_callsig_info({ 0, 1, 0 }, 2)) // fn(Int, Float, Int) -> Char
        .add_int(10)
        .add_float(0.05)
        .add_int(-4)
        .add_char(U'y');
    yama::type_info f{
        .fullname = "f"_str,
        .consts = f_consts,
        .info = yama::function_info{
            .callsig = yama::make_callsig_info({}, 2),
            .call_fn = yama::bcode_call_fn,
            .locals = 5,
            .bcode = f_bcode,
        },
    };

    EXPECT_TRUE(verif->verify(f));
}

TEST_F(StaticVerifierTests, Verify_BCode_Call_Fail_ArgRegisters_OutOfBounds) {
    const auto f_bcode =
        yama::bc::code()
        // block #1
        .add_load_const(0, 3, true) // reinit R(0) to type of call object
        .add_load_const(1, 4, true) // reinit R(1) to type of arg #1
        .add_load_const(2, 5, true) // reinit R(2) to type of arg #2
        .add_load_const(3, 6, true) // reinit R(3) to type of arg #3
        .add_load_const(4, 7, true) // reinit R(4) to type of return value
        .add_call(0, 40, 4) // [R(0), R(39)] out-of-bounds
        .add_ret(4);
    std::cerr << f_bcode.fmt_disassembly() << "\n";
    const auto f_consts =
        yama::const_table_info()
        .add_primitive_type("Int"_str)
        .add_primitive_type("Float"_str)
        .add_primitive_type("Char"_str)
        .add_function_type("g"_str, yama::make_callsig_info({ 0, 1, 0 }, 2)) // fn(Int, Float, Int) -> Char
        .add_int(10)
        .add_float(0.05)
        .add_int(-4)
        .add_char(U'y');
    yama::type_info f{
        .fullname = "f"_str,
        .consts = f_consts,
        .info = yama::function_info{
            .callsig = yama::make_callsig_info({}, 2),
            .call_fn = yama::bcode_call_fn,
            .locals = 5,
            .bcode = f_bcode,
        },
    };

    EXPECT_FALSE(verif->verify(f));
}

TEST_F(StaticVerifierTests, Verify_BCode_Call_Fail_ArgRegisters_ZeroObjects) {
    const auto f_bcode =
        yama::bc::code()
        // block #1
        // NOTE: R(A) being a valid call object ensures error couldn't be about R(A) not being one
        .add_load_const(0, 1, true) // reinit R(0) to type of call object (though this will go unused)
        .add_load_none(1, true) // reinit R(1) to type of return value
        .add_call(0, 0, 1) // B == 0 means no call object specified
        .add_ret(1);
    std::cerr << f_bcode.fmt_disassembly() << "\n";
    const auto f_consts =
        yama::const_table_info()
        .add_primitive_type("None"_str)
        .add_function_type("g"_str, yama::make_callsig_info({}, 0)); // fn() -> None
    yama::type_info f{
        .fullname = "f"_str,
        .consts = f_consts,
        .info = yama::function_info{
            .callsig = yama::make_callsig_info({}, 0),
            .call_fn = yama::bcode_call_fn,
            .locals = 5,
            .bcode = f_bcode,
        },
    };

    EXPECT_FALSE(verif->verify(f));
}

TEST_F(StaticVerifierTests, Verify_BCode_Call_Fail_RA_IllegalToUseAsCallObject) {
    const auto f_bcode =
        yama::bc::code()
        // block #1
        .add_load_const(0, 3, true) // reinit R(0) to type of call object
        .add_load_const(1, 4, true) // reinit R(1) to type of arg #1
        .add_load_const(2, 5, true) // reinit R(2) to type of arg #2
        .add_load_const(3, 6, true) // reinit R(3) to type of arg #3
        .add_load_const(4, 7, true) // reinit R(4) to type of return value
        .add_call(0, 4, 4) // R(0) cannot be used as call object
        .add_ret(4);
    std::cerr << f_bcode.fmt_disassembly() << "\n";
    const auto f_consts =
        yama::const_table_info()
        .add_primitive_type("Int"_str)
        .add_primitive_type("Float"_str)
        .add_primitive_type("Char"_str)
        .add_int(50) // <- cannot be used as call object
        .add_int(10)
        .add_float(0.05)
        .add_int(-4)
        .add_char(U'y');
    yama::type_info f{
        .fullname = "f"_str,
        .consts = f_consts,
        .info = yama::function_info{
            .callsig = yama::make_callsig_info({}, 2),
            .call_fn = yama::bcode_call_fn,
            .locals = 5,
            .bcode = f_bcode,
        },
    };

    EXPECT_FALSE(verif->verify(f));
}

TEST_F(StaticVerifierTests, Verify_BCode_Call_Fail_ParamArgRegisters_TooMany) {
    const auto f_bcode =
        yama::bc::code()
        // block #1
        .add_load_const(0, 3, true) // reinit R(0) to type of call object
        .add_load_const(1, 4, true) // reinit R(1) to type of arg #1
        .add_load_const(2, 5, true) // reinit R(2) to type of arg #2
        .add_load_const(3, 6, true) // reinit R(3) to type of arg #3
        .add_load_const(4, 6, true) // reinit R(4) to type of arg #4
        .add_load_const(5, 7, true) // reinit R(5) to type of return value
        .add_call(0, 5, 5) // 5 is too many args for call to g
        .add_ret(5);
    std::cerr << f_bcode.fmt_disassembly() << "\n";
    const auto f_consts =
        yama::const_table_info()
        .add_primitive_type("Int"_str)
        .add_primitive_type("Float"_str)
        .add_primitive_type("Char"_str)
        .add_function_type("g"_str, yama::make_callsig_info({ 0, 1, 0 }, 2)) // fn(Int, Float, Int) -> Char
        .add_int(10)
        .add_float(0.05)
        .add_int(-4)
        .add_char(U'y');
    yama::type_info f{
        .fullname = "f"_str,
        .consts = f_consts,
        .info = yama::function_info{
            .callsig = yama::make_callsig_info({}, 2),
            .call_fn = yama::bcode_call_fn,
            .locals = 6,
            .bcode = f_bcode,
        },
    };

    EXPECT_FALSE(verif->verify(f));
}

TEST_F(StaticVerifierTests, Verify_BCode_Call_Fail_ParamArgRegisters_TooFew) {
    const auto f_bcode =
        yama::bc::code()
        // block #1
        .add_load_const(0, 3, true) // reinit R(0) to type of call object
        .add_load_const(1, 4, true) // reinit R(1) to type of arg #1
        .add_load_const(2, 5, true) // reinit R(2) to type of arg #2
        .add_load_const(3, 7, true) // reinit R(5) to type of return value
        .add_call(0, 3, 3) // 3 is too few args for call to g
        .add_ret(3);
    std::cerr << f_bcode.fmt_disassembly() << "\n";
    const auto f_consts =
        yama::const_table_info()
        .add_primitive_type("Int"_str)
        .add_primitive_type("Float"_str)
        .add_primitive_type("Char"_str)
        .add_function_type("g"_str, yama::make_callsig_info({ 0, 1, 0 }, 2)) // fn(Int, Float, Int) -> Char
        .add_int(10)
        .add_float(0.05)
        .add_int(-4)
        .add_char(U'y');
    yama::type_info f{
        .fullname = "f"_str,
        .consts = f_consts,
        .info = yama::function_info{
            .callsig = yama::make_callsig_info({}, 2),
            .call_fn = yama::bcode_call_fn,
            .locals = 4,
            .bcode = f_bcode,
        },
    };

    EXPECT_FALSE(verif->verify(f));
}

TEST_F(StaticVerifierTests, Verify_BCode_Call_Fail_ParamArgRegisters_WrongTypes) {
    const auto f_bcode =
        yama::bc::code()
        // block #1
        .add_load_const(0, 3, true) // reinit R(0) to type of call object
        .add_load_const(1, 4, true) // reinit R(1) to type of arg #1
        .add_load_const(2, 5, true) // reinit R(2) to type of arg #2
        .add_load_const(3, 6, true) // reinit R(3) to type of arg #3
        .add_load_const(4, 7, true) // reinit R(4) to type of return value
        .add_call(0, 4, 4) // arg #2 is UInt, but a Float was expected
        .add_ret(4);
    std::cerr << f_bcode.fmt_disassembly() << "\n";
    const auto f_consts =
        yama::const_table_info()
        .add_primitive_type("Int"_str)
        .add_primitive_type("Float"_str)
        .add_primitive_type("Char"_str)
        .add_function_type("g"_str, yama::make_callsig_info({ 0, 1, 0 }, 2)) // fn(Int, Float, Int) -> Char
        .add_int(10)
        .add_uint(1000) // <- wrong type for arg #2 for call to g
        .add_int(-4)
        .add_char(U'y');
    yama::type_info f{
        .fullname = "f"_str,
        .consts = f_consts,
        .info = yama::function_info{
            .callsig = yama::make_callsig_info({}, 2),
            .call_fn = yama::bcode_call_fn,
            .locals = 5,
            .bcode = f_bcode,
        },
    };

    EXPECT_FALSE(verif->verify(f));
}

TEST_F(StaticVerifierTests, Verify_BCode_Call_Fail_RC_OutOfBounds) {
    const auto f_bcode =
        yama::bc::code()
        // block #1
        .add_load_const(0, 3, true) // reinit R(0) to type of call object
        .add_load_const(1, 4, true) // reinit R(1) to type of arg #1
        .add_load_const(2, 5, true) // reinit R(2) to type of arg #2
        .add_load_const(3, 6, true) // reinit R(3) to type of arg #3
        .add_load_const(4, 7, true) // reinit R(4) to type of return value
        .add_call(0, 4, 5) // R(5) out-of-bounds
        .add_ret(4);
    std::cerr << f_bcode.fmt_disassembly() << "\n";
    const auto f_consts =
        yama::const_table_info()
        .add_primitive_type("Int"_str)
        .add_primitive_type("Float"_str)
        .add_primitive_type("Char"_str)
        .add_function_type("g"_str, yama::make_callsig_info({ 0, 1, 0 }, 2)) // fn(Int, Float, Int) -> Char
        .add_int(10)
        .add_float(0.05)
        .add_int(-4)
        .add_char(U'y');
    yama::type_info f{
        .fullname = "f"_str,
        .consts = f_consts,
        .info = yama::function_info{
            .callsig = yama::make_callsig_info({}, 2),
            .call_fn = yama::bcode_call_fn,
            .locals = 5,
            .bcode = f_bcode,
        },
    };

    EXPECT_FALSE(verif->verify(f));
}

TEST_F(StaticVerifierTests, Verify_BCode_Call_Fail_RC_WrongType) {
    const auto f_bcode =
        yama::bc::code()
        // block #1
        .add_load_const(0, 3, true) // reinit R(0) to type of call object
        .add_load_const(1, 4, true) // reinit R(1) to type of arg #1
        .add_load_const(2, 5, true) // reinit R(2) to type of arg #2
        .add_load_const(3, 6, true) // reinit R(3) to type of arg #3
        .add_load_const(4, 7, true) // reinit R(4) to type of return value
        .add_call(0, 4, 4) // R(4) not type Char
        .add_ret(4);
    std::cerr << f_bcode.fmt_disassembly() << "\n";
    const auto f_consts =
        yama::const_table_info()
        .add_primitive_type("Int"_str)
        .add_primitive_type("Float"_str)
        .add_primitive_type("Char"_str)
        .add_function_type("g"_str, yama::make_callsig_info({ 0, 1, 0 }, 2)) // fn(Int, Float, Int) -> Char
        .add_int(10)
        .add_float(0.05)
        .add_int(-4)
        .add_uint(1000); // <- return type of g is Char, not UInt
    yama::type_info f{
        .fullname = "f"_str,
        .consts = f_consts,
        .info = yama::function_info{
            .callsig = yama::make_callsig_info({}, 2),
            .call_fn = yama::bcode_call_fn,
            .locals = 5,
            .bcode = f_bcode,
        },
    };

    EXPECT_FALSE(verif->verify(f));
}

TEST_F(StaticVerifierTests, Verify_BCode_CallNR) {
    const auto f_bcode =
        yama::bc::code()
        // block #1
        .add_load_const(0, 3, true) // reinit R(0) to type of call object
        .add_load_const(1, 4, true) // reinit R(1) to type of arg #1
        .add_load_const(2, 5, true) // reinit R(2) to type of arg #2
        .add_load_const(3, 6, true) // reinit R(3) to type of arg #3
        .add_call_nr(0, 4)
        .add_ret(4); // return the None object from R(4)
    std::cerr << f_bcode.fmt_disassembly() << "\n";
    const auto f_consts =
        yama::const_table_info()
        .add_primitive_type("Int"_str)
        .add_primitive_type("Float"_str)
        .add_primitive_type("Char"_str)
        .add_function_type("g"_str, yama::make_callsig_info({ 0, 1, 0 }, 2)) // fn(Int, Float, Int) -> Char
        .add_int(10)
        .add_float(0.05)
        .add_int(-4)
        .add_primitive_type("None"_str);
    yama::type_info f{
        .fullname = "f"_str,
        .consts = f_consts,
        .info = yama::function_info{
            .callsig = yama::make_callsig_info({}, 7),
            .call_fn = yama::bcode_call_fn,
            .locals = 5,
            .bcode = f_bcode,
        },
    };

    EXPECT_TRUE(verif->verify(f));
}

TEST_F(StaticVerifierTests, Verify_BCode_CallNR_Fail_ArgRegisters_OutOfBounds) {
    const auto f_bcode =
        yama::bc::code()
        // block #1
        .add_load_const(0, 3, true) // reinit R(0) to type of call object
        .add_load_const(1, 4, true) // reinit R(1) to type of arg #1
        .add_load_const(2, 5, true) // reinit R(2) to type of arg #2
        .add_load_const(3, 6, true) // reinit R(3) to type of arg #3
        .add_call_nr(0, 40) // [R(0), R(39)] out-of-bounds
        .add_load_none(0, true)
        .add_ret(0);
    std::cerr << f_bcode.fmt_disassembly() << "\n";
    const auto f_consts =
        yama::const_table_info()
        .add_primitive_type("Int"_str)
        .add_primitive_type("Float"_str)
        .add_primitive_type("Char"_str)
        .add_function_type("g"_str, yama::make_callsig_info({ 0, 1, 0 }, 2)) // fn(Int, Float, Int) -> Char
        .add_int(10)
        .add_float(0.05)
        .add_int(-4)
        .add_char(U'y');
    yama::type_info f{
        .fullname = "f"_str,
        .consts = f_consts,
        .info = yama::function_info{
            .callsig = yama::make_callsig_info({}, 2),
            .call_fn = yama::bcode_call_fn,
            .locals = 5,
            .bcode = f_bcode,
        },
    };

    EXPECT_FALSE(verif->verify(f));
}

TEST_F(StaticVerifierTests, Verify_BCode_CallNR_Fail_ArgRegisters_ZeroObjects) {
    const auto f_bcode =
        yama::bc::code()
        // block #1
        // NOTE: R(A) being a valid call object ensures error couldn't be about R(A) not being one
        .add_load_const(0, 1, true) // reinit R(0) to type of call object (though this will go unused)
        .add_call_nr(0, 0) // B == 0 means no call object specified
        .add_load_none(0, true)
        .add_ret(0);
    std::cerr << f_bcode.fmt_disassembly() << "\n";
    const auto f_consts =
        yama::const_table_info()
        .add_primitive_type("None"_str)
        .add_function_type("g"_str, yama::make_callsig_info({}, 0)); // fn() -> None
    yama::type_info f{
        .fullname = "f"_str,
        .consts = f_consts,
        .info = yama::function_info{
            .callsig = yama::make_callsig_info({}, 0),
            .call_fn = yama::bcode_call_fn,
            .locals = 5,
            .bcode = f_bcode,
        },
    };

    EXPECT_FALSE(verif->verify(f));
}

TEST_F(StaticVerifierTests, Verify_BCode_CallNR_Fail_RA_IllegalToUseAsCallObject) {
    const auto f_bcode =
        yama::bc::code()
        // block #1
        .add_load_const(0, 3, true) // reinit R(0) to type of call object
        .add_load_const(1, 4, true) // reinit R(1) to type of arg #1
        .add_load_const(2, 5, true) // reinit R(2) to type of arg #2
        .add_load_const(3, 6, true) // reinit R(3) to type of arg #3
        .add_call_nr(0, 4) // R(0) cannot be used as call object
        .add_load_none(0, true)
        .add_ret(0);
    std::cerr << f_bcode.fmt_disassembly() << "\n";
    const auto f_consts =
        yama::const_table_info()
        .add_primitive_type("Int"_str)
        .add_primitive_type("Float"_str)
        .add_primitive_type("Char"_str)
        .add_int(50) // <- cannot be used as call object
        .add_int(10)
        .add_float(0.05)
        .add_int(-4)
        .add_char(U'y');
    yama::type_info f{
        .fullname = "f"_str,
        .consts = f_consts,
        .info = yama::function_info{
            .callsig = yama::make_callsig_info({}, 2),
            .call_fn = yama::bcode_call_fn,
            .locals = 5,
            .bcode = f_bcode,
        },
    };

    EXPECT_FALSE(verif->verify(f));
}

TEST_F(StaticVerifierTests, Verify_BCode_CallNR_Fail_ParamArgRegisters_TooMany) {
    const auto f_bcode =
        yama::bc::code()
        // block #1
        .add_load_const(0, 3, true) // reinit R(0) to type of call object
        .add_load_const(1, 4, true) // reinit R(1) to type of arg #1
        .add_load_const(2, 5, true) // reinit R(2) to type of arg #2
        .add_load_const(3, 6, true) // reinit R(3) to type of arg #3
        .add_load_const(4, 6, true) // reinit R(4) to type of arg #4
        .add_call_nr(0, 5) // 5 is too many args for call to g
        .add_load_none(0, true)
        .add_ret(0);
    std::cerr << f_bcode.fmt_disassembly() << "\n";
    const auto f_consts =
        yama::const_table_info()
        .add_primitive_type("Int"_str)
        .add_primitive_type("Float"_str)
        .add_primitive_type("Char"_str)
        .add_function_type("g"_str, yama::make_callsig_info({ 0, 1, 0 }, 2)) // fn(Int, Float, Int) -> Char
        .add_int(10)
        .add_float(0.05)
        .add_int(-4)
        .add_char(U'y');
    yama::type_info f{
        .fullname = "f"_str,
        .consts = f_consts,
        .info = yama::function_info{
            .callsig = yama::make_callsig_info({}, 2),
            .call_fn = yama::bcode_call_fn,
            .locals = 6,
            .bcode = f_bcode,
        },
    };

    EXPECT_FALSE(verif->verify(f));
}

TEST_F(StaticVerifierTests, Verify_BCode_CallNR_Fail_ParamArgRegisters_TooFew) {
    const auto f_bcode =
        yama::bc::code()
        // block #1
        .add_load_const(0, 3, true) // reinit R(0) to type of call object
        .add_load_const(1, 4, true) // reinit R(1) to type of arg #1
        .add_load_const(2, 5, true) // reinit R(2) to type of arg #2
        .add_call_nr(0, 3) // 3 is too few args for call to g
        .add_load_none(0, true)
        .add_ret(0);
    std::cerr << f_bcode.fmt_disassembly() << "\n";
    const auto f_consts =
        yama::const_table_info()
        .add_primitive_type("Int"_str)
        .add_primitive_type("Float"_str)
        .add_primitive_type("Char"_str)
        .add_function_type("g"_str, yama::make_callsig_info({ 0, 1, 0 }, 2)) // fn(Int, Float, Int) -> Char
        .add_int(10)
        .add_float(0.05)
        .add_int(-4)
        .add_char(U'y');
    yama::type_info f{
        .fullname = "f"_str,
        .consts = f_consts,
        .info = yama::function_info{
            .callsig = yama::make_callsig_info({}, 2),
            .call_fn = yama::bcode_call_fn,
            .locals = 4,
            .bcode = f_bcode,
        },
    };

    EXPECT_FALSE(verif->verify(f));
}

TEST_F(StaticVerifierTests, Verify_BCode_CallNR_Fail_ParamArgRegisters_WrongTypes) {
    const auto f_bcode =
        yama::bc::code()
        // block #1
        .add_load_const(0, 3, true) // reinit R(0) to type of call object
        .add_load_const(1, 4, true) // reinit R(1) to type of arg #1
        .add_load_const(2, 5, true) // reinit R(2) to type of arg #2
        .add_load_const(3, 6, true) // reinit R(3) to type of arg #3
        .add_call_nr(0, 4) // arg #2 is UInt, but a Float was expected
        .add_load_none(0, true)
        .add_ret(0);
    std::cerr << f_bcode.fmt_disassembly() << "\n";
    const auto f_consts =
        yama::const_table_info()
        .add_primitive_type("Int"_str)
        .add_primitive_type("Float"_str)
        .add_primitive_type("Char"_str)
        .add_function_type("g"_str, yama::make_callsig_info({ 0, 1, 0 }, 2)) // fn(Int, Float, Int) -> Char
        .add_int(10)
        .add_uint(1000) // <- wrong type for arg #2 for call to g
        .add_int(-4)
        .add_char(U'y');
    yama::type_info f{
        .fullname = "f"_str,
        .consts = f_consts,
        .info = yama::function_info{
            .callsig = yama::make_callsig_info({}, 2),
            .call_fn = yama::bcode_call_fn,
            .locals = 5,
            .bcode = f_bcode,
        },
    };

    EXPECT_FALSE(verif->verify(f));
}

TEST_F(StaticVerifierTests, Verify_BCode_Ret) {
    const auto f_bcode =
        yama::bc::code()
        // block #1
        .add_load_const(0, 1, true) // reinit R(0) (to Float 0.05)
        .add_ret(0); // return Float
    std::cerr << f_bcode.fmt_disassembly() << "\n";
    const auto f_consts =
        yama::const_table_info()
        .add_primitive_type("Float"_str)
        .add_float(0.05);
    yama::type_info f{
        .fullname = "f"_str,
        .consts = f_consts,
        .info = yama::function_info{
            .callsig = yama::make_callsig_info({}, 0),
            .call_fn = yama::bcode_call_fn,
            .locals = 1,
            .bcode = f_bcode,
        },
    };

    EXPECT_TRUE(verif->verify(f));
}

TEST_F(StaticVerifierTests, Verify_BCode_Ret_Fail_RA_OutOfBounds) {
    const auto f_bcode =
        yama::bc::code()
        // block #1
        .add_ret(1); // R(1) out-of-bounds
    std::cerr << f_bcode.fmt_disassembly() << "\n";
    const auto f_consts =
        yama::const_table_info()
        .add_primitive_type("None"_str);
    yama::type_info f{
        .fullname = "f"_str,
        .consts = f_consts,
        .info = yama::function_info{
            .callsig = yama::make_callsig_info({}, 0),
            .call_fn = yama::bcode_call_fn,
            .locals = 1,
            .bcode = f_bcode,
        },
    };

    EXPECT_FALSE(verif->verify(f));
}

TEST_F(StaticVerifierTests, Verify_BCode_Ret_Fail_RA_WrongType) {
    const auto f_bcode =
        yama::bc::code()
        // block #1
        .add_load_const(0, 1, true) // reinit R(0) (to Int 10)
        .add_ret(0); // R(0) not type Float
    std::cerr << f_bcode.fmt_disassembly() << "\n";
    const auto f_consts =
        yama::const_table_info()
        .add_primitive_type("Float"_str)
        .add_int(10);
    yama::type_info f{
        .fullname = "f"_str,
        .consts = f_consts,
        .info = yama::function_info{
            .callsig = yama::make_callsig_info({}, 0),
            .call_fn = yama::bcode_call_fn,
            .locals = 1,
            .bcode = f_bcode,
        },
    };

    EXPECT_FALSE(verif->verify(f));
}

TEST_F(StaticVerifierTests, Verify_BCode_Jump) {
    const auto f_bcode =
        yama::bc::code()
        // block #1
        .add_jump(2) // jump to block #3
        // block #2 (dead code)
        // this dead code block ensures that we actually *jump over* block #2
        .add_load_const(0, 1, true) // reinit R(0) (to Int 10)
        .add_ret(0)
        // block #3
        .add_load_const(0, 2, true) // reinit R(0) (to Float 0.05)
        .add_ret(0);
    std::cerr << f_bcode.fmt_disassembly() << "\n";
    const auto f_consts =
        yama::const_table_info()
        .add_primitive_type("Float"_str)
        .add_int(10)
        .add_float(0.05);
    yama::type_info f{
        .fullname = "f"_str,
        .consts = f_consts,
        .info = yama::function_info{
            .callsig = yama::make_callsig_info({}, 0),
            .call_fn = yama::bcode_call_fn,
            .locals = 1,
            .bcode = f_bcode,
        },
    };

    EXPECT_TRUE(verif->verify(f));
}

TEST_F(StaticVerifierTests, Verify_BCode_Jump_Fail_PutsPCOutOfBounds) {
    const auto f_bcode =
        yama::bc::code()
        // block #1
        .add_jump(1); // jump to out-of-bounds instruction index
    std::cerr << f_bcode.fmt_disassembly() << "\n";
    const auto f_consts =
        yama::const_table_info()
        .add_primitive_type("None"_str);
    yama::type_info f{
        .fullname = "f"_str,
        .consts = f_consts,
        .info = yama::function_info{
            .callsig = yama::make_callsig_info({}, 0),
            .call_fn = yama::bcode_call_fn,
            .locals = 1,
            .bcode = f_bcode,
        },
    };

    EXPECT_FALSE(verif->verify(f));
}

TEST_F(StaticVerifierTests, Verify_BCode_Jump_Fail_ViolatesRegisterCoherence) {
    const auto f_bcode =
        yama::bc::code()
        // block #1
        .add_load_const(0, 1, true) // reinits R(0) (to Bool true)
        .add_jump_if(0, 2)
        // block #2
        .add_load_const(0, 2, true) // reinits R(0) (to Float 3.14159)
        .add_jump(2) // merge R(0) into block #4 as Float
        // block #3
        .add_load_const(0, 3, true) // reinits R(0) (to Int 100)
        .add_jump(0) // merge R(0) into block #4 as Int (violating register coherence w/ above R(0) being Float)
        // block #5
        .add_load_none(0, true) // reinits R(0) (to None)
        .add_ret(0); // return None object
    std::cerr << f_bcode.fmt_disassembly() << "\n";
    const auto f_consts =
        yama::const_table_info()
        .add_primitive_type("None"_str)
        .add_bool(true)
        .add_float(3.14159)
        .add_int(100);
    yama::type_info f{
        .fullname = "f"_str,
        .consts = f_consts,
        .info = yama::function_info{
            .callsig = yama::make_callsig_info({}, 0),
            .call_fn = yama::bcode_call_fn,
            .locals = 1,
            .bcode = f_bcode,
        },
    };

    EXPECT_FALSE(verif->verify(f));
}

TEST_F(StaticVerifierTests, Verify_BCode_JumpIf) {
    const auto f_bcode =
        yama::bc::code()
        // block #1
        .add_load_const(0, 1, true) // reinit R(0) (to Bool true)
        .add_jump_if(0, 2) // jump to block #3, or fallthrough to block #2
        // block #2
        .add_load_const(0, 2, true) // reinit R(0) (to Float 10.5)
        .add_ret(0)
        // block #3
        .add_load_const(0, 3, true) // reinit R(0) (to Float 0.05)
        .add_ret(0);
    std::cerr << f_bcode.fmt_disassembly() << "\n";
    const auto f_consts =
        yama::const_table_info()
        .add_primitive_type("Float"_str)
        .add_bool(true)
        .add_float(10.5)
        .add_float(0.05);
    yama::type_info f{
        .fullname = "f"_str,
        .consts = f_consts,
        .info = yama::function_info{
            .callsig = yama::make_callsig_info({}, 0),
            .call_fn = yama::bcode_call_fn,
            .locals = 1,
            .bcode = f_bcode,
        },
    };

    EXPECT_TRUE(verif->verify(f));
}

TEST_F(StaticVerifierTests, Verify_BCode_JumpIf_Fail_RA_OutOfBounds) {
    const auto f_bcode =
        yama::bc::code()
        // block #1
        .add_load_const(0, 1, true) // reinit R(0) (to Bool true)
        .add_jump_if(1, 2) // R(1) out-of-bounds
        // block #2
        .add_load_const(0, 2, true) // reinit R(0) (to Int 10)
        .add_ret(0)
        // block #3
        .add_load_const(0, 3, true) // reinit R(0) (to Float 0.05)
        .add_ret(0);
    std::cerr << f_bcode.fmt_disassembly() << "\n";
    const auto f_consts =
        yama::const_table_info()
        .add_primitive_type("Float"_str)
        .add_bool(true)
        .add_int(10)
        .add_float(0.05);
    yama::type_info f{
        .fullname = "f"_str,
        .consts = f_consts,
        .info = yama::function_info{
            .callsig = yama::make_callsig_info({}, 0),
            .call_fn = yama::bcode_call_fn,
            .locals = 1,
            .bcode = f_bcode,
        },
    };

    EXPECT_FALSE(verif->verify(f));
}

TEST_F(StaticVerifierTests, Verify_BCode_JumpIf_Fail_RA_WrongType) {
    const auto f_bcode =
        yama::bc::code()
        // block #1
        .add_load_const(0, 1, true) // reinit R(0) (to Int -4)
        .add_jump_if(0, 2) // R(0) not type Bool
        // block #2
        .add_load_const(0, 2, true) // reinit R(0) (to Int 10)
        .add_ret(0)
        // block #3
        .add_load_const(0, 3, true) // reinit R(0) (to Float 0.05)
        .add_ret(0);
    std::cerr << f_bcode.fmt_disassembly() << "\n";
    const auto f_consts =
        yama::const_table_info()
        .add_primitive_type("Float"_str)
        .add_int(-4)
        .add_int(10)
        .add_float(0.05);
    yama::type_info f{
        .fullname = "f"_str,
        .consts = f_consts,
        .info = yama::function_info{
            .callsig = yama::make_callsig_info({}, 0),
            .call_fn = yama::bcode_call_fn,
            .locals = 1,
            .bcode = f_bcode,
        },
    };

    EXPECT_FALSE(verif->verify(f));
}

TEST_F(StaticVerifierTests, Verify_BCode_JumpIf_Fail_PutsPCOutOfBounds) {
    const auto f_bcode =
        yama::bc::code()
        // block #1
        .add_load_const(0, 1, true) // reinit R(0) (to Bool true)
        .add_jump_if(0, 10'000) // branches to out-of-bounds instruction
        // block #2
        .add_load_const(0, 2, true) // reinit R(0) (to Int 10)
        .add_ret(0)
        // block #3
        .add_load_const(0, 3, true) // reinit R(0) (to Float 0.05)
        .add_ret(0);
    std::cerr << f_bcode.fmt_disassembly() << "\n";
    const auto f_consts =
        yama::const_table_info()
        .add_primitive_type("Float"_str)
        .add_bool(true)
        .add_int(10)
        .add_float(0.05);
    yama::type_info f{
        .fullname = "f"_str,
        .consts = f_consts,
        .info = yama::function_info{
            .callsig = yama::make_callsig_info({}, 0),
            .call_fn = yama::bcode_call_fn,
            .locals = 1,
            .bcode = f_bcode,
        },
    };

    EXPECT_FALSE(verif->verify(f));
}

TEST_F(StaticVerifierTests, Verify_BCode_JumpIf_Fail_ViolatesRegisterCoherence) {
    const auto f_bcode =
        yama::bc::code()
        // block #1
        .add_load_const(1, 1, true) // reinits R(1) (to Bool true)
        .add_jump_if(1, 4)
        // block #2
        .add_load_const(0, 2, true) // reinits R(0) (to Float 3.14159)
        .add_jump_if(1, 6) // merge R(0) into block #6 as Float
        // block #3
        // this block is here just to provide a place to fallthrough to
        .add_load_none(0, true)
        .add_ret(0)
        // block #4
        .add_load_const(0, 3, true) // reinits R(0) (to Int 100)
        .add_jump_if(1, 2) // merge R(0) into block #6 as Int (violating register coherence w/ above R(0) being Float)
        // block #5
        // this block is here just to provide a place to fallthrough to
        .add_load_none(0, true)
        .add_ret(0)
        // block #6
        .add_load_none(0, true) // reinits R(0) (to None)
        .add_ret(0); // return None object
    std::cerr << f_bcode.fmt_disassembly() << "\n";
    const auto f_consts =
        yama::const_table_info()
        .add_primitive_type("None"_str)
        .add_bool(true)
        .add_float(3.14159)
        .add_int(100);
    yama::type_info f{
        .fullname = "f"_str,
        .consts = f_consts,
        .info = yama::function_info{
            .callsig = yama::make_callsig_info({}, 0),
            .call_fn = yama::bcode_call_fn,
            .locals = 2,
            .bcode = f_bcode,
        },
    };

    EXPECT_FALSE(verif->verify(f));
}

