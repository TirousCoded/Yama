

#include <gtest/gtest.h>

#include <yama/core/general.h>
#include <yama/core/context.h>
#include <yama/debug-impls/stderr_debug.h>
#include <yama/mas-impls/heap_mas.h>
#include <yama/domain-impls/domain_st.h>


using namespace yama::string_literals;


class ContextTests : public testing::Test {
public:

    std::shared_ptr<yama::debug> dbg;
    std::shared_ptr<yama::mas> mas;
    std::shared_ptr<yama::domain> dm;
    std::shared_ptr<yama::context> ctx;


protected:

    void SetUp() override final {
        dbg = std::make_shared<yama::stderr_debug>();
        mas = std::make_shared<yama::heap_mas>(dbg);
        dm = std::make_shared<yama::domain_st>(yama::res(mas), dbg);
        ctx = std::make_shared<yama::context>(yama::res(dm), dbg);
    }

    void TearDown() override final {
        //
    }
};


TEST_F(ContextTests, GetDomain_Basics) {
    EXPECT_EQ(ctx->get_domain(), dm);
}

TEST_F(ContextTests, DM_Basics) {
    EXPECT_EQ(&(ctx->dm()), dm.get());
}

TEST_F(ContextTests, GetMAS_Basics) {
    EXPECT_EQ(ctx->get_mas(), mas);
}

