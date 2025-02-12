

#pragma once


#include <gtest/gtest.h>

#include <yama/core/res.h>
#include <yama/core/domain.h>


struct DomainImplTestsParam final {
    std::function<yama::res<yama::domain>(yama::domain_config config, std::shared_ptr<yama::debug> dbg)> factory;
};

class DomainImplTests : public testing::TestWithParam<DomainImplTestsParam> {
public:

    std::shared_ptr<yama::dsignal_debug> dbg;


    inline void SetUp() override final {
        dbg = std::make_shared<yama::dsignal_debug>(std::make_shared<yama::stderr_debug>());
    }

    inline void TearDown() override final {
        //
    }
};

