﻿

#include <filesystem>

#include <gtest/gtest.h>

#include <taul/all.h>

#include <yama/core/asserts.h>
#include <yama/core/general.h>
#include <yama/core/scalars.h>
#include <yama/core/debug.h>
#include <yama/core/yama_gram.h>


// IMPORTANT: see yama_gram.cpp for config about whether or not the below unit tests
//            are to use dynamic loading or source generation to acquire the grammar


using namespace yama::string_literals;


class YamaSpecTests : public testing::Test {
protected:
    
    std::shared_ptr<yama::debug> dbg;
    std::shared_ptr<taul::logger> lgr;
    // it's useful to share this between tests
    static std::optional<taul::grammar> gram;


    void SetUp() {
        dbg = std::make_shared<yama::stderr_debug>();
        lgr = std::make_shared<taul::stderr_logger>();
        if (!gram) {
            gram = std::make_optional(yama::yama_gram());
        }
        YAMA_ASSERT(gram);
    }

    void TearDown() {
        //
    }
};

std::optional<taul::grammar> YamaSpecTests::gram = std::nullopt;


TEST_F(YamaSpecTests, GrammarLoads) {
    ASSERT_TRUE(gram);
}

TEST_F(YamaSpecTests, LPRs) {
    ASSERT_TRUE(gram);

    EXPECT_EQ(gram->nonsupport_lprs(), 33);

    ASSERT_TRUE(gram->has_lpr("TRUE"_str));
    ASSERT_TRUE(gram->has_lpr("FALSE"_str));
    ASSERT_TRUE(gram->has_lpr("INF"_str));
    ASSERT_TRUE(gram->has_lpr("NAN"_str));
    ASSERT_TRUE(gram->has_lpr("VAR"_str));
    ASSERT_TRUE(gram->has_lpr("FN"_str));
    ASSERT_TRUE(gram->has_lpr("IF"_str));
    ASSERT_TRUE(gram->has_lpr("ELSE"_str));
    ASSERT_TRUE(gram->has_lpr("LOOP"_str));
    ASSERT_TRUE(gram->has_lpr("BREAK"_str));
    ASSERT_TRUE(gram->has_lpr("CONTINUE"_str));
    ASSERT_TRUE(gram->has_lpr("RETURN"_str));

    EXPECT_EQ(gram->lpr("TRUE"_str)->qualifier(), taul::qualifier::none);
    EXPECT_EQ(gram->lpr("FALSE"_str)->qualifier(), taul::qualifier::none);
    EXPECT_EQ(gram->lpr("INF"_str)->qualifier(), taul::qualifier::none);
    EXPECT_EQ(gram->lpr("NAN"_str)->qualifier(), taul::qualifier::none);
    EXPECT_EQ(gram->lpr("VAR"_str)->qualifier(), taul::qualifier::none);
    EXPECT_EQ(gram->lpr("FN"_str)->qualifier(), taul::qualifier::none);
    EXPECT_EQ(gram->lpr("IF"_str)->qualifier(), taul::qualifier::none);
    EXPECT_EQ(gram->lpr("ELSE"_str)->qualifier(), taul::qualifier::none);
    EXPECT_EQ(gram->lpr("LOOP"_str)->qualifier(), taul::qualifier::none);
    EXPECT_EQ(gram->lpr("BREAK"_str)->qualifier(), taul::qualifier::none);
    EXPECT_EQ(gram->lpr("CONTINUE"_str)->qualifier(), taul::qualifier::none);
    EXPECT_EQ(gram->lpr("RETURN"_str)->qualifier(), taul::qualifier::none);

    ASSERT_TRUE(gram->has_lpr("R_ARROW"_str));
    ASSERT_TRUE(gram->has_lpr("ASSIGN"_str));
    //ASSERT_TRUE(gram->has_lpr("MINUS"_str));
    ASSERT_TRUE(gram->has_lpr("L_ROUND"_str));
    ASSERT_TRUE(gram->has_lpr("R_ROUND"_str));
    ASSERT_TRUE(gram->has_lpr("L_CURLY"_str));
    ASSERT_TRUE(gram->has_lpr("R_CURLY"_str));
    ASSERT_TRUE(gram->has_lpr("COMMA"_str));
    ASSERT_TRUE(gram->has_lpr("COLON"_str));
    ASSERT_TRUE(gram->has_lpr("SEMI"_str));

    EXPECT_EQ(gram->lpr("R_ARROW"_str)->qualifier(), taul::qualifier::none);
    EXPECT_EQ(gram->lpr("ASSIGN"_str)->qualifier(), taul::qualifier::none);
    //EXPECT_EQ(gram->lpr("MINUS"_str)->qualifier(), taul::qualifier::none);
    EXPECT_EQ(gram->lpr("L_ROUND"_str)->qualifier(), taul::qualifier::none);
    EXPECT_EQ(gram->lpr("R_ROUND"_str)->qualifier(), taul::qualifier::none);
    EXPECT_EQ(gram->lpr("L_CURLY"_str)->qualifier(), taul::qualifier::none);
    EXPECT_EQ(gram->lpr("R_CURLY"_str)->qualifier(), taul::qualifier::none);
    EXPECT_EQ(gram->lpr("COMMA"_str)->qualifier(), taul::qualifier::none);
    EXPECT_EQ(gram->lpr("COLON"_str)->qualifier(), taul::qualifier::none);
    EXPECT_EQ(gram->lpr("SEMI"_str)->qualifier(), taul::qualifier::none);

    ASSERT_TRUE(gram->has_lpr("IDENTIFIER"_str));

    EXPECT_EQ(gram->lpr("IDENTIFIER"_str)->qualifier(), taul::qualifier::none);

    ASSERT_TRUE(gram->has_lpr("FLOAT"_str));
    ASSERT_TRUE(gram->has_lpr("INT_DEC"_str));
    ASSERT_TRUE(gram->has_lpr("INT_HEX"_str));
    ASSERT_TRUE(gram->has_lpr("INT_BIN"_str));
    ASSERT_TRUE(gram->has_lpr("UINT_DEC"_str));
    ASSERT_TRUE(gram->has_lpr("UINT_HEX"_str));
    ASSERT_TRUE(gram->has_lpr("UINT_BIN"_str));
    ASSERT_TRUE(gram->has_lpr("CHAR"_str));

    EXPECT_EQ(gram->lpr("FLOAT"_str)->qualifier(), taul::qualifier::none);
    EXPECT_EQ(gram->lpr("INT_DEC"_str)->qualifier(), taul::qualifier::none);
    EXPECT_EQ(gram->lpr("INT_HEX"_str)->qualifier(), taul::qualifier::none);
    EXPECT_EQ(gram->lpr("INT_BIN"_str)->qualifier(), taul::qualifier::none);
    EXPECT_EQ(gram->lpr("UINT_DEC"_str)->qualifier(), taul::qualifier::none);
    EXPECT_EQ(gram->lpr("UINT_HEX"_str)->qualifier(), taul::qualifier::none);
    EXPECT_EQ(gram->lpr("UINT_BIN"_str)->qualifier(), taul::qualifier::none);
    EXPECT_EQ(gram->lpr("CHAR"_str)->qualifier(), taul::qualifier::none);

    ASSERT_TRUE(gram->has_lpr("WHITESPACE"_str));
    ASSERT_TRUE(gram->has_lpr("NEWLINE"_str));
    ASSERT_TRUE(gram->has_lpr("SL_COMMENT"_str));

    EXPECT_EQ(gram->lpr("WHITESPACE"_str)->qualifier(), taul::qualifier::skip);
    EXPECT_EQ(gram->lpr("NEWLINE"_str)->qualifier(), taul::qualifier::skip);
    EXPECT_EQ(gram->lpr("SL_COMMENT"_str)->qualifier(), taul::qualifier::skip);
}

// TODO: replace w/ TAUL library utils for unit testing Yama spec LPRs

#define _SETUP_FOR_LPR(_NAME_) \
ASSERT_TRUE(gram);\
ASSERT_TRUE(gram->has_lpr(_NAME_));\
taul::source_reader reader(""_str);\
taul::lexer lexer(gram.value());\
lexer.bind_source(&reader);\
auto _succeed =\
[&](taul::str input, taul::source_len expected_len) {\
    auto expected = taul::token::normal(gram.value(), _NAME_, 0, expected_len);\
    reader.change_input(input);\
    lexer.reset();\
    auto actual = lexer.next();\
    YAMA_LOG(dbg, yama::general_c, "_succeed({}, {})", input, expected_len);\
    EXPECT_EQ(expected, actual);\
    };\
auto _fail =\
[&](taul::str input) {\
    reader.change_input(input);\
    lexer.reset();\
    auto actual = lexer.next();\
    YAMA_LOG(dbg, yama::general_c, "_fail({})", input);\
    EXPECT_NE(actual.id, gram->lpr(_NAME_)->id());\
    };\
((void)0)

// TODO: replace w/ TAUL library utils for unit testing Yama spec LPRs

