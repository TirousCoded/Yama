

#include <gtest/gtest.h>

#include <yama/core/general.h>
#include <yama/core/res.h>
#include <yama/core/module.h>
#include <yama/core/verifier.h>

#include "../../../utils/module_helper.h"


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


class VerifTests : public testing::Test {
public:
    std::shared_ptr<yama::dsignal_debug> dbg;
    module_helper mh;
    void should_pass();
    void should_fail();
protected:
    void SetUp() override final {
        dbg = std::make_shared<yama::dsignal_debug>(std::make_shared<yama::stderr_debug>());
    }

    void TearDown() override final {
        //
    }
private:
    bool _test();
};

void VerifTests::should_pass() {
    EXPECT_TRUE(_test());
}
void VerifTests::should_fail() {
    EXPECT_FALSE(_test());
}
bool VerifTests::_test() {
    return yama::verifier(dbg).verify(
        mh.result(),
        // Map names 'yama' and 'abc' to arbitrary parcel IDs.
        yama::parcel_metadata{ "self"_str, { "yama"_str, "abc"_str } },
        "abc"_str);
}


// MODULE-WIDE TESTS

TEST_F(VerifTests, Empty) {
    should_pass();
}

TEST_F(VerifTests, Populated) {
    static_assert(yama::kinds == 4);

    // primitive
    auto A_consts =
        yama::const_table();
    mh.add_primitive(
        "A"_str,
        A_consts,
        yama::ptype::bool0);

    // function (native)
    auto B_consts =
        yama::const_table()
        .add_primitive_type("abc:b"_str)
        .add_function_type("abc:c"_str, yama::make_callsig({ 0 }, 1)) // ie. 'fn(b) -> c'
        .add_primitive_type("abc:d"_str);
    mh.add_function(
        "B"_str,
        B_consts,
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
        yama::const_table()
        .add_primitive_type("yama:Float"_str)
        .add_float(0.05)
        .add_float(3.14159);
    mh.add_function(
        "C"_str,
        C_consts,
        yama::make_callsig({}, 0),
        2,
        C_bcode);

    // method (native)
    auto D_consts =
        yama::const_table()
        .add_primitive_type("abc:b"_str)
        .add_function_type("abc:c"_str, yama::make_callsig({ 0 }, 1)) // ie. 'fn(b) -> c'
        .add_primitive_type("abc:d"_str);
    mh.add_method(
        "A"_str,
        "D"_str,
        D_consts,
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
        yama::const_table()
        .add_primitive_type("yama:Float"_str)
        .add_float(0.05)
        .add_float(3.14159);
    mh.add_method(
        "A"_str,
        "E"_str,
        E_consts,
        yama::make_callsig({}, 0),
        2,
        E_bcode);

    // struct
    auto F_consts =
        yama::const_table();
    mh.add_struct(
        "F"_str,
        F_consts);

    should_pass();
}

TEST_F(VerifTests, Fail_OneOrMoreItemsFailedVerify) {
    // Test w/ populated module w/ one of the types therein (here it's C) failing
    // static verification, which brings the whole module down w/ it.
    
    // We don't care if the impl stops after encountering one item in error, or if
    // it keeps going, so we generally don't care here *what* the error was.

    auto A_consts =
        yama::const_table()
        // illegal out-of-bounds constant index (for param type of b)
        .add_function_type("abc:b"_str, yama::make_callsig({ 2 }, 1)) // error!
        .add_primitive_type("abc:c"_str);
    mh.add_primitive(
        "A"_str,
        A_consts,
        yama::ptype::bool0);

    auto B_consts =
        yama::const_table()
        .add_primitive_type("abc:b"_str)
        .add_function_type("abc:c"_str, yama::make_callsig({ 0 }, 1)) // ie. 'fn(b) -> c'
        .add_primitive_type("abc:d"_str);
    mh.add_function(
        "B"_str,
        B_consts,
        yama::make_callsig({ 0, 1, 2 }, 0),
        4,
        yama::noop_call_fn);

    auto C_bcode =
        yama::bc::code()
        // block #1
        .add_put_const(yama::newtop, 1) // inits R(0) (to Float 0.05)
        .add_put_const(yama::newtop, 2) // inits R(1) (to Float 3.14159)
        .add_copy(0, 1) // no reinit, so R(1) MUST be Float already
        .add_ret(200); // error! R(200) is out-of-bounds!
    auto C_consts =
        yama::const_table()
        .add_primitive_type("yama:Float"_str)
        .add_float(0.05)
        .add_float(3.14159);
    mh.add_function(
        "C"_str,
        C_consts,
        yama::make_callsig({}, 0),
        2,
        C_bcode);

    auto D_consts =
        yama::const_table();
    mh.add_struct(
        "D"_str,
        D_consts);

    should_fail(); // Don't care what exactly the error was.
}


// PER-ITEM TESTS

TEST_F(VerifTests, DisregardBCodeIfCallFnIsNotBCodeCallFn) {
    const auto f_bcode =
        yama::bc::code(); // Otherwise illegal empty bcode.
    yama::println("{}", f_bcode.fmt_disassembly());
    const auto f_consts =
        yama::const_table()
        .add_primitive_type("yama:None"_str);
    mh.add_function(
        "f"_str,
        f_consts,
        yama::make_callsig({}, 0),
        1,
        // Not bcode_call_fn.
        yama::noop_call_fn);
    mh.bind_bcode("f"_str, f_bcode); // Should be disregarded.

    should_pass();
}

TEST_F(VerifTests, Fail_CallFnIsBCodeCallFnButNoBCodeFound) {
    const auto f_consts =
        yama::const_table()
        .add_primitive_type("yama:None"_str);
    mh.add_function(
        "f"_str,
        f_consts,
        yama::make_callsig({}, 0),
        1,
        // bcode_call_fn, but no bcode descriptor found.
        yama::bcode_call_fn);

    should_fail();

    EXPECT_EQ(dbg->count(yama::dsignal::verif_binary_not_found), 1);
}

TEST_F(VerifTests, Fail_MemberItemOwnerNotInModule) {
    // This error arises from a member item referring to an owner item which is not found.
    // This error only involves the unqualified name of a item (ie. it's not about constsyms.)

    auto A_consts =
        yama::const_table()
        .add_primitive_type("abc:b"_str);
    mh.add_method(
        "Missing"_str, // error! 'Missing' is not an item in module!
        "A"_str,
        A_consts,
        yama::make_callsig({}, 0),
        4,
        yama::noop_call_fn);

    should_fail();

    EXPECT_EQ(dbg->count(yama::dsignal::verif_owner_not_in_module), 1);
}

TEST_F(VerifTests, Fail_InvalidUnqualifiedName_Malformed) {
    // TODO: stub

    //should_fail();

    //EXPECT_EQ(dbg->count(yama::dsignal::verif_invalid_unqualified_name), 1);
}

TEST_F(VerifTests, Fail_CallSig_ConstIndexOutOfBounds_ParamType) {
    const auto a_consts =
        yama::const_table()
        .add_primitive_type("abc:b"_str);
    mh.add_function(
        "a"_str,
        a_consts,
        // illegal out-of-bounds constant index (for param type of a)
        yama::make_callsig({ 1 }, 0),
        4,
        yama::noop_call_fn);

    should_fail();

    EXPECT_EQ(dbg->count(yama::dsignal::verif_invalid_callsig), 1);
    EXPECT_EQ(dbg->count(yama::dsignal::verif_callsig_param_type_out_of_bounds), 1);
}

TEST_F(VerifTests, Fail_CallSig_ConstIndexOutOfBounds_ReturnType) {
    const auto a_consts =
        yama::const_table()
        .add_primitive_type("abc:b"_str);
    mh.add_function(
        "a"_str,
        a_consts,
        // illegal out-of-bounds constant index (for return type of a)
        yama::make_callsig({}, 1),
        4,
        yama::noop_call_fn);

    should_fail();

    EXPECT_EQ(dbg->count(yama::dsignal::verif_invalid_callsig), 1);
    EXPECT_EQ(dbg->count(yama::dsignal::verif_callsig_return_type_out_of_bounds), 1);
}

TEST_F(VerifTests, Fail_CallSig_ConstNotATypeConst_ParamType) {
    const auto a_consts =
        yama::const_table()
        .add_primitive_type("abc:b"_str)
        .add_int(10);
    mh.add_function(
        "a"_str,
        a_consts,
        // illegal use of non-type constant index (for param type of a)
        yama::make_callsig({ 1 }, 0),
        4,
        yama::noop_call_fn);

    should_fail();

    EXPECT_EQ(dbg->count(yama::dsignal::verif_invalid_callsig), 1);
    EXPECT_EQ(dbg->count(yama::dsignal::verif_callsig_param_type_not_type_const), 1);
}

TEST_F(VerifTests, Fail_CallSig_ConstNotATypeConst_ReturnType) {
    const auto a_consts =
        yama::const_table()
        .add_primitive_type("abc:b"_str)
        .add_int(10);
    mh.add_function(
        "a"_str,
        a_consts,
        // illegal use of non-type constant index (for return type of a)
        yama::make_callsig({}, 1),
        4,
        yama::noop_call_fn);

    should_fail();

    EXPECT_EQ(dbg->count(yama::dsignal::verif_invalid_callsig), 1);
    EXPECT_EQ(dbg->count(yama::dsignal::verif_callsig_return_type_not_type_const), 1);
}

