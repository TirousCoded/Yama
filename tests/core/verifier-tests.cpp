

#include <gtest/gtest.h>

#include <optional>

#include <yama/core/general.h>
#include <yama/core/res.h>
#include <yama/core/type_info.h>
#include <yama/core/module_info.h>
#include <yama/core/verifier.h>


using namespace yama::string_literals;


class VerifierTests : public testing::Test {
public:
    std::shared_ptr<yama::dsignal_debug> dbg;
    std::shared_ptr<yama::verifier> verif;

    std::optional<yama::parcel_metadata> md;

    const yama::parcel_metadata& get_md() { return yama::deref_assert(md); }


protected:
    void SetUp() override final {
        dbg = std::make_shared<yama::dsignal_debug>(std::make_shared<yama::stderr_debug>());
        verif = yama::make_res<yama::verifier>(dbg);

        // map names 'yama' and 'abc' to arbitrary parcel IDs
        md = yama::parcel_metadata{ yama::str::lit("self"), { yama::str::lit("yama"), yama::str::lit("abc") } };
    }

    void TearDown() override final {
        //
    }
};


// IMPORTANT:
//      our policy is gonna be to only testing a single error per unit test, not testing
//      multiple different errors arising in one unit test
//
//      the reason is so that the impl is free to forgo work checking for other errors
//      after one is found, nor any complexity about certain errors having to be grouped
//      together w/ others
//
//      one error may correspond to multiple dsignal raises


// TYPE-LEVEL

TEST_F(VerifierTests, TypeLevel_General) {
    // this is just a general test of successful usage of verifier

    const auto consts =
        yama::const_table_info()
        // test w/ each constant type (w/ static verif stuff)
        .add_primitive_type("abc:a"_str)
        .add_function_type("abc:b"_str, yama::make_callsig({ 0 }, 3)) // ie. 'fn(a) -> d'
        .add_method_type("abc:a::c"_str, yama::make_callsig({ 0 }, 3)) // ie. 'fn(a) -> d'
        .add_struct_type("abc:d"_str);
    // test w/ each kind of type
    static_assert(yama::kinds == 4); // reminder
    const auto a = yama::make_primitive(
        "a"_str,
        consts,
        yama::ptype::bool0);
    const auto b = yama::make_function(
        "b"_str,
        consts,
        yama::make_callsig({ 0, 1, 3 }, 0),
        4,
        yama::noop_call_fn);
    const auto c = yama::make_method(
        "a::c"_str,
        consts,
        yama::make_callsig({ 0, 1, 3 }, 0),
        4,
        yama::noop_call_fn);
    const auto d = yama::make_struct(
        "d"_str,
        consts);

    EXPECT_TRUE(verif->verify(a, get_md(), "abc"_str));
    EXPECT_TRUE(verif->verify(b, get_md(), "abc"_str));
    EXPECT_TRUE(verif->verify(c, get_md(), "abc"_str));
    EXPECT_TRUE(verif->verify(d, get_md(), "abc"_str));
}

// TODO: I find the below mixing of tests for 'type itself' and for 'constsym' to be a bit confusing

TEST_F(VerifierTests, TypeLevel_TypeItself_Fail_InvalidUnqualifiedName_NonMemberTypeButHasOwnerPrefix) {
    const auto a = yama::make_primitive(
        "a::m"_str, // <- invalid! has owner prefix!
        {},
        yama::ptype::bool0);

    EXPECT_FALSE(verif->verify(a, get_md(), "abc"_str));

    EXPECT_EQ(dbg->count(yama::dsignal::verif_type_unqualified_name_invalid), 1);
}

TEST_F(VerifierTests, TypeLevel_TypeItself_Fail_InvalidUnqualifiedName_MemberTypeButHasNoOwnerPrefix) {
    const auto a_consts =
        yama::const_table_info()
        .add_primitive_type("abc:a"_str);
    const auto a = yama::make_method(
        "a"_str, // <- invalid! has no owner prefix!
        a_consts,
        yama::make_callsig({}, 0),
        10,
        yama::noop_call_fn);

    EXPECT_FALSE(verif->verify(a, get_md(), "abc"_str));

    EXPECT_EQ(dbg->count(yama::dsignal::verif_type_unqualified_name_invalid), 1);
}

TEST_F(VerifierTests, TypeLevel_ConstSym_Fail_InvalidQualifiedName_Malformed) {
    const auto a_consts =
        yama::const_table_info()
        .add_primitive_type("b"_str); // <- invalid! malformed qualified name
    const auto a = yama::make_function(
        "a"_str,
        a_consts,
        yama::make_callsig({}, 0),
        4,
        yama::noop_call_fn);

    EXPECT_FALSE(verif->verify(a, get_md(), "abc"_str));

    EXPECT_EQ(dbg->count(yama::dsignal::verif_constsym_qualified_name_invalid), 1);
}

TEST_F(VerifierTests, TypeLevel_ConstSym_Fail_InvalidQualifiedName_HeadNamesParcelNotSelfOrDepName) {
    const auto a_consts =
        yama::const_table_info()
        .add_primitive_type("missing:b"_str); // <- invalid! no parcel named 'missing' in self/dep names
    const auto a = yama::make_function(
        "a"_str,
        a_consts,
        yama::make_callsig({}, 0),
        4,
        yama::noop_call_fn);

    EXPECT_FALSE(verif->verify(a, get_md(), "abc"_str));

    EXPECT_EQ(dbg->count(yama::dsignal::verif_constsym_qualified_name_invalid), 1);
}

TEST_F(VerifierTests, TypeLevel_ConstSym_Fail_InvalidQualifiedName_NonMemberTypeButHasOwnerPrefix) {
    const auto a_consts =
        yama::const_table_info()
        .add_primitive_type("abc:owner::b"_str); // <- invalid! primitive types cannot have 'owner::' prefix!
    const auto a = yama::make_function(
        "a"_str,
        a_consts,
        yama::make_callsig({}, 0),
        4,
        yama::noop_call_fn);

    EXPECT_FALSE(verif->verify(a, get_md(), "abc"_str));

    EXPECT_EQ(dbg->count(yama::dsignal::verif_constsym_qualified_name_invalid), 1);
}

TEST_F(VerifierTests, TypeLevel_ConstSym_Fail_InvalidQualifiedName_MemberTypeButHasNoOwnerPrefix) {
    const auto a_consts =
        yama::const_table_info()
        .add_primitive_type("abc:a"_str)
        .add_method_type("abc:b"_str, yama::make_callsig({}, 0)); // <- invalid! method types require '<owner>::' prefix!
    const auto a = yama::make_function(
        "a"_str,
        a_consts,
        yama::make_callsig({}, 0),
        4,
        yama::noop_call_fn);

    EXPECT_FALSE(verif->verify(a, get_md(), "abc"_str));

    EXPECT_EQ(dbg->count(yama::dsignal::verif_constsym_qualified_name_invalid), 1);
}

TEST_F(VerifierTests, TypeLevel_TypeItself_Fail_CallSigConstIndexOutOfBounds_ParamType) {
    const auto a_consts =
        yama::const_table_info()
        .add_primitive_type("abc:b"_str);
    const auto a = yama::make_function(
        "a"_str,
        a_consts,
        // illegal out-of-bounds constant index (for param type of a)
        yama::make_callsig({ 1 }, 0),
        4,
        yama::noop_call_fn);

    EXPECT_FALSE(verif->verify(a, get_md(), "abc"_str));

    EXPECT_EQ(dbg->count(yama::dsignal::verif_type_callsig_invalid), 1);
    EXPECT_EQ(dbg->count(yama::dsignal::verif_callsig_param_type_out_of_bounds), 1);
}

TEST_F(VerifierTests, TypeLevel_ConstSym_Fail_CallSigConstIndexOutOfBounds_ParamType) {
    const auto a_consts =
        yama::const_table_info()
        // illegal out-of-bounds constant index (for param type of b)
        .add_function_type("abc:b"_str, yama::make_callsig({ 2 }, 1))
        .add_primitive_type("abc:c"_str);
    const auto a = yama::make_primitive(
        "a"_str,
        a_consts,
        yama::ptype::bool0);

    EXPECT_FALSE(verif->verify(a, get_md(), "abc"_str));

    EXPECT_EQ(dbg->count(yama::dsignal::verif_constsym_callsig_invalid), 1);
    EXPECT_EQ(dbg->count(yama::dsignal::verif_callsig_param_type_out_of_bounds), 1);
}

TEST_F(VerifierTests, TypeLevel_TypeItself_Fail_CallSigConstIndexOutOfBounds_ReturnType) {
    const auto a_consts =
        yama::const_table_info()
        .add_primitive_type("abc:b"_str);
    const auto a = yama::make_function(
        "a"_str,
        a_consts,
        // illegal out-of-bounds constant index (for return type of a)
        yama::make_callsig({}, 1),
        4,
        yama::noop_call_fn);

    EXPECT_FALSE(verif->verify(a, get_md(), "abc"_str));

    EXPECT_EQ(dbg->count(yama::dsignal::verif_type_callsig_invalid), 1);
    EXPECT_EQ(dbg->count(yama::dsignal::verif_callsig_return_type_out_of_bounds), 1);
}

TEST_F(VerifierTests, TypeLevel_ConstSym_Fail_CallSigConstIndexOutOfBounds_ReturnType) {
    const auto a_consts =
        yama::const_table_info()
        // illegal out-of-bounds constant index (for return type of b)
        .add_function_type("abc:b"_str, yama::make_callsig({}, 2))
        .add_primitive_type("abc:c"_str);
    const auto a = yama::make_primitive(
        "a"_str,
        a_consts,
        yama::ptype::bool0);

    EXPECT_FALSE(verif->verify(a, get_md(), "abc"_str));

    EXPECT_EQ(dbg->count(yama::dsignal::verif_constsym_callsig_invalid), 1);
    EXPECT_EQ(dbg->count(yama::dsignal::verif_callsig_return_type_out_of_bounds), 1);
}

TEST_F(VerifierTests, TypeLevel_TypeItself_Fail_CallSigConstNotATypeConst_ParamType) {
    const auto a_consts =
        yama::const_table_info()
        .add_primitive_type("abc:b"_str)
        .add_int(10);
    const auto a = yama::make_function(
        "a"_str,
        a_consts,
        // illegal use of non-type constant index (for param type of a)
        yama::make_callsig({ 1 }, 0),
        4,
        yama::noop_call_fn);

    EXPECT_FALSE(verif->verify(a, get_md(), "abc"_str));

    EXPECT_EQ(dbg->count(yama::dsignal::verif_type_callsig_invalid), 1);
    EXPECT_EQ(dbg->count(yama::dsignal::verif_callsig_param_type_not_type_const), 1);
}

TEST_F(VerifierTests, TypeLevel_ConstSym_Fail_CallSigConstNotATypeConst_ParamType) {
    const auto a_consts =
        yama::const_table_info()
        // illegal use of non-type constant index (for param type of b)
        .add_function_type("abc:b"_str, yama::make_callsig({ 2 }, 1))
        .add_primitive_type("abc:c"_str)
        .add_int(10);
    const auto a = yama::make_primitive(
        "a"_str,
        a_consts,
        yama::ptype::bool0);

    EXPECT_FALSE(verif->verify(a, get_md(), "abc"_str));

    EXPECT_EQ(dbg->count(yama::dsignal::verif_constsym_callsig_invalid), 1);
    EXPECT_EQ(dbg->count(yama::dsignal::verif_callsig_param_type_not_type_const), 1);
}

TEST_F(VerifierTests, TypeLevel_TypeItself_Fail_CallSigConstNotATypeConst_ReturnType) {
    const auto a_consts =
        yama::const_table_info()
        .add_primitive_type("abc:b"_str)
        .add_int(10);
    const auto a = yama::make_function(
        "a"_str,
        a_consts,
        // illegal use of non-type constant index (for return type of a)
        yama::make_callsig({}, 1),
        4,
        yama::noop_call_fn);

    EXPECT_FALSE(verif->verify(a, get_md(), "abc"_str));

    EXPECT_EQ(dbg->count(yama::dsignal::verif_type_callsig_invalid), 1);
    EXPECT_EQ(dbg->count(yama::dsignal::verif_callsig_return_type_not_type_const), 1);
}

TEST_F(VerifierTests, TypeLevel_ConstSym_Fail_CallSigConstNotATypeConst_ReturnType) {
    const auto a_consts =
        yama::const_table_info()
        // illegal use of non-type constant index (for return type of b)
        .add_function_type("abc:b"_str, yama::make_callsig({}, 2))
        .add_primitive_type("abc:c"_str)
        .add_int(10);
    const auto a = yama::make_primitive(
        "a"_str,
        a_consts,
        yama::ptype::bool0);

    EXPECT_FALSE(verif->verify(a, get_md(), "abc"_str));

    EXPECT_EQ(dbg->count(yama::dsignal::verif_constsym_callsig_invalid), 1);
    EXPECT_EQ(dbg->count(yama::dsignal::verif_callsig_return_type_not_type_const), 1);
}


// BYTECODE

TEST_F(VerifierTests, BCode_DisregardBCodeIfCallFnIsNotBCodeCallFn) {
    const auto f_bcode =
        yama::bc::code(); // otherwise illegal empty bcode
    yama::println("{}", f_bcode.fmt_disassembly());
    const auto f_consts =
        yama::const_table_info()
        .add_primitive_type("yama:None"_str);
    auto f = yama::make_function(
        "f"_str,
        f_consts,
        yama::make_callsig({}, 0),
        1,
        yama::noop_call_fn); // <- NOT bcode_call_fn
    f.change_bcode(f_bcode); // <- should be disregarded

    EXPECT_TRUE(verif->verify(f, get_md(), "abc"_str));
}

TEST_F(VerifierTests, BCode_TolerateDeadCode) {
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
        .add_put_none(yama::newtop)
        .add_ret(0);
    yama::println("{}", f_bcode.fmt_disassembly());
    const auto f_consts =
        yama::const_table_info()
        .add_primitive_type("yama:None"_str);
    const auto f = yama::make_function(
        "f"_str,
        f_consts,
        yama::make_callsig({}, 0),
        1,
        f_bcode);

    EXPECT_TRUE(verif->verify(f, get_md(), "abc"_str));
}

TEST_F(VerifierTests, BCode_TolerateAllControlPathsBeingCyclicalAndThusNoExitpoints) {
    const auto f_bcode =
        yama::bc::code()
        // block #1
        .add_noop()
        .add_noop()
        .add_noop()
        .add_jump(-4);
    yama::println("{}", f_bcode.fmt_disassembly());
    const auto f_consts =
        yama::const_table_info()
        .add_primitive_type("yama:None"_str);
    const auto f = yama::make_function(
        "f"_str,
        f_consts,
        yama::make_callsig({}, 0),
        1,
        f_bcode);

    EXPECT_TRUE(verif->verify(f, get_md(), "abc"_str));
}

TEST_F(VerifierTests, BCode_RegisterReinitsOverwriteOneAnother) {
    const auto f_bcode =
        yama::bc::code()
        // block #1
        .add_put_none(yama::newtop)
        .add_put_const(0, 1, true) // reinits R(0) (to Int -4)
        .add_put_const(0, 2, true) // reinits R(0) (to Char 'y')
        .add_put_const(0, 3, true) // reinits R(0) (to Float 0.05)
        .add_ret(0);
    yama::println("{}", f_bcode.fmt_disassembly());
    const auto f_consts =
        yama::const_table_info()
        .add_primitive_type("yama:Float"_str)
        .add_int(-4)
        .add_char(U'y')
        .add_float(0.05);
    const auto f = yama::make_function(
        "f"_str,
        f_consts,
        yama::make_callsig({}, 0),
        1,
        f_bcode);

    EXPECT_TRUE(verif->verify(f, get_md(), "abc"_str));
}

// IMPORTANT: these Verify_BCode_[No]Branch[_#] tests must be sure to also test that
//            register coherence allows for the reasonable changing of register types,
//            and that this extends to merging control paths

