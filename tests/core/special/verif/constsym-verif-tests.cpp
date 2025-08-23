

#include <gtest/gtest.h>

#include <optional>

#include <yama/core/general.h>
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


class ConstSymVerifTests : public testing::Test {
public:
    std::shared_ptr<yama::dsignal_debug> dbg;
    yama::const_table consts;
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

void ConstSymVerifTests::should_pass() {
    EXPECT_TRUE(_test());
}
void ConstSymVerifTests::should_fail() {
    EXPECT_FALSE(_test());
}
bool ConstSymVerifTests::_test() {
#if 1
    yama::module m{};
    EXPECT_TRUE(m.add_primitive("a"_str, consts, yama::ptype::bool0));
    return yama::verifier(dbg).verify(
        m,
        // Map names 'yama' and 'abc' to arbitrary parcel IDs.
        yama::parcel_metadata{ "self"_str, { "yama"_str, "abc"_str } },
        "abc"_str);
#else
    return yama::verifier(dbg).verify(
        yama::make_primitive("a"_str, consts, yama::ptype::bool0),
        // Map names 'yama' and 'abc' to arbitrary parcel IDs.
        yama::parcel_metadata{ "self"_str, { "yama"_str, "abc"_str } },
        "abc"_str);
#endif
}


TEST_F(ConstSymVerifTests, Success) {
    static_assert(yama::const_types == 9);
    consts
        .add_primitive_type("abc:a"_str)
        .add_function_type("abc:b"_str, yama::make_callsig({ 0 }, 0))
        .add_method_type("abc:a::c"_str, yama::make_callsig({ 0, 0 }, 1))
        .add_struct_type("abc:d"_str);

    should_pass();
}

TEST_F(ConstSymVerifTests, Fail_InvalidQualifiedName_Malformed) {
    consts
        // invalid! malformed qualified name
        .add_primitive_type("b"_str);

    should_fail();

    EXPECT_EQ(dbg->count(yama::dsignal::verif_constsym_invalid_qualified_name), 1);
}

TEST_F(ConstSymVerifTests, Fail_InvalidQualifiedName_HeadNamesParcelNotSelfOrDepName) {
    consts
        // invalid! no parcel named 'missing' in self/dep names
        .add_primitive_type("missing:b"_str);

    should_fail();

    EXPECT_EQ(dbg->count(yama::dsignal::verif_constsym_invalid_qualified_name), 1);
}

TEST_F(ConstSymVerifTests, Fail_InvalidQualifiedName_NonMemberItemButHasOwnerPrefix) {
    consts
        // invalid! primitive types cannot have 'owner::' prefix!
        .add_primitive_type("abc:owner::b"_str);

    should_fail();

    EXPECT_EQ(dbg->count(yama::dsignal::verif_constsym_invalid_qualified_name), 1);
}

TEST_F(ConstSymVerifTests, Fail_InvalidQualifiedName_MemberItemButHasNoOwnerPrefix) {
    consts
        .add_primitive_type("abc:a"_str)
        // invalid! method types require '<owner>::' prefix!
        .add_method_type("abc:b"_str, yama::make_callsig({}, 0));

    should_fail();

    EXPECT_EQ(dbg->count(yama::dsignal::verif_constsym_invalid_qualified_name), 1);
}

TEST_F(ConstSymVerifTests, Fail_CallSigConstIndexOutOfBounds_ParamType) {
    consts
        // illegal out-of-bounds constant index (for param type of b)
        .add_function_type("abc:b"_str, yama::make_callsig({ 2 }, 1))
        .add_primitive_type("abc:c"_str);

    should_fail();

    EXPECT_EQ(dbg->count(yama::dsignal::verif_constsym_invalid_callsig), 1);
    EXPECT_EQ(dbg->count(yama::dsignal::verif_callsig_param_type_out_of_bounds), 1);
}

TEST_F(ConstSymVerifTests, Fail_CallSigConstIndexOutOfBounds_ReturnType) {
    consts
        // illegal out-of-bounds constant index (for return type of b)
        .add_function_type("abc:b"_str, yama::make_callsig({}, 2))
        .add_primitive_type("abc:c"_str);

    should_fail();

    EXPECT_EQ(dbg->count(yama::dsignal::verif_constsym_invalid_callsig), 1);
    EXPECT_EQ(dbg->count(yama::dsignal::verif_callsig_return_type_out_of_bounds), 1);
}

TEST_F(ConstSymVerifTests, Fail_CallSigConstNotATypeConst_ParamType) {
    consts
        // illegal use of non-type constant index (for param type of b)
        .add_function_type("abc:b"_str, yama::make_callsig({ 2 }, 1))
        .add_primitive_type("abc:c"_str)
        .add_int(10);

    should_fail();

    EXPECT_EQ(dbg->count(yama::dsignal::verif_constsym_invalid_callsig), 1);
    EXPECT_EQ(dbg->count(yama::dsignal::verif_callsig_param_type_not_type_const), 1);
}

TEST_F(ConstSymVerifTests, Fail_CallSigConstNotATypeConst_ReturnType) {
    consts
        // illegal use of non-type constant index (for return type of b)
        .add_function_type("abc:b"_str, yama::make_callsig({}, 2))
        .add_primitive_type("abc:c"_str)
        .add_int(10);

    should_fail();

    EXPECT_EQ(dbg->count(yama::dsignal::verif_constsym_invalid_callsig), 1);
    EXPECT_EQ(dbg->count(yama::dsignal::verif_callsig_return_type_not_type_const), 1);
}

