

#include <gtest/gtest.h>

#include <yama/core/type-info-structs.h>
#include <yama/debug-layers/stderr_debug.h>
#include <yama/domain-support/static_verifier.h>


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

    const auto a = yama::type_data(yama::function_info{ "a"_str, yama::make_callsig_info({ 0, 1, 2 }, 0), a_linksyms });

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

    const auto a = yama::type_data(yama::function_info{ "a"_str, yama::make_callsig_info({ 0, 1, 2 }, 0), a_linksyms });

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

TEST_F(StaticVerifierTests, Verify_FailDueToPrimitiveTypesMustHaveNoCallSig_ForTypeItself) {
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

TEST_F(StaticVerifierTests, Verify_FailDueToPrimitiveTypesMustHaveNoCallSig_ForLinkSymbol) {
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

TEST_F(StaticVerifierTests, Verify_FailDueToFunctionTypesMustHaveCallSig_ForTypeItself) {
    // illegal function type w/out callsig

    const auto a = yama::type_data(yama::function_info{ "a"_str, std::nullopt });

    ASSERT_FALSE(a.verified());

    const auto result = verif->verify(a);

    EXPECT_FALSE(result);
    EXPECT_FALSE(a.verified());
}

TEST_F(StaticVerifierTests, Verify_FailDueToFunctionTypesMustHaveCallSig_ForLinkSymbol) {
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