TEST_F(VerifierTests, BCode_NoBranch) {
    const auto f_bcode =
        yama::bc::code()
        // block #1
        .add_put_none(yama::newtop) // inits R(0) (to None)
        .add_noop()
        .add_put_const(yama::newtop, 1) // inits R(1) (to Float 3.14159)
        .add_noop()
        .add_ret(1);
    yama::println("{}", f_bcode.fmt_disassembly());
    const auto f_consts =
        yama::const_table_info()
        .add_primitive_type("yama:Float"_str)
        .add_float(3.14159);
    const auto f = yama::make_function(
        "f"_str,
        f_consts,
        yama::make_callsig({}, 0),
        2,
        f_bcode);

    EXPECT_TRUE(verif->verify(f, get_md(), "abc"_str));
}

TEST_F(VerifierTests, BCode_Branch_MultipleExitpoints) {
    const auto f_bcode =
        yama::bc::code()
        // block #1
        .add_put_const(yama::newtop, 3) // inits R(0) (to Bool true)
        .add_jump_true(1, 2)
        // block #2 (exitpoint)
        .add_put_const(yama::newtop, 1) // inits R(0) (to Float 3.14159)
        .add_ret(0)
        // block #3 (exitpoint)
        .add_put_const(yama::newtop, 2) // inits R(0) (to Float 0.05)
        .add_ret(0);
    yama::println("{}", f_bcode.fmt_disassembly());
    const auto f_consts =
        yama::const_table_info()
        .add_primitive_type("yama:Float"_str)
        .add_float(3.14159)
        .add_float(0.05)
        .add_bool(true);
    const auto f = yama::make_function(
        "f"_str,
        f_consts,
        yama::make_callsig({}, 0),
        1,
        f_bcode);

    EXPECT_TRUE(verif->verify(f, get_md(), "abc"_str));
}

TEST_F(VerifierTests, BCode_Branch_ControlPathsMerge) {
    const auto f_bcode =
        yama::bc::code()
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
    yama::println("{}", f_bcode.fmt_disassembly());
    const auto f_consts =
        yama::const_table_info()
        .add_primitive_type("yama:Float"_str)
        .add_float(3.14159)
        .add_float(0.05)
        .add_bool(true);
    const auto f = yama::make_function(
        "f"_str,
        f_consts,
        yama::make_callsig({}, 0),
        2,
        f_bcode);

    EXPECT_TRUE(verif->verify(f, get_md(), "abc"_str));
}

TEST_F(VerifierTests, BCode_Branch_CyclicalControlPath) {
    const auto f_bcode =
        yama::bc::code()
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
    yama::println("{}", f_bcode.fmt_disassembly());
    const auto f_consts =
        yama::const_table_info()
        .add_primitive_type("yama:Float"_str)
        .add_int(10)
        .add_int(-4)
        .add_float(0.05)
        .add_bool(true)
        .add_int(100);
    const auto f = yama::make_function(
        "f"_str,
        f_consts,
        yama::make_callsig({}, 0),
        2,
        f_bcode);

    EXPECT_TRUE(verif->verify(f, get_md(), "abc"_str));
}

TEST_F(VerifierTests, BCode_Branch_FallthroughDueToLastInstrOfBlockNotBeingBranchOrExitpoint) {
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
        .add_put_arg(yama::newtop, 100, true) // <- error!
        .add_jump(-3);
    yama::println("{}", f_bcode.fmt_disassembly());
    const auto f_consts =
        yama::const_table_info()
        .add_primitive_type("yama:None"_str);
    const auto f = yama::make_function(
        "f"_str,
        f_consts,
        yama::make_callsig({}, 0),
        1,
        f_bcode);

    EXPECT_FALSE(verif->verify(f, get_md(), "abc"_str));
}

TEST_F(VerifierTests, BCode_Fail_BinaryIsEmpty) {
    const auto f_bcode =
        yama::bc::code();
    yama::println("{}", f_bcode.fmt_disassembly());
    const auto f_consts =
        yama::const_table_info()
        .add_primitive_type("yama:Float"_str);
    const auto f = yama::make_function(
        "f"_str,
        f_consts,
        yama::make_callsig({}, 0),
        1,
        f_bcode);

    EXPECT_FALSE(verif->verify(f, get_md(), "abc"_str));

    EXPECT_EQ(dbg->count(yama::dsignal::verif_binary_is_empty), 1);
}

TEST_F(VerifierTests, BCode_Fail_FinalBlockFallthroughToOutOfBoundsInstrs) {
    const auto f_bcode =
        yama::bc::code()
        // block #1
        .add_put_const(yama::newtop, 1) // inits R(0) (to Bool true)
        // block #2
        .add_noop() // jump destination (so R(0) is always Bool)
        .add_noop()
        // we're using jump_if to test more nuanced scenario than just a simple fallthrough due to no branch or exitpoint
        .add_jump_true(1, -3); // illegal fallthrough to out-of-bounds instruction
    yama::println("{}", f_bcode.fmt_disassembly());
    const auto f_consts =
        yama::const_table_info()
        .add_primitive_type("yama:Bool"_str)
        .add_bool(true);
    const auto f = yama::make_function(
        "f"_str,
        f_consts,
        yama::make_callsig({}, 0),
        1,
        f_bcode);

    EXPECT_FALSE(verif->verify(f, get_md(), "abc"_str));

    EXPECT_EQ(dbg->count(yama::dsignal::verif_fallthrough_puts_PC_out_of_bounds), 1);
}

static_assert(yama::bc::opcodes == 14);

// TODO: is there a reason to have both #_RA_IsNewtopButPushingOverflows and
//       #_PushingMustNotOverflow tests? or is this unneeded duplication?

TEST_F(VerifierTests, BCode_Noop) {
    const auto f_bcode =
        yama::bc::code()
        // block #1
        .add_noop()
        .add_noop()
        .add_noop()
        .add_put_none(yama::newtop)
        .add_ret(0);
    yama::println("{}", f_bcode.fmt_disassembly());
    const auto f_consts =
        yama::const_table_info()
        .add_primitive_type("yama:None"_str);
    const auto f = yama::make_function(
        "f"_str,
        f_consts,
        yama::make_callsig({}, 0),
        1,
        f_bcode);

    EXPECT_TRUE(verif->verify(f, get_md(), "abc"_str));
}

TEST_F(VerifierTests, BCode_Pop) {
    const auto f_bcode =
        yama::bc::code()
        // block #1
        .add_put_none(yama::newtop) // inits R(0) (to None)
        .add_pop(1) // pops R(0)
        .add_put_const(yama::newtop, 1) // inits R(0) (to Int 31)
        .add_ret(0);
    yama::println("{}", f_bcode.fmt_disassembly());
    const auto f_consts =
        yama::const_table_info()
        .add_primitive_type("yama:Int"_str)
        .add_int(31);
    const auto f = yama::make_function(
        "f"_str,
        f_consts,
        yama::make_callsig({}, 0),
        1,
        f_bcode);

    EXPECT_TRUE(verif->verify(f, get_md(), "abc"_str));
}

TEST_F(VerifierTests, BCode_Pop_Zero) {
    const auto f_bcode =
        yama::bc::code()
        // block #1
        .add_put_const(yama::newtop, 1) // inits R(0) (to Int 31)
        .add_pop(0) // pops *nothing*
        .add_ret(0);
    yama::println("{}", f_bcode.fmt_disassembly());
    const auto f_consts =
        yama::const_table_info()
        .add_primitive_type("yama:Int"_str)
        .add_int(31);
    const auto f = yama::make_function(
        "f"_str,
        f_consts,
        yama::make_callsig({}, 0),
        1,
        f_bcode);

    EXPECT_TRUE(verif->verify(f, get_md(), "abc"_str));
}

TEST_F(VerifierTests, BCode_Pop_Many) {
    const auto f_bcode =
        yama::bc::code()
        // block #1
        .add_put_none(yama::newtop) // inits R(0) (to None)
        .add_put_none(yama::newtop) // inits R(1) (to None)
        .add_put_none(yama::newtop) // inits R(2) (to None)
        .add_put_none(yama::newtop) // inits R(3) (to None)
        .add_put_none(yama::newtop) // inits R(4) (to None)
        .add_pop(5) // pops R(0), R(1), ..., R(4)
        .add_put_const(yama::newtop, 1) // inits R(0) (to Int 31)
        .add_ret(0);
    yama::println("{}", f_bcode.fmt_disassembly());
    const auto f_consts =
        yama::const_table_info()
        .add_primitive_type("yama:Int"_str)
        .add_int(31);
    const auto f = yama::make_function(
        "f"_str,
        f_consts,
        yama::make_callsig({}, 0),
        5,
        f_bcode);

    EXPECT_TRUE(verif->verify(f, get_md(), "abc"_str));
}

TEST_F(VerifierTests, BCode_Pop_MoreThanAreOnStack) {
    const auto f_bcode =
        yama::bc::code()
        // block #1
        .add_put_none(yama::newtop) // inits R(0) (to None)
        .add_pop(100) // pops R(0) (and more if there were any)
        .add_put_const(yama::newtop, 1) // inits R(0) (to Int 31)
        .add_ret(0);
    yama::println("{}", f_bcode.fmt_disassembly());
    const auto f_consts =
        yama::const_table_info()
        .add_primitive_type("yama:Int"_str)
        .add_int(31);
    const auto f = yama::make_function(
        "f"_str,
        f_consts,
        yama::make_callsig({}, 0),
        1,
        f_bcode);

    EXPECT_TRUE(verif->verify(f, get_md(), "abc"_str));
}

TEST_F(VerifierTests, BCode_PutNone) {

    const auto f_bcode =
        yama::bc::code()
        // block #1
        .add_put_const(yama::newtop, 1) // helper
        .add_call(1, yama::newtop) // call helper, putting None into R(0)
        .add_put_none(0) // no reinit, so R(0) MUST be None already
        .add_ret(0);
    yama::println("{}", f_bcode.fmt_disassembly());
    const auto f_consts =
        yama::const_table_info()
        .add_primitive_type("yama:None"_str)
        // quick-n'-dirty fn to let us push None w/out put_none
        .add_function_type("abc:helper"_str, yama::make_callsig({}, 0));
    const auto f = yama::make_function(
        "f"_str,
        f_consts,
        yama::make_callsig({}, 0),
        1,
        f_bcode);

    EXPECT_TRUE(verif->verify(f, get_md(), "abc"_str));
}

TEST_F(VerifierTests, BCode_PutNone_Reinit) {
    const auto f_bcode =
        yama::bc::code()
        // block #1
        .add_put_const(yama::newtop, 1) // inits R(0) (to Int 10)
        .add_put_none(0, true) // reinits R(0) (to None) via index
        .add_ret(0);
    yama::println("{}", f_bcode.fmt_disassembly());
    const auto f_consts =
        yama::const_table_info()
        .add_primitive_type("yama:None"_str)
        .add_int(10);
    const auto f = yama::make_function(
        "f"_str,
        f_consts,
        yama::make_callsig({}, 0),
        1,
        f_bcode);

    EXPECT_TRUE(verif->verify(f, get_md(), "abc"_str));
}

TEST_F(VerifierTests, BCode_PutNone_Newtop) {
    const auto f_bcode =
        yama::bc::code()
        // block #1
        .add_put_none(yama::newtop) // inits R(0) (to None) via newtop
        .add_ret(0);
    yama::println("{}", f_bcode.fmt_disassembly());
    const auto f_consts =
        yama::const_table_info()
        .add_primitive_type("yama:None"_str)
        .add_int(10);
    const auto f = yama::make_function(
        "f"_str,
        f_consts,
        yama::make_callsig({}, 0),
        1,
        f_bcode);

    EXPECT_TRUE(verif->verify(f, get_md(), "abc"_str));
}

TEST_F(VerifierTests, BCode_PutNone_Newtop_MayBeMarkedReinit) {
    const auto f_bcode =
        yama::bc::code()
        // block #1
        .add_put_none(yama::newtop, true) // inits R(0) (to None) via newtop
        .add_ret(0);
    yama::println("{}", f_bcode.fmt_disassembly());
    const auto f_consts =
        yama::const_table_info()
        .add_primitive_type("yama:None"_str)
        .add_int(10);
    const auto f = yama::make_function(
        "f"_str,
        f_consts,
        yama::make_callsig({}, 0),
        1,
        f_bcode);

    EXPECT_TRUE(verif->verify(f, get_md(), "abc"_str));
}

TEST_F(VerifierTests, BCode_PutNone_Fail_RA_OutOfBounds) {
    const auto f_bcode =
        yama::bc::code()
        // block #1
        .add_put_none(yama::newtop) // inits R(0) (to None)
        // push then pop R(1) to better test impl robustness
        .add_put_none(yama::newtop) // inits R(1) (to None)
        .add_pop(1) // pops R(1)
        .add_put_none(1) // R(1) out-of-bounds
        .add_ret(0);
    yama::println("{}", f_bcode.fmt_disassembly());
    const auto f_consts =
        yama::const_table_info()
        .add_primitive_type("yama:None"_str);
    const auto f = yama::make_function(
        "f"_str,
        f_consts,
        yama::make_callsig({}, 0),
        2,
        f_bcode);

    EXPECT_FALSE(verif->verify(f, get_md(), "abc"_str));

    EXPECT_EQ(dbg->count(yama::dsignal::verif_RA_out_of_bounds), 1);
}

TEST_F(VerifierTests, BCode_PutNone_Fail_RA_IsNewtopButPushingOverflows) {
    const auto f_bcode =
        yama::bc::code()
        // block #1
        .add_put_none(yama::newtop) // inits R(0) (to None)
        .add_put_none(yama::newtop) // overflow!
        .add_ret(0);
    yama::println("{}", f_bcode.fmt_disassembly());
    const auto f_consts =
        yama::const_table_info()
        .add_primitive_type("yama:None"_str);
    const auto f = yama::make_function(
        "f"_str,
        f_consts,
        yama::make_callsig({}, 0),
        1,
        f_bcode);

    EXPECT_FALSE(verif->verify(f, get_md(), "abc"_str));

    EXPECT_EQ(dbg->count(yama::dsignal::verif_pushing_overflows), 1);
}

TEST_F(VerifierTests, BCode_PutNone_Fail_RA_WrongType_AndNotReinit) {
    const auto f_bcode =
        yama::bc::code()
        // block #1
        .add_put_const(yama::newtop, 1) // inits R(0) (to Int 10)
        .add_put_none(0) // R(0) not type None
        .add_ret(0);
    yama::println("{}", f_bcode.fmt_disassembly());
    const auto f_consts =
        yama::const_table_info()
        .add_primitive_type("yama:None"_str)
        .add_int(10);
    const auto f = yama::make_function(
        "f"_str,
        f_consts,
        yama::make_callsig({}, 0),
        1,
        f_bcode);

    EXPECT_FALSE(verif->verify(f, get_md(), "abc"_str));

    EXPECT_EQ(dbg->count(yama::dsignal::verif_RA_wrong_type), 1);
}

TEST_F(VerifierTests, BCode_PutNone_Fail_PushingMustNotOverflow) {
    const auto f_bcode =
        yama::bc::code()
        // block #1
        .add_put_none(yama::newtop) // R(0) not type None
        .add_put_none(yama::newtop) // overflow!
        .add_ret(0);
    yama::println("{}", f_bcode.fmt_disassembly());
    const auto f_consts =
        yama::const_table_info()
        .add_primitive_type("yama:None"_str)
        .add_int(10);
    const auto f = yama::make_function(
        "f"_str,
        f_consts,
        yama::make_callsig({}, 0),
        1,
        f_bcode);

    EXPECT_FALSE(verif->verify(f, get_md(), "abc"_str));

    EXPECT_EQ(dbg->count(yama::dsignal::verif_pushing_overflows), 1);
}

TEST_F(VerifierTests, BCode_PutConst) {
    const auto f_bcode =
        yama::bc::code()
        // block #1
        .add_put_const(yama::newtop, 1) // inits R(0) (to Int 10)
        .add_put_const(0, 2) // no reinit, so R(0) MUST be Int already
        .add_ret(0);
    yama::println("{}", f_bcode.fmt_disassembly());
    const auto f_consts =
        yama::const_table_info()
        .add_primitive_type("yama:Int"_str)
        .add_int(10)
        .add_int(-4);
    const auto f = yama::make_function(
        "f"_str,
        f_consts,
        yama::make_callsig({}, 0),
        1,
        f_bcode);

    EXPECT_TRUE(verif->verify(f, get_md(), "abc"_str));
}

