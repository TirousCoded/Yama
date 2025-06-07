

#include <gtest/gtest.h>

#include <yama/core/general.h>
#include <yama/core/callsig_info.h>
#include <yama/core/const_table_info.h>


using namespace yama::string_literals;


TEST(CallSigInfoTests, Equality) {
    const auto a1 = yama::make_callsig({ 0, 1, 2 }, 1);
    const auto a2 = yama::make_callsig({ 0, 1, 2 }, 1);
    const auto b = yama::make_callsig({ 0, 1, 2 }, 3);
    const auto c = yama::make_callsig({ 0, 3, 2 }, 1);

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
    const auto a = yama::make_callsig({ 0, 1, 2 }, 1);
    const auto b = yama::make_callsig({ 0, 1 }, 1);
    const auto c = yama::make_callsig({ 0, 1, 2, 3 }, 1);

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
    const auto consts =
        yama::const_table_info()
        .add_primitive_type("a"_str)
        .add_primitive_type("b"_str)
        .add_primitive_type("c"_str);

    std::string expected = "fn(a, b, c) -> b";
    std::string actual = yama::make_callsig({ 0, 1, 2 }, 1).fmt(consts);

    std::cerr << expected << "\n" << actual << "\n";

    EXPECT_EQ(expected, actual);
}

// TODO: later on, when we replace links w/ consts fully, add tests for when a callsig_info
//       illegally refers to constants which are not valid object types
//
//       also gonna need static verif tests for that too!

TEST(CallSigInfoTests, Fmt_OutOfBoundsIndices) {
    const auto consts =
        yama::const_table_info()
        .add_primitive_type("a"_str)
        .add_primitive_type("b"_str)
        .add_primitive_type("c"_str);

    std::string expected = "fn(a, b, <illegal(3)>) -> <illegal(7)>";
    std::string actual = yama::make_callsig({ 0, 1, 3 }, 7).fmt(consts);

    std::cerr << expected << "\n" << actual << "\n";

    EXPECT_EQ(expected, actual);
}

