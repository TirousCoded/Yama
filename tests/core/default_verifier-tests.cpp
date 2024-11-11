

#include <gtest/gtest.h>

#include <yama/core/verifier.h>

#include "../parameterized/verifier-impl-tests.h"


using namespace yama::string_literals;


static VerifierImplTestsParam _make_param_1() {
    auto factory =
        [](std::shared_ptr<yama::debug> dbg) -> yama::res<yama::verifier> {
        return yama::make_res<yama::default_verifier>(dbg);
        };
    return VerifierImplTestsParam{ factory };
}

INSTANTIATE_TEST_SUITE_P(
    DefaultVerifier,
    VerifierImplTests,
    testing::Values(_make_param_1()));

