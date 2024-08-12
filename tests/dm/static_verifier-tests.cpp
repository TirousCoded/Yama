

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
    yama::type_info a{
        .fullname = "a"_str,
        .linksyms = a_linksyms,
        .info = yama::function_info{
            .callsig = yama::make_callsig_info({ 0, 1, 2 }, 0),
            .call_fn = yama::noop_call_fn,
            .locals = 4,
        },
    };

    EXPECT_TRUE(verif->verify(a));
}

TEST_F(StaticVerifierTests, Verify_FailDueToCallSigLinkIndexOutOfBounds_ParamType_ForTypeItself) {
    const std::vector<yama::linksym> a_linksyms{
        yama::make_linksym("b"_str, yama::kind::primitive),
    };
    yama::type_info a{
        .fullname = "a"_str,
        .linksyms = a_linksyms,
        .info = yama::function_info{
            // illegal out-of-bounds link index (for param type of a)
            .callsig = yama::make_callsig_info({ 1 }, 0),
            .call_fn = yama::noop_call_fn,
            .locals = 4,
        },
    };

    EXPECT_FALSE(verif->verify(a));
}

TEST_F(StaticVerifierTests, Verify_FailDueToCallSigLinkIndexOutOfBounds_ParamType_ForLinkSymbol) {
    const std::vector<yama::linksym> a_linksyms{
        // illegal out-of-bounds link index (for param type of b)
        yama::make_linksym("b"_str, yama::kind::function, yama::make_callsig_info({ 2 }, 1)),
        yama::make_linksym("c"_str, yama::kind::primitive),
    };
    yama::type_info a{
        .fullname = "a"_str,
        .linksyms = a_linksyms,
        .info = yama::primitive_info{
            .ptype = yama::ptype::bool0,
        },
    };

    EXPECT_FALSE(verif->verify(a));
}

TEST_F(StaticVerifierTests, Verify_FailDueToCallSigLinkIndexOutOfBounds_ReturnType_ForTypeItself) {
    const std::vector<yama::linksym> a_linksyms{
        yama::make_linksym("b"_str, yama::kind::primitive),
    };
    yama::type_info a{
        .fullname = "a"_str,
        .linksyms = a_linksyms,
        .info = yama::function_info{
            // illegal out-of-bounds link index (for return type of a)
            .callsig = yama::make_callsig_info({}, 1),
            .call_fn = yama::noop_call_fn,
            .locals = 4,
        },
    };

    EXPECT_FALSE(verif->verify(a));
}

TEST_F(StaticVerifierTests, Verify_FailDueToCallSigLinkIndexOutOfBounds_ReturnType_ForLinkSymbol) {
    const std::vector<yama::linksym> a_linksyms{
        // illegal out-of-bounds link index (for return type of b)
        yama::make_linksym("b"_str, yama::kind::function, yama::make_callsig_info({}, 2)),
        yama::make_linksym("c"_str, yama::kind::primitive),
    };
    yama::type_info a{
        .fullname = "a"_str,
        .linksyms = a_linksyms,
        .info = yama::primitive_info{
            .ptype = yama::ptype::bool0,
        },
    };

    EXPECT_FALSE(verif->verify(a));
}

static_assert(yama::kinds == 2);

// primitives

TEST_F(StaticVerifierTests, Verify_Primitive_FailDueToMustHaveNoCallSig_ForLinkSymbol) {
    // NOTE: the type verif->verify is called w/ DOES NOT MATTER, as this test is about
    //       the type OF THE LINK SYMBOL

    const std::vector<yama::linksym> a_linksyms{
        // illegal primitive type w/ callsig
        yama::make_linksym("b"_str, yama::kind::primitive, yama::make_callsig_info({ 1 }, 1)), // <- callsig 'fn(c) -> c'
        yama::make_linksym("c"_str, yama::kind::primitive),
    };
    yama::type_info a{
        .fullname = "a"_str,
        .linksyms = a_linksyms,
        .info = yama::primitive_info{
            .ptype = yama::ptype::bool0,
        },
    };

    EXPECT_FALSE(verif->verify(a));
}

// functions

TEST_F(StaticVerifierTests, Verify_Function_FailDueToMustHaveCallSig_ForLinkSymbol) {
    // NOTE: the type verif->verify is called w/ DOES NOT MATTER, as this test is about
    //       the type OF THE LINK SYMBOL

    const std::vector<yama::linksym> a_linksyms{
        // illegal function type w/out callsig
        yama::make_linksym("b"_str, yama::kind::function),
    };
    yama::type_info a{
        .fullname = "a"_str,
        .linksyms = a_linksyms,
        .info = yama::primitive_info{
            .ptype = yama::ptype::bool0,
        },
    };

    EXPECT_FALSE(verif->verify(a));
}

