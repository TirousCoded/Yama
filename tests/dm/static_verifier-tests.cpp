

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

    const std::vector<yama::linksym> a_linksyms{
        yama::make_linksym("b"_str, yama::kind::primitive),
        yama::make_linksym("c"_str, yama::kind::function, yama::make_callsig_info({ 0 }, 1)), // <- callsig 'fn(b) -> c'
        yama::make_linksym("d"_str, yama::kind::primitive),
    };
    yama::function_info a_info{
        "a"_str,
        yama::make_callsig_info({ 0, 1, 2 }, 0),
        a_linksyms,
        yama::noop_call_fn,
        4,
    };
    const auto a = yama::type_data(a_info);

    ASSERT_FALSE(a.verified());

    const auto result = verif->verify(a);

    EXPECT_TRUE(result);
    EXPECT_TRUE(a.verified());
}

TEST_F(StaticVerifierTests, Verify_AlreadyVerifiedTypeData) {
    const std::vector<yama::linksym> a_linksyms{
        yama::make_linksym("b"_str, yama::kind::primitive),
        yama::make_linksym("c"_str, yama::kind::function, yama::make_callsig_info({ 0 }, 1)), // <- callsig 'fn(b) -> c'
        yama::make_linksym("d"_str, yama::kind::primitive),
    };
    yama::function_info a_info{
        "a"_str,
        yama::make_callsig_info({ 0, 1, 2 }, 0),
        a_linksyms,
        yama::noop_call_fn,
        4,
    };
    const auto a = yama::type_data(a_info);

    ASSERT_TRUE(verif->verify(a));
    ASSERT_TRUE(a.verified());

    const auto result = verif->verify(a);

    EXPECT_TRUE(result);
    EXPECT_TRUE(a.verified());
}

TEST_F(StaticVerifierTests, Verify_FailDueToCallSigLinkIndexOutOfBounds_ParamType_ForTypeItself) {
    const std::vector<yama::linksym> a_linksyms{
        yama::make_linksym("b"_str, yama::kind::primitive),
    };

    // illegal out-of-bounds link index (for param type of a)

    const auto a = yama::type_data(yama::function_info{ "a"_str, yama::make_callsig_info({ 1 }, 0), a_linksyms });

    ASSERT_FALSE(a.verified());

    const auto result = verif->verify(a);

    EXPECT_FALSE(result);
    EXPECT_FALSE(a.verified());
}

TEST_F(StaticVerifierTests, Verify_FailDueToCallSigLinkIndexOutOfBounds_ParamType_ForLinkSymbol) {
    const std::vector<yama::linksym> a_linksyms{
        // illegal out-of-bounds link index (for param type of b)
        yama::make_linksym("b"_str, yama::kind::function, yama::make_callsig_info({ 2 }, 1)),
        yama::make_linksym("c"_str, yama::kind::primitive),
    };

    const auto a = yama::type_data(yama::primitive_info{ "a"_str, std::nullopt, a_linksyms });

    ASSERT_FALSE(a.verified());

    const auto result = verif->verify(a);

    EXPECT_FALSE(result);
    EXPECT_FALSE(a.verified());
}

TEST_F(StaticVerifierTests, Verify_FailDueToCallSigLinkIndexOutOfBounds_ReturnType_ForTypeItself) {
    const std::vector<yama::linksym> a_linksyms{
        yama::make_linksym("b"_str, yama::kind::primitive),
    };

    // illegal out-of-bounds link index (for return type of a)

    const auto a = yama::type_data(yama::function_info{ "a"_str, yama::make_callsig_info({}, 1), a_linksyms });

    ASSERT_FALSE(a.verified());

    const auto result = verif->verify(a);

    EXPECT_FALSE(result);
    EXPECT_FALSE(a.verified());
}

