

#include <gtest/gtest.h>

#include <yama/core/domain.h>

#include "../parameterized/domain-impl-tests.h"
#include "../parameterized/domain-impl-loading-tests.h"


using namespace yama::string_literals;


// default_domain tests

static DomainImplTestsParam _make_param_1() {
    auto factory =
        [](std::shared_ptr<yama::debug> dbg) -> yama::res<yama::domain> {
        return yama::make_res<yama::default_domain>(dbg);
        };
    return DomainImplTestsParam{ factory };
}

INSTANTIATE_TEST_SUITE_P(
    DefaultDomain,
    DomainImplTests,
    testing::Values(_make_param_1()));


// default_domain loading tests

static DomainImplLoadingTestsParam _make_param_2() {
    auto factory =
        [](std::shared_ptr<yama::debug> dbg) -> yama::res<yama::domain> {
        return yama::make_res<yama::default_domain>(dbg);
        };
    return DomainImplLoadingTestsParam{ factory };
}

INSTANTIATE_TEST_SUITE_P(
    DefaultDomain,
    DomainImplLoadingTests,
    testing::Values(_make_param_2()));

