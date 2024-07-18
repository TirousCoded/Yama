

#include <gtest/gtest.h>

#include <yama/core/res.h>
#include <yama/domain-support/fullnamed_type.h>


using namespace yama::string_literals;


struct test_fullnamed_1 final {
    yama::str fullname_v;


    yama::str fullname() const { return fullname_v; }
};

struct test_fullnamed_2 final {
    yama::str fullname;
};


TEST(FullnamedTypeTests, Usage) {
    const auto type1 = test_fullnamed_1{ "abc"_str };
    const auto type2 = yama::make_res<test_fullnamed_1>("abc"_str);
    const auto type3 = test_fullnamed_2{ "abc"_str };
    const auto type4 = yama::make_res<test_fullnamed_2>("abc"_str);

    EXPECT_EQ(yama::dm::fullname_of(type1), "abc"_str);
    EXPECT_EQ(yama::dm::fullname_of(type2), "abc"_str);
    EXPECT_EQ(yama::dm::fullname_of(type3), "abc"_str);
    EXPECT_EQ(yama::dm::fullname_of(type4), "abc"_str);
}

