

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

TEST_F(StaticVerifierTests, Verify_FailDueToCallSigConstIndexOutOfBounds_ParamType_ForTypeItself) {
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

TEST_F(StaticVerifierTests, Verify_FailDueToCallSigConstIndexOutOfBounds_ParamType_ForLinkSymbol) {
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

TEST_F(StaticVerifierTests, Verify_FailDueToCallSigConstIndexOutOfBounds_ReturnType_ForTypeItself) {
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

TEST_F(StaticVerifierTests, Verify_FailDueToCallSigConstIndexOutOfBounds_ReturnType_ForLinkSymbol) {
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

TEST_F(StaticVerifierTests, Verify_FailDueToCallSigConstNotATypeConst_ParamType_ForTypeItself) {
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

TEST_F(StaticVerifierTests, Verify_FailDueToCallSigConstNotATypeConst_ParamType_ForLinkSymbol) {
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

TEST_F(StaticVerifierTests, Verify_FailDueToCallSigConstNotATypeConst_ReturnType_ForTypeItself) {
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

TEST_F(StaticVerifierTests, Verify_FailDueToCallSigConstNotATypeConst_ReturnType_ForLinkSymbol) {
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

