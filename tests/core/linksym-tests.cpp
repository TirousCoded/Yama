

#include <gtest/gtest.h>

#include <yama/core/general.h>
#include <yama/core/linksym.h>


using namespace yama::string_literals;


TEST(LinkSymTests, Fmt_NoCallSig) {
    std::string expected = "(function) abc.def:a";
    std::string actual = 
        yama::make_linksym(
            "abc.def:a"_str, 
            yama::kind::function).fmt({});

    std::cerr << expected << "\n" << actual << "\n";

    EXPECT_EQ(expected, actual);
}

TEST(LinkSymTests, Fmt_CallSig) {
    const std::vector<yama::linksym> linksyms{
        yama::make_linksym("a"_str, yama::kind::primitive),
        yama::make_linksym("b"_str, yama::kind::primitive),
        yama::make_linksym("c"_str, yama::kind::primitive),
    };

    std::string expected = "(function) abc.def:a [fn(a, b, c) -> b]";
    std::string actual =
        yama::make_linksym(
            "abc.def:a"_str,
            yama::kind::function,
            yama::make_callsig_info({ 0, 1, 2 }, 1)).fmt(linksyms);

    std::cerr << expected << "\n" << actual << "\n";

    EXPECT_EQ(expected, actual);
}

