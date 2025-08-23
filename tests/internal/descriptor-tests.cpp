

#include <gtest/gtest.h>

#include <unordered_set>

#include <yama/core/module.h>


struct A { int v; };
struct B { float v; };
struct C { char v; };
struct D {};

using DescsMap = yama::internal::desc_map<size_t, A, B, C>;


static_assert(DescsMap::supports<A>);
static_assert(DescsMap::supports<B>);
static_assert(DescsMap::supports<C>);
static_assert(DescsMap::supports<A, B, C>);
static_assert(!DescsMap::supports<D>);
static_assert(!DescsMap::supports<A, B, C, D>);


TEST(DescsMapTests, Count) {
    DescsMap dm{};

    EXPECT_EQ(dm.count(), 0);

    dm.bind<A>(0, 50);
    dm.bind<B>(0, 13.55f);
    dm.bind<A>(10, 1);
    dm.bind<B>(101, 1.4f);

    EXPECT_EQ(dm.count(), 3);
}

TEST(DescsMapTests, CountWith) {
    DescsMap dm{};

    {
        EXPECT_EQ(dm.count_with<>(), 0);
        EXPECT_EQ(dm.count_with<A>(), 0);
        EXPECT_EQ(dm.count_with<B>(), 0);
        EXPECT_EQ(dm.count_with<C>(), 0);
        auto v0 = dm.count_with<A, B>();
        EXPECT_EQ(v0, 0);
        auto v1 = dm.count_with<A, B, C>();
        EXPECT_EQ(v1, 0);
    }

    dm.bind<A>(0, 50);
    dm.bind<B>(0, 13.55f);
    dm.bind<A>(10, 1);
    dm.bind<B>(101, 1.4f);

    {
        EXPECT_EQ(dm.count_with<>(), 3);
        EXPECT_EQ(dm.count_with<A>(), 2);
        EXPECT_EQ(dm.count_with<B>(), 2);
        EXPECT_EQ(dm.count_with<C>(), 0);
        auto v0 = dm.count_with<A, B>();
        EXPECT_EQ(v0, 1);
        auto v = dm.count_with<A, B, C>();
        EXPECT_EQ(v, 0);
    }
}

TEST(DescsMapTests, AllOf) {
    DescsMap dm{};

    dm.bind<A>(0, 50);
    dm.bind<B>(0, 13.55f);
    dm.bind<A>(10, 1);
    dm.bind<B>(101, 1.4f);

    {
        yama::lid_t id = 0;
        EXPECT_TRUE(dm.all_of<>(id));
        EXPECT_TRUE(dm.all_of<A>(id));
        EXPECT_TRUE(dm.all_of<B>(id));
        EXPECT_FALSE(dm.all_of<C>(id));
        auto v0 = dm.all_of<A, B>(id);
        EXPECT_TRUE(v0);
        auto v1 = dm.all_of<A, C>(id);
        EXPECT_FALSE(v1);
        auto v2 = dm.all_of<B, C>(id);
        EXPECT_FALSE(v2);
        auto v3 = dm.all_of<A, B, C>(id);
        EXPECT_FALSE(v3);
    }
    {
        yama::lid_t id = 10;
        EXPECT_TRUE(dm.all_of<>(id));
        EXPECT_TRUE(dm.all_of<A>(id));
        EXPECT_FALSE(dm.all_of<B>(id));
        EXPECT_FALSE(dm.all_of<C>(id));
        auto v0 = dm.all_of<A, B>(id);
        EXPECT_FALSE(v0);
        auto v1 = dm.all_of<A, C>(id);
        EXPECT_FALSE(v1);
        auto v2 = dm.all_of<B, C>(id);
        EXPECT_FALSE(v2);
        auto v3 = dm.all_of<A, B, C>(id);
        EXPECT_FALSE(v3);
    }
    {
        yama::lid_t id = 101;
        EXPECT_TRUE(dm.all_of<>(id));
        EXPECT_FALSE(dm.all_of<A>(id));
        EXPECT_TRUE(dm.all_of<B>(id));
        EXPECT_FALSE(dm.all_of<C>(id));
        auto v0 = dm.all_of<A, B>(id);
        EXPECT_FALSE(v0);
        auto v1 = dm.all_of<A, C>(id);
        EXPECT_FALSE(v1);
        auto v2 = dm.all_of<B, C>(id);
        EXPECT_FALSE(v2);
        auto v3 = dm.all_of<A, B, C>(id);
        EXPECT_FALSE(v3);
    }
    {
        yama::lid_t id = 10'000; // Valid ID, just has no descriptors.
        EXPECT_TRUE(dm.all_of<>(id));
        EXPECT_FALSE(dm.all_of<A>(id));
        EXPECT_FALSE(dm.all_of<B>(id));
        EXPECT_FALSE(dm.all_of<C>(id));
        auto v0 = dm.all_of<A, B>(id);
        EXPECT_FALSE(v0);
        auto v1 = dm.all_of<A, C>(id);
        EXPECT_FALSE(v1);
        auto v2 = dm.all_of<B, C>(id);
        EXPECT_FALSE(v2);
        auto v3 = dm.all_of<A, B, C>(id);
        EXPECT_FALSE(v3);
    }
}

