

#include <gtest/gtest.h>

#include <yama/mas-impls/heap_mas.h>
#include <yama/domain-impls/domain_st.h>

#include "../parameterized/domain-impl-tests.h"


static DomainImplTestsParam _make_param_1() {
    auto factory =
        [](std::shared_ptr<yama::debug> dbg) -> yama::res<yama::domain> {
        return yama::make_res<yama::domain_st>(yama::make_res<yama::heap_mas>(), dbg);
        };
    return DomainImplTestsParam{ factory };
}

INSTANTIATE_TEST_SUITE_P(
    DomainST,
    DomainImplTests,
    testing::Values(_make_param_1()));


TEST(DomainSTTests, GetMAS) {
    const auto dbg = yama::make_res<yama::stderr_debug>();
    const auto mas = yama::make_res<yama::heap_mas>(dbg);
    const auto dm = yama::make_res<yama::domain_st>(mas, dbg);

    EXPECT_EQ(dm->get_mas(), mas);
}

