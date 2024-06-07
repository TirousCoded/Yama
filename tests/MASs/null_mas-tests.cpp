

#include <gtest/gtest.h>

#include <yama/MASs/null_mas.h>


TEST(NullMASTests, AllocatorAllocateAndDeallocate) {
    yama::null_mas mas0{};
    auto a = mas0.get<int>();

    auto block0 = a.allocate(1);
    auto block1 = a.allocate(10);
    auto block2 = a.allocate(256);

    EXPECT_FALSE(block0);
    EXPECT_FALSE(block1);
    EXPECT_FALSE(block2);
}

TEST(NullMASTests, AllocatorMaxSize) {
    yama::null_mas mas0{};
    auto a = mas0.get<int>();
    
    EXPECT_EQ(a.max_size(), 0);
}

TEST(NullMASTests, AllocationAlignment) {
    // nothing to do as null_mas can't allocate blocks
}