TEST(DescsMapTests, AnyOf) {
    DescsMap dm{};

    dm.bind<A>(0, 50);
    dm.bind<B>(0, 13.55f);
    dm.bind<A>(10, 1);
    dm.bind<B>(101, 1.4f);

    {
        yama::lid_t id = 0;
        EXPECT_FALSE(dm.any_of<>(id));
        EXPECT_TRUE(dm.any_of<A>(id));
        EXPECT_TRUE(dm.any_of<B>(id));
        EXPECT_FALSE(dm.any_of<C>(id));
        auto v0 = dm.any_of<A, B>(id);
        EXPECT_TRUE(v0);
        auto v1 = dm.any_of<A, C>(id);
        EXPECT_TRUE(v1);
        auto v2 = dm.any_of<B, C>(id);
        EXPECT_TRUE(v2);
        auto v3 = dm.any_of<A, B, C>(id);
        EXPECT_TRUE(v3);
    }
    {
        yama::lid_t id = 10;
        EXPECT_FALSE(dm.any_of<>(id));
        EXPECT_TRUE(dm.any_of<A>(id));
        EXPECT_FALSE(dm.any_of<B>(id));
        EXPECT_FALSE(dm.any_of<C>(id));
        auto v0 = dm.any_of<A, B>(id);
        EXPECT_TRUE(v0);
        auto v1 = dm.any_of<A, C>(id);
        EXPECT_TRUE(v1);
        auto v2 = dm.any_of<B, C>(id);
        EXPECT_FALSE(v2);
        auto v3 = dm.any_of<A, B, C>(id);
        EXPECT_TRUE(v3);
    }
    {
        yama::lid_t id = 101;
        EXPECT_FALSE(dm.any_of<>(id));
        EXPECT_FALSE(dm.any_of<A>(id));
        EXPECT_TRUE(dm.any_of<B>(id));
        EXPECT_FALSE(dm.any_of<C>(id));
        auto v0 = dm.any_of<A, B>(id);
        EXPECT_TRUE(v0);
        auto v1 = dm.any_of<A, C>(id);
        EXPECT_FALSE(v1);
        auto v2 = dm.any_of<B, C>(id);
        EXPECT_TRUE(v2);
        auto v3 = dm.any_of<A, B, C>(id);
        EXPECT_TRUE(v3);
    }
    {
        yama::lid_t id = 10'000; // Valid ID, just has no descriptors.
        EXPECT_FALSE(dm.any_of<>(id));
        EXPECT_FALSE(dm.any_of<A>(id));
        EXPECT_FALSE(dm.any_of<B>(id));
        EXPECT_FALSE(dm.any_of<C>(id));
        auto v0 = dm.any_of<A, B>(id);
        EXPECT_FALSE(v0);
        auto v1 = dm.any_of<A, C>(id);
        EXPECT_FALSE(v1);
        auto v2 = dm.any_of<B, C>(id);
        EXPECT_FALSE(v2);
        auto v3 = dm.any_of<A, B, C>(id);
        EXPECT_FALSE(v3);
    }
}

