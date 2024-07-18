

#include <gtest/gtest.h>

#include <yama/domain-support/res_db.h>


using namespace yama::string_literals;


struct test_res final {
    yama::str fullname;
};

static_assert(yama::dm::fullnamed_type<test_res>);

using test_res_db = yama::dm::res_db<test_res>;


TEST(ResDBTests, Ctor) {
    test_res_db db{};

    EXPECT_EQ(db.size(), 0);
}

TEST(ResDBTests, Size) {
    test_res_db db{};

    EXPECT_EQ(db.size(), 0);

    ASSERT_TRUE(db.push(test_res{ "a"_str }));

    EXPECT_EQ(db.size(), 1);

    ASSERT_TRUE(db.push(test_res{ "b"_str }));

    EXPECT_EQ(db.size(), 2);
}

TEST(ResDBTests, Exists) {
    test_res_db db{};

    ASSERT_TRUE(db.push(test_res{ "a"_str }));
    ASSERT_TRUE(db.push(test_res{ "c"_str }));

    EXPECT_TRUE(db.exists("a"_str));
    EXPECT_FALSE(db.exists("b"_str));
    EXPECT_TRUE(db.exists("c"_str));
}

TEST(ResDBTests, Pull) {
    test_res_db db{};

    ASSERT_TRUE(db.push(test_res{ "a"_str }));
    ASSERT_TRUE(db.push(test_res{ "c"_str }));

    auto a = db.pull("a"_str);
    auto b = db.pull("b"_str);
    auto c = db.pull("c"_str);

    EXPECT_TRUE(a);
    EXPECT_FALSE(b);
    EXPECT_TRUE(c);
    if (a) EXPECT_EQ(yama::dm::fullname_of(a), "a"_str);
    if (c) EXPECT_EQ(yama::dm::fullname_of(c), "c"_str);
}

TEST(ResDBTests, Push) {
    test_res_db db{};

    ASSERT_TRUE(db.push(test_res{ "a"_str }));
    ASSERT_TRUE(db.push(test_res{ "b"_str }));
    ASSERT_TRUE(db.push(test_res{ "c"_str }));

    EXPECT_EQ(db.size(), 3);
    EXPECT_TRUE(db.exists("a"_str));
    EXPECT_TRUE(db.exists("b"_str));
    EXPECT_TRUE(db.exists("c"_str));
}

TEST(ResDBTests, Push_FailDueToFullnameAlreadyTaken) {
    test_res_db db{};

    ASSERT_TRUE(db.push(test_res{ "a"_str }));

    EXPECT_EQ(db.size(), 1);
    EXPECT_TRUE(db.exists("a"_str));

    ASSERT_FALSE(db.push(test_res{ "a"_str }));

    EXPECT_EQ(db.size(), 1);
    EXPECT_TRUE(db.exists("a"_str));
}

TEST(ResDBTests, Reset) {
    test_res_db db{};

    ASSERT_TRUE(db.push(test_res{ "a"_str }));
    ASSERT_TRUE(db.push(test_res{ "b"_str }));
    ASSERT_TRUE(db.push(test_res{ "c"_str }));

    ASSERT_EQ(db.size(), 3);
    ASSERT_TRUE(db.exists("a"_str));
    ASSERT_TRUE(db.exists("b"_str));
    ASSERT_TRUE(db.exists("c"_str));

    db.reset();

    EXPECT_EQ(db.size(), 0);
    EXPECT_FALSE(db.exists("a"_str));
    EXPECT_FALSE(db.exists("b"_str));
    EXPECT_FALSE(db.exists("c"_str));
}

TEST(ResDBTests, Transfer) {
    test_res_db db1{}, db2{};

    ASSERT_TRUE(db1.push(test_res{ "a"_str }));
    ASSERT_TRUE(db1.push(test_res{ "b"_str }));
    ASSERT_TRUE(db1.push(test_res{ "c"_str }));

    ASSERT_TRUE(db2.push(test_res{ "d"_str }));

    ASSERT_EQ(db1.size(), 3);
    ASSERT_TRUE(db1.exists("a"_str));
    ASSERT_TRUE(db1.exists("b"_str));
    ASSERT_TRUE(db1.exists("c"_str));
    ASSERT_FALSE(db1.exists("d"_str));

    ASSERT_EQ(db2.size(), 1);
    ASSERT_FALSE(db2.exists("a"_str));
    ASSERT_FALSE(db2.exists("b"_str));
    ASSERT_FALSE(db2.exists("c"_str));
    ASSERT_TRUE(db2.exists("d"_str));

    EXPECT_TRUE(db1.transfer(db2));

    EXPECT_EQ(db1.size(), 0);
    EXPECT_FALSE(db1.exists("a"_str));
    EXPECT_FALSE(db1.exists("b"_str));
    EXPECT_FALSE(db1.exists("c"_str));
    EXPECT_FALSE(db1.exists("d"_str));

    EXPECT_EQ(db2.size(), 4);
    EXPECT_TRUE(db2.exists("a"_str));
    EXPECT_TRUE(db2.exists("b"_str));
    EXPECT_TRUE(db2.exists("c"_str));
    EXPECT_TRUE(db2.exists("d"_str));
}

TEST(ResDBTests, Transfer_FailDueToOverlap) {
    test_res_db db1{}, db2{};

    ASSERT_TRUE(db1.push(test_res{ "a"_str }));
    ASSERT_TRUE(db1.push(test_res{ "b"_str }));
    ASSERT_TRUE(db1.push(test_res{ "c"_str }));

    ASSERT_TRUE(db2.push(test_res{ "b"_str })); // <- overlap!
    ASSERT_TRUE(db2.push(test_res{ "d"_str }));

    ASSERT_EQ(db1.size(), 3);
    ASSERT_TRUE(db1.exists("a"_str));
    ASSERT_TRUE(db1.exists("b"_str));
    ASSERT_TRUE(db1.exists("c"_str));
    ASSERT_FALSE(db1.exists("d"_str));

    ASSERT_EQ(db2.size(), 2);
    ASSERT_FALSE(db2.exists("a"_str));
    ASSERT_TRUE(db2.exists("b"_str));
    ASSERT_FALSE(db2.exists("c"_str));
    ASSERT_TRUE(db2.exists("d"_str));

    EXPECT_FALSE(db1.transfer(db2)); // <- failure!

    EXPECT_EQ(db1.size(), 3);
    EXPECT_TRUE(db1.exists("a"_str));
    EXPECT_TRUE(db1.exists("b"_str));
    EXPECT_TRUE(db1.exists("c"_str));
    EXPECT_FALSE(db1.exists("d"_str));

    EXPECT_EQ(db2.size(), 2);
    EXPECT_FALSE(db2.exists("a"_str));
    EXPECT_TRUE(db2.exists("b"_str));
    EXPECT_FALSE(db2.exists("c"_str));
    EXPECT_TRUE(db2.exists("d"_str));
}

