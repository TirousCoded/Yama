

#pragma once


#include <gtest/gtest.h>

#include <yama/core/res.h>
#include <yama/core/debug.h>
#include <yama/core/domain.h>


struct DomainImplLoadingTestsParam final {
    std::function<yama::res<yama::domain>(std::shared_ptr<yama::debug> dbg)> factory;
};

class DomainImplLoadingTests : public testing::TestWithParam<DomainImplLoadingTestsParam> {
public:

    std::shared_ptr<yama::dsignal_debug> dbg;


    inline void SetUp() override final {
        dbg = std::make_shared<yama::dsignal_debug>(std::make_shared<yama::stderr_debug>());
    }

    inline void TearDown() override final {
        //
    }
};