TEST_F(VerifierTests, BCode_PutConst_Reinit) {
    const auto f_bcode =
        yama::bc::code()
        // block #1
        .add_put_none(yama::newtop) // inits R(0) (to None)
        .add_put_const(0, 1, true) // reinits R(0) (to Int 10) via index
        .add_ret(0);
    yama::println("{}", f_bcode.fmt_disassembly());
    const auto f_consts =
        yama::const_table_info()
        .add_primitive_type("yama:Int"_str)
        .add_int(10);
    const auto f = yama::make_function(
        "f"_str,
        f_consts,
        yama::make_callsig({}, 0),
        1,
        f_bcode);

    EXPECT_TRUE(verif->verify(f, get_md(), "abc"_str));
}

TEST_F(VerifierTests, BCode_PutConst_Newtop) {
    const auto f_bcode =
        yama::bc::code()
        // block #1
        .add_put_const(yama::newtop, 1) // inits R(0) (to Int 10) via newtop
        .add_ret(0);
    yama::println("{}", f_bcode.fmt_disassembly());
    const auto f_consts =
        yama::const_table_info()
        .add_primitive_type("yama:Int"_str)
        .add_int(10);
    const auto f = yama::make_function(
        "f"_str,
        f_consts,
        yama::make_callsig({}, 0),
        1,
        f_bcode);

    EXPECT_TRUE(verif->verify(f, get_md(), "abc"_str));
}

TEST_F(VerifierTests, BCode_PutConst_Newtop_MayBeMarkedReinit) {
    const auto f_bcode =
        yama::bc::code()
        // block #1
        .add_put_const(yama::newtop, 1, true) // inits R(0) (to Int 10) via newtop
        .add_ret(0);
    yama::println("{}", f_bcode.fmt_disassembly());
    const auto f_consts =
        yama::const_table_info()
        .add_primitive_type("yama:Int"_str)
        .add_int(10);
    const auto f = yama::make_function(
        "f"_str,
        f_consts,
        yama::make_callsig({}, 0),
        1,
        f_bcode);

    EXPECT_TRUE(verif->verify(f, get_md(), "abc"_str));
}

TEST_F(VerifierTests, BCode_PutConst_Fail_RA_OutOfBounds) {
    const auto f_bcode =
        yama::bc::code()
        // block #1
        .add_put_const(yama::newtop, 1) // inits R(0) (to Int 10) <- return value
        // push then pop R(1) to better test impl robustness
        .add_put_none(yama::newtop) // inits R(1) (to None)
        .add_pop(1) // pops R(1)
        .add_put_const(1, 1, true) // R(1) out-of-bounds
        .add_ret(0);
    yama::println("{}", f_bcode.fmt_disassembly());
    const auto f_consts =
        yama::const_table_info()
        .add_primitive_type("yama:None"_str)
        .add_int(10);
    const auto f = yama::make_function(
        "f"_str,
        f_consts,
        yama::make_callsig({}, 0),
        2,
        f_bcode);

    EXPECT_FALSE(verif->verify(f, get_md(), "abc"_str));

    EXPECT_EQ(dbg->count(yama::dsignal::verif_RA_out_of_bounds), 1);
}

TEST_F(VerifierTests, BCode_PutConst_Fail_RA_IsNewtopButPushingOverflows) {
    const auto f_bcode =
        yama::bc::code()
        // block #1
        .add_put_const(yama::newtop, 1) // inits R(0) (to Int 10) <- return value
        .add_put_const(yama::newtop, 1) // overflow!
        .add_ret(0);
    yama::println("{}", f_bcode.fmt_disassembly());
    const auto f_consts =
        yama::const_table_info()
        .add_primitive_type("yama:Int"_str)
        .add_int(10);
    const auto f = yama::make_function(
        "f"_str,
        f_consts,
        yama::make_callsig({}, 0),
        1,
        f_bcode);

    EXPECT_FALSE(verif->verify(f, get_md(), "abc"_str));

    EXPECT_EQ(dbg->count(yama::dsignal::verif_pushing_overflows), 1);
}

TEST_F(VerifierTests, BCode_PutConst_Fail_KoB_OutOfBounds) {
    const auto f_bcode =
        yama::bc::code()
        // block #1
        .add_put_const(yama::newtop, 1) // inits R(0) (to Int 10)
        .add_put_const(0, 2) // Ko(2) out-of-bounds
        .add_ret(0);
    yama::println("{}", f_bcode.fmt_disassembly());
    const auto f_consts =
        yama::const_table_info()
        .add_primitive_type("yama:Int"_str)
        .add_int(10);
    const auto f = yama::make_function(
        "f"_str,
        f_consts,
        yama::make_callsig({}, 0),
        1,
        f_bcode);

    EXPECT_FALSE(verif->verify(f, get_md(), "abc"_str));

    EXPECT_EQ(dbg->count(yama::dsignal::verif_KoB_out_of_bounds), 1);
}

TEST_F(VerifierTests, BCode_PutConst_Fail_KoB_NotAnObjectConst) {
    const auto f_bcode =
        yama::bc::code()
        // block #1
        .add_put_const(yama::newtop, 1) // inits R(0) (to Int 10)
        .add_put_const(0, 0) // Ko(0) not an object constant
        .add_ret(0);
    yama::println("{}", f_bcode.fmt_disassembly());
    const auto f_consts =
        yama::const_table_info()
        .add_primitive_type("yama:Int"_str)
        .add_int(10);
    const auto f = yama::make_function(
        "f"_str,
        f_consts,
        yama::make_callsig({}, 0),
        1,
        f_bcode);

    EXPECT_FALSE(verif->verify(f, get_md(), "abc"_str));

    EXPECT_EQ(dbg->count(yama::dsignal::verif_KoB_not_object_const), 1);
}

TEST_F(VerifierTests, BCode_PutConst_Fail_RA_And_KoB_TypesDiffer) {
    const auto f_bcode =
        yama::bc::code()
        // block #1
        .add_put_const(yama::newtop, 1) // inits R(0) (to Int 10)
        .add_put_const(0, 2) // Ko(2) not type Int
        .add_ret(0);
    yama::println("{}", f_bcode.fmt_disassembly());
    const auto f_consts =
        yama::const_table_info()
        .add_primitive_type("yama:Int"_str)
        .add_int(10)
        .add_char(U'y');
    const auto f = yama::make_function(
        "f"_str,
        f_consts,
        yama::make_callsig({}, 0),
        1,
        f_bcode);

    EXPECT_FALSE(verif->verify(f, get_md(), "abc"_str));

    EXPECT_EQ(dbg->count(yama::dsignal::verif_RA_and_KoB_types_differ), 1);
}

TEST_F(VerifierTests, BCode_PutConst_Fail_PushingMustNotOverflow) {
    const auto f_bcode =
        yama::bc::code()
        // block #1
        .add_put_const(yama::newtop, 1) // inits R(0) (to Int 10)
        .add_put_const(yama::newtop, 1) // overflow!
        .add_ret(0);
    yama::println("{}", f_bcode.fmt_disassembly());
    const auto f_consts =
        yama::const_table_info()
        .add_primitive_type("yama:Int"_str)
        .add_int(10)
        .add_char(U'y');
    const auto f = yama::make_function(
        "f"_str,
        f_consts,
        yama::make_callsig({}, 0),
        1,
        f_bcode);

    EXPECT_FALSE(verif->verify(f, get_md(), "abc"_str));

    EXPECT_EQ(dbg->count(yama::dsignal::verif_pushing_overflows), 1);
}

TEST_F(VerifierTests, BCode_PutTypeConst) {
    const auto f_bcode =
        yama::bc::code()
        // block #1
        .add_put_type_const(yama::newtop, 1) // inits R(0) (to Type yama:Bool)
        .add_put_type_const(0, 2) // no reinit, so R(0) MUST be Type already
        .add_ret(0);
    yama::println("{}", f_bcode.fmt_disassembly());
    const auto f_consts =
        yama::const_table_info()
        .add_primitive_type("yama:Type"_str)
        .add_primitive_type("yama:Bool"_str)
        .add_primitive_type("yama:Char"_str);
    const auto f = yama::make_function(
        "f"_str,
        f_consts,
        yama::make_callsig({}, 0),
        1,
        f_bcode);

    EXPECT_TRUE(verif->verify(f, get_md(), "abc"_str));
}

TEST_F(VerifierTests, BCode_PutTypeConst_Reinit) {
    const auto f_bcode =
        yama::bc::code()
        // block #1
        .add_put_none(yama::newtop) // inits R(0) (to None)
        .add_put_type_const(0, 1, true) // reinits R(0) (to Type yama:Bool) via index
        .add_ret(0);
    yama::println("{}", f_bcode.fmt_disassembly());
    const auto f_consts =
        yama::const_table_info()
        .add_primitive_type("yama:Type"_str)
        .add_primitive_type("yama:Bool"_str);
    const auto f = yama::make_function(
        "f"_str,
        f_consts,
        yama::make_callsig({}, 0),
        1,
        f_bcode);

    EXPECT_TRUE(verif->verify(f, get_md(), "abc"_str));
}

TEST_F(VerifierTests, BCode_PutTypeConst_Newtop) {
    const auto f_bcode =
        yama::bc::code()
        // block #1
        .add_put_type_const(yama::newtop, 1) // inits R(0) (to Type yama:Bool) via newtop
        .add_ret(0);
    yama::println("{}", f_bcode.fmt_disassembly());
    const auto f_consts =
        yama::const_table_info()
        .add_primitive_type("yama:Type"_str)
        .add_primitive_type("yama:Bool"_str);
    const auto f = yama::make_function(
        "f"_str,
        f_consts,
        yama::make_callsig({}, 0),
        1,
        f_bcode);

    EXPECT_TRUE(verif->verify(f, get_md(), "abc"_str));
}

TEST_F(VerifierTests, BCode_PutTypeConst_Newtop_MayBeMarkedReinit) {
    const auto f_bcode =
        yama::bc::code()
        // block #1
        .add_put_type_const(yama::newtop, 1, true) // inits R(0) (to Type yama:Bool) via newtop
        .add_ret(0);
    yama::println("{}", f_bcode.fmt_disassembly());
    const auto f_consts =
        yama::const_table_info()
        .add_primitive_type("yama:Type"_str)
        .add_primitive_type("yama:Bool"_str);
    const auto f = yama::make_function(
        "f"_str,
        f_consts,
        yama::make_callsig({}, 0),
        1,
        f_bcode);

    EXPECT_TRUE(verif->verify(f, get_md(), "abc"_str));
}

TEST_F(VerifierTests, BCode_PutTypeConst_Fail_RA_OutOfBounds) {
    const auto f_bcode =
        yama::bc::code()
        // block #1
        .add_put_const(yama::newtop, 1) // inits R(0) (to Int 10) <- return value
        // push then pop R(1) to better test impl robustness
        .add_put_none(yama::newtop) // inits R(1) (to None)
        .add_pop(1) // pops R(1)
        .add_put_type_const(1, 2, true) // R(1) out-of-bounds
        .add_ret(0);
    yama::println("{}", f_bcode.fmt_disassembly());
    const auto f_consts =
        yama::const_table_info()
        .add_primitive_type("yama:None"_str)
        .add_int(10)
        .add_primitive_type("yama:Bool"_str);
    const auto f = yama::make_function(
        "f"_str,
        f_consts,
        yama::make_callsig({}, 0),
        2,
        f_bcode);

    EXPECT_FALSE(verif->verify(f, get_md(), "abc"_str));

    EXPECT_EQ(dbg->count(yama::dsignal::verif_RA_out_of_bounds), 1);
}

TEST_F(VerifierTests, BCode_PutTypeConst_Fail_RA_IsNewtopButPushingOverflows) {
    const auto f_bcode =
        yama::bc::code()
        // block #1
        .add_put_const(yama::newtop, 1) // inits R(0) (to Int 10) <- return value
        .add_put_type_const(yama::newtop, 2) // overflow!
        .add_ret(0);
    yama::println("{}", f_bcode.fmt_disassembly());
    const auto f_consts =
        yama::const_table_info()
        .add_primitive_type("yama:Int"_str)
        .add_int(10)
        .add_primitive_type("yama:Bool"_str);
    const auto f = yama::make_function(
        "f"_str,
        f_consts,
        yama::make_callsig({}, 0),
        1,
        f_bcode);

    EXPECT_FALSE(verif->verify(f, get_md(), "abc"_str));

    EXPECT_EQ(dbg->count(yama::dsignal::verif_pushing_overflows), 1);
}

TEST_F(VerifierTests, BCode_PutTypeConst_Fail_RA_WrongType_AndNotReinit) {
    const auto f_bcode =
        yama::bc::code()
        // block #1
        .add_put_const(yama::newtop, 1) // inits R(0) (to Int 10)
        .add_put_type_const(0, 2) // R(0) not type yama:Type
        .add_ret(0);
    yama::println("{}", f_bcode.fmt_disassembly());
    const auto f_consts =
        yama::const_table_info()
        .add_primitive_type("yama:Type"_str)
        .add_int(10)
        .add_primitive_type("yama:Bool"_str);
    const auto f = yama::make_function(
        "f"_str,
        f_consts,
        yama::make_callsig({}, 0),
        1,
        f_bcode);

    EXPECT_FALSE(verif->verify(f, get_md(), "abc"_str));

    EXPECT_EQ(dbg->count(yama::dsignal::verif_RA_wrong_type), 1);
}

TEST_F(VerifierTests, BCode_PutTypeConst_Fail_KtB_OutOfBounds) {
    const auto f_bcode =
        yama::bc::code()
        // block #1
        .add_put_type_const(yama::newtop, 1) // inits R(0) (to Type yama:Bool)
        .add_put_type_const(0, 2) // Kt(2) out-of-bounds
        .add_ret(0);
    yama::println("{}", f_bcode.fmt_disassembly());
    const auto f_consts =
        yama::const_table_info()
        .add_primitive_type("yama:Type"_str)
        .add_primitive_type("yama:Bool"_str);
    const auto f = yama::make_function(
        "f"_str,
        f_consts,
        yama::make_callsig({}, 0),
        1,
        f_bcode);

    EXPECT_FALSE(verif->verify(f, get_md(), "abc"_str));

    EXPECT_EQ(dbg->count(yama::dsignal::verif_KtB_out_of_bounds), 1);
}

TEST_F(VerifierTests, BCode_PutTypeConst_Fail_KtB_NotATypeConst) {
    const auto f_bcode =
        yama::bc::code()
        // block #1
        .add_put_type_const(yama::newtop, 0) // inits R(0) (to Type yama:Type)
        .add_put_type_const(0, 1) // Kt(1) not a type constant
        .add_ret(0);
    yama::println("{}", f_bcode.fmt_disassembly());
    const auto f_consts =
        yama::const_table_info()
        .add_primitive_type("yama:Type"_str)
        .add_int(10);
    const auto f = yama::make_function(
        "f"_str,
        f_consts,
        yama::make_callsig({}, 0),
        1,
        f_bcode);

    EXPECT_FALSE(verif->verify(f, get_md(), "abc"_str));

    EXPECT_EQ(dbg->count(yama::dsignal::verif_KtB_not_type_const), 1);
}

TEST_F(VerifierTests, BCode_PutTypeConst_Fail_PushingMustNotOverflow) {
    const auto f_bcode =
        yama::bc::code()
        // block #1
        .add_put_type_const(yama::newtop, 1) // inits R(0) (to Type yama:Bool)
        .add_put_type_const(yama::newtop, 1) // overflow!
        .add_ret(0);
    yama::println("{}", f_bcode.fmt_disassembly());
    const auto f_consts =
        yama::const_table_info()
        .add_primitive_type("yama:Type"_str)
        .add_primitive_type("yama:Bool"_str);
    const auto f = yama::make_function(
        "f"_str,
        f_consts,
        yama::make_callsig({}, 0),
        1,
        f_bcode);

    EXPECT_FALSE(verif->verify(f, get_md(), "abc"_str));

    EXPECT_EQ(dbg->count(yama::dsignal::verif_pushing_overflows), 1);
}