TEST(DescsMapTests, NoneOf) {
    DescsMap dm{};

    dm.bind<A>(0, 50);
    dm.bind<B>(0, 13.55f);
    dm.bind<A>(10, 1);
    dm.bind<B>(101, 1.4f);

    {
        yama::lid_t id = 0;
        EXPECT_TRUE(dm.none_of<>(id));
        EXPECT_FALSE(dm.none_of<A>(id));
        EXPECT_FALSE(dm.none_of<B>(id));
        EXPECT_TRUE(dm.none_of<C>(id));
        auto v0 = dm.none_of<A, B>(id);
        EXPECT_FALSE(v0);
        auto v1 = dm.none_of<A, C>(id);
        EXPECT_FALSE(v1);
        auto v2 = dm.none_of<B, C>(id);
        EXPECT_FALSE(v2);
        auto v3 = dm.none_of<A, B, C>(id);
        EXPECT_FALSE(v3);
    }
    {
        yama::lid_t id = 10;
        EXPECT_TRUE(dm.none_of<>(id));
        EXPECT_FALSE(dm.none_of<A>(id));
        EXPECT_TRUE(dm.none_of<B>(id));
        EXPECT_TRUE(dm.none_of<C>(id));
        auto v0 = dm.none_of<A, B>(id);
        EXPECT_FALSE(v0);
        auto v1 = dm.none_of<A, C>(id);
        EXPECT_FALSE(v1);
        auto v2 = dm.none_of<B, C>(id);
        EXPECT_TRUE(v2);
        auto v3 = dm.none_of<A, B, C>(id);
        EXPECT_FALSE(v3);
    }
    {
        yama::lid_t id = 101;
        EXPECT_TRUE(dm.none_of<>(id));
        EXPECT_TRUE(dm.none_of<A>(id));
        EXPECT_FALSE(dm.none_of<B>(id));
        EXPECT_TRUE(dm.none_of<C>(id));
        auto v0 = dm.none_of<A, B>(id);
        EXPECT_FALSE(v0);
        auto v1 = dm.none_of<A, C>(id);
        EXPECT_TRUE(v1);
        auto v2 = dm.none_of<B, C>(id);
        EXPECT_FALSE(v2);
        auto v3 = dm.none_of<A, B, C>(id);
        EXPECT_FALSE(v3);
    }
    {
        yama::lid_t id = 10'000; // Valid ID, just has no descriptors.
        EXPECT_TRUE(dm.none_of<>(id));
        EXPECT_TRUE(dm.none_of<A>(id));
        EXPECT_TRUE(dm.none_of<B>(id));
        EXPECT_TRUE(dm.none_of<C>(id));
        auto v0 = dm.none_of<A, B>(id);
        EXPECT_TRUE(v0);
        auto v1 = dm.none_of<A, C>(id);
        EXPECT_TRUE(v1);
        auto v2 = dm.none_of<B, C>(id);
        EXPECT_TRUE(v2);
        auto v3 = dm.none_of<A, B, C>(id);
        EXPECT_TRUE(v3);
    }
}

