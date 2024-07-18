

#include <gtest/gtest.h>

#include <yama/core/linksym.h>
#include <yama/core/callsig_info.h>


using namespace yama::string_literals;


TEST(CallSigInfoTests, VerifyIndices) {
    std::vector<yama::linksym> linksyms{
        yama::make_linksym("a"_str, yama::kind::primitive),
        yama::make_linksym("b"_str, yama::kind::primitive),
        yama::make_linksym("c"_str, yama::kind::primitive),
    };

    EXPECT_TRUE(yama::make_callsig_info({ 0, 1, 2 }, 1).verify_indices(linksyms));
}

TEST(CallSigInfoTests, VerifyIndices_FailDueToOutOfBoundsParamTypeLinkIndex) {
    std::vector<yama::linksym> linksyms{
        yama::make_linksym("a"_str, yama::kind::primitive),
        yama::make_linksym("b"_str, yama::kind::primitive),
        yama::make_linksym("c"_str, yama::kind::primitive),
    };

    EXPECT_FALSE(yama::make_callsig_info({ 0, 1, 7 }, 1).verify_indices(linksyms));
}

TEST(CallSigInfoTests, VerifyIndices_FailDueToOutOfBoundsReturnTypeLinkIndex) {
    std::vector<yama::linksym> linksyms{
        yama::make_linksym("a"_str, yama::kind::primitive),
        yama::make_linksym("b"_str, yama::kind::primitive),
        yama::make_linksym("c"_str, yama::kind::primitive),
    };

    EXPECT_FALSE(yama::make_callsig_info({ 0, 1, 2 }, 7).verify_indices(linksyms));
}

TEST(CallSigInfoTests, Equality) {
    const auto a1 = yama::make_callsig_info({ 0, 1, 2 }, 1);
    const auto a2 = yama::make_callsig_info({ 0, 1, 2 }, 1);
    const auto b = yama::make_callsig_info({ 0, 1, 2 }, 3);
    const auto c = yama::make_callsig_info({ 0, 3, 2 }, 1);

    EXPECT_EQ(a1, a1);
    EXPECT_EQ(a1, a2);
    EXPECT_NE(a1, b);
    EXPECT_NE(a1, c);

    EXPECT_EQ(a2, a2);
    EXPECT_NE(a2, b);
    EXPECT_NE(a2, c);

    EXPECT_EQ(b, b);
    EXPECT_NE(b, c);

    EXPECT_EQ(c, c);
}

TEST(CallSigInfoTests, Equality_DiffParamCounts) {
    const auto a = yama::make_callsig_info({ 0, 1, 2 }, 1);
    const auto b = yama::make_callsig_info({ 0, 1 }, 1);
    const auto c = yama::make_callsig_info({ 0, 1, 2, 3 }, 1);

    EXPECT_EQ(a, a);
    EXPECT_NE(a, b);
    EXPECT_NE(a, c);

    EXPECT_EQ(b, b);
    EXPECT_NE(b, c);

    EXPECT_EQ(c, c);
}

// NOTE: these fmt tests exist as yama::callsig_info's fmt behaviour
//       is important to have be consistent as it's required for things
//       like link symbols to work correctly

TEST(CallSigInfoTests, Fmt) {
    std::vector<yama::linksym> linksyms{
        yama::make_linksym("a"_str, yama::kind::primitive),
        yama::make_linksym("b"_str, yama::kind::primitive),
        yama::make_linksym("c"_str, yama::kind::primitive),
    };

    std::string expected = "fn(a, b, c) -> b";
    std::string actual = yama::make_callsig_info({ 0, 1, 2 }, 1).fmt(linksyms);

    std::cerr << expected << "\n" << actual << "\n";

    EXPECT_EQ(expected, actual);
}

TEST(CallSigInfoTests, Fmt_OutOfBoundsIndices) {
    std::vector<yama::linksym> linksyms{
        yama::make_linksym("a"_str, yama::kind::primitive),
        yama::make_linksym("b"_str, yama::kind::primitive),
        yama::make_linksym("c"_str, yama::kind::primitive),
    };

    std::string expected = "fn(a, b, <out-of-bounds(3)>) -> <out-of-bounds(7)>";
    std::string actual = yama::make_callsig_info({ 0, 1, 3 }, 7).fmt(linksyms);

    std::cerr << expected << "\n" << actual << "\n";

    EXPECT_EQ(expected, actual);
}