#define _KW_TEST(_KW_, _LITERAL_, _LEN_) \
TEST_F(YamaSpecTests, _KW_) {\
    _SETUP_FOR_LPR(taul::str::lit(#_KW_));\
\
    constexpr taul::source_len len = _LEN_;\
\
    _succeed(_LITERAL_ ""_str, len);\
    _succeed(_LITERAL_ " "_str, len);\
    _succeed(_LITERAL_ "\r\n"_str, len);\
    _succeed(_LITERAL_ "//abc"_str, len);\
    _succeed(_LITERAL_ "?"_str, len);\
\
    _fail(_LITERAL_ "a"_str);\
    _fail(_LITERAL_ "0"_str);\
    _fail(_LITERAL_ "_"_str);\
    _fail(""_str);\
    _fail(" "_str);\
    _fail("abc"_str);\
    _fail("123"_str);\
    _fail("?*&"_str);\
}

// need this quick-n'-dirty *alt* version to account for NAN being taken by a preprocessor def

#define _KW_TEST_ALT(_KW_, _LITERAL_, _LEN_) \
TEST_F(YamaSpecTests, _KW_ ## 0) {\
    _SETUP_FOR_LPR(taul::str::lit(#_KW_));\
\
    constexpr taul::source_len len = _LEN_;\
\
    _succeed(_LITERAL_ ""_str, len);\
    _succeed(_LITERAL_ " "_str, len);\
    _succeed(_LITERAL_ "\r\n"_str, len);\
    _succeed(_LITERAL_ "//abc"_str, len);\
    _succeed(_LITERAL_ "?"_str, len);\
\
    _fail(_LITERAL_ "a"_str);\
    _fail(_LITERAL_ "0"_str);\
    _fail(_LITERAL_ "_"_str);\
    _fail(""_str);\
    _fail(" "_str);\
    _fail("abc"_str);\
    _fail("123"_str);\
    _fail("?*&"_str);\
}

_KW_TEST(TRUE, "true", 4);
_KW_TEST(FALSE, "false", 5);
_KW_TEST(INF, "inf", 3);
_KW_TEST_ALT(NAN, "nan", 3);
_KW_TEST(VAR, "var", 3);
_KW_TEST(FN, "fn", 2);
_KW_TEST(IF, "if", 2);
_KW_TEST(ELSE, "else", 4);
_KW_TEST(LOOP, "loop", 4);
_KW_TEST(BREAK, "break", 5);
_KW_TEST(CONTINUE, "continue", 8);
_KW_TEST(RETURN, "return", 6);

// TODO: replace w/ TAUL library utils for unit testing Yama spec LPRs

#define _OP_TEST(_OP_, _LITERAL_, _LEN_) \
TEST_F(YamaSpecTests, _OP_) {\
    _SETUP_FOR_LPR(taul::str::lit(#_OP_));\
\
    constexpr taul::source_len len = _LEN_;\
\
    _succeed(_LITERAL_ ""_str, len);\
    _succeed(_LITERAL_ " "_str, len);\
    _succeed(_LITERAL_ "\r\n"_str, len);\
    _succeed(_LITERAL_ "//abc"_str, len);\
    _succeed(_LITERAL_ "?"_str, len);\
    _succeed(_LITERAL_ "a"_str, len);\
    /*if (_LITERAL_ != "-"_str) _succeed(_LITERAL_ "1"_str, len);*/\
    _succeed(_LITERAL_ "1u"_str, len);\
    _succeed(_LITERAL_ "_"_str, len);\
\
    _fail(""_str);\
    _fail(" "_str);\
    _fail("a"_str);\
    _fail("1"_str);\
}

_OP_TEST(R_ARROW, "->"_str, 2);
_OP_TEST(ASSIGN, "="_str, 1);
//_OP_TEST(MINUS, "-"_str, 1);
_OP_TEST(L_ROUND, "("_str, 1);
_OP_TEST(R_ROUND, ")"_str, 1);
_OP_TEST(L_CURLY, "{"_str, 1);
_OP_TEST(R_CURLY, "}"_str, 1);
_OP_TEST(COMMA, ","_str, 1);
_OP_TEST(COLON, ":"_str, 1);
_OP_TEST(SEMI, ";"_str, 1);

TEST_F(YamaSpecTests, IDENTIFIER) {
    _SETUP_FOR_LPR("IDENTIFIER"_str);

    _succeed("a"_str, 1);
    _succeed("_"_str, 1);
    _succeed("abc"_str, 3);
    _succeed("defghi"_str, 6);
    _succeed("_abc"_str, 4);
    _succeed("a123"_str, 4);
    _succeed("_123"_str, 4);
    _succeed("abc\r\n"_str, 3);
    _succeed("abc#abc"_str, 3);
    _succeed("abc?"_str, 3);

    _fail(""_str);
    _fail(" "_str);
    _fail("123"_str);
    _fail("?*&"_str);
}

TEST_F(YamaSpecTests, FLOAT) {
    _SETUP_FOR_LPR("FLOAT"_str);

    _succeed("0.0"_str, 3);
    _succeed("-0.0"_str, 4);

    _succeed("1.0"_str, 3);
    _succeed("2.0"_str, 3);
    _succeed("3.0"_str, 3);
    _succeed("4.0"_str, 3);
    _succeed("5.0"_str, 3);
    _succeed("6.0"_str, 3);
    _succeed("7.0"_str, 3);
    _succeed("8.0"_str, 3);
    _succeed("9.0"_str, 3);
    _succeed("10.0"_str, 4);
    _succeed("1000.0"_str, 6);
    _succeed("1_0_0_0.0"_str, 9);
    _succeed("0001000.0"_str, 9);
    _succeed("0_0_0_1_0_0_0.0"_str, 15);
    
    _succeed("-1.0"_str, 4);
    _succeed("-2.0"_str, 4);
    _succeed("-3.0"_str, 4);
    _succeed("-4.0"_str, 4);
    _succeed("-5.0"_str, 4);
    _succeed("-6.0"_str, 4);
    _succeed("-7.0"_str, 4);
    _succeed("-8.0"_str, 4);
    _succeed("-9.0"_str, 4);
    _succeed("-10.0"_str, 5);
    _succeed("-1000.0"_str, 7);
    _succeed("-1_0_0_0.0"_str, 10);
    _succeed("-0001000.0"_str, 10);
    _succeed("-0_0_0_1_0_0_0.0"_str, 16);

    _succeed("0.1"_str, 3);
    _succeed("0.2"_str, 3);
    _succeed("0.3"_str, 3);
    _succeed("0.4"_str, 3);
    _succeed("0.5"_str, 3);
    _succeed("0.6"_str, 3);
    _succeed("0.7"_str, 3);
    _succeed("0.8"_str, 3);
    _succeed("0.9"_str, 3);
    _succeed("0.01"_str, 4);
    _succeed("0.0001"_str, 6);
    _succeed("0.0_0_0_1"_str, 9);
    _succeed("0000.0001"_str, 9);
    _succeed("0_0_0_0.0_0_0_1"_str, 15);
    
    _succeed("-0.1"_str, 4);
    _succeed("-0.2"_str, 4);
    _succeed("-0.3"_str, 4);
    _succeed("-0.4"_str, 4);
    _succeed("-0.5"_str, 4);
    _succeed("-0.6"_str, 4);
    _succeed("-0.7"_str, 4);
    _succeed("-0.8"_str, 4);
    _succeed("-0.9"_str, 4);
    _succeed("-0.01"_str, 5);
    _succeed("-0.0001"_str, 7);
    _succeed("-0.0_0_0_1"_str, 10);
    _succeed("-0000.0001"_str, 10);
    _succeed("-0_0_0_0.0_0_0_1"_str, 16);
    
    _succeed(".0"_str, 2);
    _succeed(".1"_str, 2);
    _succeed(".2"_str, 2);
    _succeed(".3"_str, 2);
    _succeed(".4"_str, 2);
    _succeed(".5"_str, 2);
    _succeed(".6"_str, 2);
    _succeed(".7"_str, 2);
    _succeed(".8"_str, 2);
    _succeed(".9"_str, 2);
    _succeed(".01"_str, 3);
    _succeed(".0001"_str, 5);
    _succeed(".0_0_0_1"_str, 8);
    
    _succeed("-.0"_str, 3);
    _succeed("-.1"_str, 3);
    _succeed("-.2"_str, 3);
    _succeed("-.3"_str, 3);
    _succeed("-.4"_str, 3);
    _succeed("-.5"_str, 3);
    _succeed("-.6"_str, 3);
    _succeed("-.7"_str, 3);
    _succeed("-.8"_str, 3);
    _succeed("-.9"_str, 3);
    _succeed("-.01"_str, 4);
    _succeed("-.0001"_str, 6);
    _succeed("-.0_0_0_1"_str, 9);
    
    _succeed("987654321.123456789"_str, 19);
    _succeed("123456789.987654321"_str, 19);
    
    _succeed("-987654321.123456789"_str, 20);
    _succeed("-123456789.987654321"_str, 20);

    _succeed("1.03e101"_str, 8);
    _succeed("2.0e-30"_str, 7);
    _succeed("2.0e-3_0"_str, 8);
    _succeed("2.0e+30"_str, 7);
    _succeed("2.0e+3_0"_str, 8);
    
    _succeed("-1.03e101"_str, 9);
    _succeed("-2.0e-30"_str, 8);
    _succeed("-2.0e-3_0"_str, 9);
    _succeed("-2.0e+30"_str, 8);
    _succeed("-2.0e+3_0"_str, 9);
    
    _succeed("1e101"_str, 5);
    _succeed("2e-30"_str, 5);
    _succeed("2e-3_0"_str, 6);
    _succeed("2e+30"_str, 5);
    _succeed("2e+3_0"_str, 6);
    
    _succeed("-1e101"_str, 6);
    _succeed("-2e-30"_str, 6);
    _succeed("-2e-3_0"_str, 7);
    _succeed("-2e+30"_str, 6);
    _succeed("-2e+3_0"_str, 7);
    
    _succeed(".1e101"_str, 6);
    _succeed(".2e-30"_str, 6);
    _succeed(".2e-3_0"_str, 7);
    _succeed(".2e+30"_str, 6);
    _succeed(".2e+3_0"_str, 7);
    
    _succeed("-.1e101"_str, 7);
    _succeed("-.2e-30"_str, 7);
    _succeed("-.2e-3_0"_str, 8);
    _succeed("-.2e+30"_str, 7);
    _succeed("-.2e+3_0"_str, 8);
    
    _fail(""_str);
    _fail(" "_str);
    _fail("-"_str);
    _fail("?*&"_str);
    _fail("true"_str);
    _fail("1002"_str); // allow for fmt_float/parse_float, but not in Yama lexing, due to ambiguity w/ integers
    _fail("0x1e2f"_str);
    _fail("0b1011"_str);

    _fail("0."_str);
    _fail("."_str);
    _fail("0.0__0"_str);
    _fail("0.0_"_str);
    _fail("0._0"_str);
    _fail("0_.0"_str);
    _fail("_0.0"_str);
    _fail("0.0a"_str);
    
    _fail("0.e"_str);
    _fail("0.0e"_str);
    _fail("e0"_str);
    _fail("0.0e0_"_str);
    _fail("0.0e_0"_str);
    _fail("0.0_e0"_str);
    _fail("0.0e0a"_str);

    _fail("_-0.0"_str);
    _fail("-_0.0"_str);
    _fail("-e0"_str);
    
    // inf and nan aren't covered by this LPR

    _fail("inf"_str);
    _fail("nan"_str);
}

TEST_F(YamaSpecTests, INT_DEC) {
    _SETUP_FOR_LPR("INT_DEC"_str);

    _succeed("0"_str, 1);
    _succeed("1"_str, 1);
    _succeed("2"_str, 1);
    _succeed("3"_str, 1);
    _succeed("4"_str, 1);
    _succeed("5"_str, 1);
    _succeed("6"_str, 1);
    _succeed("7"_str, 1);
    _succeed("8"_str, 1);
    _succeed("9"_str, 1);
    _succeed("10"_str, 2);
    _succeed("1000"_str, 4);
    _succeed("1928"_str, 4);
    _succeed("123456789"_str, 9);
    _succeed("000000789"_str, 9);
    _succeed("1_2_3_4_5_6_7_8_9"_str, 17);
    _succeed("0_0_0_0_0_0_7_8_9"_str, 17);
    
    _succeed("-0"_str, 2);
    _succeed("-1"_str, 2);
    _succeed("-2"_str, 2);
    _succeed("-3"_str, 2);
    _succeed("-4"_str, 2);
    _succeed("-5"_str, 2);
    _succeed("-6"_str, 2);
    _succeed("-7"_str, 2);
    _succeed("-8"_str, 2);
    _succeed("-9"_str, 2);
    _succeed("-10"_str, 3);
    _succeed("-1000"_str, 5);
    _succeed("-1928"_str, 5);
    _succeed("-123456789"_str, 10);
    _succeed("-000000789"_str, 10);
    _succeed("-1_2_3_4_5_6_7_8_9"_str, 18);
    _succeed("-0_0_0_0_0_0_7_8_9"_str, 18);

    _fail(""_str);
    _fail(" "_str);
    _fail("-"_str);
    _fail("?*&"_str);
    _fail("true"_str);
    _fail("100u"_str); // legal uint, but not int
    _fail("0x1e2f"_str); // legal int, but not base-10
    _fail("0b1011"_str); // legal int, but not base-10
    _fail("10a"_str); // legal int, except for the 'a' at the end
    _fail("1__0"_str);
    _fail("10__"_str);
    _fail("__10"_str);
    _fail("_-10"_str);
    _fail("-_10"_str);
    _fail("-10u"_str); // legal int, except for the 'u' at the end (this prevents uint misinterpret)
}

TEST_F(YamaSpecTests, INT_HEX) {
    _SETUP_FOR_LPR("INT_HEX"_str);

    _succeed("0x0"_str, 3);
    _succeed("0x1"_str, 3);
    _succeed("0x2"_str, 3);
    _succeed("0x3"_str, 3);
    _succeed("0x4"_str, 3);
    _succeed("0x5"_str, 3);
    _succeed("0x6"_str, 3);
    _succeed("0x7"_str, 3);
    _succeed("0x8"_str, 3);
    _succeed("0x9"_str, 3);
    _succeed("0xa"_str, 3);
    _succeed("0xb"_str, 3);
    _succeed("0xc"_str, 3);
    _succeed("0xd"_str, 3);
    _succeed("0xe"_str, 3);
    _succeed("0xf"_str, 3);
    _succeed("0xA"_str, 3);
    _succeed("0xB"_str, 3);
    _succeed("0xC"_str, 3);
    _succeed("0xD"_str, 3);
    _succeed("0xE"_str, 3);
    _succeed("0xF"_str, 3);
    _succeed("0x10"_str, 4);
    _succeed("0x1000"_str, 6);
    _succeed("0xf1E9"_str, 6);
    _succeed("0x000000789abcdefABCDEF"_str, 23);
    _succeed("0x123456789abcdefABCDEF"_str, 23);
    _succeed("0x1_2_3_4_5_6_7_8_9_a_b_c_d_e_f_A_B_C_D_E_F"_str, 43);
    _succeed("0x0_0_0_0_0_0_7_8_9_a_b_c_d_e_f_A_B_C_D_E_F"_str, 43);
    
    _succeed("-0x0"_str, 4);
    _succeed("-0x1"_str, 4);
    _succeed("-0x2"_str, 4);
    _succeed("-0x3"_str, 4);
    _succeed("-0x4"_str, 4);
    _succeed("-0x5"_str, 4);
    _succeed("-0x6"_str, 4);
    _succeed("-0x7"_str, 4);
    _succeed("-0x8"_str, 4);
    _succeed("-0x9"_str, 4);
    _succeed("-0xa"_str, 4);
    _succeed("-0xb"_str, 4);
    _succeed("-0xc"_str, 4);
    _succeed("-0xd"_str, 4);
    _succeed("-0xe"_str, 4);
    _succeed("-0xf"_str, 4);
    _succeed("-0xA"_str, 4);
    _succeed("-0xB"_str, 4);
    _succeed("-0xC"_str, 4);
    _succeed("-0xD"_str, 4);
    _succeed("-0xE"_str, 4);
    _succeed("-0xF"_str, 4);
    _succeed("-0x10"_str, 5);
    _succeed("-0x1000"_str, 7);
    _succeed("-0xf1E9"_str, 7);
    _succeed("-0x000000789abcdefABCDEF"_str, 24);
    _succeed("-0x123456789abcdefABCDEF"_str, 24);
    _succeed("-0x1_2_3_4_5_6_7_8_9_a_b_c_d_e_f_A_B_C_D_E_F"_str, 44);
    _succeed("-0x0_0_0_0_0_0_7_8_9_a_b_c_d_e_f_A_B_C_D_E_F"_str, 44);
    
    _fail(""_str);
    _fail(" "_str);
    _fail("-"_str);
    _fail("?*&"_str);
    _fail("true"_str);
    _fail("0x10u"_str); // legal uint, but not int
    _fail("1002"_str); // legal int, but not hex
    _fail("0b1011"_str); // legal int, but not hex
    _fail("0x10g"_str); // legal int, except for the 'g' at the end
    _fail("0x1__0"_str);
    _fail("0x10__"_str);
    _fail("__0x10"_str);
    _fail("0_x10"_str);
    _fail("0x_10"_str);
    _fail("0x"_str);
    _fail("_-0x10"_str);
    _fail("-_0x10"_str);
    _fail("-0x10u"_str); // legal int, except for the 'u' at the end (this prevents uint misinterpret)
}

TEST_F(YamaSpecTests, INT_BIN) {
    _SETUP_FOR_LPR("INT_BIN"_str);

    _succeed("0b0"_str, 3);
    _succeed("0b1"_str, 3);
    _succeed("0b10"_str, 4);
    _succeed("0b1000"_str, 6);
    _succeed("0b1011"_str, 6);
    _succeed("0b101010101"_str, 11);
    _succeed("0b000000101"_str, 11);
    _succeed("0b1_0_1_0_1_0_1_0_1"_str, 19);
    _succeed("0b0_0_0_0_0_0_1_0_1"_str, 19);
    
    _succeed("-0b0"_str, 4);
    _succeed("-0b1"_str, 4);
    _succeed("-0b10"_str, 5);
    _succeed("-0b1000"_str, 7);
    _succeed("-0b1011"_str, 7);
    _succeed("-0b101010101"_str, 12);
    _succeed("-0b000000101"_str, 12);
    _succeed("-0b1_0_1_0_1_0_1_0_1"_str, 20);
    _succeed("-0b0_0_0_0_0_0_1_0_1"_str, 20);

    _fail(""_str);
    _fail(" "_str);
    _fail("-"_str);
    _fail("?*&"_str);
    _fail("true"_str);
    _fail("0b10u"_str); // legal uint, but not int
    _fail("1002"_str); // legal int, but not binary
    _fail("0x1e2f"_str); // legal int, but not binary
    _fail("0b10a"_str); // legal int, except for the 'a' at the end
    _fail("0b1__0"_str);
    _fail("0b10__"_str);
    _fail("__0b10"_str);
    _fail("0_b10"_str);
    _fail("0b_10"_str);
    _fail("0b"_str);
    _fail("_-0b10"_str);
    _fail("-_0b10"_str);
    _fail("-0b10u"_str); // legal int, except for the 'u' at the end (this prevents uint misinterpret)
}

TEST_F(YamaSpecTests, UINT_DEC) {
    _SETUP_FOR_LPR("UINT_DEC"_str);

    _succeed("0u"_str, 2);
    _succeed("1u"_str, 2);
    _succeed("2u"_str, 2);
    _succeed("3u"_str, 2);
    _succeed("4u"_str, 2);
    _succeed("5u"_str, 2);
    _succeed("6u"_str, 2);
    _succeed("7u"_str, 2);
    _succeed("8u"_str, 2);
    _succeed("9u"_str, 2);
    _succeed("10u"_str, 3);
    _succeed("1000u"_str, 5);
    _succeed("1928u"_str, 5);
    _succeed("123456789u"_str, 10);
    _succeed("000000789u"_str, 10);
    _succeed("1_2_3_4_5_6_7_8_9u"_str, 18);
    _succeed("0_0_0_0_0_0_7_8_9u"_str, 18);

    _fail(""_str);
    _fail(" "_str);
    _fail("?*&"_str);
    _fail("true"_str);
    _fail("100"_str); // legal int, but not uint
    _fail("0x1e2fu"_str); // legal uint, but not base-10
    _fail("0b1011u"_str); // legal uint, but not base-10
    _fail("10ua"_str); // legal uint, except for the 'a' at the end
    _fail("1__0u"_str);
    _fail("10__u"_str);
    _fail("10u__"_str);
    _fail("__10u"_str);
}

TEST_F(YamaSpecTests, UINT_HEX) {
    _SETUP_FOR_LPR("UINT_HEX"_str);

    _succeed("0x0u"_str, 4);
    _succeed("0x1u"_str, 4);
    _succeed("0x2u"_str, 4);
    _succeed("0x3u"_str, 4);
    _succeed("0x4u"_str, 4);
    _succeed("0x5u"_str, 4);
    _succeed("0x6u"_str, 4);
    _succeed("0x7u"_str, 4);
    _succeed("0x8u"_str, 4);
    _succeed("0x9u"_str, 4);
    _succeed("0xau"_str, 4);
    _succeed("0xbu"_str, 4);
    _succeed("0xcu"_str, 4);
    _succeed("0xdu"_str, 4);
    _succeed("0xeu"_str, 4);
    _succeed("0xfu"_str, 4);
    _succeed("0xAu"_str, 4);
    _succeed("0xBu"_str, 4);
    _succeed("0xCu"_str, 4);
    _succeed("0xDu"_str, 4);
    _succeed("0xEu"_str, 4);
    _succeed("0xFu"_str, 4);
    _succeed("0x10u"_str, 5);
    _succeed("0x1000u"_str, 7);
    _succeed("0xf1E9u"_str, 7);
    _succeed("0x000000789abcdefABCDEFu"_str, 24);
    _succeed("0x123456789abcdefABCDEFu"_str, 24);
    _succeed("0x1_2_3_4_5_6_7_8_9_a_b_c_d_e_f_A_B_C_D_E_Fu"_str, 44);
    _succeed("0x0_0_0_0_0_0_7_8_9_a_b_c_d_e_f_A_B_C_D_E_Fu"_str, 44);

    _fail(""_str);
    _fail(" "_str);
    _fail("?*&"_str);
    _fail("true"_str);
    _fail("0x10"_str); // legal int, but not uint
    _fail("1002u"_str); // legal uint, but not hex
    _fail("0b1011u"_str); // legal uint, but not hex
    _fail("0x10ug"_str); // legal uint, except for the 'g' at the end
    _fail("0x1__0u"_str);
    _fail("0x10__u"_str);
    _fail("0x10u__"_str);
    _fail("__0x10u"_str);
    _fail("0_x10u"_str);
    _fail("0x_10u"_str);
    _fail("0xu"_str);
}

TEST_F(YamaSpecTests, UINT_BIN) {
    _SETUP_FOR_LPR("UINT_BIN"_str);

    _succeed("0b0u"_str, 4);
    _succeed("0b1u"_str, 4);
    _succeed("0b10u"_str, 5);
    _succeed("0b1000u"_str, 7);
    _succeed("0b1011u"_str, 7);
    _succeed("0b101010101u"_str, 12);
    _succeed("0b000000101u"_str, 12);
    _succeed("0b1_0_1_0_1_0_1_0_1u"_str, 20);
    _succeed("0b0_0_0_0_0_0_1_0_1u"_str, 20);

    _fail(""_str);
    _fail(" "_str);
    _fail("?*&"_str);
    _fail("true"_str);
    _fail("0b10"_str); // legal int, but not uint
    _fail("1002u"_str); // legal uint, but not binary
    _fail("0x1e2fu"_str); // legal uint, but not binary
    _fail("0b10ua"_str); // legal uint, except for the 'a' at the end
    _fail("0b1__0u"_str);
    _fail("0b10__u"_str);
    _fail("0b10u__"_str);
    _fail("__0b10u"_str);
    _fail("0_b10u"_str);
    _fail("0b_10u"_str);
    _fail("0bu"_str);
}

TEST_F(YamaSpecTests, CHAR) {
    _SETUP_FOR_LPR("CHAR"_str);

    for (yama::char_t i = 0x00; i < 0x80; i++) {
        if (char(i) == '\'') continue;
        if (char(i) == '\\') continue;
        _succeed(yama::str(std::string("'") + char(i) + "'abc"), 3); // ignores 'abc' suffix
    }

    _succeed(yama::str(taul::utf8_s(u8"'魂'abc")), 5); // ignores 'abc' suffix

    _succeed("''"_str, 2); // legal here, but semantic error
    _succeed("'abc'"_str, 5); // legal here, but semantic error
    _succeed("'123'"_str, 5); // legal here, but semantic error
    _succeed("'!@#'"_str, 5); // legal here, but semantic error

    _succeed("'\"'"_str, 3); // double quotes may be used unqualified in CHAR

    // escape sequences

    _succeed("'\\0'"_str, 4);
    _succeed("'\\a'"_str, 4);
    _succeed("'\\b'"_str, 4);
    _succeed("'\\f'"_str, 4);
    _succeed("'\\n'"_str, 4);
    _succeed("'\\r'"_str, 4);
    _succeed("'\\t'"_str, 4);
    _succeed("'\\v'"_str, 4);
    _succeed("'\\''"_str, 4);
    _succeed("'\\\"'"_str, 4);
    _succeed("'\\\\'"_str, 4);
    
    _succeed("'\\x00'"_str, 6);
    _succeed("'\\x08'"_str, 6);
    _succeed("'\\xff'"_str, 6);
    _succeed("'\\xFa'"_str, 6);
    _succeed("'\\xe4'"_str, 6);
    _succeed("'\\xBD'"_str, 6);
    
    _succeed("'\\u5b00'"_str, 8);
    _succeed("'\\u5b08'"_str, 8);
    _succeed("'\\u5bff'"_str, 8);
    _succeed("'\\u5bFa'"_str, 8);
    _succeed("'\\u5be4'"_str, 8);
    _succeed("'\\u5bBD'"_str, 8);
    
    _succeed("'\\U5b00Ee16'"_str, 12);
    _succeed("'\\U5b08Ee16'"_str, 12);
    _succeed("'\\U5bffEe16'"_str, 12);
    _succeed("'\\U5bFaEe16'"_str, 12);
    _succeed("'\\U5be4Ee16'"_str, 12);
    _succeed("'\\U5bBDEe16'"_str, 12);

    _succeed("'\\p'"_str, 4); // escaping random char
    _succeed("'\\x0p'"_str, 6); // escaping random char (breaking \x~ escape seq)
    _succeed("'\\u000p'"_str, 8); // escaping random char (breaking \u~ escape seq)
    _succeed("'\\U0000000p'"_str, 12); // escaping random char (breaking \U~ escape seq)

    _fail(""_str);
    _fail(" "_str);
    _fail("abc"_str);
    _fail("123"_str);
    _fail("?*&"_str);

    _fail("'\\'"_str);
    _fail("'"_str);
    _fail("'abc"_str);
    _fail("abc'"_str);
}

TEST_F(YamaSpecTests, WHITESPACE) {
    _SETUP_FOR_LPR("WHITESPACE"_str);
    lexer.cut_skip_tokens = false;

    _succeed(" "_str, 1);
    _succeed(" abc"_str, 1);
    _succeed(" 123"_str, 1);
    _succeed(" _"_str, 1);
    _succeed(" \r\n"_str, 1);
    _succeed(" #abc"_str, 1);
    _succeed(" ?"_str, 1);
    _succeed("\t"_str, 1);
    _succeed("\tabc"_str, 1);
    _succeed("\t123"_str, 1);
    _succeed("\t_"_str, 1);
    _succeed("\t\r\n"_str, 1);
    _succeed("\t#abc"_str, 1);
    _succeed("\t?"_str, 1);
    _succeed("  \t \t\t "_str, 7);
    _succeed("  \t \t\t abc"_str, 7);
    _succeed("  \t \t\t 123"_str, 7);
    _succeed("  \t \t\t _"_str, 7);
    _succeed("  \t \t\t \r\n"_str, 7);
    _succeed("  \t \t\t #abc"_str, 7);
    _succeed("  \t \t\t ?"_str, 7);

    _fail(""_str);
    _fail("\r\n"_str);
    _fail("abc"_str);
    _fail("123"_str);
    _fail("?*&"_str);
}

TEST_F(YamaSpecTests, NEWLINE) {
    _SETUP_FOR_LPR("NEWLINE"_str);
    lexer.cut_skip_tokens = false;

    _succeed("\r"_str, 1);
    _succeed("\rabc"_str, 1);
    _succeed("\r123"_str, 1);
    _succeed("\r_"_str, 1);
    _succeed("\r\r\n"_str, 1);
    _succeed("\r#abc"_str, 1);
    _succeed("\r?"_str, 1);
    _succeed("\n"_str, 1);
    _succeed("\nabc"_str, 1);
    _succeed("\n123"_str, 1);
    _succeed("\n_"_str, 1);
    _succeed("\n\r\n"_str, 1);
    _succeed("\n#abc"_str, 1);
    _succeed("\n?"_str, 1);
    _succeed("\r\n"_str, 2);
    _succeed("\r\nabc"_str, 2);
    _succeed("\r\n123"_str, 2);
    _succeed("\r\n_"_str, 2);
    _succeed("\r\n\r\n"_str, 2);
    _succeed("\r\n#abc"_str, 2);
    _succeed("\r\n?"_str, 2);

    _fail(""_str);
    _fail(" "_str);
    _fail("\f"_str);
    _fail("abc"_str);
    _fail("123"_str);
    _fail("?*&"_str);
}

TEST_F(YamaSpecTests, SL_COMMENT) {
    _SETUP_FOR_LPR("SL_COMMENT"_str);
    lexer.cut_skip_tokens = false;

    _succeed("//"_str, 2);
    _succeed("// abc"_str, 6);
    _succeed("// abcabc"_str, 9);
    _succeed("// abc123"_str, 9);
    _succeed("// abc_"_str, 7);
    _succeed("// abc#abc"_str, 10);
    _succeed("// abc?"_str, 7);
    _succeed("// abc\r"_str, 6); // stop just before newline
    _succeed("// abc\n"_str, 6); // stop just before newline
    _succeed("// abc\r\n"_str, 6); // stop just before newline

    _fail(""_str);
    _fail(" "_str);
    _fail("\r\n"_str);
    _fail("abc"_str);
    _fail("123"_str);
    _fail("?*&"_str);
    _fail("/"_str);
    _fail("/*"_str);
    _fail("\\"_str);
}


// defining these two quick-n'-dirty helpers to simplify creation of parse trees for testing

// these should be replaced later by unit testing utils provided by TAUL v1.0, whatever we
// end up deciding those should be

class syntactic_node_autocloser final {
public:

    syntactic_node_autocloser(taul::parse_tree& pt) 
        : _pt(&pt) {}

    ~syntactic_node_autocloser() noexcept {
        yama::deref_assert(_pt).close();
    }


private:

    taul::parse_tree* _pt;
};

class parse_tree_builder final {
public:

    parse_tree_builder(taul::grammar gram) 
        : _gram(gram), 
        _pt(gram),
        _pos(0) {}


    void skip(taul::source_len len) {
        _pos += len;
    }

    // NOTE: I worry that doing 'skip_text("\n")' will be incorrect on Windows,
    //       due to the whole CRLF thing

    void skip_newline() {
        skip(2); // skip CRLF <- TODO: won't this break on linux/macOS?
    }

    void skip_text(yama::str txt) {
        skip(taul::source_len(txt.length()));
    }
    
    void lexical(const yama::str& lpr_name, taul::source_len len) {
        _pt.lexical(lpr_name, _pos, len);
        skip(len);
    }

    void failure(taul::source_len len) {
        _pt.failure(_pos, len);
        skip(len);
    }

    void end() {
        _pt.end(_pos);
    }

    // the idea is use syntactic by entering a '{...}' block scope, acquiring a 
    // an RAII object from syntactic, storing it in a local variable, then using
    // this API to fill out said nodes contents, then closing the syntactic node
    // by exiting the '{...}' block scope, trigger RAII to close it

    syntactic_node_autocloser syntactic(const yama::str& ppr_name) {
        _pt.syntactic(ppr_name, _pos);
        return syntactic_node_autocloser(_pt);
    }

    // done is used to acquire the final parse_tree built

    // do not use the parse_tree_builder after calling done

    taul::parse_tree done() noexcept {
        return std::move(_pt);
    }


private:

    taul::grammar _gram;
    taul::parse_tree _pt;
    taul::source_pos _pos;
};


static std::shared_ptr<taul::parser> prep_for_ppr_test(
    std::shared_ptr<taul::logger> lgr,
    taul::grammar gram,
    const taul::source_code& src) {
    auto reader = std::make_shared<taul::source_reader>(src);
    auto lexer = std::make_shared<taul::lexer>(gram);
    auto parser = std::make_shared<taul::parser>(gram, lgr);
    lexer->bind_source(reader);
    parser->bind_source(lexer);
    parser->reset(); // flush pipeline
    return parser;
}


static bool pprs_ready = false;

TEST_F(YamaSpecTests, PPRs) {
    ASSERT_TRUE(gram);

    EXPECT_EQ(gram->pprs(), 27);

    EXPECT_TRUE(gram->has_ppr("Chunk"_str));

    EXPECT_TRUE(gram->has_ppr("Decl"_str));

    EXPECT_TRUE(gram->has_ppr("VarDecl"_str));
    EXPECT_TRUE(gram->has_ppr("FnDecl"_str));

    ASSERT_TRUE(gram->has_ppr("CallSig"_str));
    ASSERT_TRUE(gram->has_ppr("ParamDecl"_str));
    ASSERT_TRUE(gram->has_ppr("Result"_str));

    ASSERT_TRUE(gram->has_ppr("Block"_str));

    ASSERT_TRUE(gram->has_ppr("Stmt"_str));

    ASSERT_TRUE(gram->has_ppr("ExprStmt"_str));
    ASSERT_TRUE(gram->has_ppr("IfStmt"_str));
    ASSERT_TRUE(gram->has_ppr("LoopStmt"_str));
    ASSERT_TRUE(gram->has_ppr("BreakStmt"_str));
    ASSERT_TRUE(gram->has_ppr("ContinueStmt"_str));
    ASSERT_TRUE(gram->has_ppr("ReturnStmt"_str));

    ASSERT_TRUE(gram->has_ppr("Expr"_str));

    ASSERT_TRUE(gram->has_ppr("PrimaryExpr"_str));
    
    ASSERT_TRUE(gram->has_ppr("Lit"_str));

    ASSERT_TRUE(gram->has_ppr("IntLit"_str));
    ASSERT_TRUE(gram->has_ppr("UIntLit"_str));
    ASSERT_TRUE(gram->has_ppr("FloatLit"_str));
    ASSERT_TRUE(gram->has_ppr("BoolLit"_str));
    ASSERT_TRUE(gram->has_ppr("CharLit"_str));
    
    ASSERT_TRUE(gram->has_ppr("Assign"_str));
    ASSERT_TRUE(gram->has_ppr("Args"_str));

    ASSERT_TRUE(gram->has_ppr("TypeAnnot"_str));
    ASSERT_TRUE(gram->has_ppr("TypeSpec"_str));

    pprs_ready = true;
}


TEST_F(YamaSpecTests, Chunk_Empty) {
    ASSERT_TRUE(pprs_ready);

    taul::source_code input{};
    ASSERT_TRUE(input.add_file(std::filesystem::current_path() / "support-files/chunk-empty.txt"));
    
    auto parser = prep_for_ppr_test(lgr, *gram, input);
    ASSERT_TRUE(parser);

    // IMPORTANT: remember that TAUL starts building syntax tree at the first NON-CUT TOKEN, meaning
    //            that here, basically everything except the end-of-input has been cut, so we should
    //            expect syntax tree to start at the vary end of the input

    parse_tree_builder b(*gram);
    b.skip_newline();
    b.skip_newline();
    b.skip_text("// empty"_str);
    b.skip_newline();
    b.skip_newline();
    {
        auto node0 = b.syntactic("Chunk"_str);
    }

    auto expected = b.done();
    auto actual = parser->parse("Chunk"_str);

    YAMA_LOG(dbg, yama::general_c, "expected:\n{}", expected);
    YAMA_LOG(dbg, yama::general_c, "actual:\n{}", actual);

    EXPECT_EQ(expected.fmt(), actual.fmt()); // easier to diagnose problems by comparing strings
}

TEST_F(YamaSpecTests, Chunk_NonEmpty) {
    ASSERT_TRUE(pprs_ready);

    taul::source_code input{};
    ASSERT_TRUE(input.add_file(std::filesystem::current_path() / "support-files/chunk-nonempty.txt"));
    
    auto parser = prep_for_ppr_test(lgr, *gram, input);
    ASSERT_TRUE(parser);

    parse_tree_builder b(*gram);
    b.skip_newline();
    b.skip_newline();
    {
        auto node0 = b.syntactic("Chunk"_str);
        {
            auto node1 = b.syntactic("Decl"_str);
            {
                auto node2 = b.syntactic("VarDecl"_str);
                b.lexical("VAR"_str, 3);
                b.skip_text(" "_str);
                b.lexical("IDENTIFIER"_str, 1);
                b.lexical("SEMI"_str, 1);
            }
        }
        b.skip_newline();
        b.skip_newline();
        {
            auto node1 = b.syntactic("Decl"_str);
            {
                auto node2 = b.syntactic("FnDecl"_str);
                b.lexical("FN"_str, 2);
                b.skip_text(" "_str);
                b.lexical("IDENTIFIER"_str, 3);
                {
                    auto node3 = b.syntactic("CallSig"_str);
                    b.lexical("L_ROUND"_str, 1);
                    b.lexical("R_ROUND"_str, 1);
                }
                b.skip_text(" "_str);
                {
                    auto node3 = b.syntactic("Block"_str);
                    b.lexical("L_CURLY"_str, 1);
                    b.lexical("R_CURLY"_str, 1);
                }
            }
        }
        b.skip_newline();
        b.skip_newline();
    }

    auto expected = b.done();
    auto actual = parser->parse("Chunk"_str);

    YAMA_LOG(dbg, yama::general_c, "expected:\n{}", expected);
    YAMA_LOG(dbg, yama::general_c, "actual:\n{}", actual);

    EXPECT_EQ(expected.fmt(), actual.fmt()); // easier to diagnose problems by comparing strings
}

TEST_F(YamaSpecTests, Decl_VarDecl) {
    ASSERT_TRUE(pprs_ready);

    taul::source_code input{};
    input.add_str(""_str, "var a;"_str);
    
    auto parser = prep_for_ppr_test(lgr, *gram, input);
    ASSERT_TRUE(parser);

    parse_tree_builder b(*gram);
    {
        auto node1 = b.syntactic("Decl"_str);
        {
            auto node2 = b.syntactic("VarDecl"_str);
            b.lexical("VAR"_str, 3);
            b.skip_text(" "_str);
            b.lexical("IDENTIFIER"_str, 1);
            b.lexical("SEMI"_str, 1);
        }
    }

    auto expected = b.done();
    auto actual = parser->parse("Decl"_str);

    YAMA_LOG(dbg, yama::general_c, "expected:\n{}", expected);
    YAMA_LOG(dbg, yama::general_c, "actual:\n{}", actual);

    EXPECT_EQ(expected.fmt(), actual.fmt()); // easier to diagnose problems by comparing strings
}

TEST_F(YamaSpecTests, Decl_FnDecl) {
    ASSERT_TRUE(pprs_ready);

    taul::source_code input{};
    input.add_str(""_str, "fn abc() {}"_str);

    auto parser = prep_for_ppr_test(lgr, *gram, input);
    ASSERT_TRUE(parser);

    parse_tree_builder b(*gram);
    {
        auto node1 = b.syntactic("Decl"_str);
        {
            auto node2 = b.syntactic("FnDecl"_str);
            b.lexical("FN"_str, 2);
            b.skip_text(" "_str);
            b.lexical("IDENTIFIER"_str, 3);
            {
                auto node3 = b.syntactic("CallSig"_str);
                b.lexical("L_ROUND"_str, 1);
                b.lexical("R_ROUND"_str, 1);
            }
            b.skip_text(" "_str);
            {
                auto node3 = b.syntactic("Block"_str);
                b.lexical("L_CURLY"_str, 1);
                b.lexical("R_CURLY"_str, 1);
            }
        }
    }

    auto expected = b.done();
    auto actual = parser->parse("Decl"_str);

    YAMA_LOG(dbg, yama::general_c, "expected:\n{}", expected);
    YAMA_LOG(dbg, yama::general_c, "actual:\n{}", actual);

    EXPECT_EQ(expected.fmt(), actual.fmt()); // easier to diagnose problems by comparing strings
}

TEST_F(YamaSpecTests, VarDecl_NoTypeAnnot_NoAssign) {
    ASSERT_TRUE(pprs_ready);

    taul::source_code input{};
    input.add_str(""_str, "var a;"_str);
    
    auto parser = prep_for_ppr_test(lgr, *gram, input);
    ASSERT_TRUE(parser);

    parse_tree_builder b(*gram);
    {
        auto node1 = b.syntactic("VarDecl"_str);
        b.lexical("VAR"_str, 3);
        b.skip_text(" "_str);
        b.lexical("IDENTIFIER"_str, 1);
        b.lexical("SEMI"_str, 1);
    }

    auto expected = b.done();
    auto actual = parser->parse("VarDecl"_str);

    YAMA_LOG(dbg, yama::general_c, "expected:\n{}", expected);
    YAMA_LOG(dbg, yama::general_c, "actual:\n{}", actual);

    EXPECT_EQ(expected.fmt(), actual.fmt()); // easier to diagnose problems by comparing strings
}

TEST_F(YamaSpecTests, VarDecl_TypeAnnot_NoAssign) {
    ASSERT_TRUE(pprs_ready);

    taul::source_code input{};
    input.add_str(""_str, "var a: Int;"_str);
    
    auto parser = prep_for_ppr_test(lgr, *gram, input);
    ASSERT_TRUE(parser);

    parse_tree_builder b(*gram);
    {
        auto node1 = b.syntactic("VarDecl"_str);
        b.lexical("VAR"_str, 3);
        b.skip_text(" "_str);
        b.lexical("IDENTIFIER"_str, 1);
        {
            auto node2 = b.syntactic("TypeAnnot"_str);
            b.lexical("COLON"_str, 1);
            b.skip_text(" "_str);
            {
                auto node3 = b.syntactic("TypeSpec"_str);
                b.lexical("IDENTIFIER"_str, 3);
            }
        }
        b.lexical("SEMI"_str, 1);
    }

    auto expected = b.done();
    auto actual = parser->parse("VarDecl"_str);

    YAMA_LOG(dbg, yama::general_c, "expected:\n{}", expected);
    YAMA_LOG(dbg, yama::general_c, "actual:\n{}", actual);

    EXPECT_EQ(expected.fmt(), actual.fmt()); // easier to diagnose problems by comparing strings
}

TEST_F(YamaSpecTests, VarDecl_NoTypeAnnot_Assign) {
    ASSERT_TRUE(pprs_ready);

    taul::source_code input{};
    input.add_str(""_str, "var a = 300;"_str);

    auto parser = prep_for_ppr_test(lgr, *gram, input);
    ASSERT_TRUE(parser);

    parse_tree_builder b(*gram);
    {
        auto node1 = b.syntactic("VarDecl"_str);
        b.lexical("VAR"_str, 3);
        b.skip_text(" "_str);
        b.lexical("IDENTIFIER"_str, 1);
        b.skip_text(" "_str);
        {
            auto node2 = b.syntactic("Assign"_str);
            b.lexical("ASSIGN"_str, 1);
            b.skip_text(" "_str);
            {
                auto node3 = b.syntactic("Expr"_str);
                {
                    auto node4 = b.syntactic("PrimaryExpr"_str);
                    {
                        auto node5 = b.syntactic("Lit"_str);
                        {
                            auto node6 = b.syntactic("IntLit"_str);
                            b.lexical("INT_DEC"_str, 3);
                        }
                    }
                }
            }
        }
        b.lexical("SEMI"_str, 1);
    }

    auto expected = b.done();
    auto actual = parser->parse("VarDecl"_str);

    YAMA_LOG(dbg, yama::general_c, "expected:\n{}", expected);
    YAMA_LOG(dbg, yama::general_c, "actual:\n{}", actual);

    EXPECT_EQ(expected.fmt(), actual.fmt()); // easier to diagnose problems by comparing strings
}

TEST_F(YamaSpecTests, VarDecl_TypeAnnot_Assign) {
    ASSERT_TRUE(pprs_ready);

    taul::source_code input{};
    input.add_str(""_str, "var a: Int = 300;"_str);

    auto parser = prep_for_ppr_test(lgr, *gram, input);
    ASSERT_TRUE(parser);

    parse_tree_builder b(*gram);
    {
        auto node1 = b.syntactic("VarDecl"_str);
        b.lexical("VAR"_str, 3);
        b.skip_text(" "_str);
        b.lexical("IDENTIFIER"_str, 1);
        {
            auto node2 = b.syntactic("TypeAnnot"_str);
            b.lexical("COLON"_str, 1);
            b.skip_text(" "_str);
            {
                auto node3 = b.syntactic("TypeSpec"_str);
                b.lexical("IDENTIFIER"_str, 3);
            }
        }
        b.skip_text(" "_str);
        {
            auto node2 = b.syntactic("Assign"_str);
            b.lexical("ASSIGN"_str, 1);
            b.skip_text(" "_str);
            {
                auto node3 = b.syntactic("Expr"_str);
                {
                    auto node4 = b.syntactic("PrimaryExpr"_str);
                    {
                        auto node5 = b.syntactic("Lit"_str);
                        {
                            auto node6 = b.syntactic("IntLit"_str);
                            b.lexical("INT_DEC"_str, 3);
                        }
                    }
                }
            }
        }
        b.lexical("SEMI"_str, 1);
    }

    auto expected = b.done();
    auto actual = parser->parse("VarDecl"_str);

    YAMA_LOG(dbg, yama::general_c, "expected:\n{}", expected);
    YAMA_LOG(dbg, yama::general_c, "actual:\n{}", actual);

    EXPECT_EQ(expected.fmt(), actual.fmt()); // easier to diagnose problems by comparing strings
}

TEST_F(YamaSpecTests, FnDecl) {
    ASSERT_TRUE(pprs_ready);

    taul::source_code input{};
    input.add_str(""_str, "fn abc() {}"_str);
    
    auto parser = prep_for_ppr_test(lgr, *gram, input);
    ASSERT_TRUE(parser);

    parse_tree_builder b(*gram);
    {
        auto node1 = b.syntactic("FnDecl"_str);
        b.lexical("FN"_str, 2);
        b.skip_text(" "_str);
        b.lexical("IDENTIFIER"_str, 3);
        {
            auto node2 = b.syntactic("CallSig"_str);
            b.lexical("L_ROUND"_str, 1);
            b.lexical("R_ROUND"_str, 1);
        }
        b.skip_text(" "_str);
        {
            auto node2 = b.syntactic("Block"_str);
            b.lexical("L_CURLY"_str, 1);
            b.lexical("R_CURLY"_str, 1);
        }
    }

    auto expected = b.done();
    auto actual = parser->parse("FnDecl"_str);

    YAMA_LOG(dbg, yama::general_c, "expected:\n{}", expected);
    YAMA_LOG(dbg, yama::general_c, "actual:\n{}", actual);

    EXPECT_EQ(expected.fmt(), actual.fmt()); // easier to diagnose problems by comparing strings
}

// IMPORTANT: 'CallSig', 'ParamDecl', and 'Result' will be tested as a unit

TEST_F(YamaSpecTests, CallSig_NoParams_NoResult) {
    ASSERT_TRUE(pprs_ready);

    taul::source_code input{};
    input.add_str(""_str, "()"_str);
    
    auto parser = prep_for_ppr_test(lgr, *gram, input);
    ASSERT_TRUE(parser);

    parse_tree_builder b(*gram);
    {
        auto node0 = b.syntactic("CallSig"_str);
        b.lexical("L_ROUND"_str, 1);
        b.lexical("R_ROUND"_str, 1);
    }

    auto expected = b.done();
    auto actual = parser->parse("CallSig"_str);

    YAMA_LOG(dbg, yama::general_c, "expected:\n{}", expected);
    YAMA_LOG(dbg, yama::general_c, "actual:\n{}", actual);

    EXPECT_EQ(expected.fmt(), actual.fmt()); // easier to diagnose problems by comparing strings
}

TEST_F(YamaSpecTests, CallSig_NoParams_Result) {
    ASSERT_TRUE(pprs_ready);

    taul::source_code input{};
    input.add_str(""_str, "() -> Int"_str);
    
    auto parser = prep_for_ppr_test(lgr, *gram, input);
    ASSERT_TRUE(parser);

    parse_tree_builder b(*gram);
    {
        auto node0 = b.syntactic("CallSig"_str);
        b.lexical("L_ROUND"_str, 1);
        b.lexical("R_ROUND"_str, 1);
        b.skip_text(" "_str);
        {
            auto node1 = b.syntactic("Result"_str);
            b.lexical("R_ARROW"_str, 2);
            b.skip_text(" "_str);
            {
                auto node2 = b.syntactic("TypeSpec"_str);
                b.lexical("IDENTIFIER"_str, 3);
            }
        }
    }

    auto expected = b.done();
    auto actual = parser->parse("CallSig"_str);

    YAMA_LOG(dbg, yama::general_c, "expected:\n{}", expected);
    YAMA_LOG(dbg, yama::general_c, "actual:\n{}", actual);

    EXPECT_EQ(expected.fmt(), actual.fmt()); // easier to diagnose problems by comparing strings
}

TEST_F(YamaSpecTests, CallSig_Params_NoResult) {
    ASSERT_TRUE(pprs_ready);

    taul::source_code input{};
    input.add_str(""_str, "(a, b: Int, c: Float, d)"_str);

    auto parser = prep_for_ppr_test(lgr, *gram, input);
    ASSERT_TRUE(parser);

    parse_tree_builder b(*gram);
    {
        auto node0 = b.syntactic("CallSig"_str);
        b.lexical("L_ROUND"_str, 1);
        {
            auto node2 = b.syntactic("ParamDecl"_str);
            b.lexical("IDENTIFIER"_str, 1);
        }
        b.lexical("COMMA"_str, 1);
        b.skip_text(" "_str);
        {
            auto node2 = b.syntactic("ParamDecl"_str);
            b.lexical("IDENTIFIER"_str, 1);
            {
                auto node3 = b.syntactic("TypeAnnot"_str);
                b.lexical("COLON"_str, 1);
                b.skip_text(" "_str);
                {
                    auto node4 = b.syntactic("TypeSpec"_str);
                    b.lexical("IDENTIFIER"_str, 3);
                }
            }
        }
        b.lexical("COMMA"_str, 1);
        b.skip_text(" "_str);
        {
            auto node2 = b.syntactic("ParamDecl"_str);
            b.lexical("IDENTIFIER"_str, 1);
            {
                auto node3 = b.syntactic("TypeAnnot"_str);
                b.lexical("COLON"_str, 1);
                b.skip_text(" "_str);
                {
                    auto node4 = b.syntactic("TypeSpec"_str);
                    b.lexical("IDENTIFIER"_str, 5);
                }
            }
        }
        b.lexical("COMMA"_str, 1);
        b.skip_text(" "_str);
        {
            auto node2 = b.syntactic("ParamDecl"_str);
            b.lexical("IDENTIFIER"_str, 1);
        }
        b.lexical("R_ROUND"_str, 1);
    }

    auto expected = b.done();
    auto actual = parser->parse("CallSig"_str);

    YAMA_LOG(dbg, yama::general_c, "expected:\n{}", expected);
    YAMA_LOG(dbg, yama::general_c, "actual:\n{}", actual);

    EXPECT_EQ(expected.fmt(), actual.fmt()); // easier to diagnose problems by comparing strings
}

TEST_F(YamaSpecTests, CallSig_Params_Result) {
    ASSERT_TRUE(pprs_ready);

    taul::source_code input{};
    input.add_str(""_str, "(a, b: Int, c: Float, d) -> Int"_str);

    auto parser = prep_for_ppr_test(lgr, *gram, input);
    ASSERT_TRUE(parser);

    parse_tree_builder b(*gram);
    {
        auto node0 = b.syntactic("CallSig"_str);
        b.lexical("L_ROUND"_str, 1);
        {
            auto node2 = b.syntactic("ParamDecl"_str);
            b.lexical("IDENTIFIER"_str, 1);
        }
        b.lexical("COMMA"_str, 1);
        b.skip_text(" "_str);
        {
            auto node2 = b.syntactic("ParamDecl"_str);
            b.lexical("IDENTIFIER"_str, 1);
            {
                auto node3 = b.syntactic("TypeAnnot"_str);
                b.lexical("COLON"_str, 1);
                b.skip_text(" "_str);
                {
                    auto node4 = b.syntactic("TypeSpec"_str);
                    b.lexical("IDENTIFIER"_str, 3);
                }
            }
        }
        b.lexical("COMMA"_str, 1);
        b.skip_text(" "_str);
        {
            auto node2 = b.syntactic("ParamDecl"_str);
            b.lexical("IDENTIFIER"_str, 1);
            {
                auto node3 = b.syntactic("TypeAnnot"_str);
                b.lexical("COLON"_str, 1);
                b.skip_text(" "_str);
                {
                    auto node4 = b.syntactic("TypeSpec"_str);
                    b.lexical("IDENTIFIER"_str, 5);
                }
            }
        }
        b.lexical("COMMA"_str, 1);
        b.skip_text(" "_str);
        {
            auto node2 = b.syntactic("ParamDecl"_str);
            b.lexical("IDENTIFIER"_str, 1);
        }
        b.lexical("R_ROUND"_str, 1);
        b.skip_text(" "_str);
        {
            auto node1 = b.syntactic("Result"_str);
            b.lexical("R_ARROW"_str, 2);
            b.skip_text(" "_str);
            {
                auto node2 = b.syntactic("TypeSpec"_str);
                b.lexical("IDENTIFIER"_str, 3);
            }
        }
    }

    auto expected = b.done();
    auto actual = parser->parse("CallSig"_str);

    YAMA_LOG(dbg, yama::general_c, "expected:\n{}", expected);
    YAMA_LOG(dbg, yama::general_c, "actual:\n{}", actual);

    EXPECT_EQ(expected.fmt(), actual.fmt()); // easier to diagnose problems by comparing strings
}

TEST_F(YamaSpecTests, Block_NoStmt) {
    ASSERT_TRUE(pprs_ready);

    taul::source_code input{};
    input.add_str(""_str, "{}"_str);

    auto parser = prep_for_ppr_test(lgr, *gram, input);
    ASSERT_TRUE(parser);

    parse_tree_builder b(*gram);
    {
        auto node0 = b.syntactic("Block"_str);
        b.lexical("L_CURLY"_str, 1);
        b.lexical("R_CURLY"_str, 1);
    }

    auto expected = b.done();
    auto actual = parser->parse("Block"_str);

    YAMA_LOG(dbg, yama::general_c, "expected:\n{}", expected);
    YAMA_LOG(dbg, yama::general_c, "actual:\n{}", actual);

    EXPECT_EQ(expected.fmt(), actual.fmt()); // easier to diagnose problems by comparing strings
}

TEST_F(YamaSpecTests, Block_Stmt) {
    ASSERT_TRUE(pprs_ready);

    taul::source_code input{};
    input.add_str(""_str, "{ var a; var b; }"_str);

    auto parser = prep_for_ppr_test(lgr, *gram, input);
    ASSERT_TRUE(parser);

    parse_tree_builder b(*gram);
    {
        auto node0 = b.syntactic("Block"_str);
        b.lexical("L_CURLY"_str, 1);
        b.skip_text(" "_str);
        {
            auto node1 = b.syntactic("Stmt"_str);
            {
                auto node2 = b.syntactic("Decl"_str);
                {
                    auto node3 = b.syntactic("VarDecl"_str);
                    b.lexical("VAR"_str, 3);
                    b.skip_text(" "_str);
                    b.lexical("IDENTIFIER"_str, 1);
                    b.lexical("SEMI"_str, 1);
                }
            }
        }
        b.skip_text(" "_str);
        {
            auto node1 = b.syntactic("Stmt"_str);
            {
                auto node2 = b.syntactic("Decl"_str);
                {
                    auto node3 = b.syntactic("VarDecl"_str);
                    b.lexical("VAR"_str, 3);
                    b.skip_text(" "_str);
                    b.lexical("IDENTIFIER"_str, 1);
                    b.lexical("SEMI"_str, 1);
                }
            }
        }
        b.skip_text(" "_str);
        b.lexical("R_CURLY"_str, 1);
    }

    auto expected = b.done();
    auto actual = parser->parse("Block"_str);

    YAMA_LOG(dbg, yama::general_c, "expected:\n{}", expected);
    YAMA_LOG(dbg, yama::general_c, "actual:\n{}", actual);

    EXPECT_EQ(expected.fmt(), actual.fmt()); // easier to diagnose problems by comparing strings
}

TEST_F(YamaSpecTests, Stmt_Decl) {
    ASSERT_TRUE(pprs_ready);

    taul::source_code input{};
    input.add_str(""_str, "var a;"_str);

    auto parser = prep_for_ppr_test(lgr, *gram, input);
    ASSERT_TRUE(parser);

    parse_tree_builder b(*gram);
    {
        auto node0 = b.syntactic("Stmt"_str);
        {
            auto node1 = b.syntactic("Decl"_str);
            {
                auto node2 = b.syntactic("VarDecl"_str);
                b.lexical("VAR"_str, 3);
                b.skip_text(" "_str);
                b.lexical("IDENTIFIER"_str, 1);
                b.lexical("SEMI"_str, 1);
            }
        }
    }

    auto expected = b.done();
    auto actual = parser->parse("Stmt"_str);

    YAMA_LOG(dbg, yama::general_c, "expected:\n{}", expected);
    YAMA_LOG(dbg, yama::general_c, "actual:\n{}", actual);

    EXPECT_EQ(expected.fmt(), actual.fmt()); // easier to diagnose problems by comparing strings
}

TEST_F(YamaSpecTests, Stmt_ExprStmt) {
    ASSERT_TRUE(pprs_ready);

    taul::source_code input{};
    input.add_str(""_str, "300;"_str);

    auto parser = prep_for_ppr_test(lgr, *gram, input);
    ASSERT_TRUE(parser);

    parse_tree_builder b(*gram);
    {
        auto node0 = b.syntactic("Stmt"_str);
        {
            auto node1 = b.syntactic("ExprStmt"_str);
            {
                auto node2 = b.syntactic("Expr"_str);
                {
                    auto node3 = b.syntactic("PrimaryExpr"_str);
                    {
                        auto node4 = b.syntactic("Lit"_str);
                        {
                            auto node5 = b.syntactic("IntLit"_str);
                            b.lexical("INT_DEC"_str, 3);
                        }
                    }
                }
            }
            b.lexical("SEMI"_str, 1);
        }
    }

    auto expected = b.done();
    auto actual = parser->parse("Stmt"_str);

    YAMA_LOG(dbg, yama::general_c, "expected:\n{}", expected);
    YAMA_LOG(dbg, yama::general_c, "actual:\n{}", actual);

    EXPECT_EQ(expected.fmt(), actual.fmt()); // easier to diagnose problems by comparing strings
}

TEST_F(YamaSpecTests, Stmt_IfStmt) {
    ASSERT_TRUE(pprs_ready);

    taul::source_code input{};
    input.add_str(""_str, "if (true) {}"_str);

    auto parser = prep_for_ppr_test(lgr, *gram, input);
    ASSERT_TRUE(parser);

    parse_tree_builder b(*gram);
    {
        auto node0 = b.syntactic("Stmt"_str);
        {
            auto node1 = b.syntactic("IfStmt"_str);
            b.lexical("IF"_str, 2);
            b.skip_text(" "_str);
            b.lexical("L_ROUND"_str, 1);
            {
                auto node2 = b.syntactic("Expr"_str);
                {
                    auto node3 = b.syntactic("PrimaryExpr"_str);
                    {
                        auto node4 = b.syntactic("Lit"_str);
                        {
                            auto node5 = b.syntactic("BoolLit"_str);
                            b.lexical("TRUE"_str, 4);
                        }
                    }
                }
            }
            b.lexical("R_ROUND"_str, 1);
            b.skip_text(" "_str);
            {
                auto node2 = b.syntactic("Block"_str);
                b.lexical("L_CURLY"_str, 1);
                b.lexical("R_CURLY"_str, 1);
            }
        }
    }

    auto expected = b.done();
    auto actual = parser->parse("Stmt"_str);

    YAMA_LOG(dbg, yama::general_c, "expected:\n{}", expected);
    YAMA_LOG(dbg, yama::general_c, "actual:\n{}", actual);

    EXPECT_EQ(expected.fmt(), actual.fmt()); // easier to diagnose problems by comparing strings
}

TEST_F(YamaSpecTests, Stmt_LoopStmt) {
    ASSERT_TRUE(pprs_ready);

    taul::source_code input{};
    input.add_str(""_str, "loop {}"_str);

    auto parser = prep_for_ppr_test(lgr, *gram, input);
    ASSERT_TRUE(parser);

    parse_tree_builder b(*gram);
    {
        auto node0 = b.syntactic("Stmt"_str);
        {
            auto node1 = b.syntactic("LoopStmt"_str);
            b.lexical("LOOP"_str, 4);
            b.skip_text(" "_str);
            {
                auto node2 = b.syntactic("Block"_str);
                b.lexical("L_CURLY"_str, 1);
                b.lexical("R_CURLY"_str, 1);
            }
        }
    }

    auto expected = b.done();
    auto actual = parser->parse("Stmt"_str);

    YAMA_LOG(dbg, yama::general_c, "expected:\n{}", expected);
    YAMA_LOG(dbg, yama::general_c, "actual:\n{}", actual);

    EXPECT_EQ(expected.fmt(), actual.fmt()); // easier to diagnose problems by comparing strings
}

TEST_F(YamaSpecTests, Stmt_BreakStmt) {
    ASSERT_TRUE(pprs_ready);

    taul::source_code input{};
    input.add_str(""_str, "break;"_str);

    auto parser = prep_for_ppr_test(lgr, *gram, input);
    ASSERT_TRUE(parser);

    parse_tree_builder b(*gram);
    {
        auto node0 = b.syntactic("Stmt"_str);
        {
            auto node1 = b.syntactic("BreakStmt"_str);
            b.lexical("BREAK"_str, 5);
            b.lexical("SEMI"_str, 1);
        }
    }

    auto expected = b.done();
    auto actual = parser->parse("Stmt"_str);

    YAMA_LOG(dbg, yama::general_c, "expected:\n{}", expected);
    YAMA_LOG(dbg, yama::general_c, "actual:\n{}", actual);

    EXPECT_EQ(expected.fmt(), actual.fmt()); // easier to diagnose problems by comparing strings
}

TEST_F(YamaSpecTests, Stmt_ContinueStmt) {
    ASSERT_TRUE(pprs_ready);

    taul::source_code input{};
    input.add_str(""_str, "continue;"_str);

    auto parser = prep_for_ppr_test(lgr, *gram, input);
    ASSERT_TRUE(parser);

    parse_tree_builder b(*gram);
    {
        auto node0 = b.syntactic("Stmt"_str);
        {
            auto node1 = b.syntactic("ContinueStmt"_str);
            b.lexical("CONTINUE"_str, 8);
            b.lexical("SEMI"_str, 1);
        }
    }

    auto expected = b.done();
    auto actual = parser->parse("Stmt"_str);

    YAMA_LOG(dbg, yama::general_c, "expected:\n{}", expected);
    YAMA_LOG(dbg, yama::general_c, "actual:\n{}", actual);

    EXPECT_EQ(expected.fmt(), actual.fmt()); // easier to diagnose problems by comparing strings
}

TEST_F(YamaSpecTests, Stmt_ReturnStmt) {
    ASSERT_TRUE(pprs_ready);

    taul::source_code input{};
    input.add_str(""_str, "return;"_str);

    auto parser = prep_for_ppr_test(lgr, *gram, input);
    ASSERT_TRUE(parser);

    parse_tree_builder b(*gram);
    {
        auto node0 = b.syntactic("Stmt"_str);
        {
            auto node1 = b.syntactic("ReturnStmt"_str);
            b.lexical("RETURN"_str, 6);
            b.lexical("SEMI"_str, 1);
        }
    }

    auto expected = b.done();
    auto actual = parser->parse("Stmt"_str);

    YAMA_LOG(dbg, yama::general_c, "expected:\n{}", expected);
    YAMA_LOG(dbg, yama::general_c, "actual:\n{}", actual);

    EXPECT_EQ(expected.fmt(), actual.fmt()); // easier to diagnose problems by comparing strings
}

TEST_F(YamaSpecTests, ExprStmt) {
    ASSERT_TRUE(pprs_ready);

    taul::source_code input{};
    input.add_str(""_str, "300;"_str);

    auto parser = prep_for_ppr_test(lgr, *gram, input);
    ASSERT_TRUE(parser);

    parse_tree_builder b(*gram);
    {
        auto node0 = b.syntactic("ExprStmt"_str);
        {
            auto node1 = b.syntactic("Expr"_str);
            {
                auto node2 = b.syntactic("PrimaryExpr"_str);
                {
                    auto node3 = b.syntactic("Lit"_str);
                    {
                        auto node4 = b.syntactic("IntLit"_str);
                        b.lexical("INT_DEC"_str, 3);
                    }
                }
            }
        }
        b.lexical("SEMI"_str, 1);
    }

    auto expected = b.done();
    auto actual = parser->parse("ExprStmt"_str);

    YAMA_LOG(dbg, yama::general_c, "expected:\n{}", expected);
    YAMA_LOG(dbg, yama::general_c, "actual:\n{}", actual);

    EXPECT_EQ(expected.fmt(), actual.fmt()); // easier to diagnose problems by comparing strings
}

TEST_F(YamaSpecTests, ExprStmt_AssignStmt) {
    ASSERT_TRUE(pprs_ready);

    taul::source_code input{};
    input.add_str(""_str, "300 = abc;"_str);

    auto parser = prep_for_ppr_test(lgr, *gram, input);
    ASSERT_TRUE(parser);

    parse_tree_builder b(*gram);
    {
        auto node0 = b.syntactic("ExprStmt"_str);
        {
            auto node1 = b.syntactic("Expr"_str);
            {
                auto node2 = b.syntactic("PrimaryExpr"_str);
                {
                    auto node3 = b.syntactic("Lit"_str);
                    {
                        auto node4 = b.syntactic("IntLit"_str);
                        b.lexical("INT_DEC"_str, 3);
                    }
                }
            }
        }
        b.skip_text(" "_str);
        {
            auto node1 = b.syntactic("Assign"_str);
            b.lexical("ASSIGN"_str, 1);
            b.skip_text(" "_str);
            {
                auto node2 = b.syntactic("Expr"_str);
                {
                    auto node3 = b.syntactic("PrimaryExpr"_str);
                    b.lexical("IDENTIFIER"_str, 3);
                }
            }
        }
        b.lexical("SEMI"_str, 1);
    }

    auto expected = b.done();
    auto actual = parser->parse("ExprStmt"_str);

    YAMA_LOG(dbg, yama::general_c, "expected:\n{}", expected);
    YAMA_LOG(dbg, yama::general_c, "actual:\n{}", actual);

    EXPECT_EQ(expected.fmt(), actual.fmt()); // easier to diagnose problems by comparing strings
}

TEST_F(YamaSpecTests, IfStmt_NoElse) {
    ASSERT_TRUE(pprs_ready);

    taul::source_code input{};
    input.add_str(""_str, "if (true) {}"_str);

    auto parser = prep_for_ppr_test(lgr, *gram, input);
    ASSERT_TRUE(parser);

    parse_tree_builder b(*gram);
    {
        auto node0 = b.syntactic("IfStmt"_str);
        b.lexical("IF"_str, 2);
        b.skip_text(" "_str);
        b.lexical("L_ROUND"_str, 1);
        {
            auto node1 = b.syntactic("Expr"_str);
            {
                auto node2 = b.syntactic("PrimaryExpr"_str);
                {
                    auto node3 = b.syntactic("Lit"_str);
                    {
                        auto node4 = b.syntactic("BoolLit"_str);
                        b.lexical("TRUE"_str, 4);
                    }
                }
            }
        }
        b.lexical("R_ROUND"_str, 1);
        b.skip_text(" "_str);
        {
            auto node1 = b.syntactic("Block"_str);
            b.lexical("L_CURLY"_str, 1);
            b.lexical("R_CURLY"_str, 1);
        }
    }

    auto expected = b.done();
    auto actual = parser->parse("IfStmt"_str);

    YAMA_LOG(dbg, yama::general_c, "expected:\n{}", expected);
    YAMA_LOG(dbg, yama::general_c, "actual:\n{}", actual);

    EXPECT_EQ(expected.fmt(), actual.fmt()); // easier to diagnose problems by comparing strings
}

TEST_F(YamaSpecTests, IfStmt_Else) {
    ASSERT_TRUE(pprs_ready);

    taul::source_code input{};
    input.add_str(""_str, "if (true) {} else {}"_str);

    auto parser = prep_for_ppr_test(lgr, *gram, input);
    ASSERT_TRUE(parser);

    parse_tree_builder b(*gram);
    {
        auto node0 = b.syntactic("IfStmt"_str);
        b.lexical("IF"_str, 2);
        b.skip_text(" "_str);
        b.lexical("L_ROUND"_str, 1);
        {
            auto node1 = b.syntactic("Expr"_str);
            {
                auto node2 = b.syntactic("PrimaryExpr"_str);
                {
                    auto node3 = b.syntactic("Lit"_str);
                    {
                        auto node4 = b.syntactic("BoolLit"_str);
                        b.lexical("TRUE"_str, 4);
                    }
                }
            }
        }
        b.lexical("R_ROUND"_str, 1);
        b.skip_text(" "_str);
        {
            auto node1 = b.syntactic("Block"_str);
            b.lexical("L_CURLY"_str, 1);
            b.lexical("R_CURLY"_str, 1);
        }
        b.skip_text(" "_str);
        b.lexical("ELSE"_str, 4);
        b.skip_text(" "_str);
        {
            auto node1 = b.syntactic("Block"_str);
            b.lexical("L_CURLY"_str, 1);
            b.lexical("R_CURLY"_str, 1);
        }
    }

    auto expected = b.done();
    auto actual = parser->parse("IfStmt"_str);

    YAMA_LOG(dbg, yama::general_c, "expected:\n{}", expected);
    YAMA_LOG(dbg, yama::general_c, "actual:\n{}", actual);

    EXPECT_EQ(expected.fmt(), actual.fmt()); // easier to diagnose problems by comparing strings
}

TEST_F(YamaSpecTests, IfStmt_ElseIf) {
    ASSERT_TRUE(pprs_ready);

    taul::source_code input{};
    input.add_str(""_str, "if (true) {} else if (false) {}"_str);

    auto parser = prep_for_ppr_test(lgr, *gram, input);
    ASSERT_TRUE(parser);

    parse_tree_builder b(*gram);
    {
        auto node0 = b.syntactic("IfStmt"_str);
        b.lexical("IF"_str, 2);
        b.skip_text(" "_str);
        b.lexical("L_ROUND"_str, 1);
        {
            auto node1 = b.syntactic("Expr"_str);
            {
                auto node2 = b.syntactic("PrimaryExpr"_str);
                {
                    auto node3 = b.syntactic("Lit"_str);
                    {
                        auto node4 = b.syntactic("BoolLit"_str);
                        b.lexical("TRUE"_str, 4);
                    }
                }
            }
        }
        b.lexical("R_ROUND"_str, 1);
        b.skip_text(" "_str);
        {
            auto node1 = b.syntactic("Block"_str);
            b.lexical("L_CURLY"_str, 1);
            b.lexical("R_CURLY"_str, 1);
        }
        b.skip_text(" "_str);
        b.lexical("ELSE"_str, 4);
        b.skip_text(" "_str);
        {
            auto node1 = b.syntactic("IfStmt"_str);
            b.lexical("IF"_str, 2);
            b.skip_text(" "_str);
            b.lexical("L_ROUND"_str, 1);
            {
                auto node2 = b.syntactic("Expr"_str);
                {
                    auto node3 = b.syntactic("PrimaryExpr"_str);
                    {
                        auto node4 = b.syntactic("Lit"_str);
                        {
                            auto node5 = b.syntactic("BoolLit"_str);
                            b.lexical("FALSE"_str, 5);
                        }
                    }
                }
            }
            b.lexical("R_ROUND"_str, 1);
            b.skip_text(" "_str);
            {
                auto node2 = b.syntactic("Block"_str);
                b.lexical("L_CURLY"_str, 1);
                b.lexical("R_CURLY"_str, 1);
            }
        }
    }

    auto expected = b.done();
    auto actual = parser->parse("IfStmt"_str);

    YAMA_LOG(dbg, yama::general_c, "expected:\n{}", expected);
    YAMA_LOG(dbg, yama::general_c, "actual:\n{}", actual);

    EXPECT_EQ(expected.fmt(), actual.fmt()); // easier to diagnose problems by comparing strings
}

TEST_F(YamaSpecTests, LoopStmt) {
    ASSERT_TRUE(pprs_ready);

    taul::source_code input{};
    input.add_str(""_str, "loop {}"_str);

    auto parser = prep_for_ppr_test(lgr, *gram, input);
    ASSERT_TRUE(parser);

    parse_tree_builder b(*gram);
    {
        auto node0 = b.syntactic("LoopStmt"_str);
        b.lexical("LOOP"_str, 4);
        b.skip_text(" "_str);
        {
            auto node1 = b.syntactic("Block"_str);
            b.lexical("L_CURLY"_str, 1);
            b.lexical("R_CURLY"_str, 1);
        }
    }

    auto expected = b.done();
    auto actual = parser->parse("LoopStmt"_str);

    YAMA_LOG(dbg, yama::general_c, "expected:\n{}", expected);
    YAMA_LOG(dbg, yama::general_c, "actual:\n{}", actual);

    EXPECT_EQ(expected.fmt(), actual.fmt()); // easier to diagnose problems by comparing strings
}

TEST_F(YamaSpecTests, BreakStmt) {
    ASSERT_TRUE(pprs_ready);

    taul::source_code input{};
    input.add_str(""_str, "break;"_str);

    auto parser = prep_for_ppr_test(lgr, *gram, input);
    ASSERT_TRUE(parser);

    parse_tree_builder b(*gram);
    {
        auto node0 = b.syntactic("BreakStmt"_str);
        b.lexical("BREAK"_str, 5);
        b.lexical("SEMI"_str, 1);
    }

    auto expected = b.done();
    auto actual = parser->parse("BreakStmt"_str);

    YAMA_LOG(dbg, yama::general_c, "expected:\n{}", expected);
    YAMA_LOG(dbg, yama::general_c, "actual:\n{}", actual);

    EXPECT_EQ(expected.fmt(), actual.fmt()); // easier to diagnose problems by comparing strings
}

TEST_F(YamaSpecTests, ContinueStmt) {
    ASSERT_TRUE(pprs_ready);

    taul::source_code input{};
    input.add_str(""_str, "continue;"_str);

    auto parser = prep_for_ppr_test(lgr, *gram, input);
    ASSERT_TRUE(parser);

    parse_tree_builder b(*gram);
    {
        auto node0 = b.syntactic("ContinueStmt"_str);
        b.lexical("CONTINUE"_str, 8);
        b.lexical("SEMI"_str, 1);
    }

    auto expected = b.done();
    auto actual = parser->parse("ContinueStmt"_str);

    YAMA_LOG(dbg, yama::general_c, "expected:\n{}", expected);
    YAMA_LOG(dbg, yama::general_c, "actual:\n{}", actual);

    EXPECT_EQ(expected.fmt(), actual.fmt()); // easier to diagnose problems by comparing strings
}

TEST_F(YamaSpecTests, ReturnStmt_NoExpr) {
    ASSERT_TRUE(pprs_ready);

    taul::source_code input{};
    input.add_str(""_str, "return;"_str);

    auto parser = prep_for_ppr_test(lgr, *gram, input);
    ASSERT_TRUE(parser);

    parse_tree_builder b(*gram);
    {
        auto node0 = b.syntactic("ReturnStmt"_str);
        b.lexical("RETURN"_str, 6);
        b.lexical("SEMI"_str, 1);
    }

    auto expected = b.done();
    auto actual = parser->parse("ReturnStmt"_str);

    YAMA_LOG(dbg, yama::general_c, "expected:\n{}", expected);
    YAMA_LOG(dbg, yama::general_c, "actual:\n{}", actual);

    EXPECT_EQ(expected.fmt(), actual.fmt()); // easier to diagnose problems by comparing strings
}

TEST_F(YamaSpecTests, ReturnStmt_Expr) {
    ASSERT_TRUE(pprs_ready);

    taul::source_code input{};
    input.add_str(""_str, "return 3.14159;"_str);

    auto parser = prep_for_ppr_test(lgr, *gram, input);
    ASSERT_TRUE(parser);

    parse_tree_builder b(*gram);
    {
        auto node0 = b.syntactic("ReturnStmt"_str);
        b.lexical("RETURN"_str, 6);
        b.skip_text(" "_str);
        {
            auto node1 = b.syntactic("Expr"_str);
            {
                auto node2 = b.syntactic("PrimaryExpr"_str);
                {
                    auto node3 = b.syntactic("Lit"_str);
                    {
                        auto node4 = b.syntactic("FloatLit"_str);
                        b.lexical("FLOAT"_str, 7);
                    }
                }
            }
        }
        b.lexical("SEMI"_str, 1);
    }

    auto expected = b.done();
    auto actual = parser->parse("ReturnStmt"_str);

    YAMA_LOG(dbg, yama::general_c, "expected:\n{}", expected);
    YAMA_LOG(dbg, yama::general_c, "actual:\n{}", actual);

    EXPECT_EQ(expected.fmt(), actual.fmt()); // easier to diagnose problems by comparing strings
}

TEST_F(YamaSpecTests, Expr_PrimaryExpr) {
    ASSERT_TRUE(pprs_ready);

    taul::source_code input{};
    input.add_str(""_str, "300"_str);

    auto parser = prep_for_ppr_test(lgr, *gram, input);
    ASSERT_TRUE(parser);

    parse_tree_builder b(*gram);
    {
        auto node0 = b.syntactic("Expr"_str);
        {
            auto node1 = b.syntactic("PrimaryExpr"_str);
            {
                auto node2 = b.syntactic("Lit"_str);
                {
                    auto node3 = b.syntactic("IntLit"_str);
                    b.lexical("INT_DEC"_str, 3);
                }
            }
        }
    }

    auto expected = b.done();
    auto actual = parser->parse("Expr"_str);

    YAMA_LOG(dbg, yama::general_c, "expected:\n{}", expected);
    YAMA_LOG(dbg, yama::general_c, "actual:\n{}", actual);

    EXPECT_EQ(expected.fmt(), actual.fmt()); // easier to diagnose problems by comparing strings
}

TEST_F(YamaSpecTests, Expr_CallExpr) {
    ASSERT_TRUE(pprs_ready);

    taul::source_code input{};
    input.add_str(""_str, "a()"_str);

    auto parser = prep_for_ppr_test(lgr, *gram, input);
    ASSERT_TRUE(parser);

    parse_tree_builder b(*gram);
    {
        auto node0 = b.syntactic("Expr"_str);
        {
            auto node1 = b.syntactic("PrimaryExpr"_str);
            b.lexical("IDENTIFIER"_str, 1);
        }
        {
            auto node1 = b.syntactic("Args"_str);
            b.lexical("L_ROUND"_str, 1);
            b.lexical("R_ROUND"_str, 1);
        }
    }

    auto expected = b.done();
    auto actual = parser->parse("Expr"_str);

    YAMA_LOG(dbg, yama::general_c, "expected:\n{}", expected);
    YAMA_LOG(dbg, yama::general_c, "actual:\n{}", actual);

    EXPECT_EQ(expected.fmt(), actual.fmt()); // easier to diagnose problems by comparing strings
}

TEST_F(YamaSpecTests, Expr_SuffixNesting) {
    ASSERT_TRUE(pprs_ready);

    taul::source_code input{};
    input.add_str(""_str, "a()()()"_str);

    auto parser = prep_for_ppr_test(lgr, *gram, input);
    ASSERT_TRUE(parser);

    parse_tree_builder b(*gram);
    {
        auto node0 = b.syntactic("Expr"_str);
        {
            auto node1 = b.syntactic("PrimaryExpr"_str);
            b.lexical("IDENTIFIER"_str, 1);
        }
        {
            auto node1 = b.syntactic("Args"_str);
            b.lexical("L_ROUND"_str, 1);
            b.lexical("R_ROUND"_str, 1);
        }
        {
            auto node1 = b.syntactic("Args"_str);
            b.lexical("L_ROUND"_str, 1);
            b.lexical("R_ROUND"_str, 1);
        }
        {
            auto node1 = b.syntactic("Args"_str);
            b.lexical("L_ROUND"_str, 1);
            b.lexical("R_ROUND"_str, 1);
        }
    }

    auto expected = b.done();
    auto actual = parser->parse("Expr"_str);

    YAMA_LOG(dbg, yama::general_c, "expected:\n{}", expected);
    YAMA_LOG(dbg, yama::general_c, "actual:\n{}", actual);

    EXPECT_EQ(expected.fmt(), actual.fmt()); // easier to diagnose problems by comparing strings
}

TEST_F(YamaSpecTests, PrimaryExpr_IdentifierExpr) {
    ASSERT_TRUE(pprs_ready);

    taul::source_code input{};
    input.add_str(""_str, "abc"_str);

    auto parser = prep_for_ppr_test(lgr, *gram, input);
    ASSERT_TRUE(parser);

    parse_tree_builder b(*gram);
    {
        auto node0 = b.syntactic("PrimaryExpr"_str);
        b.lexical("IDENTIFIER"_str, 3);
    }

    auto expected = b.done();
    auto actual = parser->parse("PrimaryExpr"_str);

    YAMA_LOG(dbg, yama::general_c, "expected:\n{}", expected);
    YAMA_LOG(dbg, yama::general_c, "actual:\n{}", actual);

    EXPECT_EQ(expected.fmt(), actual.fmt()); // easier to diagnose problems by comparing strings
}

TEST_F(YamaSpecTests, PrimaryExpr_LitExpr) {
    ASSERT_TRUE(pprs_ready);

    taul::source_code input{};
    input.add_str(""_str, "300"_str);

    auto parser = prep_for_ppr_test(lgr, *gram, input);
    ASSERT_TRUE(parser);

    parse_tree_builder b(*gram);
    {
        auto node0 = b.syntactic("PrimaryExpr"_str);
        {
            auto node1 = b.syntactic("Lit"_str);
            {
                auto node2 = b.syntactic("IntLit"_str);
                b.lexical("INT_DEC"_str, 3);
            }
        }
    }

    auto expected = b.done();
    auto actual = parser->parse("PrimaryExpr"_str);

    YAMA_LOG(dbg, yama::general_c, "expected:\n{}", expected);
    YAMA_LOG(dbg, yama::general_c, "actual:\n{}", actual);

    EXPECT_EQ(expected.fmt(), actual.fmt()); // easier to diagnose problems by comparing strings
}

TEST_F(YamaSpecTests, Lit_IntLit) {
    ASSERT_TRUE(pprs_ready);

    taul::source_code input{};
    input.add_str(""_str, "300"_str);

    auto parser = prep_for_ppr_test(lgr, *gram, input);
    ASSERT_TRUE(parser);

    parse_tree_builder b(*gram);
    {
        auto node0 = b.syntactic("Lit"_str);
        {
            auto node1 = b.syntactic("IntLit"_str);
            b.lexical("INT_DEC"_str, 3);
        }
    }

    auto expected = b.done();
    auto actual = parser->parse("Lit"_str);

    YAMA_LOG(dbg, yama::general_c, "expected:\n{}", expected);
    YAMA_LOG(dbg, yama::general_c, "actual:\n{}", actual);

    EXPECT_EQ(expected.fmt(), actual.fmt()); // easier to diagnose problems by comparing strings
}

TEST_F(YamaSpecTests, Lit_UIntLit) {
    ASSERT_TRUE(pprs_ready);

    taul::source_code input{};
    input.add_str(""_str, "300u"_str);

    auto parser = prep_for_ppr_test(lgr, *gram, input);
    ASSERT_TRUE(parser);

    parse_tree_builder b(*gram);
    {
        auto node0 = b.syntactic("Lit"_str);
        {
            auto node1 = b.syntactic("UIntLit"_str);
            b.lexical("UINT_DEC"_str, 4);
        }
    }

    auto expected = b.done();
    auto actual = parser->parse("Lit"_str);

    YAMA_LOG(dbg, yama::general_c, "expected:\n{}", expected);
    YAMA_LOG(dbg, yama::general_c, "actual:\n{}", actual);

    EXPECT_EQ(expected.fmt(), actual.fmt()); // easier to diagnose problems by comparing strings
}

TEST_F(YamaSpecTests, Lit_FloatLit) {
    ASSERT_TRUE(pprs_ready);

    taul::source_code input{};
    input.add_str(""_str, "30.04"_str);

    auto parser = prep_for_ppr_test(lgr, *gram, input);
    ASSERT_TRUE(parser);

    parse_tree_builder b(*gram);
    {
        auto node0 = b.syntactic("Lit"_str);
        {
            auto node1 = b.syntactic("FloatLit"_str);
            b.lexical("FLOAT"_str, 5);
        }
    }

    auto expected = b.done();
    auto actual = parser->parse("Lit"_str);

    YAMA_LOG(dbg, yama::general_c, "expected:\n{}", expected);
    YAMA_LOG(dbg, yama::general_c, "actual:\n{}", actual);

    EXPECT_EQ(expected.fmt(), actual.fmt()); // easier to diagnose problems by comparing strings
}

TEST_F(YamaSpecTests, Lit_BoolLit) {
    ASSERT_TRUE(pprs_ready);

    taul::source_code input{};
    input.add_str(""_str, "true"_str);

    auto parser = prep_for_ppr_test(lgr, *gram, input);
    ASSERT_TRUE(parser);

    parse_tree_builder b(*gram);
    {
        auto node0 = b.syntactic("Lit"_str);
        {
            auto node1 = b.syntactic("BoolLit"_str);
            b.lexical("TRUE"_str, 4);
        }
    }

    auto expected = b.done();
    auto actual = parser->parse("Lit"_str);

    YAMA_LOG(dbg, yama::general_c, "expected:\n{}", expected);
    YAMA_LOG(dbg, yama::general_c, "actual:\n{}", actual);

    EXPECT_EQ(expected.fmt(), actual.fmt()); // easier to diagnose problems by comparing strings
}

TEST_F(YamaSpecTests, Lit_CharLit) {
    ASSERT_TRUE(pprs_ready);

    taul::source_code input{};
    input.add_str(""_str, "'y'"_str);

    auto parser = prep_for_ppr_test(lgr, *gram, input);
    ASSERT_TRUE(parser);

    parse_tree_builder b(*gram);
    {
        auto node0 = b.syntactic("Lit"_str);
        {
            auto node1 = b.syntactic("CharLit"_str);
            b.lexical("CHAR"_str, 3);
        }
    }

    auto expected = b.done();
    auto actual = parser->parse("Lit"_str);

    YAMA_LOG(dbg, yama::general_c, "expected:\n{}", expected);
    YAMA_LOG(dbg, yama::general_c, "actual:\n{}", actual);

    EXPECT_EQ(expected.fmt(), actual.fmt()); // easier to diagnose problems by comparing strings
}

TEST_F(YamaSpecTests, IntLit_Dec) {
    ASSERT_TRUE(pprs_ready);

    taul::source_code input{};
    input.add_str(""_str, "300"_str);

    auto parser = prep_for_ppr_test(lgr, *gram, input);
    ASSERT_TRUE(parser);

    parse_tree_builder b(*gram);
    {
        auto node0 = b.syntactic("IntLit"_str);
        b.lexical("INT_DEC"_str, 3);
    }

    auto expected = b.done();
    auto actual = parser->parse("IntLit"_str);

    YAMA_LOG(dbg, yama::general_c, "expected:\n{}", expected);
    YAMA_LOG(dbg, yama::general_c, "actual:\n{}", actual);

    EXPECT_EQ(expected.fmt(), actual.fmt()); // easier to diagnose problems by comparing strings
}

TEST_F(YamaSpecTests, IntLit_Hex) {
    ASSERT_TRUE(pprs_ready);

    taul::source_code input{};
    input.add_str(""_str, "0x300"_str);

    auto parser = prep_for_ppr_test(lgr, *gram, input);
    ASSERT_TRUE(parser);

    parse_tree_builder b(*gram);
    {
        auto node0 = b.syntactic("IntLit"_str);
        b.lexical("INT_HEX"_str, 5);
    }

    auto expected = b.done();
    auto actual = parser->parse("IntLit"_str);

    YAMA_LOG(dbg, yama::general_c, "expected:\n{}", expected);
    YAMA_LOG(dbg, yama::general_c, "actual:\n{}", actual);

    EXPECT_EQ(expected.fmt(), actual.fmt()); // easier to diagnose problems by comparing strings
}

TEST_F(YamaSpecTests, IntLit_Bin) {
    ASSERT_TRUE(pprs_ready);

    taul::source_code input{};
    input.add_str(""_str, "0b101"_str);

    auto parser = prep_for_ppr_test(lgr, *gram, input);
    ASSERT_TRUE(parser);

    parse_tree_builder b(*gram);
    {
        auto node0 = b.syntactic("IntLit"_str);
        b.lexical("INT_BIN"_str, 5);
    }

    auto expected = b.done();
    auto actual = parser->parse("IntLit"_str);

    YAMA_LOG(dbg, yama::general_c, "expected:\n{}", expected);
    YAMA_LOG(dbg, yama::general_c, "actual:\n{}", actual);

    EXPECT_EQ(expected.fmt(), actual.fmt()); // easier to diagnose problems by comparing strings
}

TEST_F(YamaSpecTests, UIntLit_Dec) {
    ASSERT_TRUE(pprs_ready);

    taul::source_code input{};
    input.add_str(""_str, "300u"_str);

    auto parser = prep_for_ppr_test(lgr, *gram, input);
    ASSERT_TRUE(parser);

    parse_tree_builder b(*gram);
    {
        auto node0 = b.syntactic("UIntLit"_str);
        b.lexical("UINT_DEC"_str, 4);
    }

    auto expected = b.done();
    auto actual = parser->parse("UIntLit"_str);

    YAMA_LOG(dbg, yama::general_c, "expected:\n{}", expected);
    YAMA_LOG(dbg, yama::general_c, "actual:\n{}", actual);

    EXPECT_EQ(expected.fmt(), actual.fmt()); // easier to diagnose problems by comparing strings
}

TEST_F(YamaSpecTests, UIntLit_Hex) {
    ASSERT_TRUE(pprs_ready);

    taul::source_code input{};
    input.add_str(""_str, "0x300u"_str);

    auto parser = prep_for_ppr_test(lgr, *gram, input);
    ASSERT_TRUE(parser);

    parse_tree_builder b(*gram);
    {
        auto node0 = b.syntactic("UIntLit"_str);
        b.lexical("UINT_HEX"_str, 6);
    }

    auto expected = b.done();
    auto actual = parser->parse("UIntLit"_str);

    YAMA_LOG(dbg, yama::general_c, "expected:\n{}", expected);
    YAMA_LOG(dbg, yama::general_c, "actual:\n{}", actual);

    EXPECT_EQ(expected.fmt(), actual.fmt()); // easier to diagnose problems by comparing strings
}

TEST_F(YamaSpecTests, UIntLit_Bin) {
    ASSERT_TRUE(pprs_ready);

    taul::source_code input{};
    input.add_str(""_str, "0b101u"_str);

    auto parser = prep_for_ppr_test(lgr, *gram, input);
    ASSERT_TRUE(parser);

    parse_tree_builder b(*gram);
    {
        auto node0 = b.syntactic("UIntLit"_str);
        b.lexical("UINT_BIN"_str, 6);
    }

    auto expected = b.done();
    auto actual = parser->parse("UIntLit"_str);

    YAMA_LOG(dbg, yama::general_c, "expected:\n{}", expected);
    YAMA_LOG(dbg, yama::general_c, "actual:\n{}", actual);

    EXPECT_EQ(expected.fmt(), actual.fmt()); // easier to diagnose problems by comparing strings
}

TEST_F(YamaSpecTests, FloatLit_FLOAT) {
    ASSERT_TRUE(pprs_ready);

    taul::source_code input{};
    input.add_str(""_str, "10.003"_str);

    auto parser = prep_for_ppr_test(lgr, *gram, input);
    ASSERT_TRUE(parser);

    parse_tree_builder b(*gram);
    {
        auto node0 = b.syntactic("FloatLit"_str);
        b.lexical("FLOAT"_str, 6);
    }

    auto expected = b.done();
    auto actual = parser->parse("FloatLit"_str);

    YAMA_LOG(dbg, yama::general_c, "expected:\n{}", expected);
    YAMA_LOG(dbg, yama::general_c, "actual:\n{}", actual);

    EXPECT_EQ(expected.fmt(), actual.fmt()); // easier to diagnose problems by comparing strings
}

TEST_F(YamaSpecTests, FloatLit_INF) {
    ASSERT_TRUE(pprs_ready);

    taul::source_code input{};
    input.add_str(""_str, "inf"_str);

    auto parser = prep_for_ppr_test(lgr, *gram, input);
    ASSERT_TRUE(parser);

    parse_tree_builder b(*gram);
    {
        auto node0 = b.syntactic("FloatLit"_str);
        b.lexical("INF"_str, 3);
    }

    auto expected = b.done();
    auto actual = parser->parse("FloatLit"_str);

    YAMA_LOG(dbg, yama::general_c, "expected:\n{}", expected);
    YAMA_LOG(dbg, yama::general_c, "actual:\n{}", actual);

    EXPECT_EQ(expected.fmt(), actual.fmt()); // easier to diagnose problems by comparing strings
}

TEST_F(YamaSpecTests, FloatLit_NAN) {
    ASSERT_TRUE(pprs_ready);

    taul::source_code input{};
    input.add_str(""_str, "nan"_str);

    auto parser = prep_for_ppr_test(lgr, *gram, input);
    ASSERT_TRUE(parser);

    parse_tree_builder b(*gram);
    {
        auto node0 = b.syntactic("FloatLit"_str);
        b.lexical("NAN"_str, 3);
    }

    auto expected = b.done();
    auto actual = parser->parse("FloatLit"_str);

    YAMA_LOG(dbg, yama::general_c, "expected:\n{}", expected);
    YAMA_LOG(dbg, yama::general_c, "actual:\n{}", actual);

    EXPECT_EQ(expected.fmt(), actual.fmt()); // easier to diagnose problems by comparing strings
}

TEST_F(YamaSpecTests, BoolLit_True) {
    ASSERT_TRUE(pprs_ready);

    taul::source_code input{};
    input.add_str(""_str, "true"_str);

    auto parser = prep_for_ppr_test(lgr, *gram, input);
    ASSERT_TRUE(parser);

    parse_tree_builder b(*gram);
    {
        auto node0 = b.syntactic("BoolLit"_str);
        b.lexical("TRUE"_str, 4);
    }

    auto expected = b.done();
    auto actual = parser->parse("BoolLit"_str);

    YAMA_LOG(dbg, yama::general_c, "expected:\n{}", expected);
    YAMA_LOG(dbg, yama::general_c, "actual:\n{}", actual);

    EXPECT_EQ(expected.fmt(), actual.fmt()); // easier to diagnose problems by comparing strings
}

TEST_F(YamaSpecTests, BoolLit_False) {
    ASSERT_TRUE(pprs_ready);

    taul::source_code input{};
    input.add_str(""_str, "false"_str);

    auto parser = prep_for_ppr_test(lgr, *gram, input);
    ASSERT_TRUE(parser);

    parse_tree_builder b(*gram);
    {
        auto node0 = b.syntactic("BoolLit"_str);
        b.lexical("FALSE"_str, 5);
    }

    auto expected = b.done();
    auto actual = parser->parse("BoolLit"_str);

    YAMA_LOG(dbg, yama::general_c, "expected:\n{}", expected);
    YAMA_LOG(dbg, yama::general_c, "actual:\n{}", actual);

    EXPECT_EQ(expected.fmt(), actual.fmt()); // easier to diagnose problems by comparing strings
}

TEST_F(YamaSpecTests, CharLit) {
    ASSERT_TRUE(pprs_ready);

    taul::source_code input{};
    input.add_str(""_str, "'y'"_str);

    auto parser = prep_for_ppr_test(lgr, *gram, input);
    ASSERT_TRUE(parser);

    parse_tree_builder b(*gram);
    {
        auto node0 = b.syntactic("CharLit"_str);
        b.lexical("CHAR"_str, 3);
    }

    auto expected = b.done();
    auto actual = parser->parse("CharLit"_str);

    YAMA_LOG(dbg, yama::general_c, "expected:\n{}", expected);
    YAMA_LOG(dbg, yama::general_c, "actual:\n{}", actual);

    EXPECT_EQ(expected.fmt(), actual.fmt()); // easier to diagnose problems by comparing strings
}

TEST_F(YamaSpecTests, Assign) {
    ASSERT_TRUE(pprs_ready);

    taul::source_code input{};
    input.add_str(""_str, "= 10"_str);

    auto parser = prep_for_ppr_test(lgr, *gram, input);
    ASSERT_TRUE(parser);

    parse_tree_builder b(*gram);
    {
        auto node0 = b.syntactic("Assign"_str);
        b.lexical("ASSIGN"_str, 1);
        b.skip_text(" "_str);
        {
            auto node1 = b.syntactic("Expr"_str);
            {
                auto node2 = b.syntactic("PrimaryExpr"_str);
                {
                    auto node3 = b.syntactic("Lit"_str);
                    {
                        auto node4 = b.syntactic("IntLit"_str);
                        b.lexical("INT_DEC"_str, 2);
                    }
                }
            }
        }
    }

    auto expected = b.done();
    auto actual = parser->parse("Assign"_str);

    YAMA_LOG(dbg, yama::general_c, "expected:\n{}", expected);
    YAMA_LOG(dbg, yama::general_c, "actual:\n{}", actual);

    EXPECT_EQ(expected.fmt(), actual.fmt()); // easier to diagnose problems by comparing strings
}

TEST_F(YamaSpecTests, Args_Empty) {
    ASSERT_TRUE(pprs_ready);

    taul::source_code input{};
    input.add_str(""_str, "()"_str);

    auto parser = prep_for_ppr_test(lgr, *gram, input);
    ASSERT_TRUE(parser);

    parse_tree_builder b(*gram);
    {
        auto node0 = b.syntactic("Args"_str);
        b.lexical("L_ROUND"_str, 1);
        b.lexical("R_ROUND"_str, 1);
    }

    auto expected = b.done();
    auto actual = parser->parse("Args"_str);

    YAMA_LOG(dbg, yama::general_c, "expected:\n{}", expected);
    YAMA_LOG(dbg, yama::general_c, "actual:\n{}", actual);

    EXPECT_EQ(expected.fmt(), actual.fmt()); // easier to diagnose problems by comparing strings
}

TEST_F(YamaSpecTests, Args_NonEmpty) {
    ASSERT_TRUE(pprs_ready);

    taul::source_code input{};
    input.add_str(""_str, "(10, 3, 100)"_str);

    auto parser = prep_for_ppr_test(lgr, *gram, input);
    ASSERT_TRUE(parser);

    parse_tree_builder b(*gram);
    {
        auto node0 = b.syntactic("Args"_str);
        b.lexical("L_ROUND"_str, 1);
        {
            auto node1 = b.syntactic("Expr"_str);
            {
                auto node2 = b.syntactic("PrimaryExpr"_str);
                {
                    auto node3 = b.syntactic("Lit"_str);
                    {
                        auto node4 = b.syntactic("IntLit"_str);
                        b.lexical("INT_DEC"_str, 2);
                    }
                }
            }
        }
        b.lexical("COMMA"_str, 1);
        b.skip_text(" "_str);
        {
            auto node1 = b.syntactic("Expr"_str);
            {
                auto node2 = b.syntactic("PrimaryExpr"_str);
                {
                    auto node3 = b.syntactic("Lit"_str);
                    {
                        auto node4 = b.syntactic("IntLit"_str);
                        b.lexical("INT_DEC"_str, 1);
                    }
                }
            }
        }
        b.lexical("COMMA"_str, 1);
        b.skip_text(" "_str);
        {
            auto node1 = b.syntactic("Expr"_str);
            {
                auto node2 = b.syntactic("PrimaryExpr"_str);
                {
                    auto node3 = b.syntactic("Lit"_str);
                    {
                        auto node4 = b.syntactic("IntLit"_str);
                        b.lexical("INT_DEC"_str, 3);
                    }
                }
            }
        }
        b.lexical("R_ROUND"_str, 1);
    }

    auto expected = b.done();
    auto actual = parser->parse("Args"_str);

    YAMA_LOG(dbg, yama::general_c, "expected:\n{}", expected);
    YAMA_LOG(dbg, yama::general_c, "actual:\n{}", actual);

    EXPECT_EQ(expected.fmt(), actual.fmt()); // easier to diagnose problems by comparing strings
}

TEST_F(YamaSpecTests, TypeAnnot) {
    ASSERT_TRUE(pprs_ready);

    taul::source_code input{};
    input.add_str(""_str, ": Float"_str);

    auto parser = prep_for_ppr_test(lgr, *gram, input);
    ASSERT_TRUE(parser);

    parse_tree_builder b(*gram);
    {
        auto node0 = b.syntactic("TypeAnnot"_str);
        b.lexical("COLON"_str, 1);
        b.skip_text(" "_str);
        {
            auto node1 = b.syntactic("TypeSpec"_str);
            b.lexical("IDENTIFIER"_str, 5);
        }
    }

    auto expected = b.done();
    auto actual = parser->parse("TypeAnnot"_str);

    YAMA_LOG(dbg, yama::general_c, "expected:\n{}", expected);
    YAMA_LOG(dbg, yama::general_c, "actual:\n{}", actual);

    EXPECT_EQ(expected.fmt(), actual.fmt()); // easier to diagnose problems by comparing strings
}

TEST_F(YamaSpecTests, TypeSpec) {
    ASSERT_TRUE(pprs_ready);

    taul::source_code input{};
    input.add_str(""_str, "Float"_str);

    auto parser = prep_for_ppr_test(lgr, *gram, input);
    ASSERT_TRUE(parser);

    parse_tree_builder b(*gram);
    {
        auto node0 = b.syntactic("TypeSpec"_str);
        b.lexical("IDENTIFIER"_str, 5);
    }

    auto expected = b.done();
    auto actual = parser->parse("TypeSpec"_str);

    YAMA_LOG(dbg, yama::general_c, "expected:\n{}", expected);
    YAMA_LOG(dbg, yama::general_c, "actual:\n{}", actual);

    EXPECT_EQ(expected.fmt(), actual.fmt()); // easier to diagnose problems by comparing strings
}

