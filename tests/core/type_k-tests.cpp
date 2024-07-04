

#include <gtest/gtest.h>

#include <yama/core/type_k.h>


using namespace yama::string_literals;


// IMPORTANT:
//      these tests don't cover hashing and formatting


TEST(TypeKTests, QType) {
    EXPECT_EQ(yama::qs::qtype_of<yama::type_k>(), "type");
}

TEST(TypeKTests, Equality) {
    yama::type_k ka1{
        .fullname = "abc"_str,
    };
    yama::type_k ka2{
        .fullname = "abc"_str,
    };
    yama::type_k kb{
        .fullname = "def"_str,
    };


    EXPECT_TRUE(ka1 == ka1);
    EXPECT_TRUE(ka1 == ka2);
    EXPECT_FALSE(ka1 == kb);

    EXPECT_TRUE(ka2 == ka1);
    EXPECT_TRUE(ka2 == ka2);
    EXPECT_FALSE(ka2 == kb);

    EXPECT_FALSE(kb == ka1);
    EXPECT_FALSE(kb == ka2);
    EXPECT_TRUE(kb == kb);


    EXPECT_FALSE(ka1 != ka1);
    EXPECT_FALSE(ka1 != ka2);
    EXPECT_TRUE(ka1 != kb);

    EXPECT_FALSE(ka2 != ka1);
    EXPECT_FALSE(ka2 != ka2);
    EXPECT_TRUE(ka2 != kb);

    EXPECT_TRUE(kb != ka1);
    EXPECT_TRUE(kb != ka2);
    EXPECT_FALSE(kb != kb);
}

