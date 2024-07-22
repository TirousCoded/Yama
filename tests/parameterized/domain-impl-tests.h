

#pragma once


#include <gtest/gtest.h>

#include <yama/core/res.h>
#include <yama/core/domain.h>
#include <yama/debug-impls/stderr_debug.h>


struct DomainImplTestsParam final {
    std::function<yama::res<yama::domain>(std::shared_ptr<yama::debug> dbg)> factory;
};

class DomainImplTests : public testing::TestWithParam<DomainImplTestsParam> {
public:

    std::shared_ptr<yama::debug> dbg;


    inline void SetUp() override final {
        dbg = std::make_shared<yama::stderr_debug>();
    }

    inline void TearDown() override final {
        //
    }
};