TEST_F(VerifierTests, BCode_PutArg) {
    const auto f_bcode =
        yama::bc::code()
        // block #1
        .add_put_const(yama::newtop, 2) // inits R(0) (to f object)
        .add_put_const(yama::newtop, 3) // inits R(1) (to Bool true)
        .add_put_const(yama::newtop, 4) // inits R(2) (to Float 0.05)
        .add_put_arg(0, 0) // load callobj f arg into R(0), w/out reinit
        .add_put_arg(1, 1) // load Bool arg into R(1), w/out reinit
        .add_put_arg(2, 2) // load Float arg into R(2), w/out reinit
        .add_ret(2);
    yama::println("{}", f_bcode.fmt_disassembly());
    const auto f_consts =
        yama::const_table_info()
        .add_primitive_type("yama:Bool"_str)
        .add_primitive_type("yama:Float"_str)
        .add_function_type("abc:f"_str, yama::make_callsig({ 0, 1 }, 1))
        .add_bool(true)
        .add_float(0.05);
    const auto f = yama::make_function(
        "f"_str,
        f_consts,
        yama::make_callsig({ 0, 1 }, 1),
        3,
        f_bcode);

    EXPECT_TRUE(verif->verify(f, get_md(), "abc"_str));
}

TEST_F(VerifierTests, BCode_PutArg_Reinit) {
    const auto f_bcode =
        yama::bc::code()
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
    yama::println("{}", f_bcode.fmt_disassembly());
    const auto f_consts =
        yama::const_table_info()
        .add_primitive_type("yama:Bool"_str)
        .add_primitive_type("yama:Float"_str)
        .add_function_type("abc:f"_str, yama::make_callsig({ 0, 1 }, 1))
        .add_bool(true)
        .add_float(0.05);
    const auto f = yama::make_function(
        "f"_str,
        f_consts,
        yama::make_callsig({ 0, 1 }, 1),
        3,
        f_bcode);

    EXPECT_TRUE(verif->verify(f, get_md(), "abc"_str));
}

TEST_F(VerifierTests, BCode_PutArg_Newtop) {
    const auto f_bcode =
        yama::bc::code()
        // block #1
        .add_put_arg(yama::newtop, 0) // load callobj f arg into R(0), w/ reinit
        .add_put_arg(yama::newtop, 1) // load Bool arg into R(1), w/ reinit
        .add_put_arg(yama::newtop, 2) // load Float arg into R(2), w/ reinit
        .add_put_const(0, 2) // no reinit, so R(0) MUST be f already
        .add_put_const(1, 3) // no reinit, so R(1) MUST be Bool already
        .add_put_const(2, 4) // no reinit, so R(2) MUST be Float already
        .add_ret(2);
    yama::println("{}", f_bcode.fmt_disassembly());
    const auto f_consts =
        yama::const_table_info()
        .add_primitive_type("yama:Bool"_str)
        .add_primitive_type("yama:Float"_str)
        .add_function_type("abc:f"_str, yama::make_callsig({ 0, 1 }, 1))
        .add_bool(true)
        .add_float(0.05);
    const auto f = yama::make_function(
        "f"_str,
        f_consts,
        yama::make_callsig({ 0, 1 }, 1),
        3,
        f_bcode);

    EXPECT_TRUE(verif->verify(f, get_md(), "abc"_str));
}

TEST_F(VerifierTests, BCode_PutArg_Newtop_MayBeMarkedReinit) {
    const auto f_bcode =
        yama::bc::code()
        // block #1
        .add_put_arg(yama::newtop, 0, true) // load callobj f arg into R(0), w/ reinit
        .add_put_arg(yama::newtop, 1, true) // load Bool arg into R(1), w/ reinit
        .add_put_arg(yama::newtop, 2, true) // load Float arg into R(2), w/ reinit
        .add_put_const(0, 2) // no reinit, so R(0) MUST be f already
        .add_put_const(1, 3) // no reinit, so R(1) MUST be Bool already
        .add_put_const(2, 4) // no reinit, so R(2) MUST be Float already
        .add_ret(2);
    yama::println("{}", f_bcode.fmt_disassembly());
    const auto f_consts =
        yama::const_table_info()
        .add_primitive_type("yama:Bool"_str)
        .add_primitive_type("yama:Float"_str)
        .add_function_type("abc:f"_str, yama::make_callsig({ 0, 1 }, 1))
        .add_bool(true)
        .add_float(0.05);
    const auto f = yama::make_function(
        "f"_str,
        f_consts,
        yama::make_callsig({ 0, 1 }, 1),
        3,
        f_bcode);

    EXPECT_TRUE(verif->verify(f, get_md(), "abc"_str));
}

TEST_F(VerifierTests, BCode_PutArg_Fail_RA_OutOfBounds) {
    const auto f_bcode =
        yama::bc::code()
        // block #1
        .add_put_const(yama::newtop, 2) // inits R(0) (to Float 0.05) <- return value
        // push then pop R(1) to better test impl robustness
        .add_put_none(yama::newtop) // inits R(1) (to None)
        .add_pop(1) // pops R(1)
        .add_put_arg(1, 0, true) // R(1) out-of-bounds
        .add_ret(0);
    yama::println("{}", f_bcode.fmt_disassembly());
    const auto f_consts =
        yama::const_table_info()
        .add_primitive_type("yama:Bool"_str)
        .add_primitive_type("yama:Float"_str)
        .add_float(0.05);
    const auto f = yama::make_function(
        "f"_str,
        f_consts,
        yama::make_callsig({ 0, 1 }, 1),
        2,
        f_bcode);

    EXPECT_FALSE(verif->verify(f, get_md(), "abc"_str));

    EXPECT_EQ(dbg->count(yama::dsignal::verif_RA_out_of_bounds), 1);
}

TEST_F(VerifierTests, BCode_PutArg_Fail_RA_IsNewtopButPushingOverflows) {
    const auto f_bcode =
        yama::bc::code()
        // block #1
        .add_put_const(yama::newtop, 2) // inits R(0) (to Float 0.05) <- return value
        .add_put_arg(yama::newtop, 0) // overflow!
        .add_ret(0);
    yama::println("{}", f_bcode.fmt_disassembly());
    const auto f_consts =
        yama::const_table_info()
        .add_primitive_type("yama:Bool"_str)
        .add_primitive_type("yama:Float"_str)
        .add_float(0.05);
    const auto f = yama::make_function(
        "f"_str,
        f_consts,
        yama::make_callsig({ 0, 1 }, 1),
        1,
        f_bcode);

    EXPECT_FALSE(verif->verify(f, get_md(), "abc"_str));

    EXPECT_EQ(dbg->count(yama::dsignal::verif_pushing_overflows), 1);
}

TEST_F(VerifierTests, BCode_PutArg_Fail_ArgB_OutOfBounds) {
    const auto f_bcode =
        yama::bc::code()
        // block #1
        .add_put_const(yama::newtop, 2) // inits R(0) (to Float 0.05)
        .add_put_arg(0, 3) // Arg(3) out-of-bounds
        .add_ret(0);
    yama::println("{}", f_bcode.fmt_disassembly());
    const auto f_consts =
        yama::const_table_info()
        .add_primitive_type("yama:Bool"_str)
        .add_primitive_type("yama:Float"_str)
        .add_float(0.05);
    const auto f = yama::make_function(
        "f"_str,
        f_consts,
        yama::make_callsig({ 0, 1 }, 1),
        1,
        f_bcode);

    EXPECT_FALSE(verif->verify(f, get_md(), "abc"_str));

    EXPECT_EQ(dbg->count(yama::dsignal::verif_ArgB_out_of_bounds), 1);
}

TEST_F(VerifierTests, BCode_PutArg_Fail_RA_And_ArgB_TypesDiffer) {
    const auto f_bcode =
        yama::bc::code()
        // block #1
        .add_put_const(yama::newtop, 2) // inits R(0) (to Float 0.05)
        .add_put_arg(0, 1) // Arg(1) not type Float
        .add_ret(0);
    yama::println("{}", f_bcode.fmt_disassembly());
    const auto f_consts =
        yama::const_table_info()
        .add_primitive_type("yama:Bool"_str)
        .add_primitive_type("yama:Float"_str)
        .add_float(0.05);
    const auto f = yama::make_function(
        "f"_str,
        f_consts,
        yama::make_callsig({ 0, 1 }, 1),
        1,
        f_bcode);

    EXPECT_FALSE(verif->verify(f, get_md(), "abc"_str));

    EXPECT_EQ(dbg->count(yama::dsignal::verif_RA_and_ArgB_types_differ), 1);
}

TEST_F(VerifierTests, BCode_PutArg_Fail_PushingMustNotOverflow) {
    const auto f_bcode =
        yama::bc::code()
        // block #1
        .add_put_arg(yama::newtop, 1)
        .add_put_arg(yama::newtop, 1) // overflow!
        .add_ret(0);
    yama::println("{}", f_bcode.fmt_disassembly());
    const auto f_consts =
        yama::const_table_info()
        .add_primitive_type("yama:Bool"_str)
        .add_primitive_type("yama:Float"_str)
        .add_float(0.05);
    const auto f = yama::make_function(
        "f"_str,
        f_consts,
        yama::make_callsig({ 0, 1 }, 1),
        1,
        f_bcode);

    EXPECT_FALSE(verif->verify(f, get_md(), "abc"_str));

    EXPECT_EQ(dbg->count(yama::dsignal::verif_pushing_overflows), 1);
}

TEST_F(VerifierTests, BCode_Copy) {
    // I once had an issue where copy instr verif was mistakenly treating R(A) as though it
    // had call instr semantics, so I rewrote this test to catch this error
    const auto f_bcode =
        yama::bc::code()
        // block #1
        .add_put_none(yama::newtop) // inits R(0) (to None)
        .add_put_const(yama::newtop, 2) // inits R(1) (to Float 3.14159)
        .add_put_none(yama::newtop) // inits R(2) (to None)
        .add_put_const(yama::newtop, 1) // inits R(3) (to Float 0.05)
        .add_copy(3, 1) // no reinit, so R(1) MUST be Float already
        .add_ret(1);
    yama::println("{}", f_bcode.fmt_disassembly());
    const auto f_consts =
        yama::const_table_info()
        .add_primitive_type("yama:Float"_str)
        .add_float(0.05)
        .add_float(3.14159);
    const auto f = yama::make_function(
        "f"_str,
        f_consts,
        yama::make_callsig({}, 0),
        4,
        f_bcode);

    EXPECT_TRUE(verif->verify(f, get_md(), "abc"_str));
}

TEST_F(VerifierTests, BCode_Copy_Reinit) {
    const auto f_bcode =
        yama::bc::code()
        // block #1
        .add_put_const(yama::newtop, 1) // inits R(0) (to Float 0.05)
        .add_put_const(yama::newtop, 2) // inits R(1) (to Int 10)
        .add_copy(0, 1, true) // reinits R(1) (to Float)
        .add_ret(1);
    yama::println("{}", f_bcode.fmt_disassembly());
    const auto f_consts =
        yama::const_table_info()
        .add_primitive_type("yama:Float"_str)
        .add_float(0.05)
        .add_int(10);
    const auto f = yama::make_function(
        "f"_str,
        f_consts,
        yama::make_callsig({}, 0),
        2,
        f_bcode);

    EXPECT_TRUE(verif->verify(f, get_md(), "abc"_str));
}

TEST_F(VerifierTests, BCode_Copy_Newtop) {
    const auto f_bcode =
        yama::bc::code()
        // block #1
        .add_put_const(yama::newtop, 1) // inits R(0) (to Float 0.05)
        .add_copy(0, yama::newtop) // inits R(1) (to Float)
        .add_ret(1);
    yama::println("{}", f_bcode.fmt_disassembly());
    const auto f_consts =
        yama::const_table_info()
        .add_primitive_type("yama:Float"_str)
        .add_float(0.05)
        .add_int(10);
    const auto f = yama::make_function(
        "f"_str,
        f_consts,
        yama::make_callsig({}, 0),
        2,
        f_bcode);

    EXPECT_TRUE(verif->verify(f, get_md(), "abc"_str));
}

TEST_F(VerifierTests, BCode_Copy_Newtop_MayBeMarkedReinit) {
    const auto f_bcode =
        yama::bc::code()
        // block #1
        .add_put_const(yama::newtop, 1) // inits R(0) (to Float 0.05)
        .add_copy(0, yama::newtop, true) // inits R(1) (to Float)
        .add_ret(1);
    yama::println("{}", f_bcode.fmt_disassembly());
    const auto f_consts =
        yama::const_table_info()
        .add_primitive_type("yama:Float"_str)
        .add_float(0.05)
        .add_int(10);
    const auto f = yama::make_function(
        "f"_str,
        f_consts,
        yama::make_callsig({}, 0),
        2,
        f_bcode);

    EXPECT_TRUE(verif->verify(f, get_md(), "abc"_str));
}

TEST_F(VerifierTests, BCode_Copy_Fail_RA_OutOfBounds) {
    const auto f_bcode =
        yama::bc::code()
        // block #1
        .add_put_const(yama::newtop, 1) // inits R(0) (to Float 0.05)
        .add_put_const(yama::newtop, 2) // inits R(1) (to Float 3.14159)
        // push then pop R(2) to better test impl robustness
        .add_put_none(yama::newtop) // inits R(2) (to None)
        .add_pop(1) // pops R(2)
        .add_copy(2, 1, true) // R(2) out-of-bounds
        .add_ret(1);
    yama::println("{}", f_bcode.fmt_disassembly());
    const auto f_consts =
        yama::const_table_info()
        .add_primitive_type("yama:Float"_str)
        .add_float(0.05)
        .add_float(3.14159);
    const auto f = yama::make_function(
        "f"_str,
        f_consts,
        yama::make_callsig({}, 0),
        3,
        f_bcode);

    EXPECT_FALSE(verif->verify(f, get_md(), "abc"_str));

    EXPECT_EQ(dbg->count(yama::dsignal::verif_RA_out_of_bounds), 1);
}

TEST_F(VerifierTests, BCode_Copy_Fail_RB_OutOfBounds) {
    const auto f_bcode =
        yama::bc::code()
        // block #1
        .add_put_const(yama::newtop, 1) // inits R(0) (to Float 0.05)
        .add_put_const(yama::newtop, 2) // inits R(1) (to Float 3.14159)
        // push then pop R(2) to better test impl robustness
        .add_put_none(yama::newtop) // inits R(2) (to None)
        .add_pop(2) // pops R(2)
        .add_copy(0, 2, true) // R(2) out-of-bounds
        .add_ret(1);
    yama::println("{}", f_bcode.fmt_disassembly());
    const auto f_consts =
        yama::const_table_info()
        .add_primitive_type("yama:Float"_str)
        .add_float(0.05)
        .add_float(3.14159);
    const auto f = yama::make_function(
        "f"_str,
        f_consts,
        yama::make_callsig({}, 0),
        3,
        f_bcode);

    EXPECT_FALSE(verif->verify(f, get_md(), "abc"_str));

    EXPECT_EQ(dbg->count(yama::dsignal::verif_RB_out_of_bounds), 1);
}

TEST_F(VerifierTests, BCode_Copy_Fail_RB_IsNewtopButPushingOverflows) {
    const auto f_bcode =
        yama::bc::code()
        // block #1
        .add_put_const(yama::newtop, 1) // inits R(0) (to Float 0.05)
        .add_copy(0, yama::newtop) // overflow!
        .add_ret(0);
    yama::println("{}", f_bcode.fmt_disassembly());
    const auto f_consts =
        yama::const_table_info()
        .add_primitive_type("yama:Float"_str)
        .add_float(0.05)
        .add_float(3.14159);
    const auto f = yama::make_function(
        "f"_str,
        f_consts,
        yama::make_callsig({}, 0),
        1,
        f_bcode);

    EXPECT_FALSE(verif->verify(f, get_md(), "abc"_str));

    EXPECT_EQ(dbg->count(yama::dsignal::verif_pushing_overflows), 1);
}

TEST_F(VerifierTests, BCode_Copy_Fail_RA_And_RB_TypesDiffer) {
    const auto f_bcode =
        yama::bc::code()
        // block #1
        .add_put_const(yama::newtop, 1) // inits R(0) (to Int 10)
        .add_put_const(yama::newtop, 2) // inits R(1) (to Float 0.05)
        .add_copy(0, 1) // R(1) not type Int
        .add_ret(1);
    yama::println("{}", f_bcode.fmt_disassembly());
    const auto f_consts =
        yama::const_table_info()
        .add_primitive_type("yama:Float"_str)
        .add_int(10)
        .add_float(0.05);
    const auto f = yama::make_function(
        "f"_str,
        f_consts,
        yama::make_callsig({}, 0),
        2,
        f_bcode);

    EXPECT_FALSE(verif->verify(f, get_md(), "abc"_str));

    EXPECT_EQ(dbg->count(yama::dsignal::verif_RA_and_RB_types_differ), 1);
}

