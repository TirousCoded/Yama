

#include <gtest/gtest.h>

#include <yama/core/callsig.h>


// NOTE: these fmt tests are so that we can depend upon the fmt
//       method in other tests which use yama::callsig, in order
//       to help summarize those tests' logic

TEST(CallSigTests, Fmt) {
    auto fmt =
        [](yama::link_index index) -> std::string {
        if (index == 0) return "a";
        else if (index == 1) return "b";
        else if (index == 2) return "c";
        else return "?";
        };

    static_assert(yama::link_formatter<decltype(fmt)>);

    std::string expected = "fn(a, b, c) -> b";
    std::string actual = yama::make_callsig({ 0, 1, 2 }, 1).fmt(fmt);

    std::cerr << expected << "\n" << actual << "\n";

    EXPECT_EQ(expected, actual);
}

