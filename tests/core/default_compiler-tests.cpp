

#include <gtest/gtest.h>

#include <yama/core/compiler.h>

#include "../parameterized/compiler-impl-tests.h"


using namespace yama::string_literals;


static CompilerImplTestsParam _make_param_1() {
    auto factory =
        [](std::shared_ptr<yama::debug> dbg) -> yama::res<yama::compiler> {
        return yama::make_res<yama::default_compiler>(dbg);
        };
    return CompilerImplTestsParam{ factory };
}

INSTANTIATE_TEST_SUITE_P(
    DefaultCompiler,
    CompilerImplTests,
    testing::Values(_make_param_1()));