TEST_F(VerifierTests, BCode_Copy_Fail_PushingMustNotOverflow) {
    const auto f_bcode =
        yama::bc::code()
        // block #1
        .add_put_const(yama::newtop, 1) // inits R(0) (to Int 10)
        .add_copy(0, yama::newtop) // overflow!
        .add_ret(1);
    yama::println("{}", f_bcode.fmt_disassembly());
    const auto f_consts =
        yama::const_table_info()
        .add_primitive_type("yama:Float"_str)
        .add_int(10)
        .add_float(0.05);
    const auto f = yama::make_function(
        "f"_str,
        f_consts,
        yama::make_callsig({}, 0),
        1,
        f_bcode);

    EXPECT_FALSE(verif->verify(f, get_md(), "abc"_str));

    EXPECT_EQ(dbg->count(yama::dsignal::verif_pushing_overflows), 1);
}

TEST_F(VerifierTests, BCode_DefaultInit) {
    const auto f_bcode =
        yama::bc::code()
        // block #1
        .add_put_const(yama::newtop, 1) // inits R(0) (to Float 10.0)
        .add_default_init(0, 0) // no reinit, so R(0) MUST be Float already
        .add_ret(0);
    yama::println("{}", f_bcode.fmt_disassembly());
    const auto f_consts =
        yama::const_table_info()
        .add_primitive_type("yama:Float"_str)
        .add_float(10.0);
    const auto f = yama::make_function(
        "f"_str,
        f_consts,
        yama::make_callsig({}, 0),
        4,
        f_bcode);

    EXPECT_TRUE(verif->verify(f, get_md(), "abc"_str));
}

TEST_F(VerifierTests, BCode_DefaultInit_Reinit) {
    const auto f_bcode =
        yama::bc::code()
        // block #1
        .add_put_const(yama::newtop, 1) // inits R(0) (to Int 10)
        .add_default_init(0, 0, true) // reinits R(0) (to Float 0.0)
        .add_ret(0);
    yama::println("{}", f_bcode.fmt_disassembly());
    const auto f_consts =
        yama::const_table_info()
        .add_primitive_type("yama:Float"_str)
        .add_int(10);
    const auto f = yama::make_function(
        "f"_str,
        f_consts,
        yama::make_callsig({}, 0),
        4,
        f_bcode);

    EXPECT_TRUE(verif->verify(f, get_md(), "abc"_str));
}

TEST_F(VerifierTests, BCode_DefaultInit_Newtop) {
    const auto f_bcode =
        yama::bc::code()
        // block #1
        .add_default_init(yama::newtop, 0) // inits R(0) (to Float 0.0)
        .add_ret(0);
    yama::println("{}", f_bcode.fmt_disassembly());
    const auto f_consts =
        yama::const_table_info()
        .add_primitive_type("yama:Float"_str);
    const auto f = yama::make_function(
        "f"_str,
        f_consts,
        yama::make_callsig({}, 0),
        4,
        f_bcode);

    EXPECT_TRUE(verif->verify(f, get_md(), "abc"_str));
}

TEST_F(VerifierTests, BCode_DefaultInit_Newtop_MayBeMarkedReinit) {
    const auto f_bcode =
        yama::bc::code()
        // block #1
        .add_default_init(yama::newtop, 0, true) // inits R(0) (to Float 0.0)
        .add_ret(0);
    yama::println("{}", f_bcode.fmt_disassembly());
    const auto f_consts =
        yama::const_table_info()
        .add_primitive_type("yama:Float"_str);
    const auto f = yama::make_function(
        "f"_str,
        f_consts,
        yama::make_callsig({}, 0),
        4,
        f_bcode);

    EXPECT_TRUE(verif->verify(f, get_md(), "abc"_str));
}

TEST_F(VerifierTests, BCode_DefaultInit_Fail_RA_OutOfBounds) {
    const auto f_bcode =
        yama::bc::code()
        // block #1
        .add_put_const(yama::newtop, 1) // inits R(0) (to Float 0.05)
        // push then pop R(1) to better test impl robustness
        .add_put_none(yama::newtop) // inits R(1) (to None)
        .add_pop(1) // pops R(1)
        .add_default_init(1, 0, true) // R(1) out-of-bounds
        .add_ret(1);
    yama::println("{}", f_bcode.fmt_disassembly());
    const auto f_consts =
        yama::const_table_info()
        .add_primitive_type("yama:Float"_str)
        .add_float(0.05);
    const auto f = yama::make_function(
        "f"_str,
        f_consts,
        yama::make_callsig({}, 0),
        3,
        f_bcode);

    EXPECT_FALSE(verif->verify(f, get_md(), "abc"_str));

    EXPECT_EQ(dbg->count(yama::dsignal::verif_RA_out_of_bounds), 1);
}

TEST_F(VerifierTests, BCode_DefaultInit_Fail_KtB_OutOfBounds) {
    const auto f_bcode =
        yama::bc::code()
        // block #1
        .add_put_const(yama::newtop, 1) // inits R(0) (to Float 10.0)
        .add_default_init(0, 2) // Kt(2) out-of-bounds
        .add_ret(0);
    yama::println("{}", f_bcode.fmt_disassembly());
    const auto f_consts =
        yama::const_table_info()
        .add_primitive_type("yama:Float"_str)
        .add_float(10.0);
    const auto f = yama::make_function(
        "f"_str,
        f_consts,
        yama::make_callsig({}, 0),
        1,
        f_bcode);

    EXPECT_FALSE(verif->verify(f, get_md(), "abc"_str));

    EXPECT_EQ(dbg->count(yama::dsignal::verif_KtB_out_of_bounds), 1);
}

TEST_F(VerifierTests, BCode_DefaultInit_Fail_KtB_NotATypeConst) {
    const auto f_bcode =
        yama::bc::code()
        // block #1
        .add_put_const(yama::newtop, 1) // inits R(0) (to Float 10.0)
        .add_default_init(0, 1) // Kt(1) not a type constant
        .add_ret(0);
    yama::println("{}", f_bcode.fmt_disassembly());
    const auto f_consts =
        yama::const_table_info()
        .add_primitive_type("yama:Float"_str)
        .add_float(10.0);
    const auto f = yama::make_function(
        "f"_str,
        f_consts,
        yama::make_callsig({}, 0),
        1,
        f_bcode);

    EXPECT_FALSE(verif->verify(f, get_md(), "abc"_str));

    EXPECT_EQ(dbg->count(yama::dsignal::verif_KtB_not_type_const), 1);
}

TEST_F(VerifierTests, BCode_DefaultInit_Fail_RA_And_KtB_TypesDiffer) {
    const auto f_bcode =
        yama::bc::code()
        // block #1
        .add_put_const(yama::newtop, 1) // inits R(0) (to Int 10)
        .add_default_init(0, 0) // Kt(0) not type Int
        .add_ret(0);
    yama::println("{}", f_bcode.fmt_disassembly());
    const auto f_consts =
        yama::const_table_info()
        .add_primitive_type("yama:Float"_str)
        .add_int(10);
    const auto f = yama::make_function(
        "f"_str,
        f_consts,
        yama::make_callsig({}, 0),
        1,
        f_bcode);

    EXPECT_FALSE(verif->verify(f, get_md(), "abc"_str));

    EXPECT_EQ(dbg->count(yama::dsignal::verif_RA_and_KtB_types_differ), 1);
}

TEST_F(VerifierTests, BCode_DefaultInit_Fail_PushingMustNotOverflow) {
    const auto f_bcode =
        yama::bc::code()
        // block #1
        .add_put_const(yama::newtop, 1) // inits R(0) (to Float 10.0)
        .add_default_init(yama::newtop, 0) // overflow!
        .add_ret(0);
    yama::println("{}", f_bcode.fmt_disassembly());
    const auto f_consts =
        yama::const_table_info()
        .add_primitive_type("yama:Float"_str)
        .add_float(10.0);
    const auto f = yama::make_function(
        "f"_str,
        f_consts,
        yama::make_callsig({}, 0),
        1,
        f_bcode);

    EXPECT_FALSE(verif->verify(f, get_md(), "abc"_str));

    EXPECT_EQ(dbg->count(yama::dsignal::verif_pushing_overflows), 1);
}

static_assert(yama::kinds == 4); // one BCode_Call_# test for each callable kind

TEST_F(VerifierTests, BCode_Call_Function) {
    const auto f_bcode =
        yama::bc::code()
        // block #1
        .add_put_const(yama::newtop, 7) // inits R(0) to type of return value
        .add_put_const(yama::newtop, 3) // inits R(1) to type of call object
        .add_put_const(yama::newtop, 4) // inits R(2) to type of arg #1
        .add_put_const(yama::newtop, 5) // inits R(3) to type of arg #2
        .add_put_const(yama::newtop, 6) // inits R(4) to type of arg #3
        .add_call(4, 0) // no reinit, so R(0) MUST be Char already
        .add_ret(0);
    yama::println("{}", f_bcode.fmt_disassembly());
    const auto f_consts =
        yama::const_table_info()
        .add_primitive_type("yama:Int"_str)
        .add_primitive_type("yama:Float"_str)
        .add_primitive_type("yama:Char"_str)
        .add_function_type("abc:g"_str, yama::make_callsig({ 0, 1, 0 }, 2)) // fn(Int, Float, Int) -> Char
        .add_int(10)
        .add_float(0.05)
        .add_int(-4)
        .add_char(U'y');
    const auto f = yama::make_function(
        "f"_str,
        f_consts,
        yama::make_callsig({}, 2),
        5,
        f_bcode);

    EXPECT_TRUE(verif->verify(f, get_md(), "abc"_str));
}

TEST_F(VerifierTests, BCode_Call_Method) {
    const auto f_bcode =
        yama::bc::code()
        // block #1
        .add_put_const(yama::newtop, 7) // inits R(0) to type of return value
        .add_put_const(yama::newtop, 3) // inits R(1) to type of call object
        .add_put_const(yama::newtop, 4) // inits R(2) to type of arg #1
        .add_put_const(yama::newtop, 5) // inits R(3) to type of arg #2
        .add_put_const(yama::newtop, 6) // inits R(4) to type of arg #3
        .add_call(4, 0) // no reinit, so R(0) MUST be Char already
        .add_ret(0);
    yama::println("{}", f_bcode.fmt_disassembly());
    const auto f_consts =
        yama::const_table_info()
        .add_primitive_type("yama:Int"_str)
        .add_primitive_type("yama:Float"_str)
        .add_primitive_type("yama:Char"_str)
        .add_method_type("abc:Something::g"_str, yama::make_callsig({ 0, 1, 0 }, 2)) // fn(Int, Float, Int) -> Char
        .add_int(10)
        .add_float(0.05)
        .add_int(-4)
        .add_char(U'y');
    const auto f = yama::make_function(
        "f"_str,
        f_consts,
        yama::make_callsig({}, 2),
        5,
        f_bcode);

    EXPECT_TRUE(verif->verify(f, get_md(), "abc"_str));
}

TEST_F(VerifierTests, BCode_Call_NoReinit) {
    const auto f_bcode =
        yama::bc::code()
        // block #1
        .add_put_const(yama::newtop, 7) // inits R(0) to type of return value
        .add_put_const(yama::newtop, 3) // inits R(1) to type of call object
        .add_put_const(yama::newtop, 4) // inits R(2) to type of arg #1
        .add_put_const(yama::newtop, 5) // inits R(3) to type of arg #2
        .add_put_const(yama::newtop, 6) // inits R(4) to type of arg #3
        .add_call(4, 0) // no reinit, so R(0) MUST be Char already
        .add_ret(0);
    yama::println("{}", f_bcode.fmt_disassembly());
    const auto f_consts =
        yama::const_table_info()
        .add_primitive_type("yama:Int"_str)
        .add_primitive_type("yama:Float"_str)
        .add_primitive_type("yama:Char"_str)
        .add_function_type("abc:g"_str, yama::make_callsig({ 0, 1, 0 }, 2)) // fn(Int, Float, Int) -> Char
        .add_int(10)
        .add_float(0.05)
        .add_int(-4)
        .add_char(U'y');
    const auto f = yama::make_function(
        "f"_str,
        f_consts,
        yama::make_callsig({}, 2),
        5,
        f_bcode);

    EXPECT_TRUE(verif->verify(f, get_md(), "abc"_str));
}

TEST_F(VerifierTests, BCode_Call_Reinit) {
    const auto f_bcode =
        yama::bc::code()
        // block #1
        .add_put_none(yama::newtop) // inits R(0) (to None)
        .add_put_const(yama::newtop, 3) // inits R(1) to type of call object
        .add_put_const(yama::newtop, 4) // inits R(2) to type of arg #1
        .add_put_const(yama::newtop, 5) // inits R(3) to type of arg #2
        .add_put_const(yama::newtop, 6) // inits R(4) to type of arg #3
        .add_call(4, 0, true) // reinits R(0) to return type Char
        .add_ret(0);
    yama::println("{}", f_bcode.fmt_disassembly());
    const auto f_consts =
        yama::const_table_info()
        .add_primitive_type("yama:Int"_str)
        .add_primitive_type("yama:Float"_str)
        .add_primitive_type("yama:Char"_str)
        .add_function_type("abc:g"_str, yama::make_callsig({ 0, 1, 0 }, 2)) // fn(Int, Float, Int) -> Char
        .add_int(10)
        .add_float(0.05)
        .add_int(-4)
        .add_char(U'y');
    const auto f = yama::make_function(
        "f"_str,
        f_consts,
        yama::make_callsig({}, 2),
        5,
        f_bcode);

    EXPECT_TRUE(verif->verify(f, get_md(), "abc"_str));
}

TEST_F(VerifierTests, BCode_Call_Newtop) {
    const auto f_bcode =
        yama::bc::code()
        // block #1
        .add_put_const(yama::newtop, 3) // inits R(0) to type of call object
        .add_put_const(yama::newtop, 4) // inits R(1) to type of arg #1
        .add_put_const(yama::newtop, 5) // inits R(2) to type of arg #2
        .add_put_const(yama::newtop, 6) // inits R(3) to type of arg #3
        .add_call(4, yama::newtop) // inits R(0) to return type Char
        .add_ret(0);
    yama::println("{}", f_bcode.fmt_disassembly());
    const auto f_consts =
        yama::const_table_info()
        .add_primitive_type("yama:Int"_str)
        .add_primitive_type("yama:Float"_str)
        .add_primitive_type("yama:Char"_str)
        .add_function_type("abc:g"_str, yama::make_callsig({ 0, 1, 0 }, 2)) // fn(Int, Float, Int) -> Char
        .add_int(10)
        .add_float(0.05)
        .add_int(-4)
        .add_char(U'y');
    const auto f = yama::make_function(
        "f"_str,
        f_consts,
        yama::make_callsig({}, 2),
        5,
        f_bcode);

    EXPECT_TRUE(verif->verify(f, get_md(), "abc"_str));
}

TEST_F(VerifierTests, BCode_Call_Newtop_MayBeMarkedReinit) {
    const auto f_bcode =
        yama::bc::code()
        // block #1
        .add_put_const(yama::newtop, 3) // inits R(0) to type of call object
        .add_put_const(yama::newtop, 4) // inits R(1) to type of arg #1
        .add_put_const(yama::newtop, 5) // inits R(2) to type of arg #2
        .add_put_const(yama::newtop, 6) // inits R(3) to type of arg #3
        .add_call(4, yama::newtop, true) // inits R(0) to return type Char
        .add_ret(0);
    yama::println("{}", f_bcode.fmt_disassembly());
    const auto f_consts =
        yama::const_table_info()
        .add_primitive_type("yama:Int"_str)
        .add_primitive_type("yama:Float"_str)
        .add_primitive_type("yama:Char"_str)
        .add_function_type("abc:g"_str, yama::make_callsig({ 0, 1, 0 }, 2)) // fn(Int, Float, Int) -> Char
        .add_int(10)
        .add_float(0.05)
        .add_int(-4)
        .add_char(U'y');
    const auto f = yama::make_function(
        "f"_str,
        f_consts,
        yama::make_callsig({}, 2),
        5,
        f_bcode);

    EXPECT_TRUE(verif->verify(f, get_md(), "abc"_str));
}

