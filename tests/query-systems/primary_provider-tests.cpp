

#include <gtest/gtest.h>

#include <yama/query-systems/all.h>


using namespace yama::string_literals;


// using a *mini* version of the test query system used in query-system-tests.cpp

struct result2 final {
    std::string s;


    bool operator==(const result2& other) const noexcept { return s == other.s; }
};

struct key2 final {
    static constexpr yama::qs::qtype_t qtype = "a";
    using result = result2;

    std::string s;

    bool operator==(const key2& other) const noexcept { return s == other.s; }
    size_t hash() const noexcept { return std::hash<decltype(s)>{}(s); }
};

YAMA_SETUP_HASH(key2, x.hash());

static_assert(yama::qs::key_type<key2>);


// the provider type under test

using test_primary_provider = yama::qs::primary_provider<key2>;


// helpers

const result2 a{ "a" };
const result2 b{ "b" };
const result2 c{ "c" };
const key2 a_k{ a.s };
const key2 b_k{ b.s };
const key2 c_k{ c.s };


TEST(PrimaryProviderTests, Push) {
    test_primary_provider prov{};

    EXPECT_EQ(prov.number(), 0);
    EXPECT_FALSE(prov.is_cached(a_k));
    EXPECT_FALSE(prov.is_cached(b_k));

    EXPECT_TRUE(prov.push(a_k, a));

    EXPECT_EQ(prov.number(), 1);
    EXPECT_TRUE(prov.is_cached(a_k));
    EXPECT_FALSE(prov.is_cached(b_k));

    EXPECT_TRUE(prov.push(b_k, b));

    EXPECT_EQ(prov.number(), 2);
    EXPECT_TRUE(prov.is_cached(a_k));
    EXPECT_TRUE(prov.is_cached(b_k));
}

TEST(PrimaryProviderTests, Push_FailDueToKeyAlreadyTaken) {
    test_primary_provider prov{};

    EXPECT_EQ(prov.number(), 0);
    EXPECT_FALSE(prov.is_cached(a_k));
    EXPECT_FALSE(prov.is_cached(b_k));

    EXPECT_TRUE(prov.push(a_k, a));

    EXPECT_EQ(prov.number(), 1);
    EXPECT_TRUE(prov.is_cached(a_k));
    EXPECT_FALSE(prov.is_cached(b_k));

    EXPECT_FALSE(prov.push(a_k, result2{ "different result data" })); // <- already pushed w/ 'a_k'

    EXPECT_EQ(prov.number(), 1);
    EXPECT_TRUE(prov.is_cached(a_k));
    EXPECT_FALSE(prov.is_cached(b_k));
}

TEST(PrimaryProviderTests, Number) {
    test_primary_provider prov{};

    EXPECT_EQ(prov.number(), 0);

    prov.push(a_k, a);

    EXPECT_EQ(prov.number(), 1);

    prov.push(b_k, b);

    EXPECT_EQ(prov.number(), 2);

    prov.push(c_k, c);

    EXPECT_EQ(prov.number(), 3);

    prov.reset();

    EXPECT_EQ(prov.number(), 0);
}

TEST(PrimaryProviderTests, IsCached) {
    test_primary_provider prov{};

    EXPECT_FALSE(prov.is_cached(a_k));
    EXPECT_FALSE(prov.is_cached(b_k));
    EXPECT_FALSE(prov.is_cached(c_k));

    prov.push(a_k, a);

    EXPECT_TRUE(prov.is_cached(a_k));
    EXPECT_FALSE(prov.is_cached(b_k));
    EXPECT_FALSE(prov.is_cached(c_k));

    prov.push(b_k, b);

    EXPECT_TRUE(prov.is_cached(a_k));
    EXPECT_TRUE(prov.is_cached(b_k));
    EXPECT_FALSE(prov.is_cached(c_k));

    prov.push(c_k, c);

    EXPECT_TRUE(prov.is_cached(a_k));
    EXPECT_TRUE(prov.is_cached(b_k));
    EXPECT_TRUE(prov.is_cached(c_k));

    prov.reset();

    EXPECT_FALSE(prov.is_cached(a_k));
    EXPECT_FALSE(prov.is_cached(b_k));
    EXPECT_FALSE(prov.is_cached(c_k));
}

