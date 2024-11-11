

#include <gtest/gtest.h>

#include <yama/core/debug.h>
#include <yama/core/mas.h>
#include <yama/mas-impls/heap_mas.h>


TEST(MASTests, AllocatorCopyCtor) {
    yama::heap_mas mas0{};
    auto a = mas0.get<int>();
    yama::mas_allocator<int> b(a);

    EXPECT_EQ(a, mas0.get<int>());
    EXPECT_EQ(b, mas0.get<int>());
}

TEST(MASTests, AllocatorMoveCtor) {
    yama::heap_mas mas0{};
    auto a = mas0.get<int>();
    yama::mas_allocator<int> b(std::move(a));

    EXPECT_EQ(b, mas0.get<int>());
}

TEST(MASTests, AllocatorRebindCtor) {
    yama::heap_mas mas0{};
    auto a = mas0.get<int>();
    yama::mas_allocator<std::string> b(a);

    EXPECT_EQ(a, mas0.get<int>());
    EXPECT_EQ(b, mas0.get<std::string>());
}

TEST(MASTests, AllocatorCopyAssign) {
    yama::heap_mas mas0{}, mas1{};
    auto a = mas0.get<int>();
    auto b = mas1.get<int>();
    b = a;

    EXPECT_EQ(a, mas0.get<int>());
    EXPECT_EQ(b, mas0.get<int>());
    EXPECT_NE(b, mas1.get<int>());
}

TEST(MASTests, AllocatorMoveAssign) {
    yama::heap_mas mas0{}, mas1{};
    auto a = mas0.get<int>();
    auto b = mas1.get<int>();
    b = std::move(a);

    EXPECT_EQ(b, mas0.get<int>());
}

TEST(MASTests, AllocatorEquality) {
    yama::heap_mas mas0{}, mas1{};
    auto a1 = mas0.get<int>();
    auto a2 = mas0.get<int>();
    auto b = mas1.get<int>();

    EXPECT_TRUE(a1.equal(a1));
    EXPECT_TRUE(a1.equal(a2));
    EXPECT_FALSE(a1.equal(b));

    EXPECT_TRUE(a2.equal(a1));
    EXPECT_TRUE(a2.equal(a2));
    EXPECT_FALSE(a2.equal(b));

    EXPECT_FALSE(b.equal(a1));
    EXPECT_FALSE(b.equal(a2));
    EXPECT_TRUE(b.equal(b));
    
    EXPECT_TRUE(a1 == a1);
    EXPECT_TRUE(a1 == a2);
    EXPECT_FALSE(a1 == b);

    EXPECT_TRUE(a2 == a1);
    EXPECT_TRUE(a2 == a2);
    EXPECT_FALSE(a2 == b);

    EXPECT_FALSE(b == a1);
    EXPECT_FALSE(b == a2);
    EXPECT_TRUE(b == b);

    EXPECT_FALSE(a1 != a1);
    EXPECT_FALSE(a1 != a2);
    EXPECT_TRUE(a1 != b);

    EXPECT_FALSE(a2 != a1);
    EXPECT_FALSE(a2 != a2);
    EXPECT_TRUE(a2 != b);

    EXPECT_TRUE(b != a1);
    EXPECT_TRUE(b != a2);
    EXPECT_FALSE(b != b);
}

TEST(MASTests, AllocatorAllocateAndDeallocate) {
    yama::stderr_debug debug{};
    yama::heap_mas mas0{};
    auto a = mas0.get<int>();

    // hard to really test this stuff outside of see to it that 
    // things don't crash...

    YAMA_LOG(&debug, yama::all_c, "{}", mas0.report());

    auto block0 = a.allocate(256);
    auto block1 = a.allocate(1);
    auto block2 = a.allocate(37);

    YAMA_LOG(&debug, yama::all_c, "{}", mas0.report());

    EXPECT_TRUE(block0);
    EXPECT_TRUE(block1);
    EXPECT_TRUE(block2);

    // dealloc in some arbitrary order

    if (block1) a.deallocate(block1, 1);
    if (block2) a.deallocate(block2, 37);
    if (block0) a.deallocate(block0, 256);

    YAMA_LOG(&debug, yama::all_c, "{}", mas0.report());
}

TEST(MASTests, AllocatorMasSize) {
    yama::heap_mas mas0{};
    auto a = mas0.get<int>();

    EXPECT_EQ(a.max_size(), std::numeric_limits<size_t>::max());
}

TEST(MASTests, AllocatorUseInAllocatorAwareContainer) {
    yama::stderr_debug debug{};
    yama::heap_mas mas0{};
    auto a = mas0.get<int>();

    {
        std::vector<int, yama::mas_allocator<int>> vec(a);
        for (size_t i = 0; i < 64; i++) {
            if (i == 0) YAMA_LOG(&debug, yama::all_c, "{}", mas0.report());
            if (i == 16) YAMA_LOG(&debug, yama::all_c, "{}", mas0.report());
            if (i == 32) YAMA_LOG(&debug, yama::all_c, "{}", mas0.report());
            if (i == 48) YAMA_LOG(&debug, yama::all_c, "{}", mas0.report());

            vec.push_back(int(i));
        }

        YAMA_LOG(&debug, yama::all_c, "{}", mas0.report());
    }
    
    YAMA_LOG(&debug, yama::all_c, "{}", mas0.report());
}