TEST_F(VerifierTests, BCode_Call_Fail_ArgRs_OutOfBounds) {
    const auto f_bcode =
        yama::bc::code()
        // block #1
        .add_put_const(yama::newtop, 3) // inits R(0) to type of call object
        .add_put_const(yama::newtop, 4) // inits R(1) to type of arg #1
        // push then pop R(2) and R(3) to better test impl robustness
        .add_put_const(yama::newtop, 5) // inits R(2) to type of arg #2
        .add_put_const(yama::newtop, 6) // inits R(3) to type of arg #3
        .add_pop(2) // pops R(2) and R(3)
        .add_call(4, yama::newtop) // [R(0), R(3)] out-of-bounds
        .add_ret(0);
    yama::println("{}", f_bcode.fmt_disassembly());
    const auto f_consts =
        yama::const_table_info()
        .add_primitive_type("yama:Int"_str)
        .add_primitive_type("yama:Float"_str)
        .add_primitive_type("yama:Char"_str)
        .add_function_type("abc:g"_str, yama::make_callsig({ 0, 1, 0 }, 2)) // fn(Int, Float, Int) -> Char
        .add_int(10)
        .add_float(0.05)
        .add_int(-4)
        .add_char(U'y');
    const auto f = yama::make_function(
        "f"_str,
        f_consts,
        yama::make_callsig({}, 2),
        4,
        f_bcode);

    EXPECT_FALSE(verif->verify(f, get_md(), "abc"_str));

    EXPECT_EQ(dbg->count(yama::dsignal::verif_ArgRs_out_of_bounds), 1);
}

TEST_F(VerifierTests, BCode_Call_Fail_ArgRs_ZeroObjects) {
    const auto f_bcode =
        yama::bc::code()
        // block #1
        .add_put_none(yama::newtop) // inits R(0) to type of return value
        // NOTE: R(top-A) being an otherwise valid call object ensures error couldn't be about R(top-A) not being one
        .add_put_const(yama::newtop, 1) // inits R(1) to type of call object (though this will go unused)
        .add_call(0, 0) // A == 0 means no call object specified
        .add_ret(0);
    yama::println("{}", f_bcode.fmt_disassembly());
    const auto f_consts =
        yama::const_table_info()
        .add_primitive_type("yama:None"_str)
        .add_function_type("abc:g"_str, yama::make_callsig({}, 0)); // fn() -> None
    const auto f = yama::make_function(
        "f"_str,
        f_consts,
        yama::make_callsig({}, 0),
        5,
        f_bcode);

    EXPECT_FALSE(verif->verify(f, get_md(), "abc"_str));

    EXPECT_EQ(dbg->count(yama::dsignal::verif_ArgRs_zero_objects), 1);
}

TEST_F(VerifierTests, BCode_Call_Fail_ArgRs_IllegalCallObject) {
    const auto f_bcode =
        yama::bc::code()
        // block #1
        .add_put_const(yama::newtop, 7) // inits R(0) to type of return value
        .add_put_const(yama::newtop, 3) // inits R(1) to type of call object
        .add_put_const(yama::newtop, 4) // inits R(2) to type of arg #1
        .add_put_const(yama::newtop, 5) // inits R(3) to type of arg #2
        .add_put_const(yama::newtop, 6) // inits R(4) to type of arg #3
        .add_call(4, 0) // R(1) cannot be used as call object
        .add_ret(0);
    yama::println("{}", f_bcode.fmt_disassembly());
    const auto f_consts =
        yama::const_table_info()
        .add_primitive_type("yama:Int"_str)
        .add_primitive_type("yama:Float"_str)
        .add_primitive_type("yama:Char"_str)
        .add_int(50) // <- cannot be used as call object
        .add_int(10)
        .add_float(0.05)
        .add_int(-4)
        .add_char(U'y');
    const auto f = yama::make_function(
        "f"_str,
        f_consts,
        yama::make_callsig({}, 2),
        5,
        f_bcode);

    EXPECT_FALSE(verif->verify(f, get_md(), "abc"_str));

    EXPECT_EQ(dbg->count(yama::dsignal::verif_ArgRs_illegal_callobj), 1);
}

TEST_F(VerifierTests, BCode_Call_Fail_ArgRs_IllegalCallObject_AttemptedCallObjectTypeIsNotInConstTable) {
    // IMPORTANT: this test exists as the verifier relies on const table for type info
    //            about things like attempted call object types, and so impl must be
    //            able to handle if this type info is missing
    // TODO: this testing need was found from a bug in our impl where it would
    //       crash if it couldn't find a const table entry for attempted call object
    //       type, w/ me feeling their may be other similar problems in our impl
    //       which our unit tests aren't catching
    const auto f_bcode =
        yama::bc::code()
        // block #1
        .add_put_const(yama::newtop, 7) // inits R(0) to type of return value
        .add_put_const(yama::newtop, 3) // inits R(1) to type of call object
        .add_put_const(yama::newtop, 4) // inits R(2) to type of arg #1
        .add_put_const(yama::newtop, 5) // inits R(3) to type of arg #2
        .add_put_const(yama::newtop, 6) // inits R(4) to type of arg #3
        .add_call(4, 0) // R(1) cannot be used as call object
        .add_ret(0);
    yama::println("{}", f_bcode.fmt_disassembly());
    const auto f_consts =
        yama::const_table_info()
        .add_primitive_type("yama:Type"_str) // <- no type yama:Int in this const table
        .add_primitive_type("yama:Float"_str)
        .add_primitive_type("yama:Char"_str)
        .add_int(50) // <- cannot be used as call object + yama:Int is not in const table
        .add_int(10)
        .add_float(0.05)
        .add_int(-4)
        .add_char(U'y');
    const auto f = yama::make_function(
        "f"_str,
        f_consts,
        yama::make_callsig({}, 2),
        5,
        f_bcode);

    EXPECT_FALSE(verif->verify(f, get_md(), "abc"_str));

    EXPECT_EQ(dbg->count(yama::dsignal::verif_ArgRs_illegal_callobj), 1);
}

TEST_F(VerifierTests, BCode_Call_Fail_ParamArgRs_TooMany) {
    const auto f_bcode =
        yama::bc::code()
        // block #1
        .add_put_const(yama::newtop, 7) // inits R(0) to type of return value
        .add_put_const(yama::newtop, 3) // inits R(1) to type of call object
        .add_put_const(yama::newtop, 4) // inits R(2) to type of arg #1
        .add_put_const(yama::newtop, 5) // inits R(3) to type of arg #2
        .add_put_const(yama::newtop, 6) // inits R(4) to type of arg #3
        .add_put_const(yama::newtop, 6) // inits R(5) to type of arg #4
        .add_call(5, 0) // 5 is too many args for call to g
        .add_ret(0);
    yama::println("{}", f_bcode.fmt_disassembly());
    const auto f_consts =
        yama::const_table_info()
        .add_primitive_type("yama:Int"_str)
        .add_primitive_type("yama:Float"_str)
        .add_primitive_type("yama:Char"_str)
        .add_function_type("abc:g"_str, yama::make_callsig({ 0, 1, 0 }, 2)) // fn(Int, Float, Int) -> Char
        .add_int(10)
        .add_float(0.05)
        .add_int(-4)
        .add_char(U'y');
    const auto f = yama::make_function(
        "f"_str,
        f_consts,
        yama::make_callsig({}, 2),
        6,
        f_bcode);

    EXPECT_FALSE(verif->verify(f, get_md(), "abc"_str));

    EXPECT_EQ(dbg->count(yama::dsignal::verif_ParamArgRs_wrong_number), 1);
}

TEST_F(VerifierTests, BCode_Call_Fail_ParamArgRs_TooFew) {
    const auto f_bcode =
        yama::bc::code()
        // block #1
        .add_put_const(yama::newtop, 7) // inits R(0) to type of return value
        .add_put_const(yama::newtop, 3) // inits R(1) to type of call object
        .add_put_const(yama::newtop, 4) // inits R(2) to type of arg #1
        .add_put_const(yama::newtop, 5) // inits R(3) to type of arg #2
        .add_call(3, 0) // 3 is too few args for call to g
        .add_ret(0);
    yama::println("{}", f_bcode.fmt_disassembly());
    const auto f_consts =
        yama::const_table_info()
        .add_primitive_type("yama:Int"_str)
        .add_primitive_type("yama:Float"_str)
        .add_primitive_type("yama:Char"_str)
        .add_function_type("abc:g"_str, yama::make_callsig({ 0, 1, 0 }, 2)) // fn(Int, Float, Int) -> Char
        .add_int(10)
        .add_float(0.05)
        .add_int(-4)
        .add_char(U'y');
    const auto f = yama::make_function(
        "f"_str,
        f_consts,
        yama::make_callsig({}, 2),
        4,
        f_bcode);

    EXPECT_FALSE(verif->verify(f, get_md(), "abc"_str));

    EXPECT_EQ(dbg->count(yama::dsignal::verif_ParamArgRs_wrong_number), 1);
}

TEST_F(VerifierTests, BCode_Call_Fail_ParamArgRs_WrongTypes) {
    const auto f_bcode =
        yama::bc::code()
        // block #1
        .add_put_const(yama::newtop, 7) // inits R(0) to type of return value
        .add_put_const(yama::newtop, 3) // inits R(1) to type of call object
        .add_put_const(yama::newtop, 4) // inits R(2) to type of arg #1
        .add_put_const(yama::newtop, 5) // inits R(3) to type of arg #2
        .add_put_const(yama::newtop, 6) // inits R(4) to type of arg #3
        .add_call(4, 0) // arg #2 is UInt, but a Float was expected
        .add_ret(0);
    yama::println("{}", f_bcode.fmt_disassembly());
    const auto f_consts =
        yama::const_table_info()
        .add_primitive_type("yama:Int"_str)
        .add_primitive_type("yama:Float"_str)
        .add_primitive_type("yama:Char"_str)
        .add_function_type("abc:g"_str, yama::make_callsig({ 0, 1, 0 }, 2)) // fn(Int, Float, Int) -> Char
        .add_int(10)
        .add_uint(1000) // <- wrong type for arg #2 for call to g
        .add_int(-4)
        .add_char(U'y');
    const auto f = yama::make_function(
        "f"_str,
        f_consts,
        yama::make_callsig({}, 2),
        5,
        f_bcode);

    EXPECT_FALSE(verif->verify(f, get_md(), "abc"_str));

    EXPECT_EQ(dbg->count(yama::dsignal::verif_ParamArgRs_wrong_types), 1);
}

TEST_F(VerifierTests, BCode_Call_Fail_RB_OutOfBounds_AfterTheCall) {
    const auto f_bcode =
        yama::bc::code()
        // block #1
        .add_put_const(yama::newtop, 3) // inits R(0) to type of call object
        .add_put_const(yama::newtop, 4) // inits R(1) to type of arg #1
        .add_put_const(yama::newtop, 5) // inits R(2) to type of arg #2
        .add_put_const(yama::newtop, 6) // inits R(3) to type of arg #3
        .add_call(4, 0, true) // R(0) out-of-bounds after the call
        .add_ret(0);
    yama::println("{}", f_bcode.fmt_disassembly());
    const auto f_consts =
        yama::const_table_info()
        .add_primitive_type("yama:Int"_str)
        .add_primitive_type("yama:Float"_str)
        .add_primitive_type("yama:Char"_str)
        .add_function_type("abc:g"_str, yama::make_callsig({ 0, 1, 0 }, 2)) // fn(Int, Float, Int) -> Char
        .add_int(10)
        .add_float(0.05)
        .add_int(-4)
        .add_char(U'y');
    const auto f = yama::make_function(
        "f"_str,
        f_consts,
        yama::make_callsig({}, 2),
        6,
        f_bcode);

    EXPECT_FALSE(verif->verify(f, get_md(), "abc"_str));

    EXPECT_EQ(dbg->count(yama::dsignal::verif_RB_out_of_bounds), 1);
}

TEST_F(VerifierTests, BCode_Call_Fail_RB_WrongType) {
    const auto f_bcode =
        yama::bc::code()
        // block #1
        .add_put_const(yama::newtop, 7) // inits R(0) to NOT type of return value
        .add_put_const(yama::newtop, 3) // inits R(1) to type of call object
        .add_put_const(yama::newtop, 4) // inits R(2) to type of arg #1
        .add_put_const(yama::newtop, 5) // inits R(3) to type of arg #2
        .add_put_const(yama::newtop, 6) // inits R(4) to type of arg #3
        .add_call(4, 0) // R(0) not type Char
        .add_ret(0);
    yama::println("{}", f_bcode.fmt_disassembly());
    const auto f_consts =
        yama::const_table_info()
        .add_primitive_type("yama:Int"_str)
        .add_primitive_type("yama:Float"_str)
        .add_primitive_type("yama:Char"_str)
        .add_function_type("abc:g"_str, yama::make_callsig({ 0, 1, 0 }, 2)) // fn(Int, Float, Int) -> Char
        .add_int(10)
        .add_float(0.05)
        .add_int(-4)
        .add_uint(1000); // <- return type of g is Char, not UInt
    const auto f = yama::make_function(
        "f"_str,
        f_consts,
        yama::make_callsig({}, 2),
        5,
        f_bcode);

    EXPECT_FALSE(verif->verify(f, get_md(), "abc"_str));

    EXPECT_EQ(dbg->count(yama::dsignal::verif_RB_wrong_type), 1);
}

static_assert(yama::kinds == 4); // one BCode_CallNR_# test for each callable kind

TEST_F(VerifierTests, BCode_CallNR_Function) {
    const auto f_bcode =
        yama::bc::code()
        // block #1
        .add_put_const(yama::newtop, 3) // inits R(0) to type of call object
        .add_put_const(yama::newtop, 4) // inits R(1) to type of arg #1
        .add_put_const(yama::newtop, 5) // inits R(2) to type of arg #2
        .add_put_const(yama::newtop, 6) // inits R(3) to type of arg #3
        .add_call_nr(4)
        .add_put_none(yama::newtop)
        .add_ret(0); // return the None object from R(0)
    yama::println("{}", f_bcode.fmt_disassembly());
    const auto f_consts =
        yama::const_table_info()
        .add_primitive_type("yama:Int"_str)
        .add_primitive_type("yama:Float"_str)
        .add_primitive_type("yama:Char"_str)
        .add_function_type("abc:g"_str, yama::make_callsig({ 0, 1, 0 }, 7)) // fn(Int, Float, Int) -> None
        .add_int(10)
        .add_float(0.05)
        .add_int(-4)
        .add_primitive_type("yama:None"_str);
    const auto f = yama::make_function(
        "f"_str,
        f_consts,
        yama::make_callsig({}, 7),
        4,
        f_bcode);

    EXPECT_TRUE(verif->verify(f, get_md(), "abc"_str));
}

TEST_F(VerifierTests, BCode_CallNR_Method) {
    const auto f_bcode =
        yama::bc::code()
        // block #1
        .add_put_const(yama::newtop, 3) // inits R(0) to type of call object
        .add_put_const(yama::newtop, 4) // inits R(1) to type of arg #1
        .add_put_const(yama::newtop, 5) // inits R(2) to type of arg #2
        .add_put_const(yama::newtop, 6) // inits R(3) to type of arg #3
        .add_call_nr(4)
        .add_put_none(yama::newtop)
        .add_ret(0); // return the None object from R(0)
    yama::println("{}", f_bcode.fmt_disassembly());
    const auto f_consts =
        yama::const_table_info()
        .add_primitive_type("yama:Int"_str)
        .add_primitive_type("yama:Float"_str)
        .add_primitive_type("yama:Char"_str)
        .add_method_type("abc:Something::g"_str, yama::make_callsig({ 0, 1, 0 }, 7)) // fn(Int, Float, Int) -> None
        .add_int(10)
        .add_float(0.05)
        .add_int(-4)
        .add_primitive_type("yama:None"_str);
    const auto f = yama::make_function(
        "f"_str,
        f_consts,
        yama::make_callsig({}, 7),
        4,
        f_bcode);

    EXPECT_TRUE(verif->verify(f, get_md(), "abc"_str));
}