TEST_F(StaticVerifierTests, Verify_FailDueToCallSigLinkIndexOutOfBounds_ReturnType_ForLinkSymbol) {
    const std::vector<yama::linksym> a_linksyms{
        // illegal out-of-bounds link index (for return type of b)
        yama::make_linksym("b"_str, yama::kind::function, yama::make_callsig_info({}, 2)),
        yama::make_linksym("c"_str, yama::kind::primitive),
    };

    const auto a = yama::type_data(yama::primitive_info{ "a"_str, std::nullopt, a_linksyms });

    ASSERT_FALSE(a.verified());

    const auto result = verif->verify(a);

    EXPECT_FALSE(result);
    EXPECT_FALSE(a.verified());
}

// NOTE: need a ***_ForTypeItself and ***_ForLinkSymbol for each kind

static_assert(yama::kinds == 2);

// primitives

TEST_F(StaticVerifierTests, Verify_Primitive_FailDueToMustHaveNoCallSig_ForTypeItself) {
    const std::vector<yama::linksym> a_linksyms{
        yama::make_linksym("b"_str, yama::kind::primitive),
    };

    // illegal primitive type w/ callsig

    const auto a = yama::type_data(yama::primitive_info{ "a"_str, yama::make_callsig_info({ 0 }, 0), a_linksyms });

    ASSERT_FALSE(a.verified());

    const auto result = verif->verify(a);

    EXPECT_FALSE(result);
    EXPECT_FALSE(a.verified());
}

TEST_F(StaticVerifierTests, Verify_Primitive_FailDueToMustHaveNoCallSig_ForLinkSymbol) {
    const std::vector<yama::linksym> a_linksyms{
        // illegal primitive type w/ callsig
        yama::make_linksym("b"_str, yama::kind::primitive, yama::make_callsig_info({ 1 }, 1)), // <- callsig 'fn(c) -> c'
        yama::make_linksym("c"_str, yama::kind::primitive),
    };

    const auto a = yama::type_data(yama::primitive_info{ "a"_str, std::nullopt, a_linksyms });

    ASSERT_FALSE(a.verified());

    const auto result = verif->verify(a);

    EXPECT_FALSE(result);
    EXPECT_FALSE(a.verified());
}

// functions

TEST_F(StaticVerifierTests, Verify_Function_FailDueToMustHaveCallSig_ForTypeItself) {
    // illegal function type w/out callsig

    const auto a = yama::type_data(yama::function_info{ "a"_str, std::nullopt });

    ASSERT_FALSE(a.verified());

    const auto result = verif->verify(a);

    EXPECT_FALSE(result);
    EXPECT_FALSE(a.verified());
}

TEST_F(StaticVerifierTests, Verify_Function_FailDueToMustHaveCallSig_ForLinkSymbol) {
    const std::vector<yama::linksym> a_linksyms{
        // illegal function type w/out callsig
        yama::make_linksym("b"_str, yama::kind::function),
    };

    const auto a = yama::type_data(yama::primitive_info{ "a"_str, std::nullopt, a_linksyms });

    ASSERT_FALSE(a.verified());

    const auto result = verif->verify(a);

    EXPECT_FALSE(result);
    EXPECT_FALSE(a.verified());
}

TEST_F(StaticVerifierTests, Verify_Function_FailDueToMaxLocalsNotBeingLargeEnoughForCallObjAndArgs) {
    // illegal function type due to max_locals not being large enough for call obj + args

    const std::vector<yama::linksym> a_linksyms{
        yama::make_linksym("None"_str, yama::kind::primitive),
    };
    const auto a_callsig_info = yama::make_callsig_info({ 0, 0, 0 }, 0); // expects 3 args
    yama::function_info a_info{
        "a"_str,
        std::make_optional(a_callsig_info),
        a_linksyms,
        yama::noop_call_fn,
        3, // <- static verif. error: 3 (max_locals) < 1 (call obj) + 3 (args)
    };
    const auto a = yama::type_data(a_info);

    ASSERT_FALSE(a.verified());

    const auto result = verif->verify(a);

    EXPECT_FALSE(result);
    EXPECT_FALSE(a.verified());
}

