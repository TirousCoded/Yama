

#pragma once


#include <gtest/gtest.h>
#include <yama/yama.h>


static void test_int(YmInt x, YmIntFmt fmt, const std::string& expectedUppercase, const std::string& expectedLowercase) {
    size_t m = ymMeasureInt(x, fmt);
    ASSERT_EQ(m, expectedUppercase.size());
    ASSERT_EQ(m, expectedLowercase.size());
    {
        auto result = ymFmtInt(x, true, fmt, nullptr);
        EXPECT_TRUE(result);
        if (result) {
            EXPECT_STREQ(result, expectedUppercase.c_str());
            std::free((void*)result);
        }
    }
    {
        auto result = ymFmtInt(x, false, fmt, nullptr);
        EXPECT_TRUE(result);
        if (result) {
            EXPECT_STREQ(result, expectedLowercase.c_str());
            std::free((void*)result);
        }
    }
    {
        std::vector<YmChar> buff{};
        buff.resize(m + 1); // '+ 1' for null-terminator.
        auto result = ymFmtInt(x, true, fmt, buff.data());
        EXPECT_EQ(result, buff.data());
        if (result) {
            EXPECT_STREQ(result, expectedUppercase.c_str());
        }
    }
    {
        std::vector<YmChar> buff{};
        buff.resize(m + 1); // '+ 1' for null-terminator.
        auto result = ymFmtInt(x, false, fmt, buff.data());
        EXPECT_EQ(result, buff.data());
        if (result) {
            EXPECT_STREQ(result, expectedLowercase.c_str());
        }
    }
}
static void test_int(YmInt x, YmIntFmt fmt, const std::string& expected) {
    test_int(x, fmt, expected, expected);
}
static void test_uint(YmUInt x, YmIntFmt fmt, const std::string& expectedUppercase, const std::string& expectedLowercase) {
    size_t m = ymMeasureUInt(x, fmt);
    ASSERT_EQ(m, expectedUppercase.size());
    ASSERT_EQ(m, expectedLowercase.size());
    {
        auto result = ymFmtUInt(x, true, fmt, nullptr);
        EXPECT_TRUE(result);
        if (result) {
            EXPECT_STREQ(result, expectedUppercase.c_str());
            std::free((void*)result);
        }
    }
    {
        auto result = ymFmtUInt(x, false, fmt, nullptr);
        EXPECT_TRUE(result);
        if (result) {
            EXPECT_STREQ(result, expectedLowercase.c_str());
            std::free((void*)result);
        }
    }
    {
        std::vector<YmChar> buff{};
        buff.resize(m + 1); // '+ 1' for null-terminator.
        auto result = ymFmtUInt(x, true, fmt, buff.data());
        EXPECT_EQ(result, buff.data());
        if (result) {
            EXPECT_STREQ(result, expectedUppercase.c_str());
        }
    }
    {
        std::vector<YmChar> buff{};
        buff.resize(m + 1); // '+ 1' for null-terminator.
        auto result = ymFmtUInt(x, false, fmt, buff.data());
        EXPECT_EQ(result, buff.data());
        if (result) {
            EXPECT_STREQ(result, expectedLowercase.c_str());
        }
    }
}
static void test_uint(YmUInt x, YmIntFmt fmt, const std::string& expected) {
    test_uint(x, fmt, expected, expected);
}
static void test_float(YmFloat x, const std::string& expected) {
    size_t m = ymMeasureFloat(x);
    ASSERT_EQ(m, expected.size());
    {
        auto result = ymFmtFloat(x, nullptr);
        EXPECT_TRUE(result);
        if (result) {
            EXPECT_STREQ(result, expected.c_str());
            std::free((void*)result);
        }
    }
    {
        std::vector<YmChar> buff{};
        buff.resize(m + 1); // '+ 1' for null-terminator.
        auto result = ymFmtFloat(x, buff.data());
        EXPECT_EQ(result, buff.data());
        if (result) {
            EXPECT_STREQ(result, expected.c_str());
        }
    }
}
static void test_rune(YmRune x, const std::string& expectedUppercase, const std::string& expectedLowercase, bool escapeQuotes, bool escapeDblQuotes, bool escapeBackslashes) {
    size_t m = ymMeasureRune(x, escapeQuotes, escapeDblQuotes, escapeBackslashes);
    ASSERT_EQ(m, expectedUppercase.size());
    ASSERT_EQ(m, expectedLowercase.size());
    {
        auto result = ymFmtRune(x, true, escapeQuotes, escapeDblQuotes, escapeBackslashes, nullptr);
        EXPECT_TRUE(result);
        if (result) {
            EXPECT_STREQ(result, expectedUppercase.c_str());
            std::free((void*)result);
        }
    }
    {
        auto result = ymFmtRune(x, false, escapeQuotes, escapeDblQuotes, escapeBackslashes, nullptr);
        EXPECT_TRUE(result);
        if (result) {
            EXPECT_STREQ(result, expectedLowercase.c_str());
            std::free((void*)result);
        }
    }
    {
        std::vector<YmChar> buff{};
        buff.resize(m + 1); // '+ 1' for null-terminator.
        auto result = ymFmtRune(x, true, escapeQuotes, escapeDblQuotes, escapeBackslashes, buff.data());
        EXPECT_EQ(result, buff.data());
        if (result) {
            EXPECT_STREQ(result, expectedUppercase.c_str());
        }
    }
    {
        std::vector<YmChar> buff{};
        buff.resize(m + 1); // '+ 1' for null-terminator.
        auto result = ymFmtRune(x, false, escapeQuotes, escapeDblQuotes, escapeBackslashes, buff.data());
        EXPECT_EQ(result, buff.data());
        if (result) {
            EXPECT_STREQ(result, expectedLowercase.c_str());
        }
    }
}
static void test_rune(YmRune x, const std::string& expected, bool escapeQuotes, bool escapeDblQuotes, bool escapeBackslashes) {
    test_rune(x, expected, expected, escapeQuotes, escapeDblQuotes, escapeBackslashes);
}