TEST(PrimaryProviderTests, Query) {
    test_primary_provider prov{};

    prov.push(a_k, a);
    //prov.push(b_k, b); <- excluded for test
    prov.push(c_k, c);

    ASSERT_TRUE(prov.is_cached(a_k));
    ASSERT_FALSE(prov.is_cached(b_k));
    ASSERT_TRUE(prov.is_cached(c_k));

    auto result_a = prov.query(a_k);
    auto result_b = prov.query(b_k);
    auto result_c = prov.query(c_k);

    EXPECT_TRUE(result_a);
    EXPECT_FALSE(result_b);
    EXPECT_TRUE(result_c);

    if (result_a) EXPECT_EQ(*result_a, a);
    if (result_c) EXPECT_EQ(*result_c, c);
}

TEST(PrimaryProviderTests, Fetch) {
    test_primary_provider prov{};

    prov.push(a_k, a);
    //prov.push(b_k, b); <- excluded for test
    prov.push(c_k, c);

    ASSERT_TRUE(prov.is_cached(a_k));
    ASSERT_FALSE(prov.is_cached(b_k));
    ASSERT_TRUE(prov.is_cached(c_k));

    auto result_a = prov.fetch(a_k);
    auto result_b = prov.fetch(b_k);
    auto result_c = prov.fetch(c_k);

    EXPECT_TRUE(result_a);
    EXPECT_FALSE(result_b);
    EXPECT_TRUE(result_c);

    if (result_a) EXPECT_EQ(*result_a, a);
    if (result_c) EXPECT_EQ(*result_c, c);
}

TEST(PrimaryProviderTests, Discard) {
    test_primary_provider prov{};

    prov.push(a_k, a);
    prov.push(b_k, b);
    prov.push(c_k, c);

    ASSERT_TRUE(prov.is_cached(a_k));
    ASSERT_TRUE(prov.is_cached(b_k));
    ASSERT_TRUE(prov.is_cached(c_k));

    // discard impl is a noop

    prov.discard(a_k);
    prov.discard(b_k);
    prov.discard(c_k);

    EXPECT_TRUE(prov.is_cached(a_k));
    EXPECT_TRUE(prov.is_cached(b_k));
    EXPECT_TRUE(prov.is_cached(c_k));
}

TEST(PrimaryProviderTests, DiscardAll) {
    test_primary_provider prov{};

    prov.push(a_k, a);
    prov.push(b_k, b);
    prov.push(c_k, c);

    ASSERT_EQ(prov.number(), 3);
    ASSERT_TRUE(prov.is_cached(a_k));
    ASSERT_TRUE(prov.is_cached(b_k));
    ASSERT_TRUE(prov.is_cached(c_k));

    // discard_all impl is a noop

    prov.discard_all();

    EXPECT_EQ(prov.number(), 3);
    EXPECT_TRUE(prov.is_cached(a_k));
    EXPECT_TRUE(prov.is_cached(b_k));
    EXPECT_TRUE(prov.is_cached(c_k));
}

TEST(PrimaryProviderTests, Reset) {
    test_primary_provider prov{};

    ASSERT_TRUE(prov.push(a_k, a));
    ASSERT_TRUE(prov.push(b_k, b));

    ASSERT_EQ(prov.number(), 2);
    ASSERT_TRUE(prov.is_cached(a_k));
    ASSERT_TRUE(prov.is_cached(b_k));

    prov.reset();

    EXPECT_EQ(prov.number(), 0);
    EXPECT_FALSE(prov.is_cached(a_k));
    EXPECT_FALSE(prov.is_cached(b_k));

    // should be able to re-add things after reset

    EXPECT_TRUE(prov.push(a_k, a));
    EXPECT_TRUE(prov.push(b_k, b));

    EXPECT_EQ(prov.number(), 2);
    EXPECT_TRUE(prov.is_cached(a_k));
    EXPECT_TRUE(prov.is_cached(b_k));
}

