

#include <gtest/gtest.h>
#include <yama/yama.h>


TEST(Domains, CreateAndDestroy) {
    YmDm* dm = ymNewDm();
    ASSERT_TRUE(dm);

    ymDrop(dm);
}