TEST(DescsMapTests, TryGet) {
    DescsMap dm{};

    dm.bind<A>(0, 50);
    dm.bind<B>(0, 13.55f);
    dm.bind<A>(10, 1);
    dm.bind<B>(101, 1.4f);

    {
        auto [a, b, c] = dm.try_get<A, B, C>(0);
        static_assert(std::same_as<decltype(a), A*>);
        static_assert(std::same_as<decltype(b), B*>);
        static_assert(std::same_as<decltype(c), C*>);
        EXPECT_NE(a, nullptr);
        EXPECT_NE(b, nullptr);
        EXPECT_EQ(c, nullptr);
        if (a) {
            EXPECT_EQ(a->v, 50);
        }
        if (b) {
            EXPECT_FLOAT_EQ(b->v, 13.55f);
        }
    }
    {
        auto [a, b, c] = dm.try_get<A, B, C>(10);
        EXPECT_NE(a, nullptr);
        EXPECT_EQ(b, nullptr);
        EXPECT_EQ(c, nullptr);
        if (a) {
            EXPECT_EQ(a->v, 1);
        }
    }
    {
        auto [a, b, c] = dm.try_get<A, B, C>(101);
        EXPECT_EQ(a, nullptr);
        EXPECT_NE(b, nullptr);
        EXPECT_EQ(c, nullptr);
        if (b) {
            EXPECT_FLOAT_EQ(b->v, 1.4f);
        }
    }
    {
        auto [a, b, c] = dm.try_get<A, B, C>(10'000); // Valid ID, just has no descriptors.
        EXPECT_EQ(a, nullptr);
        EXPECT_EQ(b, nullptr);
        EXPECT_EQ(c, nullptr);
    }
    {
        // Ensure all pointers are the same for each descriptor type.
        auto [a1, b1, c1] = dm.try_get<A, B, C>(0);
        auto [a2, b2, c2, a3, b3, c3] = dm.try_get<A, B, C, A, B, C>(0);
        EXPECT_EQ(a1, a2);
        EXPECT_EQ(a1, a3);
        EXPECT_EQ(a2, a3);

        EXPECT_EQ(b1, b2);
        EXPECT_EQ(b1, b3);
        EXPECT_EQ(b2, b3);

        EXPECT_EQ(c1, c2);
        EXPECT_EQ(c1, c3);
        EXPECT_EQ(c2, c3);
    }
}

TEST(DescsMapTests, Get) {
    DescsMap dm{};

    dm.bind<A>(0, 50);
    dm.bind<B>(0, 13.55f);
    dm.bind<A>(10, 1);
    dm.bind<B>(101, 1.4f);

    {
        auto [a, b] = dm.get<A, B>(0);
        static_assert(std::same_as<decltype(a), A&>);
        static_assert(std::same_as<decltype(b), B&>);
        EXPECT_EQ(a.v, 50);
        EXPECT_FLOAT_EQ(b.v, 13.55f);
    }
    {
        auto [a] = dm.get<A>(10);
        EXPECT_EQ(a.v, 1);
    }
    {
        auto [b] = dm.get<B>(101);
        EXPECT_FLOAT_EQ(b.v, 1.4f);
    }
    {
        // Ensure all references are the same for each descriptor type.
        auto [a1, b1] = dm.get<A, B>(0);
        auto [a2, b2, a3, b3] = dm.get<A, B, A, B>(0);
        EXPECT_EQ(&a1, &a2);
        EXPECT_EQ(&a1, &a3);
        EXPECT_EQ(&a2, &a3);

        EXPECT_EQ(&b1, &b2);
        EXPECT_EQ(&b1, &b3);
        EXPECT_EQ(&b2, &b3);
    }
    {
        // Throws expected exceptions.
        auto fn0 = [&dm]() { dm.get<A, B, C>(0); }; // 0 has bound descriptors, just not for C
        auto fn1 = [&dm]() { dm.get<A>(10'000); }; // 10'000 has no bound descriptors at all (tests impl can handle this case)
        EXPECT_THROW(fn0(), std::out_of_range);
        EXPECT_THROW(fn1(), std::out_of_range);
    }
}

TEST(DescsMapTests, View) {
    DescsMap dm{};

    dm.bind<A>(0, 50);
    dm.bind<B>(0, 13.55f);
    dm.bind<A>(2, 150);
    dm.bind<B>(2, 113.55f);
    dm.bind<A>(10, 1);
    dm.bind<B>(101, 1.4f);

    {
        std::unordered_set<size_t> ids{};
        for (auto [id, a] : dm.view<A>()) {
            ids.insert(id);
            auto [a_] = dm.get<A>(id);
            EXPECT_EQ(&a, &a_);
        }
        EXPECT_EQ(ids.size(), 3);
        EXPECT_TRUE(ids.contains(0));
        EXPECT_TRUE(ids.contains(2));
        EXPECT_TRUE(ids.contains(10));
    }
    {
        std::unordered_set<size_t> ids{};
        for (auto [id, b] : dm.view<B>()) {
            ids.insert(id);
            auto [b_] = dm.get<B>(id);
            EXPECT_EQ(&b, &b_);
        }
        EXPECT_EQ(ids.size(), 3);
        EXPECT_TRUE(ids.contains(0));
        EXPECT_TRUE(ids.contains(2));
        EXPECT_TRUE(ids.contains(101));
    }
    {
        std::unordered_set<size_t> ids{};
        for (auto [id, a, b] : dm.view<A, B>()) {
            ids.insert(id);
            auto [a_, b_] = dm.get<A, B>(id);
            EXPECT_EQ(&a, &a_);
            EXPECT_EQ(&b, &b_);
        }
        EXPECT_EQ(ids.size(), 2);
        EXPECT_TRUE(ids.contains(0));
        EXPECT_TRUE(ids.contains(2));
    }
}

TEST(DescsMapTests, Bind) {
    DescsMap dm{};

    dm.bind<A>(0, 50);

    auto [a] = dm.try_get<A>(0);
    EXPECT_TRUE(a);
    if (a) {
        EXPECT_EQ(a->v, 50);
    }
}

TEST(DescsMapTests, Unbind) {
    DescsMap dm{};

    dm.bind<A>(0, 50);
    dm.bind<B>(0, 13.55f);
    dm.bind<A>(10, 1);
    dm.bind<B>(101, 1.4f);

    {
        auto v = dm.all_of<A, B>(0);
        EXPECT_TRUE(v);
    }

    dm.unbind<A, B>(0);

    {
        auto v = dm.none_of<A, B>(0);
        EXPECT_TRUE(v);
    }
}

TEST(DescsMapTests, UnbindEvery) {
    DescsMap dm{};

    dm.bind<A>(0, 50);
    dm.bind<B>(0, 13.55f);
    dm.bind<A>(2, 150);
    dm.bind<B>(2, 113.55f);
    dm.bind<A>(10, 1);
    dm.bind<B>(101, 1.4f);

    EXPECT_EQ(dm.count_with<A>(), 3);
    EXPECT_EQ(dm.count_with<B>(), 3);

    dm.unbind_every<A, B>();

    EXPECT_EQ(dm.count_with<A>(), 0);
    EXPECT_EQ(dm.count_with<B>(), 0);
}

