

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


// helper for subdomain tests

std::shared_ptr<yama::domain> upstream_dm = nullptr;
std::shared_ptr<yama::domain> upstream_subdm = nullptr;


// default_domain subdomain tests

static DomainImplTestsParam _make_param_3() {
    auto factory =
        [](std::shared_ptr<yama::debug> dbg) -> yama::res<yama::domain> {
        upstream_dm = std::make_shared<yama::default_domain>(dbg);
        return yama::res(upstream_dm->fork());
        };
    return DomainImplTestsParam{ factory };
}

INSTANTIATE_TEST_SUITE_P(
    DefaultDomainSubdomain,
    DomainImplTests,
    testing::Values(_make_param_3()));


// default_domain subdomain loading tests

static DomainImplLoadingTestsParam _make_param_4() {
    auto factory =
        [](std::shared_ptr<yama::debug> dbg) -> yama::res<yama::domain> {
        upstream_dm = std::make_shared<yama::default_domain>(dbg);
        return yama::res(upstream_dm->fork());
        };
    return DomainImplLoadingTestsParam{ factory };
}

INSTANTIATE_TEST_SUITE_P(
    DefaultDomainSubdomain,
    DomainImplLoadingTests,
    testing::Values(_make_param_4()));


// the reason we have tests for the subdomains of subdomains is due to how our
// impl has unique program behaviour in that case, so it'll be tested too


// default_domain subdomain subdomain tests

static DomainImplTestsParam _make_param_5() {
    auto factory =
        [](std::shared_ptr<yama::debug> dbg) -> yama::res<yama::domain> {
        upstream_dm = std::make_shared<yama::default_domain>(dbg);
        upstream_subdm = upstream_dm->fork();
        return yama::res(upstream_subdm->fork());
        };
    return DomainImplTestsParam{ factory };
}

INSTANTIATE_TEST_SUITE_P(
    DefaultDomainSubdomainSubdomain,
    DomainImplTests,
    testing::Values(_make_param_5()));


// default_domain subdomain subdomain loading tests

static DomainImplLoadingTestsParam _make_param_6() {
    auto factory =
        [](std::shared_ptr<yama::debug> dbg) -> yama::res<yama::domain> {
        upstream_dm = std::make_shared<yama::default_domain>(dbg);
        upstream_subdm = upstream_dm->fork();
        return yama::res(upstream_subdm->fork());
        };
    return DomainImplLoadingTestsParam{ factory };
}

INSTANTIATE_TEST_SUITE_P(
    DefaultDomainSubdomainSubdomain,
    DomainImplLoadingTests,
    testing::Values(_make_param_6()));


// TODO: we don't have version of below testing subdomains

TEST(DefaultDomainTests, Fork_FailIfLocked) {
    const auto dm = yama::make_res<yama::default_domain>(yama::make_res<yama::stderr_debug>());
    auto subdm = dm->fork();
    ASSERT_TRUE(subdm);
    ASSERT_TRUE(dm->locked());

    ASSERT_FALSE(dm->fork()); // fail if locked

    subdm = nullptr;
}

TEST(DefaultDomainTests, Load_FailIfLocked) {
    const auto dm = yama::make_res<yama::default_domain>(yama::make_res<yama::stderr_debug>());
    auto subdm = dm->fork();
    ASSERT_TRUE(subdm);
    ASSERT_TRUE(dm->locked());

    ASSERT_FALSE(dm->load("Int"_str));

    subdm = nullptr;
}

TEST(DefaultDomainTests, Upload_One_FailIfLocked) {
    const auto dm = yama::make_res<yama::default_domain>(yama::make_res<yama::stderr_debug>());
    auto subdm = dm->fork();
    ASSERT_TRUE(subdm);
    ASSERT_TRUE(dm->locked());

    yama::type_info abc{
        .fullname = "abc"_str,
        .consts = yama::const_table_info{},
        .info = yama::primitive_info{ .ptype = yama::ptype::int0 },
    };

    ASSERT_FALSE(dm->upload(abc));

    subdm = nullptr;
}

TEST(DefaultDomainTests, Upload_Span_FailIfLocked) {
    const auto dm = yama::make_res<yama::default_domain>(yama::make_res<yama::stderr_debug>());
    auto subdm = dm->fork();
    ASSERT_TRUE(subdm);
    ASSERT_TRUE(dm->locked());

    yama::type_info abc{
        .fullname = "abc"_str,
        .consts = yama::const_table_info{},
        .info = yama::primitive_info{.ptype = yama::ptype::int0 },
    };
    yama::type_info def{
        .fullname = "def"_str,
        .consts = yama::const_table_info{},
        .info = yama::primitive_info{.ptype = yama::ptype::int0 },
    };
    yama::type_info ghi{
        .fullname = "ghi"_str,
        .consts = yama::const_table_info{},
        .info = yama::primitive_info{.ptype = yama::ptype::int0 },
    };
    const std::vector<yama::type_info> vec{
        abc,
        def,
        ghi,
    };

    ASSERT_FALSE(dm->upload(std::span(vec.begin(), vec.end())));

    subdm = nullptr;
}

