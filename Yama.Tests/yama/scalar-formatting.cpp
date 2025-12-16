

#include <format>

#include <gtest/gtest.h>
#include <taul/strings.h>
#include <taul/unicode.h>
#include <yama/yama.h>

#include "scalar-formatting-helpers.h"


TEST(ScalarFormatting, Int) {
    test_int(0, YmIntFmt_Dec, "0");

    test_int(1, YmIntFmt_Dec, "1");
    test_int(2, YmIntFmt_Dec, "2");
    test_int(3, YmIntFmt_Dec, "3");
    test_int(4, YmIntFmt_Dec, "4");
    test_int(5, YmIntFmt_Dec, "5");
    test_int(6, YmIntFmt_Dec, "6");
    test_int(7, YmIntFmt_Dec, "7");
    test_int(8, YmIntFmt_Dec, "8");
    test_int(9, YmIntFmt_Dec, "9");
    test_int(10, YmIntFmt_Dec, "10");
    test_int(1234567890, YmIntFmt_Dec, "1234567890");

    test_int(-1, YmIntFmt_Dec, "-1");
    test_int(-2, YmIntFmt_Dec, "-2");
    test_int(-3, YmIntFmt_Dec, "-3");
    test_int(-4, YmIntFmt_Dec, "-4");
    test_int(-5, YmIntFmt_Dec, "-5");
    test_int(-6, YmIntFmt_Dec, "-6");
    test_int(-7, YmIntFmt_Dec, "-7");
    test_int(-8, YmIntFmt_Dec, "-8");
    test_int(-9, YmIntFmt_Dec, "-9");
    test_int(-10, YmIntFmt_Dec, "-10");
    test_int(-1234567890, YmIntFmt_Dec, "-1234567890");

    test_int(0, YmIntFmt_Hex, "0x0");

    test_int(1, YmIntFmt_Hex, "0x1");
    test_int(2, YmIntFmt_Hex, "0x2");
    test_int(3, YmIntFmt_Hex, "0x3");
    test_int(4, YmIntFmt_Hex, "0x4");
    test_int(5, YmIntFmt_Hex, "0x5");
    test_int(6, YmIntFmt_Hex, "0x6");
    test_int(7, YmIntFmt_Hex, "0x7");
    test_int(8, YmIntFmt_Hex, "0x8");
    test_int(9, YmIntFmt_Hex, "0x9");
    test_int(10, YmIntFmt_Hex, "0xA", "0xa");
    test_int(11, YmIntFmt_Hex, "0xB", "0xb");
    test_int(12, YmIntFmt_Hex, "0xC", "0xc");
    test_int(13, YmIntFmt_Hex, "0xD", "0xd");
    test_int(14, YmIntFmt_Hex, "0xE", "0xe");
    test_int(15, YmIntFmt_Hex, "0xF", "0xf");
    test_int(16, YmIntFmt_Hex, "0x10");
    test_int(0x1f2e3d4c5b4a, YmIntFmt_Hex, "0x1F2E3D4C5B4A", "0x1f2e3d4c5b4a");

    test_int(-1, YmIntFmt_Hex, "-0x1");
    test_int(-2, YmIntFmt_Hex, "-0x2");
    test_int(-3, YmIntFmt_Hex, "-0x3");
    test_int(-4, YmIntFmt_Hex, "-0x4");
    test_int(-5, YmIntFmt_Hex, "-0x5");
    test_int(-6, YmIntFmt_Hex, "-0x6");
    test_int(-7, YmIntFmt_Hex, "-0x7");
    test_int(-8, YmIntFmt_Hex, "-0x8");
    test_int(-9, YmIntFmt_Hex, "-0x9");
    test_int(-10, YmIntFmt_Hex, "-0xA", "-0xa");
    test_int(-11, YmIntFmt_Hex, "-0xB", "-0xb");
    test_int(-12, YmIntFmt_Hex, "-0xC", "-0xc");
    test_int(-13, YmIntFmt_Hex, "-0xD", "-0xd");
    test_int(-14, YmIntFmt_Hex, "-0xE", "-0xe");
    test_int(-15, YmIntFmt_Hex, "-0xF", "-0xf");
    test_int(-16, YmIntFmt_Hex, "-0x10");
    test_int(-0x1f2e3d4c5b4a, YmIntFmt_Hex, "-0x1F2E3D4C5B4A", "-0x1f2e3d4c5b4a");

    test_int(0, YmIntFmt_Bin, "0b0");

    test_int(1, YmIntFmt_Bin, "0b1");
    test_int(2, YmIntFmt_Bin, "0b10");
    test_int(0b10101100, YmIntFmt_Bin, "0b10101100");

    test_int(-1, YmIntFmt_Bin, "-0b1");
    test_int(-2, YmIntFmt_Bin, "-0b10");
    test_int(-0b10101100, YmIntFmt_Bin, "-0b10101100");
}