TEST_F(VerifierTests, BCode_CallNR_Fail_ArgRs_OutOfBounds) {
    const auto f_bcode =
        yama::bc::code()
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
    yama::println("{}", f_bcode.fmt_disassembly());
    const auto f_consts =
        yama::const_table_info()
        .add_primitive_type("yama:Int"_str)
        .add_primitive_type("yama:Float"_str)
        .add_primitive_type("yama:Char"_str)
        .add_function_type("abc:g"_str, yama::make_callsig({ 0, 1, 0 }, 2)) // fn(Int, Float, Int) -> Char
        .add_int(10)
        .add_float(0.05)
        .add_int(-4)
        .add_char(U'y');
    const auto f = yama::make_function(
        "f"_str,
        f_consts,
        yama::make_callsig({}, 2),
        4,
        f_bcode);

    EXPECT_FALSE(verif->verify(f, get_md(), "abc"_str));

    EXPECT_EQ(dbg->count(yama::dsignal::verif_ArgRs_out_of_bounds), 1);
}

TEST_F(VerifierTests, BCode_CallNR_Fail_ArgRs_ZeroObjects) {
    const auto f_bcode =
        yama::bc::code()
        // block #1
        // NOTE: R(top-A) being a valid call object ensures error couldn't be about R(top-A) not being one
        .add_put_const(yama::newtop, 1) // inits R(0) to type of call object (though this will go unused)
        .add_call_nr(0) // A == 0 means no call object specified
        .add_put_none(0, true)
        .add_ret(0);
    yama::println("{}", f_bcode.fmt_disassembly());
    const auto f_consts =
        yama::const_table_info()
        .add_primitive_type("yama:None"_str)
        .add_function_type("abc:g"_str, yama::make_callsig({}, 0)); // fn() -> None
    const auto f = yama::make_function(
        "f"_str,
        f_consts,
        yama::make_callsig({}, 0),
        5,
        f_bcode);

    EXPECT_FALSE(verif->verify(f, get_md(), "abc"_str));

    EXPECT_EQ(dbg->count(yama::dsignal::verif_ArgRs_zero_objects), 1);
}

TEST_F(VerifierTests, BCode_CallNR_Fail_ArgRs_IllegalCallObject) {
    const auto f_bcode =
        yama::bc::code()
        // block #1
        .add_put_const(yama::newtop, 3) // inits R(0) to type of call object
        .add_put_const(yama::newtop, 4) // inits R(1) to type of arg #1
        .add_put_const(yama::newtop, 5) // inits R(2) to type of arg #2
        .add_put_const(yama::newtop, 6) // inits R(3) to type of arg #3
        .add_call_nr(4) // R(0) cannot be used as call object
        .add_put_none(0, true)
        .add_ret(0);
    yama::println("{}", f_bcode.fmt_disassembly());
    const auto f_consts =
        yama::const_table_info()
        .add_primitive_type("yama:Int"_str)
        .add_primitive_type("yama:Float"_str)
        .add_primitive_type("yama:Char"_str)
        .add_int(50) // <- cannot be used as call object
        .add_int(10)
        .add_float(0.05)
        .add_int(-4)
        .add_char(U'y');
    const auto f = yama::make_function(
        "f"_str,
        f_consts,
        yama::make_callsig({}, 2),
        5,
        f_bcode);

    EXPECT_FALSE(verif->verify(f, get_md(), "abc"_str));

    EXPECT_EQ(dbg->count(yama::dsignal::verif_ArgRs_illegal_callobj), 1);
}

TEST_F(VerifierTests, BCode_CallNR_Fail_ArgRs_IllegalCallObject_AttemptedCallObjectTypeIsNotInConstTable) {
    // IMPORTANT: this test exists as the verifier relies on const table for type info
    //            about things like attempted call object types, and so impl must be
    //            able to handle if this type info is missing
    // TODO: this testing need was found from a bug in our impl where it would
    //       crash if it couldn't find a const table entry for attempted call object
    //       type, w/ me feeling their may be other similar problems in our impl
    //       which our unit tests aren't catching
    const auto f_bcode =
        yama::bc::code()
        // block #1
        .add_put_const(yama::newtop, 3) // inits R(0) to type of call object
        .add_put_const(yama::newtop, 4) // inits R(1) to type of arg #1
        .add_put_const(yama::newtop, 5) // inits R(2) to type of arg #2
        .add_put_const(yama::newtop, 6) // inits R(3) to type of arg #3
        .add_call_nr(4) // R(0) cannot be used as call object
        .add_put_none(0, true)
        .add_ret(0);
    yama::println("{}", f_bcode.fmt_disassembly());
    const auto f_consts =
        yama::const_table_info()
        .add_primitive_type("yama:Type"_str) // <- no type yama:Int in this const table
        .add_primitive_type("yama:Float"_str)
        .add_primitive_type("yama:Char"_str)
        .add_int(50) // <- cannot be used as call object + yama:Int is not in const table
        .add_int(10)
        .add_float(0.05)
        .add_int(-4)
        .add_char(U'y');
    const auto f = yama::make_function(
        "f"_str,
        f_consts,
        yama::make_callsig({}, 2),
        5,
        f_bcode);

    EXPECT_FALSE(verif->verify(f, get_md(), "abc"_str));

    EXPECT_EQ(dbg->count(yama::dsignal::verif_ArgRs_illegal_callobj), 1);
}

TEST_F(VerifierTests, BCode_CallNR_Fail_ParamArgRs_TooMany) {
    const auto f_bcode =
        yama::bc::code()
        // block #1
        .add_put_const(yama::newtop, 3) // inits R(0) to type of call object
        .add_put_const(yama::newtop, 4) // inits R(1) to type of arg #1
        .add_put_const(yama::newtop, 5) // inits R(2) to type of arg #2
        .add_put_const(yama::newtop, 6) // inits R(3) to type of arg #3
        .add_put_const(yama::newtop, 6) // inits R(4) to type of arg #4
        .add_call_nr(5) // 5 is too many args for call to g
        .add_put_none(0, true)
        .add_ret(0);
    yama::println("{}", f_bcode.fmt_disassembly());
    const auto f_consts =
        yama::const_table_info()
        .add_primitive_type("yama:Int"_str)
        .add_primitive_type("yama:Float"_str)
        .add_primitive_type("yama:Char"_str)
        .add_function_type("abc:g"_str, yama::make_callsig({ 0, 1, 0 }, 2)) // fn(Int, Float, Int) -> Char
        .add_int(10)
        .add_float(0.05)
        .add_int(-4)
        .add_char(U'y');
    const auto f = yama::make_function(
        "f"_str,
        f_consts,
        yama::make_callsig({}, 2),
        6,
        f_bcode);

    EXPECT_FALSE(verif->verify(f, get_md(), "abc"_str));

    EXPECT_EQ(dbg->count(yama::dsignal::verif_ParamArgRs_wrong_number), 1);
}

TEST_F(VerifierTests, BCode_CallNR_Fail_ParamArgRs_TooFew) {
    const auto f_bcode =
        yama::bc::code()
        // block #1
        .add_put_const(yama::newtop, 3) // inits R(0) to type of call object
        .add_put_const(yama::newtop, 4) // inits R(1) to type of arg #1
        .add_put_const(yama::newtop, 5) // inits R(2) to type of arg #2
        .add_call_nr(3) // 3 is too few args for call to g
        .add_put_none(0, true)
        .add_ret(0);
    yama::println("{}", f_bcode.fmt_disassembly());
    const auto f_consts =
        yama::const_table_info()
        .add_primitive_type("yama:Int"_str)
        .add_primitive_type("yama:Float"_str)
        .add_primitive_type("yama:Char"_str)
        .add_function_type("abc:g"_str, yama::make_callsig({ 0, 1, 0 }, 2)) // fn(Int, Float, Int) -> Char
        .add_int(10)
        .add_float(0.05)
        .add_int(-4)
        .add_char(U'y');
    const auto f = yama::make_function(
        "f"_str,
        f_consts,
        yama::make_callsig({}, 2),
        4,
        f_bcode);

    EXPECT_FALSE(verif->verify(f, get_md(), "abc"_str));

    EXPECT_EQ(dbg->count(yama::dsignal::verif_ParamArgRs_wrong_number), 1);
}

TEST_F(VerifierTests, BCode_CallNR_Fail_ParamArgRs_WrongTypes) {
    const auto f_bcode =
        yama::bc::code()
        // block #1
        .add_put_const(yama::newtop, 3) // inits R(0) to type of call object
        .add_put_const(yama::newtop, 4) // inits R(1) to type of arg #1
        .add_put_const(yama::newtop, 5) // inits R(2) to type of arg #2
        .add_put_const(yama::newtop, 6) // inits R(3) to type of arg #3
        .add_call_nr(4) // arg #2 is UInt, but a Float was expected
        .add_put_none(0, true)
        .add_ret(0);
    yama::println("{}", f_bcode.fmt_disassembly());
    const auto f_consts =
        yama::const_table_info()
        .add_primitive_type("yama:Int"_str)
        .add_primitive_type("yama:Float"_str)
        .add_primitive_type("yama:Char"_str)
        .add_function_type("abc:g"_str, yama::make_callsig({ 0, 1, 0 }, 2)) // fn(Int, Float, Int) -> Char
        .add_int(10)
        .add_uint(1000) // <- wrong type for arg #2 for call to g
        .add_int(-4)
        .add_char(U'y');
    const auto f = yama::make_function(
        "f"_str,
        f_consts,
        yama::make_callsig({}, 2),
        5,
        f_bcode);

    EXPECT_FALSE(verif->verify(f, get_md(), "abc"_str));

    EXPECT_EQ(dbg->count(yama::dsignal::verif_ParamArgRs_wrong_types), 1);
}

TEST_F(VerifierTests, BCode_Ret) {
    const auto f_bcode =
        yama::bc::code()
        // block #1
        .add_put_const(yama::newtop, 1) // inits R(0) (to Float 0.05)
        .add_ret(0); // return Float
    yama::println("{}", f_bcode.fmt_disassembly());
    const auto f_consts =
        yama::const_table_info()
        .add_primitive_type("yama:Float"_str)
        .add_float(0.05);
    const auto f = yama::make_function(
        "f"_str,
        f_consts,
        yama::make_callsig({}, 0),
        1,
        f_bcode);

    EXPECT_TRUE(verif->verify(f, get_md(), "abc"_str));
}

TEST_F(VerifierTests, BCode_Ret_Fail_RA_OutOfBounds) {
    const auto f_bcode =
        yama::bc::code()
        // block #1
        // push then pop R(0) to better test impl robustness
        .add_put_none(yama::newtop) // inits R(0) (to None)
        .add_pop(1) // pops R(0)
        .add_ret(0); // R(0) out-of-bounds
    yama::println("{}", f_bcode.fmt_disassembly());
    const auto f_consts =
        yama::const_table_info()
        .add_primitive_type("yama:None"_str);
    const auto f = yama::make_function(
        "f"_str,
        f_consts,
        yama::make_callsig({}, 0),
        1,
        f_bcode);

    EXPECT_FALSE(verif->verify(f, get_md(), "abc"_str));

    EXPECT_EQ(dbg->count(yama::dsignal::verif_RA_out_of_bounds), 1);
}

TEST_F(VerifierTests, BCode_Ret_Fail_RA_WrongType) {
    const auto f_bcode =
        yama::bc::code()
        // block #1
        .add_put_const(yama::newtop, 1) // inits R(0) (to Int 10)
        .add_ret(0); // R(0) not type Float
    yama::println("{}", f_bcode.fmt_disassembly());
    const auto f_consts =
        yama::const_table_info()
        .add_primitive_type("yama:Float"_str)
        .add_int(10);
    const auto f = yama::make_function(
        "f"_str,
        f_consts,
        yama::make_callsig({}, 0),
        1,
        f_bcode);

    EXPECT_FALSE(verif->verify(f, get_md(), "abc"_str));

    EXPECT_EQ(dbg->count(yama::dsignal::verif_RA_wrong_type), 1);
}

TEST_F(VerifierTests, BCode_Jump) {
    const auto f_bcode =
        yama::bc::code()
        // block #1
        .add_jump(2) // jump to block #3
        // block #2 (dead code)
        // this dead code block ensures that we actually *jump over* block #2
        .add_put_const(yama::newtop, 1) // inits R(0) (to Int 10)
        .add_ret(0) // <- static verif error if reachable
        // block #3
        .add_put_const(yama::newtop, 2) // inits R(0) (to Float 0.05)
        .add_ret(0);
    yama::println("{}", f_bcode.fmt_disassembly());
    const auto f_consts =
        yama::const_table_info()
        .add_primitive_type("yama:Float"_str)
        .add_int(10)
        .add_float(0.05);
    const auto f = yama::make_function(
        "f"_str,
        f_consts,
        yama::make_callsig({}, 0),
        1,
        f_bcode);

    EXPECT_TRUE(verif->verify(f, get_md(), "abc"_str));
}

TEST_F(VerifierTests, BCode_Jump_Fail_PutsPCOutOfBounds) {
    const auto f_bcode =
        yama::bc::code()
        // block #1
        .add_jump(1); // jump to out-of-bounds instruction index
    yama::println("{}", f_bcode.fmt_disassembly());
    const auto f_consts =
        yama::const_table_info()
        .add_primitive_type("yama:None"_str);
    const auto f = yama::make_function(
        "f"_str,
        f_consts,
        yama::make_callsig({}, 0),
        1,
        f_bcode);

    EXPECT_FALSE(verif->verify(f, get_md(), "abc"_str));

    EXPECT_EQ(dbg->count(yama::dsignal::verif_puts_PC_out_of_bounds), 1);
}

TEST_F(VerifierTests, BCode_Jump_Fail_ViolatesRegisterCoherence) {
    const auto f_bcode =
        yama::bc::code()
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
    yama::println("{}", f_bcode.fmt_disassembly());
    const auto f_consts =
        yama::const_table_info()
        .add_primitive_type("yama:None"_str)
        .add_bool(true)
        .add_float(3.14159)
        .add_int(100);
    const auto f = yama::make_function(
        "f"_str,
        f_consts,
        yama::make_callsig({}, 0),
        1,
        f_bcode);

    EXPECT_FALSE(verif->verify(f, get_md(), "abc"_str));

    EXPECT_EQ(dbg->count(yama::dsignal::verif_violates_register_coherence), 1);
}

TEST_F(VerifierTests, BCode_JumpTrue_PopOne) {
    const auto f_bcode =
        yama::bc::code()
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
    yama::println("{}", f_bcode.fmt_disassembly());
    const auto f_consts =
        yama::const_table_info()
        .add_primitive_type("yama:None"_str)
        .add_bool(true);
    const auto f = yama::make_function(
        "f"_str,
        f_consts,
        yama::make_callsig({}, 0),
        2,
        f_bcode);

    EXPECT_TRUE(verif->verify(f, get_md(), "abc"_str));
}

TEST_F(VerifierTests, BCode_JumpTrue_PopZero) {
    const auto f_bcode =
        yama::bc::code()
        // block #1
        .add_put_const(yama::newtop, 1) // inits R(0) (to Bool true)
        .add_jump_true(0, 1) // jump to block #3, or fallthrough to block #2, *popping nothing*
        // block #2
        // expect R(0) to still be good
        .add_ret(0)
        // block #3
        // expect R(0) to still be good
        .add_ret(0);
    yama::println("{}", f_bcode.fmt_disassembly());
    const auto f_consts =
        yama::const_table_info()
        .add_primitive_type("yama:Bool"_str)
        .add_bool(true);
    const auto f = yama::make_function(
        "f"_str,
        f_consts,
        yama::make_callsig({}, 0),
        1,
        f_bcode);

    EXPECT_TRUE(verif->verify(f, get_md(), "abc"_str));
}

TEST_F(VerifierTests, BCode_JumpTrue_PopMany) {
    const auto f_bcode =
        yama::bc::code()
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
    yama::println("{}", f_bcode.fmt_disassembly());
    const auto f_consts =
        yama::const_table_info()
        .add_primitive_type("yama:None"_str)
        .add_bool(true);
    const auto f = yama::make_function(
        "f"_str,
        f_consts,
        yama::make_callsig({}, 0),
        5,
        f_bcode);

    EXPECT_TRUE(verif->verify(f, get_md(), "abc"_str));
}

