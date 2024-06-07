

#include <gtest/gtest.h>

#include <yama/MASs/heap_mas.h>


TEST(HeapMASTests, AllocatorAllocateAndDeallocate) {
    yama::heap_mas mas0{};
    auto a = mas0.get<int>();

    auto block0 = a.allocate(1);
    auto block1 = a.allocate(10);
    auto block2 = a.allocate(256);

    EXPECT_TRUE(block0);
    EXPECT_TRUE(block1);
    EXPECT_TRUE(block2);

    if (block0) a.deallocate(block0, 1);
    if (block1) a.deallocate(block1, 10);
    if (block2) a.deallocate(block2, 256);
}

TEST(HeapMASTests, AllocatorMaxSize) {
    yama::heap_mas mas0{};
    auto a = mas0.get<int>();

    EXPECT_EQ(a.max_size(), std::numeric_limits<size_t>::max());
}

TEST(HeapMASTests, AllocationAlignment) {
    yama::heap_mas mas0{};
    auto a = mas0.get<int>();

    auto block0 = a.allocate(1107);

    ASSERT_TRUE(block0);

    {
        // get block0 as raw integer, converting to void* to try and
        // make sure nothing *strange* happens during conversion, as
        // I worry might otherwise maybe

        auto integer = (uintptr_t)(void*)block0;

        // assert alignment is as expected

        EXPECT_TRUE(integer % alignof(std::max_align_t) == 0);
    }

    a.deallocate(block0, 1107);
}