TEST(ScalarFormatting, UInt) {
    test_uint(0, YmIntFmt_Dec, "0u");
    test_uint(1, YmIntFmt_Dec, "1u");
    test_uint(2, YmIntFmt_Dec, "2u");
    test_uint(3, YmIntFmt_Dec, "3u");
    test_uint(4, YmIntFmt_Dec, "4u");
    test_uint(5, YmIntFmt_Dec, "5u");
    test_uint(6, YmIntFmt_Dec, "6u");
    test_uint(7, YmIntFmt_Dec, "7u");
    test_uint(8, YmIntFmt_Dec, "8u");
    test_uint(9, YmIntFmt_Dec, "9u");
    test_uint(10, YmIntFmt_Dec, "10u");
    test_uint(1234567890, YmIntFmt_Dec, "1234567890u");

    test_uint(0, YmIntFmt_Hex, "0x0u");
    test_uint(1, YmIntFmt_Hex, "0x1u");
    test_uint(2, YmIntFmt_Hex, "0x2u");
    test_uint(3, YmIntFmt_Hex, "0x3u");
    test_uint(4, YmIntFmt_Hex, "0x4u");
    test_uint(5, YmIntFmt_Hex, "0x5u");
    test_uint(6, YmIntFmt_Hex, "0x6u");
    test_uint(7, YmIntFmt_Hex, "0x7u");
    test_uint(8, YmIntFmt_Hex, "0x8u");
    test_uint(9, YmIntFmt_Hex, "0x9u");
    test_uint(10, YmIntFmt_Hex, "0xAu", "0xau");
    test_uint(11, YmIntFmt_Hex, "0xBu", "0xbu");
    test_uint(12, YmIntFmt_Hex, "0xCu", "0xcu");
    test_uint(13, YmIntFmt_Hex, "0xDu", "0xdu");
    test_uint(14, YmIntFmt_Hex, "0xEu", "0xeu");
    test_uint(15, YmIntFmt_Hex, "0xFu", "0xfu");
    test_uint(16, YmIntFmt_Hex, "0x10u");
    test_uint(0x1f2e3d4c5b4a, YmIntFmt_Hex, "0x1F2E3D4C5B4Au", "0x1f2e3d4c5b4au");

    test_uint(0, YmIntFmt_Bin, "0b0u");
    test_uint(1, YmIntFmt_Bin, "0b1u");
    test_uint(2, YmIntFmt_Bin, "0b10u");
    test_uint(0b10101100, YmIntFmt_Bin, "0b10101100u");
}

TEST(ScalarFormatting, Float) {
    // TODO: It's really hard to properly unit test ymFmtFloat, as it basically
    //       just wraps std::format, and its behaviour is hard for us to predict,
    //       since for any given float value, there's multiple equally valid ways
    //       of expressing it.
    //
    //       Maybe try at some point to write better tests for this.

    test_float(0.0, "0");
    test_float(3.14159, "3.14159");
    test_float(YM_INF, "inf");
}

TEST(ScalarFormatting, Bool) {
    ASSERT_TRUE(ymFmtBool(YM_TRUE));
    ASSERT_TRUE(ymFmtBool(YM_FALSE));
    EXPECT_STREQ(ymFmtBool(YM_TRUE), "true");
    EXPECT_STREQ(ymFmtBool(YM_FALSE), "false");
}

TEST(ScalarFormatting, Rune) {
    for (YmRune c = 0; c <= 127; c++) {
        const auto escapeSeqChars = std::u32string_view(U"\0\a\b\f\n\r\t\v\'\"\\", 12);
        if (taul::in_set(escapeSeqChars, c)) {
            continue;
        }
        if (taul::is_visible_ascii(c)) {
            test_rune(c, std::format("{}", (YmChar)c), bool(), bool(), bool());
        }
        else {
            size_t low = size_t(c) % 16;
            size_t high = size_t(c) / 16;
            std::string hex_uc = std::format("\\x{}{}", taul::hex_uppercase[high], taul::hex_uppercase[low]);
            std::string hex_lc = std::format("\\x{}{}", taul::hex_lowercase[high], taul::hex_lowercase[low]);
            test_rune(c, hex_uc, hex_lc, bool(), bool(), bool());
        }
    }

    test_rune(U'\0', "\\0", bool(), bool(), bool());
    test_rune(U'\a', "\\a", bool(), bool(), bool());
    test_rune(U'\b', "\\b", bool(), bool(), bool());
    test_rune(U'\f', "\\f", bool(), bool(), bool());
    test_rune(U'\n', "\\n", bool(), bool(), bool());
    test_rune(U'\r', "\\r", bool(), bool(), bool());
    test_rune(U'\t', "\\t", bool(), bool(), bool());
    test_rune(U'\v', "\\v", bool(), bool(), bool());

    test_rune(U'\'', "\\'", true, bool(), bool());
    test_rune(U'\'', "'", false, bool(), bool());
    
    test_rune(U'\"', "\\\"", bool(), true, bool());
    test_rune(U'\"', "\"", bool(), false, bool());
    
    test_rune(U'\\', "\\\\", bool(), bool(), true);
    test_rune(U'\\', "\\", bool(), bool(), false);

    test_rune(U'\x80', "\\x80", bool(), bool(), bool());
    test_rune(U'Δ', "\\u0394", bool(), bool(), bool());
    test_rune(U'魂', "\\u9B42", "\\u9b42", bool(), bool(), bool());
    test_rune(U'💩', "\\U0001F4A9", "\\U0001f4a9", bool(), bool(), bool());

    test_rune(YmRune(0xd8a2), "\\uD8A2", "\\ud8a2", bool(), bool(), bool());
    test_rune(YmRune(0x110000), "\\uFFFD", "\\ufffd", bool(), bool(), bool()); // � == U+FFFD
}

