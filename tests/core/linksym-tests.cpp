

#include <gtest/gtest.h>

#include <yama/core/general.h>
#include <yama/core/linksym.h>


using namespace yama::string_literals;


TEST(LinkSymTests, Fmt_NoCallSig) {
    std::string expected = "(function) abc.def:a";
    std::string actual = 
        yama::make_linksym(
            "abc.def:a"_str, 
            yama::kind::function).fmt();

    std::cerr << expected << "\n" << actual << "\n";

    EXPECT_EQ(expected, actual);
}

TEST(LinkSymTests, Fmt_CallSig) {
    std::string expected = "(function) abc.def:a [fn(a, b, c) -> b]";
    std::string actual = 
        yama::make_linksym(
            "abc.def:a"_str, 
            yama::kind::function, 
            "fn(a, b, c) -> b"_str).fmt();

    std::cerr << expected << "\n" << actual << "\n";

    EXPECT_EQ(expected, actual);
}

