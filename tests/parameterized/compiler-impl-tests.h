

#pragma once


#include <gtest/gtest.h>

#include <yama/core/res.h>
#include <yama/core/compiler.h>
#include <yama/core/domain.h>
#include <yama/core/context.h>


struct CompilerImplTestsParam final {
    std::function<yama::res<yama::compiler>(std::shared_ptr<yama::debug> dbg)> factory;
};

class CompilerImplTests : public testing::TestWithParam<CompilerImplTestsParam> {
public:

    std::shared_ptr<yama::dsignal_debug> dbg;
    std::shared_ptr<yama::domain> dm;
    std::shared_ptr<yama::context> ctx;

    std::shared_ptr<yama::compiler> comp;

    bool ready = false; // if fixture setup correctly


    void SetUp() override final;

    inline void TearDown() override final {
        //
    }
};

