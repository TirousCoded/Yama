

#include <gtest/gtest.h>

#include <yama/core/general.h>
#include <yama/core/linksym.h>


using namespace yama::string_literals;


// NOTE: these fmt tests are so that we can depend upon the fmt
//       method in other tests which use yama::linksym, in order
//       to help summarize those tests' logic

TEST(LinkSymTests, Fmt_NoCallSig) {
    auto fmt =
        [](yama::link_index index) -> std::string {
        if (index == 0) return "a";
        else if (index == 1) return "b";
        else if (index == 2) return "c";
        else return "?";
        };

    static_assert(yama::link_formatter<decltype(fmt)>);

    std::string expected = "(function) abc.def:a";
    std::string actual = 
        yama::make_linksym(
            "abc.def:a"_str, 
            yama::kind::function).fmt(fmt);

    std::cerr << expected << "\n" << actual << "\n";

    EXPECT_EQ(expected, actual);
}

TEST(LinkSymTests, Fmt_CallSig) {
    auto fmt =
        [](yama::link_index index) -> std::string {
        if (index == 0) return "a";
        else if (index == 1) return "b";
        else if (index == 2) return "c";
        else return "?";
        };

    static_assert(yama::link_formatter<decltype(fmt)>);

    std::string expected = "(function) abc.def:a ( fn(a, b, c) -> b )";
    std::string actual = 
        yama::make_linksym(
            "abc.def:a"_str, 
            yama::kind::function, 
            yama::make_callsig({ 0, 1, 2 }, 1)).fmt(fmt);

    std::cerr << expected << "\n" << actual << "\n";

    EXPECT_EQ(expected, actual);
}