TEST_F(VerifierTests, BCode_JumpTrue_PopMoreThanAreOnStack) {
    const auto f_bcode =
        yama::bc::code()
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
    yama::println("{}", f_bcode.fmt_disassembly());
    const auto f_consts =
        yama::const_table_info()
        .add_primitive_type("yama:None"_str)
        .add_bool(true);
    const auto f = yama::make_function(
        "f"_str,
        f_consts,
        yama::make_callsig({}, 0),
        5,
        f_bcode);

    EXPECT_TRUE(verif->verify(f, get_md(), "abc"_str));
}

TEST_F(VerifierTests, BCode_JumpTrue_Fail_RTop_DoesNotExist) {
    const auto f_bcode =
        yama::bc::code()
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
    yama::println("{}", f_bcode.fmt_disassembly());
    const auto f_consts =
        yama::const_table_info()
        .add_primitive_type("yama:Float"_str)
        .add_bool(true)
        .add_int(10)
        .add_float(0.05);
    const auto f = yama::make_function(
        "f"_str,
        f_consts,
        yama::make_callsig({}, 0),
        1,
        f_bcode);

    EXPECT_FALSE(verif->verify(f, get_md(), "abc"_str));

    EXPECT_EQ(dbg->count(yama::dsignal::verif_RTop_does_not_exist), 1);
}

TEST_F(VerifierTests, BCode_JumpTrue_Fail_RTop_WrongType) {
    const auto f_bcode =
        yama::bc::code()
        // block #1
        .add_put_const(yama::newtop, 1) // inits R(0) (to Int -4)
        .add_jump_true(1, 2) // R(0) not type Bool
        // block #2
        .add_put_const(yama::newtop, 3) // inits R(0) (to Float 0.05)
        .add_ret(0)
        // block #3
        .add_put_const(yama::newtop, 3) // inits R(0) (to Float 0.05)
        .add_ret(0);
    yama::println("{}", f_bcode.fmt_disassembly());
    const auto f_consts =
        yama::const_table_info()
        .add_primitive_type("yama:Float"_str)
        .add_int(-4)
        .add_int(10)
        .add_float(0.05);
    const auto f = yama::make_function(
        "f"_str,
        f_consts,
        yama::make_callsig({}, 0),
        1,
        f_bcode);

    EXPECT_FALSE(verif->verify(f, get_md(), "abc"_str));

    EXPECT_EQ(dbg->count(yama::dsignal::verif_RTop_wrong_type), 1);
}

TEST_F(VerifierTests, BCode_JumpTrue_Fail_PutsPCOutOfBounds) {
    const auto f_bcode =
        yama::bc::code()
        // block #1
        .add_put_const(yama::newtop, 1) // inits R(0) (to Bool true)
        .add_jump_true(1, 10'000) // branches to out-of-bounds instruction
        // block #2
        .add_put_const(yama::newtop, 3) // inits R(0) (to Float 0.05)
        .add_ret(0)
        // block #3
        .add_put_const(yama::newtop, 3) // inits R(0) (to Float 0.05)
        .add_ret(0);
    yama::println("{}", f_bcode.fmt_disassembly());
    const auto f_consts =
        yama::const_table_info()
        .add_primitive_type("yama:Float"_str)
        .add_bool(true)
        .add_int(10)
        .add_float(0.05);
    const auto f = yama::make_function(
        "f"_str,
        f_consts,
        yama::make_callsig({}, 0),
        1,
        f_bcode);

    EXPECT_FALSE(verif->verify(f, get_md(), "abc"_str));

    EXPECT_EQ(dbg->count(yama::dsignal::verif_puts_PC_out_of_bounds), 1);
}

TEST_F(VerifierTests, BCode_JumpTrue_Fail_ViolatesRegisterCoherence) {
    const auto f_bcode =
        yama::bc::code()
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
    yama::println("{}", f_bcode.fmt_disassembly());
    const auto f_consts =
        yama::const_table_info()
        .add_primitive_type("yama:None"_str)
        .add_bool(true)
        .add_float(3.14159)
        .add_int(100);
    const auto f = yama::make_function(
        "f"_str,
        f_consts,
        yama::make_callsig({}, 0),
        2,
        f_bcode);

    EXPECT_FALSE(verif->verify(f, get_md(), "abc"_str));

    EXPECT_EQ(dbg->count(yama::dsignal::verif_violates_register_coherence), 1);
}

TEST_F(VerifierTests, BCode_JumpFalse_PopOne) {
    const auto f_bcode =
        yama::bc::code()
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
    yama::println("{}", f_bcode.fmt_disassembly());
    const auto f_consts =
        yama::const_table_info()
        .add_primitive_type("yama:None"_str)
        .add_bool(true);
    const auto f = yama::make_function(
        "f"_str,
        f_consts,
        yama::make_callsig({}, 0),
        2,
        f_bcode);

    EXPECT_TRUE(verif->verify(f, get_md(), "abc"_str));
}

TEST_F(VerifierTests, BCode_JumpFalse_PopZero) {
    const auto f_bcode =
        yama::bc::code()
        // block #1
        .add_put_const(yama::newtop, 1) // inits R(0) (to Bool true)
        .add_jump_false(0, 1) // jump to block #3, or fallthrough to block #2, *popping nothing*
        // block #2
        // expect R(0) to still be good
        .add_ret(0)
        // block #3
        // expect R(0) to still be good
        .add_ret(0);
    yama::println("{}", f_bcode.fmt_disassembly());
    const auto f_consts =
        yama::const_table_info()
        .add_primitive_type("yama:Bool"_str)
        .add_bool(true);
    const auto f = yama::make_function(
        "f"_str,
        f_consts,
        yama::make_callsig({}, 0),
        1,
        f_bcode);

    EXPECT_TRUE(verif->verify(f, get_md(), "abc"_str));
}

TEST_F(VerifierTests, BCode_JumpFalse_PopMany) {
    const auto f_bcode =
        yama::bc::code()
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
    yama::println("{}", f_bcode.fmt_disassembly());
    const auto f_consts =
        yama::const_table_info()
        .add_primitive_type("yama:None"_str)
        .add_bool(true);
    const auto f = yama::make_function(
        "f"_str,
        f_consts,
        yama::make_callsig({}, 0),
        5,
        f_bcode);

    EXPECT_TRUE(verif->verify(f, get_md(), "abc"_str));
}

TEST_F(VerifierTests, BCode_JumpFalse_PopMoreThanAreOnStack) {
    const auto f_bcode =
        yama::bc::code()
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
    yama::println("{}", f_bcode.fmt_disassembly());
    const auto f_consts =
        yama::const_table_info()
        .add_primitive_type("yama:None"_str)
        .add_bool(true);
    const auto f = yama::make_function(
        "f"_str,
        f_consts,
        yama::make_callsig({}, 0),
        5,
        f_bcode);

    EXPECT_TRUE(verif->verify(f, get_md(), "abc"_str));
}

TEST_F(VerifierTests, BCode_JumpFalse_Fail_RTop_DoesNotExist) {
    const auto f_bcode =
        yama::bc::code()
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
    yama::println("{}", f_bcode.fmt_disassembly());
    const auto f_consts =
        yama::const_table_info()
        .add_primitive_type("yama:Float"_str)
        .add_bool(true)
        .add_int(10)
        .add_float(0.05);
    const auto f = yama::make_function(
        "f"_str,
        f_consts,
        yama::make_callsig({}, 0),
        1,
        f_bcode);

    EXPECT_FALSE(verif->verify(f, get_md(), "abc"_str));

    EXPECT_EQ(dbg->count(yama::dsignal::verif_RTop_does_not_exist), 1);
}

TEST_F(VerifierTests, BCode_JumpFalse_Fail_RTop_WrongType) {
    const auto f_bcode =
        yama::bc::code()
        // block #1
        .add_put_const(yama::newtop, 1) // inits R(0) (to Int -4)
        .add_jump_false(1, 2) // R(0) not type Bool
        // block #2
        .add_put_const(yama::newtop, 3) // inits R(0) (to Float 0.05)
        .add_ret(0)
        // block #3
        .add_put_const(yama::newtop, 3) // inits R(0) (to Float 0.05)
        .add_ret(0);
    yama::println("{}", f_bcode.fmt_disassembly());
    const auto f_consts =
        yama::const_table_info()
        .add_primitive_type("yama:Float"_str)
        .add_int(-4)
        .add_int(10)
        .add_float(0.05);
    const auto f = yama::make_function(
        "f"_str,
        f_consts,
        yama::make_callsig({}, 0),
        1,
        f_bcode);

    EXPECT_FALSE(verif->verify(f, get_md(), "abc"_str));

    EXPECT_EQ(dbg->count(yama::dsignal::verif_RTop_wrong_type), 1);
}

TEST_F(VerifierTests, BCode_JumpFalse_Fail_PutsPCOutOfBounds) {
    const auto f_bcode =
        yama::bc::code()
        // block #1
        .add_put_const(yama::newtop, 1) // inits R(0) (to Bool true)
        .add_jump_false(1, 10'000) // branches to out-of-bounds instruction
        // block #2
        .add_put_const(yama::newtop, 3) // inits R(0) (to Float 0.05)
        .add_ret(0)
        // block #3
        .add_put_const(yama::newtop, 3) // inits R(0) (to Float 0.05)
        .add_ret(0);
    yama::println("{}", f_bcode.fmt_disassembly());
    const auto f_consts =
        yama::const_table_info()
        .add_primitive_type("yama:Float"_str)
        .add_bool(true)
        .add_int(10)
        .add_float(0.05);
    const auto f = yama::make_function(
        "f"_str,
        f_consts,
        yama::make_callsig({}, 0),
        1,
        f_bcode);

    EXPECT_FALSE(verif->verify(f, get_md(), "abc"_str));

    EXPECT_EQ(dbg->count(yama::dsignal::verif_puts_PC_out_of_bounds), 1);
}

TEST_F(VerifierTests, BCode_JumpFalse_Fail_ViolatesRegisterCoherence) {
    const auto f_bcode =
        yama::bc::code()
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
    yama::println("{}", f_bcode.fmt_disassembly());
    const auto f_consts =
        yama::const_table_info()
        .add_primitive_type("yama:None"_str)
        .add_bool(true)
        .add_float(3.14159)
        .add_int(100);
    const auto f = yama::make_function(
        "f"_str,
        f_consts,
        yama::make_callsig({}, 0),
        2,
        f_bcode);

    EXPECT_FALSE(verif->verify(f, get_md(), "abc"_str));

    EXPECT_EQ(dbg->count(yama::dsignal::verif_violates_register_coherence), 1);
}


// MODULE-LEVEL

TEST_F(VerifierTests, ModuleLevel_Empty) {
    // test w/ empty module_info

    yama::module_factory mf{};

    auto m = mf.done();

    EXPECT_TRUE(verif->verify(m, get_md(), "abc"_str));
}

TEST_F(VerifierTests, ModuleLevel_Populated) {
    // test w/ populated module_info

    yama::module_factory mf{};

    static_assert(yama::kinds == 4);

    // primitive
    auto A_consts =
        yama::const_table_info();
    mf.add_primitive(
        "A"_str,
        std::move(A_consts),
        yama::ptype::bool0);

    // function (native)
    auto B_consts =
        yama::const_table_info()
        .add_primitive_type("abc:b"_str)
        .add_function_type("abc:c"_str, yama::make_callsig({ 0 }, 1)) // ie. 'fn(b) -> c'
        .add_primitive_type("abc:d"_str);
    mf.add_function(
        "B"_str,
        std::move(B_consts),
        yama::make_callsig({ 0, 1, 2 }, 0),
        4,
        yama::noop_call_fn);

    // function (bcode)
    auto C_bcode =
        yama::bc::code()
        // block #1
        .add_put_const(yama::newtop, 1) // inits R(0) (to Float 0.05)
        .add_put_const(yama::newtop, 2) // inits R(1) (to Float 3.14159)
        .add_copy(0, 1) // no reinit, so R(1) MUST be Float already
        .add_ret(1);
    auto C_consts =
        yama::const_table_info()
        .add_primitive_type("yama:Float"_str)
        .add_float(0.05)
        .add_float(3.14159);
    mf.add_function(
        "C"_str,
        std::move(C_consts),
        yama::make_callsig({}, 0),
        2,
        std::move(C_bcode));
    
    // method (native)
    auto D_consts =
        yama::const_table_info()
        .add_primitive_type("abc:b"_str)
        .add_function_type("abc:c"_str, yama::make_callsig({ 0 }, 1)) // ie. 'fn(b) -> c'
        .add_primitive_type("abc:d"_str);
    mf.add_method(
        "A::D"_str,
        std::move(D_consts),
        yama::make_callsig({ 0, 1, 2 }, 0),
        4,
        yama::noop_call_fn);

    // method (bcode)
    auto E_bcode =
        yama::bc::code()
        // block #1
        .add_put_const(yama::newtop, 1) // inits R(0) (to Float 0.05)
        .add_put_const(yama::newtop, 2) // inits R(1) (to Float 3.14159)
        .add_copy(0, 1) // no reinit, so R(1) MUST be Float already
        .add_ret(1);
    auto E_consts =
        yama::const_table_info()
        .add_primitive_type("yama:Float"_str)
        .add_float(0.05)
        .add_float(3.14159);
    mf.add_method(
        "A::E"_str,
        std::move(E_consts),
        yama::make_callsig({}, 0),
        2,
        std::move(E_bcode));

    // struct
    auto F_consts =
        yama::const_table_info();
    mf.add_struct(
        "F"_str,
        std::move(F_consts));

    auto m = mf.done();

    EXPECT_TRUE(verif->verify(m, get_md(), "abc"_str));
}

TEST_F(VerifierTests, ModuleLevel_Populated_Fail_OneOrMoreTypeInfoFailedVerify) {
    // test w/ populated module_info w/ one of the types therein (here it's C) failing
    // static verification, which brings the whole module down w/ it

    // we don't care if the impl stops after encountering one type_info in error, or if
    // it keeps going, so we generally don't care here *what* the error which arose was

    yama::module_factory mf{};

    auto A_consts =
        yama::const_table_info()
        // illegal out-of-bounds constant index (for param type of b)
        .add_function_type("abc:b"_str, yama::make_callsig({ 2 }, 1)) // <- ERROR!!!
        .add_primitive_type("abc:c"_str);

    mf.add_primitive(
        "A"_str,
        std::move(A_consts),
        yama::ptype::bool0);

    auto B_consts =
        yama::const_table_info()
        .add_primitive_type("abc:b"_str)
        .add_function_type("abc:c"_str, yama::make_callsig({ 0 }, 1)) // ie. 'fn(b) -> c'
        .add_primitive_type("abc:d"_str);

    mf.add_function(
        "B"_str,
        std::move(B_consts),
        yama::make_callsig({ 0, 1, 2 }, 0),
        4,
        yama::noop_call_fn);

    auto C_bcode =
        yama::bc::code()
        // block #1
        .add_put_const(yama::newtop, 1) // inits R(0) (to Float 0.05)
        .add_put_const(yama::newtop, 2) // inits R(1) (to Float 3.14159)
        .add_copy(0, 1) // no reinit, so R(1) MUST be Float already
        .add_ret(200); // <- ERROR!!! R(200) is out-of-bounds!
    auto C_consts =
        yama::const_table_info()
        .add_primitive_type("yama:Float"_str)
        .add_float(0.05)
        .add_float(3.14159);

    mf.add_function(
        "C"_str,
        std::move(C_consts),
        yama::make_callsig({}, 0),
        2,
        std::move(C_bcode));

    auto D_consts =
        yama::const_table_info();

    mf.add_struct(
        "D"_str,
        std::move(D_consts));

    auto m = mf.done();

    EXPECT_FALSE(verif->verify(m, get_md(), "abc"_str)); // <- we don't care what exactly the error is reported to be
}

TEST_F(VerifierTests, ModuleLevel_Populated_Fail_OwnerNotInModule) {
    // this error arises from a member type referring to an owner type which is not found
    // in the module

    // this error only involves the unqualified name of a type (ie. it's not about constsyms)

    yama::module_factory mf{};

    auto A_consts =
        yama::const_table_info()
        .add_primitive_type("abc:b"_str);
    mf.add_method(
        "Missing::A"_str, // <- error! 'Missing' is not a type in module!
        A_consts,
        yama::make_callsig({}, 0),
        4,
        yama::noop_call_fn);

    auto m = mf.done();

    EXPECT_FALSE(verif->verify(m, get_md(), "abc"_str));

    EXPECT_EQ(dbg->count(yama::dsignal::verif_type_owner_not_in_module), 1);
}

