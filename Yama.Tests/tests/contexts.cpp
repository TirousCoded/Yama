

#include <gtest/gtest.h>
#include <yama/yama.h>


TEST(Contexts, CreateAndDestroy) {
    YmDm* dm = ymNewDm();
    ASSERT_TRUE(dm);
    YmCtx* ctx = ymNewCtx(dm);
    ASSERT_TRUE(ctx);

    ymDrop(ctx);
    ymDrop(dm);
}

