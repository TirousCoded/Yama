

#include <gtest/gtest.h>

#include <taul/strings.h>

#include <yama/core/scalars.h>


TEST(ScalarsTests, FmtIntDec) {
    EXPECT_EQ(yama::fmt_int_dec(0), "0");

    EXPECT_EQ(yama::fmt_int_dec(1), "1");
    EXPECT_EQ(yama::fmt_int_dec(2), "2");
    EXPECT_EQ(yama::fmt_int_dec(3), "3");
    EXPECT_EQ(yama::fmt_int_dec(4), "4");
    EXPECT_EQ(yama::fmt_int_dec(5), "5");
    EXPECT_EQ(yama::fmt_int_dec(6), "6");
    EXPECT_EQ(yama::fmt_int_dec(7), "7");
    EXPECT_EQ(yama::fmt_int_dec(8), "8");
    EXPECT_EQ(yama::fmt_int_dec(9), "9");
    EXPECT_EQ(yama::fmt_int_dec(10), "10");
    EXPECT_EQ(yama::fmt_int_dec(1234567890), "1234567890");

    EXPECT_EQ(yama::fmt_int_dec(-1), "-1");
    EXPECT_EQ(yama::fmt_int_dec(-2), "-2");
    EXPECT_EQ(yama::fmt_int_dec(-3), "-3");
    EXPECT_EQ(yama::fmt_int_dec(-4), "-4");
    EXPECT_EQ(yama::fmt_int_dec(-5), "-5");
    EXPECT_EQ(yama::fmt_int_dec(-6), "-6");
    EXPECT_EQ(yama::fmt_int_dec(-7), "-7");
    EXPECT_EQ(yama::fmt_int_dec(-8), "-8");
    EXPECT_EQ(yama::fmt_int_dec(-9), "-9");
    EXPECT_EQ(yama::fmt_int_dec(-10), "-10");
    EXPECT_EQ(yama::fmt_int_dec(-1234567890), "-1234567890");
}

TEST(ScalarsTests, FmtIntHex) {
    EXPECT_EQ(yama::fmt_int_hex(0), "0x0");

    EXPECT_EQ(yama::fmt_int_hex(1), "0x1");
    EXPECT_EQ(yama::fmt_int_hex(2), "0x2");
    EXPECT_EQ(yama::fmt_int_hex(3), "0x3");
    EXPECT_EQ(yama::fmt_int_hex(4), "0x4");
    EXPECT_EQ(yama::fmt_int_hex(5), "0x5");
    EXPECT_EQ(yama::fmt_int_hex(6), "0x6");
    EXPECT_EQ(yama::fmt_int_hex(7), "0x7");
    EXPECT_EQ(yama::fmt_int_hex(8), "0x8");
    EXPECT_EQ(yama::fmt_int_hex(9), "0x9");
    EXPECT_EQ(yama::fmt_int_hex(10), "0xa");
    EXPECT_EQ(yama::fmt_int_hex(11), "0xb");
    EXPECT_EQ(yama::fmt_int_hex(12), "0xc");
    EXPECT_EQ(yama::fmt_int_hex(13), "0xd");
    EXPECT_EQ(yama::fmt_int_hex(14), "0xe");
    EXPECT_EQ(yama::fmt_int_hex(15), "0xf");
    EXPECT_EQ(yama::fmt_int_hex(10, true), "0xA");
    EXPECT_EQ(yama::fmt_int_hex(11, true), "0xB");
    EXPECT_EQ(yama::fmt_int_hex(12, true), "0xC");
    EXPECT_EQ(yama::fmt_int_hex(13, true), "0xD");
    EXPECT_EQ(yama::fmt_int_hex(14, true), "0xE");
    EXPECT_EQ(yama::fmt_int_hex(15, true), "0xF");
    EXPECT_EQ(yama::fmt_int_hex(16), "0x10");
    EXPECT_EQ(yama::fmt_int_hex(0x1f2e3d4c5b4a), "0x1f2e3d4c5b4a");
    EXPECT_EQ(yama::fmt_int_hex(0x1f2e3d4c5b4a, true), "0x1F2E3D4C5B4A");

    EXPECT_EQ(yama::fmt_int_hex(-1), "-0x1");
    EXPECT_EQ(yama::fmt_int_hex(-2), "-0x2");
    EXPECT_EQ(yama::fmt_int_hex(-3), "-0x3");
    EXPECT_EQ(yama::fmt_int_hex(-4), "-0x4");
    EXPECT_EQ(yama::fmt_int_hex(-5), "-0x5");
    EXPECT_EQ(yama::fmt_int_hex(-6), "-0x6");
    EXPECT_EQ(yama::fmt_int_hex(-7), "-0x7");
    EXPECT_EQ(yama::fmt_int_hex(-8), "-0x8");
    EXPECT_EQ(yama::fmt_int_hex(-9), "-0x9");
    EXPECT_EQ(yama::fmt_int_hex(-10), "-0xa");
    EXPECT_EQ(yama::fmt_int_hex(-11), "-0xb");
    EXPECT_EQ(yama::fmt_int_hex(-12), "-0xc");
    EXPECT_EQ(yama::fmt_int_hex(-13), "-0xd");
    EXPECT_EQ(yama::fmt_int_hex(-14), "-0xe");
    EXPECT_EQ(yama::fmt_int_hex(-15), "-0xf");
    EXPECT_EQ(yama::fmt_int_hex(-10, true), "-0xA");
    EXPECT_EQ(yama::fmt_int_hex(-11, true), "-0xB");
    EXPECT_EQ(yama::fmt_int_hex(-12, true), "-0xC");
    EXPECT_EQ(yama::fmt_int_hex(-13, true), "-0xD");
    EXPECT_EQ(yama::fmt_int_hex(-14, true), "-0xE");
    EXPECT_EQ(yama::fmt_int_hex(-15, true), "-0xF");
    EXPECT_EQ(yama::fmt_int_hex(-16), "-0x10");
    EXPECT_EQ(yama::fmt_int_hex(-0x1f2e3d4c5b4a), "-0x1f2e3d4c5b4a");
    EXPECT_EQ(yama::fmt_int_hex(-0x1f2e3d4c5b4a, true), "-0x1F2E3D4C5B4A");
}

TEST(ScalarsTests, FmtIntBin) {
    EXPECT_EQ(yama::fmt_int_bin(0), "0b0");

    EXPECT_EQ(yama::fmt_int_bin(1), "0b1");
    EXPECT_EQ(yama::fmt_int_bin(2), "0b10");
    EXPECT_EQ(yama::fmt_int_bin(0b10101100), "0b10101100");

    EXPECT_EQ(yama::fmt_int_bin(-1), "-0b1");
    EXPECT_EQ(yama::fmt_int_bin(-2), "-0b10");
    EXPECT_EQ(yama::fmt_int_bin(-0b10101100), "-0b10101100");
}

TEST(ScalarsTests, FmtInt) {
    EXPECT_EQ(yama::fmt_int(0, yama::int_fmt::dec), "0");

    EXPECT_EQ(yama::fmt_int(1, yama::int_fmt::dec), "1");
    EXPECT_EQ(yama::fmt_int(2, yama::int_fmt::dec), "2");
    EXPECT_EQ(yama::fmt_int(3, yama::int_fmt::dec), "3");
    EXPECT_EQ(yama::fmt_int(4, yama::int_fmt::dec), "4");
    EXPECT_EQ(yama::fmt_int(5, yama::int_fmt::dec), "5");
    EXPECT_EQ(yama::fmt_int(6, yama::int_fmt::dec), "6");
    EXPECT_EQ(yama::fmt_int(7, yama::int_fmt::dec), "7");
    EXPECT_EQ(yama::fmt_int(8, yama::int_fmt::dec), "8");
    EXPECT_EQ(yama::fmt_int(9, yama::int_fmt::dec), "9");
    EXPECT_EQ(yama::fmt_int(10, yama::int_fmt::dec), "10");
    EXPECT_EQ(yama::fmt_int(1234567890, yama::int_fmt::dec), "1234567890");

    EXPECT_EQ(yama::fmt_int(-1, yama::int_fmt::dec), "-1");
    EXPECT_EQ(yama::fmt_int(-2, yama::int_fmt::dec), "-2");
    EXPECT_EQ(yama::fmt_int(-3, yama::int_fmt::dec), "-3");
    EXPECT_EQ(yama::fmt_int(-4, yama::int_fmt::dec), "-4");
    EXPECT_EQ(yama::fmt_int(-5, yama::int_fmt::dec), "-5");
    EXPECT_EQ(yama::fmt_int(-6, yama::int_fmt::dec), "-6");
    EXPECT_EQ(yama::fmt_int(-7, yama::int_fmt::dec), "-7");
    EXPECT_EQ(yama::fmt_int(-8, yama::int_fmt::dec), "-8");
    EXPECT_EQ(yama::fmt_int(-9, yama::int_fmt::dec), "-9");
    EXPECT_EQ(yama::fmt_int(-10, yama::int_fmt::dec), "-10");
    EXPECT_EQ(yama::fmt_int(-1234567890, yama::int_fmt::dec), "-1234567890");

    EXPECT_EQ(yama::fmt_int(0, yama::int_fmt::hex), "0x0");

    EXPECT_EQ(yama::fmt_int(1, yama::int_fmt::hex), "0x1");
    EXPECT_EQ(yama::fmt_int(2, yama::int_fmt::hex), "0x2");
    EXPECT_EQ(yama::fmt_int(3, yama::int_fmt::hex), "0x3");
    EXPECT_EQ(yama::fmt_int(4, yama::int_fmt::hex), "0x4");
    EXPECT_EQ(yama::fmt_int(5, yama::int_fmt::hex), "0x5");
    EXPECT_EQ(yama::fmt_int(6, yama::int_fmt::hex), "0x6");
    EXPECT_EQ(yama::fmt_int(7, yama::int_fmt::hex), "0x7");
    EXPECT_EQ(yama::fmt_int(8, yama::int_fmt::hex), "0x8");
    EXPECT_EQ(yama::fmt_int(9, yama::int_fmt::hex), "0x9");
    EXPECT_EQ(yama::fmt_int(10, yama::int_fmt::hex), "0xa");
    EXPECT_EQ(yama::fmt_int(11, yama::int_fmt::hex), "0xb");
    EXPECT_EQ(yama::fmt_int(12, yama::int_fmt::hex), "0xc");
    EXPECT_EQ(yama::fmt_int(13, yama::int_fmt::hex), "0xd");
    EXPECT_EQ(yama::fmt_int(14, yama::int_fmt::hex), "0xe");
    EXPECT_EQ(yama::fmt_int(15, yama::int_fmt::hex), "0xf");
    EXPECT_EQ(yama::fmt_int(10, yama::int_fmt::hex, true), "0xA");
    EXPECT_EQ(yama::fmt_int(11, yama::int_fmt::hex, true), "0xB");
    EXPECT_EQ(yama::fmt_int(12, yama::int_fmt::hex, true), "0xC");
    EXPECT_EQ(yama::fmt_int(13, yama::int_fmt::hex, true), "0xD");
    EXPECT_EQ(yama::fmt_int(14, yama::int_fmt::hex, true), "0xE");
    EXPECT_EQ(yama::fmt_int(15, yama::int_fmt::hex, true), "0xF");
    EXPECT_EQ(yama::fmt_int(16, yama::int_fmt::hex), "0x10");
    EXPECT_EQ(yama::fmt_int(0x1f2e3d4c5b4a, yama::int_fmt::hex), "0x1f2e3d4c5b4a");
    EXPECT_EQ(yama::fmt_int(0x1f2e3d4c5b4a, yama::int_fmt::hex, true), "0x1F2E3D4C5B4A");

    EXPECT_EQ(yama::fmt_int(-1, yama::int_fmt::hex), "-0x1");
    EXPECT_EQ(yama::fmt_int(-2, yama::int_fmt::hex), "-0x2");
    EXPECT_EQ(yama::fmt_int(-3, yama::int_fmt::hex), "-0x3");
    EXPECT_EQ(yama::fmt_int(-4, yama::int_fmt::hex), "-0x4");
    EXPECT_EQ(yama::fmt_int(-5, yama::int_fmt::hex), "-0x5");
    EXPECT_EQ(yama::fmt_int(-6, yama::int_fmt::hex), "-0x6");
    EXPECT_EQ(yama::fmt_int(-7, yama::int_fmt::hex), "-0x7");
    EXPECT_EQ(yama::fmt_int(-8, yama::int_fmt::hex), "-0x8");
    EXPECT_EQ(yama::fmt_int(-9, yama::int_fmt::hex), "-0x9");
    EXPECT_EQ(yama::fmt_int(-10, yama::int_fmt::hex), "-0xa");
    EXPECT_EQ(yama::fmt_int(-11, yama::int_fmt::hex), "-0xb");
    EXPECT_EQ(yama::fmt_int(-12, yama::int_fmt::hex), "-0xc");
    EXPECT_EQ(yama::fmt_int(-13, yama::int_fmt::hex), "-0xd");
    EXPECT_EQ(yama::fmt_int(-14, yama::int_fmt::hex), "-0xe");
    EXPECT_EQ(yama::fmt_int(-15, yama::int_fmt::hex), "-0xf");
    EXPECT_EQ(yama::fmt_int(-10, yama::int_fmt::hex, true), "-0xA");
    EXPECT_EQ(yama::fmt_int(-11, yama::int_fmt::hex, true), "-0xB");
    EXPECT_EQ(yama::fmt_int(-12, yama::int_fmt::hex, true), "-0xC");
    EXPECT_EQ(yama::fmt_int(-13, yama::int_fmt::hex, true), "-0xD");
    EXPECT_EQ(yama::fmt_int(-14, yama::int_fmt::hex, true), "-0xE");
    EXPECT_EQ(yama::fmt_int(-15, yama::int_fmt::hex, true), "-0xF");
    EXPECT_EQ(yama::fmt_int(-16, yama::int_fmt::hex), "-0x10");
    EXPECT_EQ(yama::fmt_int(-0x1f2e3d4c5b4a, yama::int_fmt::hex), "-0x1f2e3d4c5b4a");
    EXPECT_EQ(yama::fmt_int(-0x1f2e3d4c5b4a, yama::int_fmt::hex, true), "-0x1F2E3D4C5B4A");

    EXPECT_EQ(yama::fmt_int(0, yama::int_fmt::bin), "0b0");

    EXPECT_EQ(yama::fmt_int(1, yama::int_fmt::bin), "0b1");
    EXPECT_EQ(yama::fmt_int(2, yama::int_fmt::bin), "0b10");
    EXPECT_EQ(yama::fmt_int(0b10101100, yama::int_fmt::bin), "0b10101100");

    EXPECT_EQ(yama::fmt_int(-1, yama::int_fmt::bin), "-0b1");
    EXPECT_EQ(yama::fmt_int(-2, yama::int_fmt::bin), "-0b10");
    EXPECT_EQ(yama::fmt_int(-0b10101100, yama::int_fmt::bin), "-0b10101100");
}

TEST(ScalarsTests, FmtUIntDec) {
    EXPECT_EQ(yama::fmt_uint_dec(0), "0u");
    EXPECT_EQ(yama::fmt_uint_dec(1), "1u");
    EXPECT_EQ(yama::fmt_uint_dec(2), "2u");
    EXPECT_EQ(yama::fmt_uint_dec(3), "3u");
    EXPECT_EQ(yama::fmt_uint_dec(4), "4u");
    EXPECT_EQ(yama::fmt_uint_dec(5), "5u");
    EXPECT_EQ(yama::fmt_uint_dec(6), "6u");
    EXPECT_EQ(yama::fmt_uint_dec(7), "7u");
    EXPECT_EQ(yama::fmt_uint_dec(8), "8u");
    EXPECT_EQ(yama::fmt_uint_dec(9), "9u");
    EXPECT_EQ(yama::fmt_uint_dec(10), "10u");
    EXPECT_EQ(yama::fmt_uint_dec(1234567890), "1234567890u");
}

TEST(ScalarsTests, FmtUIntHex) {
    EXPECT_EQ(yama::fmt_uint_hex(0), "0x0u");
    EXPECT_EQ(yama::fmt_uint_hex(1), "0x1u");
    EXPECT_EQ(yama::fmt_uint_hex(2), "0x2u");
    EXPECT_EQ(yama::fmt_uint_hex(3), "0x3u");
    EXPECT_EQ(yama::fmt_uint_hex(4), "0x4u");
    EXPECT_EQ(yama::fmt_uint_hex(5), "0x5u");
    EXPECT_EQ(yama::fmt_uint_hex(6), "0x6u");
    EXPECT_EQ(yama::fmt_uint_hex(7), "0x7u");
    EXPECT_EQ(yama::fmt_uint_hex(8), "0x8u");
    EXPECT_EQ(yama::fmt_uint_hex(9), "0x9u");
    EXPECT_EQ(yama::fmt_uint_hex(10), "0xau");
    EXPECT_EQ(yama::fmt_uint_hex(11), "0xbu");
    EXPECT_EQ(yama::fmt_uint_hex(12), "0xcu");
    EXPECT_EQ(yama::fmt_uint_hex(13), "0xdu");
    EXPECT_EQ(yama::fmt_uint_hex(14), "0xeu");
    EXPECT_EQ(yama::fmt_uint_hex(15), "0xfu");
    EXPECT_EQ(yama::fmt_uint_hex(10, true), "0xAu");
    EXPECT_EQ(yama::fmt_uint_hex(11, true), "0xBu");
    EXPECT_EQ(yama::fmt_uint_hex(12, true), "0xCu");
    EXPECT_EQ(yama::fmt_uint_hex(13, true), "0xDu");
    EXPECT_EQ(yama::fmt_uint_hex(14, true), "0xEu");
    EXPECT_EQ(yama::fmt_uint_hex(15, true), "0xFu");
    EXPECT_EQ(yama::fmt_uint_hex(16), "0x10u");
    EXPECT_EQ(yama::fmt_uint_hex(0x1f2e3d4c5b4a), "0x1f2e3d4c5b4au");
    EXPECT_EQ(yama::fmt_uint_hex(0x1f2e3d4c5b4a, true), "0x1F2E3D4C5B4Au");
}

TEST(ScalarsTests, FmtUIntBin) {
    EXPECT_EQ(yama::fmt_uint_bin(0), "0b0u");
    EXPECT_EQ(yama::fmt_uint_bin(1), "0b1u");
    EXPECT_EQ(yama::fmt_uint_bin(2), "0b10u");
    EXPECT_EQ(yama::fmt_uint_bin(0b10101100), "0b10101100u");
}

TEST(ScalarsTests, FmtUInt) {
    EXPECT_EQ(yama::fmt_uint(0, yama::int_fmt::dec), "0u");
    EXPECT_EQ(yama::fmt_uint(1, yama::int_fmt::dec), "1u");
    EXPECT_EQ(yama::fmt_uint(2, yama::int_fmt::dec), "2u");
    EXPECT_EQ(yama::fmt_uint(3, yama::int_fmt::dec), "3u");
    EXPECT_EQ(yama::fmt_uint(4, yama::int_fmt::dec), "4u");
    EXPECT_EQ(yama::fmt_uint(5, yama::int_fmt::dec), "5u");
    EXPECT_EQ(yama::fmt_uint(6, yama::int_fmt::dec), "6u");
    EXPECT_EQ(yama::fmt_uint(7, yama::int_fmt::dec), "7u");
    EXPECT_EQ(yama::fmt_uint(8, yama::int_fmt::dec), "8u");
    EXPECT_EQ(yama::fmt_uint(9, yama::int_fmt::dec), "9u");
    EXPECT_EQ(yama::fmt_uint(10, yama::int_fmt::dec), "10u");
    EXPECT_EQ(yama::fmt_uint(1234567890, yama::int_fmt::dec), "1234567890u");

    EXPECT_EQ(yama::fmt_uint(0, yama::int_fmt::hex), "0x0u");
    EXPECT_EQ(yama::fmt_uint(1, yama::int_fmt::hex), "0x1u");
    EXPECT_EQ(yama::fmt_uint(2, yama::int_fmt::hex), "0x2u");
    EXPECT_EQ(yama::fmt_uint(3, yama::int_fmt::hex), "0x3u");
    EXPECT_EQ(yama::fmt_uint(4, yama::int_fmt::hex), "0x4u");
    EXPECT_EQ(yama::fmt_uint(5, yama::int_fmt::hex), "0x5u");
    EXPECT_EQ(yama::fmt_uint(6, yama::int_fmt::hex), "0x6u");
    EXPECT_EQ(yama::fmt_uint(7, yama::int_fmt::hex), "0x7u");
    EXPECT_EQ(yama::fmt_uint(8, yama::int_fmt::hex), "0x8u");
    EXPECT_EQ(yama::fmt_uint(9, yama::int_fmt::hex), "0x9u");
    EXPECT_EQ(yama::fmt_uint(10, yama::int_fmt::hex), "0xau");
    EXPECT_EQ(yama::fmt_uint(11, yama::int_fmt::hex), "0xbu");
    EXPECT_EQ(yama::fmt_uint(12, yama::int_fmt::hex), "0xcu");
    EXPECT_EQ(yama::fmt_uint(13, yama::int_fmt::hex), "0xdu");
    EXPECT_EQ(yama::fmt_uint(14, yama::int_fmt::hex), "0xeu");
    EXPECT_EQ(yama::fmt_uint(15, yama::int_fmt::hex), "0xfu");
    EXPECT_EQ(yama::fmt_uint(10, yama::int_fmt::hex, true), "0xAu");
    EXPECT_EQ(yama::fmt_uint(11, yama::int_fmt::hex, true), "0xBu");
    EXPECT_EQ(yama::fmt_uint(12, yama::int_fmt::hex, true), "0xCu");
    EXPECT_EQ(yama::fmt_uint(13, yama::int_fmt::hex, true), "0xDu");
    EXPECT_EQ(yama::fmt_uint(14, yama::int_fmt::hex, true), "0xEu");
    EXPECT_EQ(yama::fmt_uint(15, yama::int_fmt::hex, true), "0xFu");
    EXPECT_EQ(yama::fmt_uint(16, yama::int_fmt::hex), "0x10u");
    EXPECT_EQ(yama::fmt_uint(0x1f2e3d4c5b4a, yama::int_fmt::hex), "0x1f2e3d4c5b4au");
    EXPECT_EQ(yama::fmt_uint(0x1f2e3d4c5b4a, yama::int_fmt::hex, true), "0x1F2E3D4C5B4Au");

    EXPECT_EQ(yama::fmt_uint(0, yama::int_fmt::bin), "0b0u");
    EXPECT_EQ(yama::fmt_uint(1, yama::int_fmt::bin), "0b1u");
    EXPECT_EQ(yama::fmt_uint(2, yama::int_fmt::bin), "0b10u");
    EXPECT_EQ(yama::fmt_uint(0b10101100, yama::int_fmt::bin), "0b10101100u");
}

TEST(ScalarsTests, FmtFloat) {
    // TODO: it's really hard to properly unit test fmt_float, as it
    //       basically just wraps std::format, and its behaviour is
    //       hard for us to predict, since for any given float value,
    //       there's multiple equally-valid ways of expressing it
    //
    //       maybe try at some point to write some proper tests for this
}

TEST(ScalarsTests, FmtBool) {
    EXPECT_EQ(yama::fmt_bool(true), "true");
    EXPECT_EQ(yama::fmt_bool(false), "false");
}

TEST(ScalarsTests, FmtChar) {
    for (yama::char_t c = 0; c <= 127; c++) {
        if (c == U'\0') {
            EXPECT_EQ(yama::fmt_char(c), "\\0") << std::format("size_t(c) == {}", size_t(c));
        }
        else if (c == U'\a') {
            EXPECT_EQ(yama::fmt_char(c), "\\a") << std::format("size_t(c) == {}", size_t(c));
        }
        else if (c == U'\b') {
            EXPECT_EQ(yama::fmt_char(c), "\\b") << std::format("size_t(c) == {}", size_t(c));
        }
        else if (c == U'\f') {
            EXPECT_EQ(yama::fmt_char(c), "\\f") << std::format("size_t(c) == {}", size_t(c));
        }
        else if (c == U'\n') {
            EXPECT_EQ(yama::fmt_char(c), "\\n") << std::format("size_t(c) == {}", size_t(c));
        }
        else if (c == U'\r') {
            EXPECT_EQ(yama::fmt_char(c), "\\r") << std::format("size_t(c) == {}", size_t(c));
        }
        else if (c == U'\t') {
            EXPECT_EQ(yama::fmt_char(c), "\\t") << std::format("size_t(c) == {}", size_t(c));
        }
        else if (c == U'\v') {
            EXPECT_EQ(yama::fmt_char(c), "\\v") << std::format("size_t(c) == {}", size_t(c));
        }
        else if (c == U'\'') {
            EXPECT_EQ(yama::fmt_char(c), "\\'") << std::format("size_t(c) == {}", size_t(c));
            EXPECT_EQ(yama::fmt_char(c, false, false), "'") << std::format("size_t(c) == {}", size_t(c));
        }
        else if (c == U'"') {
            EXPECT_EQ(yama::fmt_char(c), "\\\"") << std::format("size_t(c) == {}", size_t(c));
            EXPECT_EQ(yama::fmt_char(c, false, false, false), "\"") << std::format("size_t(c) == {}", size_t(c));
        }
        else if (c == U'\\') {
            EXPECT_EQ(yama::fmt_char(c), "\\\\") << std::format("size_t(c) == {}", size_t(c));
            EXPECT_EQ(yama::fmt_char(c, false, false, false, false), "\\") << std::format("size_t(c) == {}", size_t(c));
        }
        else if (taul::is_visible_ascii(c)) {
            EXPECT_EQ(yama::fmt_char(c), std::format("{}", char(c))) << std::format("size_t(c) == {}", size_t(c));
        }
        else {
            size_t low = size_t(c) % 16;
            size_t high = size_t(c) / 16;
            EXPECT_EQ(yama::fmt_char(c), std::format("\\x{}{}", taul::hex_lowercase[high], taul::hex_lowercase[low])) << std::format("size_t(c) == {}", size_t(c));
            EXPECT_EQ(yama::fmt_char(c, true), std::format("\\x{}{}", taul::hex_uppercase[high], taul::hex_uppercase[low])) << std::format("size_t(c) == {}", size_t(c));
        }
    }
    // non-ASCII Unicode
    EXPECT_EQ(yama::fmt_char(U'\x80'), "\\x80"); // 1 byte hex escape seq
    EXPECT_EQ(yama::fmt_char(U'Δ'), "\\u0394"); // 2 byte hex escape seq
    EXPECT_EQ(yama::fmt_char(U'魂'), "\\u9b42"); // 2 byte hex escape seq
    EXPECT_EQ(yama::fmt_char(U'💩'), "\\U0001f4a9"); // 4 byte hex escape seq
    // illegal Unicode
    EXPECT_EQ(yama::fmt_char((yama::char_t)0xd8a2), "\\ud8a2"); // UTF-16 surrogate value
    // non-Unicode
    EXPECT_EQ(yama::fmt_char((yama::char_t)0x110000), "?"); // first value after Unicode codespace
}

// need this to make EXPECT_EQ macro not scream at me

template<typename T>
yama::parsed<T> mk(T v, size_t len, bool overflow = false, bool underflow = false) { return { v, len, overflow, underflow }; }

#define _TEST_SUCC(x, y) \
{ \
    const auto _X_ = x; \
    const auto _Y_ = y; \
    EXPECT_TRUE(_X_); \
    if (_X_) { \
        if constexpr (std::is_floating_point_v<decltype(_X_.value().v)>) { \
            EXPECT_DOUBLE_EQ((yama::float_t)_X_.value().v, (yama::float_t)_Y_.v); \
        } \
        else { \
            EXPECT_EQ(_X_.value().v, _Y_.v); \
        } \
        EXPECT_EQ(_X_.value().bytes, _Y_.bytes); \
        EXPECT_EQ(_X_.value().overflow, _Y_.overflow); \
        EXPECT_EQ(_X_.value().underflow, _Y_.underflow); \
    } \
} \
((void)0)

#define _TEST_FAIL(x) \
EXPECT_FALSE(x); \
((void)0)

TEST(ScalarsTests, ParseInt) {
    // decimal

    _TEST_SUCC(yama::parse_int("0"), mk<yama::int_t>(0, 1));
    _TEST_SUCC(yama::parse_int("1"), mk<yama::int_t>(1, 1));
    _TEST_SUCC(yama::parse_int("2"), mk<yama::int_t>(2, 1));
    _TEST_SUCC(yama::parse_int("3"), mk<yama::int_t>(3, 1));
    _TEST_SUCC(yama::parse_int("4"), mk<yama::int_t>(4, 1));
    _TEST_SUCC(yama::parse_int("5"), mk<yama::int_t>(5, 1));
    _TEST_SUCC(yama::parse_int("6"), mk<yama::int_t>(6, 1));
    _TEST_SUCC(yama::parse_int("7"), mk<yama::int_t>(7, 1));
    _TEST_SUCC(yama::parse_int("8"), mk<yama::int_t>(8, 1));
    _TEST_SUCC(yama::parse_int("9"), mk<yama::int_t>(9, 1));

    _TEST_SUCC(yama::parse_int("-0"), mk<yama::int_t>(0, 2));
    _TEST_SUCC(yama::parse_int("-1"), mk<yama::int_t>(-1, 2));
    _TEST_SUCC(yama::parse_int("-2"), mk<yama::int_t>(-2, 2));
    _TEST_SUCC(yama::parse_int("-3"), mk<yama::int_t>(-3, 2));
    _TEST_SUCC(yama::parse_int("-4"), mk<yama::int_t>(-4, 2));
    _TEST_SUCC(yama::parse_int("-5"), mk<yama::int_t>(-5, 2));
    _TEST_SUCC(yama::parse_int("-6"), mk<yama::int_t>(-6, 2));
    _TEST_SUCC(yama::parse_int("-7"), mk<yama::int_t>(-7, 2));
    _TEST_SUCC(yama::parse_int("-8"), mk<yama::int_t>(-8, 2));
    _TEST_SUCC(yama::parse_int("-9"), mk<yama::int_t>(-9, 2));

    _TEST_SUCC(yama::parse_int("0_0_1"), mk<yama::int_t>(1, 5));
    _TEST_SUCC(yama::parse_int("1_0_1"), mk<yama::int_t>(101, 5));
    _TEST_SUCC(yama::parse_int("2_0_1"), mk<yama::int_t>(201, 5));
    _TEST_SUCC(yama::parse_int("3_0_1"), mk<yama::int_t>(301, 5));
    _TEST_SUCC(yama::parse_int("4_0_1"), mk<yama::int_t>(401, 5));
    _TEST_SUCC(yama::parse_int("5_0_1"), mk<yama::int_t>(501, 5));
    _TEST_SUCC(yama::parse_int("6_0_1"), mk<yama::int_t>(601, 5));
    _TEST_SUCC(yama::parse_int("7_0_1"), mk<yama::int_t>(701, 5));
    _TEST_SUCC(yama::parse_int("8_0_1"), mk<yama::int_t>(801, 5));
    _TEST_SUCC(yama::parse_int("9_0_1"), mk<yama::int_t>(901, 5));

    _TEST_SUCC(yama::parse_int("-0_0_1"), mk<yama::int_t>(-1, 6));
    _TEST_SUCC(yama::parse_int("-1_0_1"), mk<yama::int_t>(-101, 6));
    _TEST_SUCC(yama::parse_int("-2_0_1"), mk<yama::int_t>(-201, 6));
    _TEST_SUCC(yama::parse_int("-3_0_1"), mk<yama::int_t>(-301, 6));
    _TEST_SUCC(yama::parse_int("-4_0_1"), mk<yama::int_t>(-401, 6));
    _TEST_SUCC(yama::parse_int("-5_0_1"), mk<yama::int_t>(-501, 6));
    _TEST_SUCC(yama::parse_int("-6_0_1"), mk<yama::int_t>(-601, 6));
    _TEST_SUCC(yama::parse_int("-7_0_1"), mk<yama::int_t>(-701, 6));
    _TEST_SUCC(yama::parse_int("-8_0_1"), mk<yama::int_t>(-801, 6));
    _TEST_SUCC(yama::parse_int("-9_0_1"), mk<yama::int_t>(-901, 6));

    _TEST_SUCC(yama::parse_int("1230"), mk<yama::int_t>(1230, 4));
    _TEST_SUCC(yama::parse_int("1230aa"), mk<yama::int_t>(1230, 4));
    _TEST_SUCC(yama::parse_int("-1230"), mk<yama::int_t>(-1230, 5));
    _TEST_SUCC(yama::parse_int("-1230aa"), mk<yama::int_t>(-1230, 5));
    _TEST_SUCC(yama::parse_int("001230"), mk<yama::int_t>(1230, 6));
    _TEST_SUCC(yama::parse_int("001230aa"), mk<yama::int_t>(1230, 6));
    _TEST_SUCC(yama::parse_int("-001230"), mk<yama::int_t>(-1230, 7));
    _TEST_SUCC(yama::parse_int("-001230aa"), mk<yama::int_t>(-1230, 7));

    // hexadecimal

    _TEST_SUCC(yama::parse_int("0x0"), mk<yama::int_t>(0, 3));
    _TEST_SUCC(yama::parse_int("0x1"), mk<yama::int_t>(1, 3));
    _TEST_SUCC(yama::parse_int("0x2"), mk<yama::int_t>(2, 3));
    _TEST_SUCC(yama::parse_int("0x3"), mk<yama::int_t>(3, 3));
    _TEST_SUCC(yama::parse_int("0x4"), mk<yama::int_t>(4, 3));
    _TEST_SUCC(yama::parse_int("0x5"), mk<yama::int_t>(5, 3));
    _TEST_SUCC(yama::parse_int("0x6"), mk<yama::int_t>(6, 3));
    _TEST_SUCC(yama::parse_int("0x7"), mk<yama::int_t>(7, 3));
    _TEST_SUCC(yama::parse_int("0x8"), mk<yama::int_t>(8, 3));
    _TEST_SUCC(yama::parse_int("0x9"), mk<yama::int_t>(9, 3));

    _TEST_SUCC(yama::parse_int("0xa"), mk<yama::int_t>(10, 3));
    _TEST_SUCC(yama::parse_int("0xb"), mk<yama::int_t>(11, 3));
    _TEST_SUCC(yama::parse_int("0xc"), mk<yama::int_t>(12, 3));
    _TEST_SUCC(yama::parse_int("0xd"), mk<yama::int_t>(13, 3));
    _TEST_SUCC(yama::parse_int("0xe"), mk<yama::int_t>(14, 3));
    _TEST_SUCC(yama::parse_int("0xf"), mk<yama::int_t>(15, 3));

    _TEST_SUCC(yama::parse_int("0xA"), mk<yama::int_t>(10, 3));
    _TEST_SUCC(yama::parse_int("0xB"), mk<yama::int_t>(11, 3));
    _TEST_SUCC(yama::parse_int("0xC"), mk<yama::int_t>(12, 3));
    _TEST_SUCC(yama::parse_int("0xD"), mk<yama::int_t>(13, 3));
    _TEST_SUCC(yama::parse_int("0xE"), mk<yama::int_t>(14, 3));
    _TEST_SUCC(yama::parse_int("0xF"), mk<yama::int_t>(15, 3));
    
    _TEST_SUCC(yama::parse_int("-0x0"), mk<yama::int_t>(-0, 4));
    _TEST_SUCC(yama::parse_int("-0x1"), mk<yama::int_t>(-1, 4));
    _TEST_SUCC(yama::parse_int("-0x2"), mk<yama::int_t>(-2, 4));
    _TEST_SUCC(yama::parse_int("-0x3"), mk<yama::int_t>(-3, 4));
    _TEST_SUCC(yama::parse_int("-0x4"), mk<yama::int_t>(-4, 4));
    _TEST_SUCC(yama::parse_int("-0x5"), mk<yama::int_t>(-5, 4));
    _TEST_SUCC(yama::parse_int("-0x6"), mk<yama::int_t>(-6, 4));
    _TEST_SUCC(yama::parse_int("-0x7"), mk<yama::int_t>(-7, 4));
    _TEST_SUCC(yama::parse_int("-0x8"), mk<yama::int_t>(-8, 4));
    _TEST_SUCC(yama::parse_int("-0x9"), mk<yama::int_t>(-9, 4));

    _TEST_SUCC(yama::parse_int("-0xa"), mk<yama::int_t>(-10, 4));
    _TEST_SUCC(yama::parse_int("-0xb"), mk<yama::int_t>(-11, 4));
    _TEST_SUCC(yama::parse_int("-0xc"), mk<yama::int_t>(-12, 4));
    _TEST_SUCC(yama::parse_int("-0xd"), mk<yama::int_t>(-13, 4));
    _TEST_SUCC(yama::parse_int("-0xe"), mk<yama::int_t>(-14, 4));
    _TEST_SUCC(yama::parse_int("-0xf"), mk<yama::int_t>(-15, 4));

    _TEST_SUCC(yama::parse_int("-0xA"), mk<yama::int_t>(-10, 4));
    _TEST_SUCC(yama::parse_int("-0xB"), mk<yama::int_t>(-11, 4));
    _TEST_SUCC(yama::parse_int("-0xC"), mk<yama::int_t>(-12, 4));
    _TEST_SUCC(yama::parse_int("-0xD"), mk<yama::int_t>(-13, 4));
    _TEST_SUCC(yama::parse_int("-0xE"), mk<yama::int_t>(-14, 4));
    _TEST_SUCC(yama::parse_int("-0xF"), mk<yama::int_t>(-15, 4));

    _TEST_SUCC(yama::parse_int("0x0_aE_3"), mk<yama::int_t>(0x0ae3, 8));
    _TEST_SUCC(yama::parse_int("0x1_aE_3"), mk<yama::int_t>(0x1ae3, 8));
    _TEST_SUCC(yama::parse_int("0x2_aE_3"), mk<yama::int_t>(0x2ae3, 8));
    _TEST_SUCC(yama::parse_int("0x3_aE_3"), mk<yama::int_t>(0x3ae3, 8));
    _TEST_SUCC(yama::parse_int("0x4_aE_3"), mk<yama::int_t>(0x4ae3, 8));
    _TEST_SUCC(yama::parse_int("0x5_aE_3"), mk<yama::int_t>(0x5ae3, 8));
    _TEST_SUCC(yama::parse_int("0x6_aE_3"), mk<yama::int_t>(0x6ae3, 8));
    _TEST_SUCC(yama::parse_int("0x7_aE_3"), mk<yama::int_t>(0x7ae3, 8));
    _TEST_SUCC(yama::parse_int("0x8_aE_3"), mk<yama::int_t>(0x8ae3, 8));
    _TEST_SUCC(yama::parse_int("0x9_aE_3"), mk<yama::int_t>(0x9ae3, 8));

    _TEST_SUCC(yama::parse_int("0xa_aE_3"), mk<yama::int_t>(0xaae3, 8));
    _TEST_SUCC(yama::parse_int("0xb_aE_3"), mk<yama::int_t>(0xbae3, 8));
    _TEST_SUCC(yama::parse_int("0xc_aE_3"), mk<yama::int_t>(0xcae3, 8));
    _TEST_SUCC(yama::parse_int("0xd_aE_3"), mk<yama::int_t>(0xdae3, 8));
    _TEST_SUCC(yama::parse_int("0xe_aE_3"), mk<yama::int_t>(0xeae3, 8));
    _TEST_SUCC(yama::parse_int("0xf_aE_3"), mk<yama::int_t>(0xfae3, 8));

    _TEST_SUCC(yama::parse_int("0xA_aE_3"), mk<yama::int_t>(0xaae3, 8));
    _TEST_SUCC(yama::parse_int("0xB_aE_3"), mk<yama::int_t>(0xbae3, 8));
    _TEST_SUCC(yama::parse_int("0xC_aE_3"), mk<yama::int_t>(0xcae3, 8));
    _TEST_SUCC(yama::parse_int("0xD_aE_3"), mk<yama::int_t>(0xdae3, 8));
    _TEST_SUCC(yama::parse_int("0xE_aE_3"), mk<yama::int_t>(0xeae3, 8));
    _TEST_SUCC(yama::parse_int("0xF_aE_3"), mk<yama::int_t>(0xfae3, 8));
    
    _TEST_SUCC(yama::parse_int("-0x0_aE_3"), mk<yama::int_t>(-0x0ae3, 9));
    _TEST_SUCC(yama::parse_int("-0x1_aE_3"), mk<yama::int_t>(-0x1ae3, 9));
    _TEST_SUCC(yama::parse_int("-0x2_aE_3"), mk<yama::int_t>(-0x2ae3, 9));
    _TEST_SUCC(yama::parse_int("-0x3_aE_3"), mk<yama::int_t>(-0x3ae3, 9));
    _TEST_SUCC(yama::parse_int("-0x4_aE_3"), mk<yama::int_t>(-0x4ae3, 9));
    _TEST_SUCC(yama::parse_int("-0x5_aE_3"), mk<yama::int_t>(-0x5ae3, 9));
    _TEST_SUCC(yama::parse_int("-0x6_aE_3"), mk<yama::int_t>(-0x6ae3, 9));
    _TEST_SUCC(yama::parse_int("-0x7_aE_3"), mk<yama::int_t>(-0x7ae3, 9));
    _TEST_SUCC(yama::parse_int("-0x8_aE_3"), mk<yama::int_t>(-0x8ae3, 9));
    _TEST_SUCC(yama::parse_int("-0x9_aE_3"), mk<yama::int_t>(-0x9ae3, 9));

    _TEST_SUCC(yama::parse_int("-0xa_aE_3"), mk<yama::int_t>(-0xaae3, 9));
    _TEST_SUCC(yama::parse_int("-0xb_aE_3"), mk<yama::int_t>(-0xbae3, 9));
    _TEST_SUCC(yama::parse_int("-0xc_aE_3"), mk<yama::int_t>(-0xcae3, 9));
    _TEST_SUCC(yama::parse_int("-0xd_aE_3"), mk<yama::int_t>(-0xdae3, 9));
    _TEST_SUCC(yama::parse_int("-0xe_aE_3"), mk<yama::int_t>(-0xeae3, 9));
    _TEST_SUCC(yama::parse_int("-0xf_aE_3"), mk<yama::int_t>(-0xfae3, 9));

    _TEST_SUCC(yama::parse_int("-0xA_aE_3"), mk<yama::int_t>(-0xaae3, 9));
    _TEST_SUCC(yama::parse_int("-0xB_aE_3"), mk<yama::int_t>(-0xbae3, 9));
    _TEST_SUCC(yama::parse_int("-0xC_aE_3"), mk<yama::int_t>(-0xcae3, 9));
    _TEST_SUCC(yama::parse_int("-0xD_aE_3"), mk<yama::int_t>(-0xdae3, 9));
    _TEST_SUCC(yama::parse_int("-0xE_aE_3"), mk<yama::int_t>(-0xeae3, 9));
    _TEST_SUCC(yama::parse_int("-0xF_aE_3"), mk<yama::int_t>(-0xfae3, 9));

    _TEST_SUCC(yama::parse_int("0x1aC23e4f"), mk<yama::int_t>(0x1aC23e4f, 10));
    _TEST_SUCC(yama::parse_int("0x1aC23e4fggg"), mk<yama::int_t>(0x1aC23e4f, 10));
    _TEST_SUCC(yama::parse_int("-0x1aC23e4f"), mk<yama::int_t>(-0x1aC23e4f, 11));
    _TEST_SUCC(yama::parse_int("-0x1aC23e4fggg"), mk<yama::int_t>(-0x1aC23e4f, 11));
    _TEST_SUCC(yama::parse_int("0x0001aC23e4f"), mk<yama::int_t>(0x1aC23e4f, 13));
    _TEST_SUCC(yama::parse_int("0x0001aC23e4fggg"), mk<yama::int_t>(0x1aC23e4f, 13));
    _TEST_SUCC(yama::parse_int("-0x0001aC23e4f"), mk<yama::int_t>(-0x1aC23e4f, 14));
    _TEST_SUCC(yama::parse_int("-0x0001aC23e4fggg"), mk<yama::int_t>(-0x1aC23e4f, 14));

    // binary

    _TEST_SUCC(yama::parse_int("0b0"), mk<yama::int_t>(0b0, 3));
    _TEST_SUCC(yama::parse_int("0b1"), mk<yama::int_t>(0b1, 3));

    _TEST_SUCC(yama::parse_int("-0b0"), mk<yama::int_t>(-0b0, 4));
    _TEST_SUCC(yama::parse_int("-0b1"), mk<yama::int_t>(-0b1, 4));

    _TEST_SUCC(yama::parse_int("0b0_0_1"), mk<yama::int_t>(0b001, 7));
    _TEST_SUCC(yama::parse_int("0b1_0_1"), mk<yama::int_t>(0b101, 7));

    _TEST_SUCC(yama::parse_int("-0b0_0_1"), mk<yama::int_t>(-0b001, 8));
    _TEST_SUCC(yama::parse_int("-0b1_0_1"), mk<yama::int_t>(-0b101, 8));

    _TEST_SUCC(yama::parse_int("0b101011"), mk<yama::int_t>(0b101011, 8));
    _TEST_SUCC(yama::parse_int("0b101011aaa"), mk<yama::int_t>(0b101011, 8));
    _TEST_SUCC(yama::parse_int("-0b101011"), mk<yama::int_t>(-0b101011, 9));
    _TEST_SUCC(yama::parse_int("-0b101011aaa"), mk<yama::int_t>(-0b101011, 9));
    _TEST_SUCC(yama::parse_int("0b000101011"), mk<yama::int_t>(0b101011, 11));
    _TEST_SUCC(yama::parse_int("0b000101011aaa"), mk<yama::int_t>(0b101011, 11));
    _TEST_SUCC(yama::parse_int("-0b000101011"), mk<yama::int_t>(-0b101011, 12));
    _TEST_SUCC(yama::parse_int("-0b000101011aaa"), mk<yama::int_t>(-0b101011, 12));

    // failures

    _TEST_FAIL(yama::parse_int(""));
    _TEST_FAIL(yama::parse_int(" "));
    _TEST_FAIL(yama::parse_int("!@#"));
    _TEST_FAIL(yama::parse_int("abc"));

    _TEST_FAIL(yama::parse_int("-"));
    _TEST_FAIL(yama::parse_int("+0"));

    _TEST_FAIL(yama::parse_int("_0"));
    _TEST_FAIL(yama::parse_int("0_"));
    _TEST_FAIL(yama::parse_int("0__0"));

    //_TEST_FAIL(yama::parse_int("0X0"));
    _TEST_FAIL(yama::parse_int("0x"));
    _TEST_FAIL(yama::parse_int("_0x0"));
    _TEST_FAIL(yama::parse_int("0_x0"));
    _TEST_FAIL(yama::parse_int("0x_0"));
    _TEST_FAIL(yama::parse_int("0x0_"));
    _TEST_FAIL(yama::parse_int("0x0__0"));

    //_TEST_FAIL(yama::parse_int("0B0"));
    _TEST_FAIL(yama::parse_int("0b"));
    _TEST_FAIL(yama::parse_int("_0b0"));
    _TEST_FAIL(yama::parse_int("0_b0"));
    _TEST_FAIL(yama::parse_int("0b_0"));
    _TEST_FAIL(yama::parse_int("0b0_"));
    _TEST_FAIL(yama::parse_int("0b0__0"));
}

TEST(ScalarsTests, ParseInt_Extremes) {
    // max int_t == 9223372036854775807
    // min int_t == -9223372036854775808

    // impl must be able to handle max int_t
    _TEST_SUCC(yama::parse_int("9223372036854775807"), mk<yama::int_t>(9223372036854775807, 19));

    // impl must be able to handle min int_t
    _TEST_SUCC(yama::parse_int("-9223372036854775808"), mk<yama::int_t>(-(yama::int_t)9223372036854775808, 20));
    
    // overflow (if one above max int_t)
    const auto a = yama::parse_int("9223372036854775808");
    EXPECT_TRUE(a);
    if (a) {
        // value of a->v is unspecified
        EXPECT_EQ(a->bytes, 19);
        EXPECT_TRUE(a->overflow);
        EXPECT_FALSE(a->underflow);
    }

    // underflow (if one below min int_t)
    const auto b = yama::parse_int("-9223372036854775809");
    EXPECT_TRUE(b);
    if (b) {
        // value of a->v is unspecified
        EXPECT_EQ(b->bytes, 20);
        EXPECT_FALSE(b->overflow);
        EXPECT_TRUE(b->underflow);
    }
}

TEST(ScalarsTests, ParseUInt) {
    // decimal

    _TEST_SUCC(yama::parse_uint("0u"), mk<yama::uint_t>(0, 2));
    _TEST_SUCC(yama::parse_uint("1u"), mk<yama::uint_t>(1, 2));
    _TEST_SUCC(yama::parse_uint("2u"), mk<yama::uint_t>(2, 2));
    _TEST_SUCC(yama::parse_uint("3u"), mk<yama::uint_t>(3, 2));
    _TEST_SUCC(yama::parse_uint("4u"), mk<yama::uint_t>(4, 2));
    _TEST_SUCC(yama::parse_uint("5u"), mk<yama::uint_t>(5, 2));
    _TEST_SUCC(yama::parse_uint("6u"), mk<yama::uint_t>(6, 2));
    _TEST_SUCC(yama::parse_uint("7u"), mk<yama::uint_t>(7, 2));
    _TEST_SUCC(yama::parse_uint("8u"), mk<yama::uint_t>(8, 2));
    _TEST_SUCC(yama::parse_uint("9u"), mk<yama::uint_t>(9, 2));

    _TEST_SUCC(yama::parse_uint("0_0_1u"), mk<yama::uint_t>(1, 6));
    _TEST_SUCC(yama::parse_uint("1_0_1u"), mk<yama::uint_t>(101, 6));
    _TEST_SUCC(yama::parse_uint("2_0_1u"), mk<yama::uint_t>(201, 6));
    _TEST_SUCC(yama::parse_uint("3_0_1u"), mk<yama::uint_t>(301, 6));
    _TEST_SUCC(yama::parse_uint("4_0_1u"), mk<yama::uint_t>(401, 6));
    _TEST_SUCC(yama::parse_uint("5_0_1u"), mk<yama::uint_t>(501, 6));
    _TEST_SUCC(yama::parse_uint("6_0_1u"), mk<yama::uint_t>(601, 6));
    _TEST_SUCC(yama::parse_uint("7_0_1u"), mk<yama::uint_t>(701, 6));
    _TEST_SUCC(yama::parse_uint("8_0_1u"), mk<yama::uint_t>(801, 6));
    _TEST_SUCC(yama::parse_uint("9_0_1u"), mk<yama::uint_t>(901, 6));

    _TEST_SUCC(yama::parse_uint("1230u"), mk<yama::uint_t>(1230, 5));
    _TEST_SUCC(yama::parse_uint("1230uaa"), mk<yama::uint_t>(1230, 5));
    _TEST_SUCC(yama::parse_uint("001230u"), mk<yama::uint_t>(1230, 7));
    _TEST_SUCC(yama::parse_uint("001230uaa"), mk<yama::uint_t>(1230, 7));

    // hexadecimal

    _TEST_SUCC(yama::parse_uint("0x0u"), mk<yama::uint_t>(0, 4));
    _TEST_SUCC(yama::parse_uint("0x1u"), mk<yama::uint_t>(1, 4));
    _TEST_SUCC(yama::parse_uint("0x2u"), mk<yama::uint_t>(2, 4));
    _TEST_SUCC(yama::parse_uint("0x3u"), mk<yama::uint_t>(3, 4));
    _TEST_SUCC(yama::parse_uint("0x4u"), mk<yama::uint_t>(4, 4));
    _TEST_SUCC(yama::parse_uint("0x5u"), mk<yama::uint_t>(5, 4));
    _TEST_SUCC(yama::parse_uint("0x6u"), mk<yama::uint_t>(6, 4));
    _TEST_SUCC(yama::parse_uint("0x7u"), mk<yama::uint_t>(7, 4));
    _TEST_SUCC(yama::parse_uint("0x8u"), mk<yama::uint_t>(8, 4));
    _TEST_SUCC(yama::parse_uint("0x9u"), mk<yama::uint_t>(9, 4));

    _TEST_SUCC(yama::parse_uint("0xau"), mk<yama::uint_t>(10, 4));
    _TEST_SUCC(yama::parse_uint("0xbu"), mk<yama::uint_t>(11, 4));
    _TEST_SUCC(yama::parse_uint("0xcu"), mk<yama::uint_t>(12, 4));
    _TEST_SUCC(yama::parse_uint("0xdu"), mk<yama::uint_t>(13, 4));
    _TEST_SUCC(yama::parse_uint("0xeu"), mk<yama::uint_t>(14, 4));
    _TEST_SUCC(yama::parse_uint("0xfu"), mk<yama::uint_t>(15, 4));

    _TEST_SUCC(yama::parse_uint("0xAu"), mk<yama::uint_t>(10, 4));
    _TEST_SUCC(yama::parse_uint("0xBu"), mk<yama::uint_t>(11, 4));
    _TEST_SUCC(yama::parse_uint("0xCu"), mk<yama::uint_t>(12, 4));
    _TEST_SUCC(yama::parse_uint("0xDu"), mk<yama::uint_t>(13, 4));
    _TEST_SUCC(yama::parse_uint("0xEu"), mk<yama::uint_t>(14, 4));
    _TEST_SUCC(yama::parse_uint("0xFu"), mk<yama::uint_t>(15, 4));
    
    _TEST_SUCC(yama::parse_uint("0x0_aE_3u"), mk<yama::uint_t>(0x0ae3, 9));
    _TEST_SUCC(yama::parse_uint("0x1_aE_3u"), mk<yama::uint_t>(0x1ae3, 9));
    _TEST_SUCC(yama::parse_uint("0x2_aE_3u"), mk<yama::uint_t>(0x2ae3, 9));
    _TEST_SUCC(yama::parse_uint("0x3_aE_3u"), mk<yama::uint_t>(0x3ae3, 9));
    _TEST_SUCC(yama::parse_uint("0x4_aE_3u"), mk<yama::uint_t>(0x4ae3, 9));
    _TEST_SUCC(yama::parse_uint("0x5_aE_3u"), mk<yama::uint_t>(0x5ae3, 9));
    _TEST_SUCC(yama::parse_uint("0x6_aE_3u"), mk<yama::uint_t>(0x6ae3, 9));
    _TEST_SUCC(yama::parse_uint("0x7_aE_3u"), mk<yama::uint_t>(0x7ae3, 9));
    _TEST_SUCC(yama::parse_uint("0x8_aE_3u"), mk<yama::uint_t>(0x8ae3, 9));
    _TEST_SUCC(yama::parse_uint("0x9_aE_3u"), mk<yama::uint_t>(0x9ae3, 9));

    _TEST_SUCC(yama::parse_uint("0xa_aE_3u"), mk<yama::uint_t>(0xaae3, 9));
    _TEST_SUCC(yama::parse_uint("0xb_aE_3u"), mk<yama::uint_t>(0xbae3, 9));
    _TEST_SUCC(yama::parse_uint("0xc_aE_3u"), mk<yama::uint_t>(0xcae3, 9));
    _TEST_SUCC(yama::parse_uint("0xd_aE_3u"), mk<yama::uint_t>(0xdae3, 9));
    _TEST_SUCC(yama::parse_uint("0xe_aE_3u"), mk<yama::uint_t>(0xeae3, 9));
    _TEST_SUCC(yama::parse_uint("0xf_aE_3u"), mk<yama::uint_t>(0xfae3, 9));

    _TEST_SUCC(yama::parse_uint("0xA_aE_3u"), mk<yama::uint_t>(0xaae3, 9));
    _TEST_SUCC(yama::parse_uint("0xB_aE_3u"), mk<yama::uint_t>(0xbae3, 9));
    _TEST_SUCC(yama::parse_uint("0xC_aE_3u"), mk<yama::uint_t>(0xcae3, 9));
    _TEST_SUCC(yama::parse_uint("0xD_aE_3u"), mk<yama::uint_t>(0xdae3, 9));
    _TEST_SUCC(yama::parse_uint("0xE_aE_3u"), mk<yama::uint_t>(0xeae3, 9));
    _TEST_SUCC(yama::parse_uint("0xF_aE_3u"), mk<yama::uint_t>(0xfae3, 9));
    
    _TEST_SUCC(yama::parse_uint("0x1aC23e4fu"), mk<yama::uint_t>(0x1aC23e4f, 11));
    _TEST_SUCC(yama::parse_uint("0x1aC23e4fuggg"), mk<yama::uint_t>(0x1aC23e4f, 11));
    _TEST_SUCC(yama::parse_uint("0x0001aC23e4fu"), mk<yama::uint_t>(0x1aC23e4f, 14));
    _TEST_SUCC(yama::parse_uint("0x0001aC23e4fuggg"), mk<yama::uint_t>(0x1aC23e4f, 14));

    // binary

    _TEST_SUCC(yama::parse_uint("0b0u"), mk<yama::uint_t>(0b0, 4));
    _TEST_SUCC(yama::parse_uint("0b1u"), mk<yama::uint_t>(0b1, 4));

    _TEST_SUCC(yama::parse_uint("0b0_0_1u"), mk<yama::uint_t>(0b001, 8));
    _TEST_SUCC(yama::parse_uint("0b1_0_1u"), mk<yama::uint_t>(0b101, 8));

    _TEST_SUCC(yama::parse_uint("0b101011u"), mk<yama::uint_t>(0b101011, 9));
    _TEST_SUCC(yama::parse_uint("0b101011uaaa"), mk<yama::uint_t>(0b101011, 9));
    _TEST_SUCC(yama::parse_uint("0b000101011u"), mk<yama::uint_t>(0b101011, 12));
    _TEST_SUCC(yama::parse_uint("0b000101011uaaa"), mk<yama::uint_t>(0b101011, 12));

    // failures

    _TEST_FAIL(yama::parse_uint(""));
    _TEST_FAIL(yama::parse_uint(" "));
    _TEST_FAIL(yama::parse_uint("!@#"));
    _TEST_FAIL(yama::parse_uint("abc"));

    _TEST_FAIL(yama::parse_uint("-0u"));
    _TEST_FAIL(yama::parse_uint("+0u"));

    _TEST_FAIL(yama::parse_uint("_0u"));
    _TEST_FAIL(yama::parse_uint("0_u"));
    _TEST_FAIL(yama::parse_uint("0__0u"));
    _TEST_FAIL(yama::parse_uint("0_u"));
    //_TEST_FAIL(yama::parse_uint("0u_"));

    _TEST_FAIL(yama::parse_uint("0X0u"));
    _TEST_FAIL(yama::parse_uint("0xu"));
    _TEST_FAIL(yama::parse_uint("_0x0u"));
    _TEST_FAIL(yama::parse_uint("0_x0u"));
    _TEST_FAIL(yama::parse_uint("0x_0u"));
    _TEST_FAIL(yama::parse_uint("0x0_u"));
    _TEST_FAIL(yama::parse_uint("0x0__0u"));
    _TEST_FAIL(yama::parse_uint("0x0_u"));
    //_TEST_FAIL(yama::parse_uint("0x0u_"));

    _TEST_FAIL(yama::parse_uint("0B0u"));
    _TEST_FAIL(yama::parse_uint("0bu"));
    _TEST_FAIL(yama::parse_uint("_0b0u"));
    _TEST_FAIL(yama::parse_uint("0_b0u"));
    _TEST_FAIL(yama::parse_uint("0b_0u"));
    _TEST_FAIL(yama::parse_uint("0b0_u"));
    _TEST_FAIL(yama::parse_uint("0b0__0u"));
    _TEST_FAIL(yama::parse_uint("0b0_u"));
    //_TEST_FAIL(yama::parse_uint("0b0u_"));
}

TEST(ScalarsTests, ParseUInt_NoU) {
    // decimal

    _TEST_SUCC(yama::parse_uint("0u", false), mk<yama::uint_t>(0, 1));
    _TEST_SUCC(yama::parse_uint("1u", false), mk<yama::uint_t>(1, 1));
    _TEST_SUCC(yama::parse_uint("2u", false), mk<yama::uint_t>(2, 1));
    _TEST_SUCC(yama::parse_uint("3u", false), mk<yama::uint_t>(3, 1));
    _TEST_SUCC(yama::parse_uint("4u", false), mk<yama::uint_t>(4, 1));
    _TEST_SUCC(yama::parse_uint("5u", false), mk<yama::uint_t>(5, 1));
    _TEST_SUCC(yama::parse_uint("6u", false), mk<yama::uint_t>(6, 1));
    _TEST_SUCC(yama::parse_uint("7u", false), mk<yama::uint_t>(7, 1));
    _TEST_SUCC(yama::parse_uint("8u", false), mk<yama::uint_t>(8, 1));
    _TEST_SUCC(yama::parse_uint("9u", false), mk<yama::uint_t>(9, 1));
    
    _TEST_SUCC(yama::parse_uint("0", false), mk<yama::uint_t>(0, 1));
    _TEST_SUCC(yama::parse_uint("1", false), mk<yama::uint_t>(1, 1));
    _TEST_SUCC(yama::parse_uint("2", false), mk<yama::uint_t>(2, 1));
    _TEST_SUCC(yama::parse_uint("3", false), mk<yama::uint_t>(3, 1));
    _TEST_SUCC(yama::parse_uint("4", false), mk<yama::uint_t>(4, 1));
    _TEST_SUCC(yama::parse_uint("5", false), mk<yama::uint_t>(5, 1));
    _TEST_SUCC(yama::parse_uint("6", false), mk<yama::uint_t>(6, 1));
    _TEST_SUCC(yama::parse_uint("7", false), mk<yama::uint_t>(7, 1));
    _TEST_SUCC(yama::parse_uint("8", false), mk<yama::uint_t>(8, 1));
    _TEST_SUCC(yama::parse_uint("9", false), mk<yama::uint_t>(9, 1));

    _TEST_SUCC(yama::parse_uint("0_0_1u", false), mk<yama::uint_t>(1, 5));
    _TEST_SUCC(yama::parse_uint("1_0_1u", false), mk<yama::uint_t>(101, 5));
    _TEST_SUCC(yama::parse_uint("2_0_1u", false), mk<yama::uint_t>(201, 5));
    _TEST_SUCC(yama::parse_uint("3_0_1u", false), mk<yama::uint_t>(301, 5));
    _TEST_SUCC(yama::parse_uint("4_0_1u", false), mk<yama::uint_t>(401, 5));
    _TEST_SUCC(yama::parse_uint("5_0_1u", false), mk<yama::uint_t>(501, 5));
    _TEST_SUCC(yama::parse_uint("6_0_1u", false), mk<yama::uint_t>(601, 5));
    _TEST_SUCC(yama::parse_uint("7_0_1u", false), mk<yama::uint_t>(701, 5));
    _TEST_SUCC(yama::parse_uint("8_0_1u", false), mk<yama::uint_t>(801, 5));
    _TEST_SUCC(yama::parse_uint("9_0_1u", false), mk<yama::uint_t>(901, 5));
    
    _TEST_SUCC(yama::parse_uint("0_0_1", false), mk<yama::uint_t>(1, 5));
    _TEST_SUCC(yama::parse_uint("1_0_1", false), mk<yama::uint_t>(101, 5));
    _TEST_SUCC(yama::parse_uint("2_0_1", false), mk<yama::uint_t>(201, 5));
    _TEST_SUCC(yama::parse_uint("3_0_1", false), mk<yama::uint_t>(301, 5));
    _TEST_SUCC(yama::parse_uint("4_0_1", false), mk<yama::uint_t>(401, 5));
    _TEST_SUCC(yama::parse_uint("5_0_1", false), mk<yama::uint_t>(501, 5));
    _TEST_SUCC(yama::parse_uint("6_0_1", false), mk<yama::uint_t>(601, 5));
    _TEST_SUCC(yama::parse_uint("7_0_1", false), mk<yama::uint_t>(701, 5));
    _TEST_SUCC(yama::parse_uint("8_0_1", false), mk<yama::uint_t>(801, 5));
    _TEST_SUCC(yama::parse_uint("9_0_1", false), mk<yama::uint_t>(901, 5));

    _TEST_SUCC(yama::parse_uint("1230u", false), mk<yama::uint_t>(1230, 4));
    _TEST_SUCC(yama::parse_uint("1230uaa", false), mk<yama::uint_t>(1230, 4));
    _TEST_SUCC(yama::parse_uint("001230u", false), mk<yama::uint_t>(1230, 6));
    _TEST_SUCC(yama::parse_uint("001230uaa", false), mk<yama::uint_t>(1230, 6));
    
    _TEST_SUCC(yama::parse_uint("1230", false), mk<yama::uint_t>(1230, 4));
    _TEST_SUCC(yama::parse_uint("1230aa", false), mk<yama::uint_t>(1230, 4));
    _TEST_SUCC(yama::parse_uint("001230", false), mk<yama::uint_t>(1230, 6));
    _TEST_SUCC(yama::parse_uint("001230aa", false), mk<yama::uint_t>(1230, 6));

    // hexadecimal

    _TEST_SUCC(yama::parse_uint("0x0u", false), mk<yama::uint_t>(0, 3));
    _TEST_SUCC(yama::parse_uint("0x1u", false), mk<yama::uint_t>(1, 3));
    _TEST_SUCC(yama::parse_uint("0x2u", false), mk<yama::uint_t>(2, 3));
    _TEST_SUCC(yama::parse_uint("0x3u", false), mk<yama::uint_t>(3, 3));
    _TEST_SUCC(yama::parse_uint("0x4u", false), mk<yama::uint_t>(4, 3));
    _TEST_SUCC(yama::parse_uint("0x5u", false), mk<yama::uint_t>(5, 3));
    _TEST_SUCC(yama::parse_uint("0x6u", false), mk<yama::uint_t>(6, 3));
    _TEST_SUCC(yama::parse_uint("0x7u", false), mk<yama::uint_t>(7, 3));
    _TEST_SUCC(yama::parse_uint("0x8u", false), mk<yama::uint_t>(8, 3));
    _TEST_SUCC(yama::parse_uint("0x9u", false), mk<yama::uint_t>(9, 3));
    
    _TEST_SUCC(yama::parse_uint("0x0", false), mk<yama::uint_t>(0, 3));
    _TEST_SUCC(yama::parse_uint("0x1", false), mk<yama::uint_t>(1, 3));
    _TEST_SUCC(yama::parse_uint("0x2", false), mk<yama::uint_t>(2, 3));
    _TEST_SUCC(yama::parse_uint("0x3", false), mk<yama::uint_t>(3, 3));
    _TEST_SUCC(yama::parse_uint("0x4", false), mk<yama::uint_t>(4, 3));
    _TEST_SUCC(yama::parse_uint("0x5", false), mk<yama::uint_t>(5, 3));
    _TEST_SUCC(yama::parse_uint("0x6", false), mk<yama::uint_t>(6, 3));
    _TEST_SUCC(yama::parse_uint("0x7", false), mk<yama::uint_t>(7, 3));
    _TEST_SUCC(yama::parse_uint("0x8", false), mk<yama::uint_t>(8, 3));
    _TEST_SUCC(yama::parse_uint("0x9", false), mk<yama::uint_t>(9, 3));

    _TEST_SUCC(yama::parse_uint("0xau", false), mk<yama::uint_t>(10, 3));
    _TEST_SUCC(yama::parse_uint("0xbu", false), mk<yama::uint_t>(11, 3));
    _TEST_SUCC(yama::parse_uint("0xcu", false), mk<yama::uint_t>(12, 3));
    _TEST_SUCC(yama::parse_uint("0xdu", false), mk<yama::uint_t>(13, 3));
    _TEST_SUCC(yama::parse_uint("0xeu", false), mk<yama::uint_t>(14, 3));
    _TEST_SUCC(yama::parse_uint("0xfu", false), mk<yama::uint_t>(15, 3));
    
    _TEST_SUCC(yama::parse_uint("0xa", false), mk<yama::uint_t>(10, 3));
    _TEST_SUCC(yama::parse_uint("0xb", false), mk<yama::uint_t>(11, 3));
    _TEST_SUCC(yama::parse_uint("0xc", false), mk<yama::uint_t>(12, 3));
    _TEST_SUCC(yama::parse_uint("0xd", false), mk<yama::uint_t>(13, 3));
    _TEST_SUCC(yama::parse_uint("0xe", false), mk<yama::uint_t>(14, 3));
    _TEST_SUCC(yama::parse_uint("0xf", false), mk<yama::uint_t>(15, 3));

    _TEST_SUCC(yama::parse_uint("0xAu", false), mk<yama::uint_t>(10, 3));
    _TEST_SUCC(yama::parse_uint("0xBu", false), mk<yama::uint_t>(11, 3));
    _TEST_SUCC(yama::parse_uint("0xCu", false), mk<yama::uint_t>(12, 3));
    _TEST_SUCC(yama::parse_uint("0xDu", false), mk<yama::uint_t>(13, 3));
    _TEST_SUCC(yama::parse_uint("0xEu", false), mk<yama::uint_t>(14, 3));
    _TEST_SUCC(yama::parse_uint("0xFu", false), mk<yama::uint_t>(15, 3));
    
    _TEST_SUCC(yama::parse_uint("0xA", false), mk<yama::uint_t>(10, 3));
    _TEST_SUCC(yama::parse_uint("0xB", false), mk<yama::uint_t>(11, 3));
    _TEST_SUCC(yama::parse_uint("0xC", false), mk<yama::uint_t>(12, 3));
    _TEST_SUCC(yama::parse_uint("0xD", false), mk<yama::uint_t>(13, 3));
    _TEST_SUCC(yama::parse_uint("0xE", false), mk<yama::uint_t>(14, 3));
    _TEST_SUCC(yama::parse_uint("0xF", false), mk<yama::uint_t>(15, 3));
    
    _TEST_SUCC(yama::parse_uint("0x0_aE_3u", false), mk<yama::uint_t>(0x0ae3, 8));
    _TEST_SUCC(yama::parse_uint("0x1_aE_3u", false), mk<yama::uint_t>(0x1ae3, 8));
    _TEST_SUCC(yama::parse_uint("0x2_aE_3u", false), mk<yama::uint_t>(0x2ae3, 8));
    _TEST_SUCC(yama::parse_uint("0x3_aE_3u", false), mk<yama::uint_t>(0x3ae3, 8));
    _TEST_SUCC(yama::parse_uint("0x4_aE_3u", false), mk<yama::uint_t>(0x4ae3, 8));
    _TEST_SUCC(yama::parse_uint("0x5_aE_3u", false), mk<yama::uint_t>(0x5ae3, 8));
    _TEST_SUCC(yama::parse_uint("0x6_aE_3u", false), mk<yama::uint_t>(0x6ae3, 8));
    _TEST_SUCC(yama::parse_uint("0x7_aE_3u", false), mk<yama::uint_t>(0x7ae3, 8));
    _TEST_SUCC(yama::parse_uint("0x8_aE_3u", false), mk<yama::uint_t>(0x8ae3, 8));
    _TEST_SUCC(yama::parse_uint("0x9_aE_3u", false), mk<yama::uint_t>(0x9ae3, 8));
    
    _TEST_SUCC(yama::parse_uint("0x0_aE_3", false), mk<yama::uint_t>(0x0ae3, 8));
    _TEST_SUCC(yama::parse_uint("0x1_aE_3", false), mk<yama::uint_t>(0x1ae3, 8));
    _TEST_SUCC(yama::parse_uint("0x2_aE_3", false), mk<yama::uint_t>(0x2ae3, 8));
    _TEST_SUCC(yama::parse_uint("0x3_aE_3", false), mk<yama::uint_t>(0x3ae3, 8));
    _TEST_SUCC(yama::parse_uint("0x4_aE_3", false), mk<yama::uint_t>(0x4ae3, 8));
    _TEST_SUCC(yama::parse_uint("0x5_aE_3", false), mk<yama::uint_t>(0x5ae3, 8));
    _TEST_SUCC(yama::parse_uint("0x6_aE_3", false), mk<yama::uint_t>(0x6ae3, 8));
    _TEST_SUCC(yama::parse_uint("0x7_aE_3", false), mk<yama::uint_t>(0x7ae3, 8));
    _TEST_SUCC(yama::parse_uint("0x8_aE_3", false), mk<yama::uint_t>(0x8ae3, 8));
    _TEST_SUCC(yama::parse_uint("0x9_aE_3", false), mk<yama::uint_t>(0x9ae3, 8));

    _TEST_SUCC(yama::parse_uint("0xa_aE_3u", false), mk<yama::uint_t>(0xaae3, 8));
    _TEST_SUCC(yama::parse_uint("0xb_aE_3u", false), mk<yama::uint_t>(0xbae3, 8));
    _TEST_SUCC(yama::parse_uint("0xc_aE_3u", false), mk<yama::uint_t>(0xcae3, 8));
    _TEST_SUCC(yama::parse_uint("0xd_aE_3u", false), mk<yama::uint_t>(0xdae3, 8));
    _TEST_SUCC(yama::parse_uint("0xe_aE_3u", false), mk<yama::uint_t>(0xeae3, 8));
    _TEST_SUCC(yama::parse_uint("0xf_aE_3u", false), mk<yama::uint_t>(0xfae3, 8));
    
    _TEST_SUCC(yama::parse_uint("0xa_aE_3", false), mk<yama::uint_t>(0xaae3, 8));
    _TEST_SUCC(yama::parse_uint("0xb_aE_3", false), mk<yama::uint_t>(0xbae3, 8));
    _TEST_SUCC(yama::parse_uint("0xc_aE_3", false), mk<yama::uint_t>(0xcae3, 8));
    _TEST_SUCC(yama::parse_uint("0xd_aE_3", false), mk<yama::uint_t>(0xdae3, 8));
    _TEST_SUCC(yama::parse_uint("0xe_aE_3", false), mk<yama::uint_t>(0xeae3, 8));
    _TEST_SUCC(yama::parse_uint("0xf_aE_3", false), mk<yama::uint_t>(0xfae3, 8));

    _TEST_SUCC(yama::parse_uint("0xA_aE_3u", false), mk<yama::uint_t>(0xaae3, 8));
    _TEST_SUCC(yama::parse_uint("0xB_aE_3u", false), mk<yama::uint_t>(0xbae3, 8));
    _TEST_SUCC(yama::parse_uint("0xC_aE_3u", false), mk<yama::uint_t>(0xcae3, 8));
    _TEST_SUCC(yama::parse_uint("0xD_aE_3u", false), mk<yama::uint_t>(0xdae3, 8));
    _TEST_SUCC(yama::parse_uint("0xE_aE_3u", false), mk<yama::uint_t>(0xeae3, 8));
    _TEST_SUCC(yama::parse_uint("0xF_aE_3u", false), mk<yama::uint_t>(0xfae3, 8));
    
    _TEST_SUCC(yama::parse_uint("0xA_aE_3", false), mk<yama::uint_t>(0xaae3, 8));
    _TEST_SUCC(yama::parse_uint("0xB_aE_3", false), mk<yama::uint_t>(0xbae3, 8));
    _TEST_SUCC(yama::parse_uint("0xC_aE_3", false), mk<yama::uint_t>(0xcae3, 8));
    _TEST_SUCC(yama::parse_uint("0xD_aE_3", false), mk<yama::uint_t>(0xdae3, 8));
    _TEST_SUCC(yama::parse_uint("0xE_aE_3", false), mk<yama::uint_t>(0xeae3, 8));
    _TEST_SUCC(yama::parse_uint("0xF_aE_3", false), mk<yama::uint_t>(0xfae3, 8));
    
    _TEST_SUCC(yama::parse_uint("0x1aC23e4fu", false), mk<yama::uint_t>(0x1aC23e4f, 10));
    _TEST_SUCC(yama::parse_uint("0x1aC23e4fuggg", false), mk<yama::uint_t>(0x1aC23e4f, 10));
    _TEST_SUCC(yama::parse_uint("0x0001aC23e4fu", false), mk<yama::uint_t>(0x1aC23e4f, 13));
    _TEST_SUCC(yama::parse_uint("0x0001aC23e4fuggg", false), mk<yama::uint_t>(0x1aC23e4f, 13));
    
    _TEST_SUCC(yama::parse_uint("0x1aC23e4f", false), mk<yama::uint_t>(0x1aC23e4f, 10));
    _TEST_SUCC(yama::parse_uint("0x1aC23e4fggg", false), mk<yama::uint_t>(0x1aC23e4f, 10));
    _TEST_SUCC(yama::parse_uint("0x0001aC23e4f", false), mk<yama::uint_t>(0x1aC23e4f, 13));
    _TEST_SUCC(yama::parse_uint("0x0001aC23e4fggg", false), mk<yama::uint_t>(0x1aC23e4f, 13));

    // binary

    _TEST_SUCC(yama::parse_uint("0b0u", false), mk<yama::uint_t>(0b0, 3));
    _TEST_SUCC(yama::parse_uint("0b1u", false), mk<yama::uint_t>(0b1, 3));
    
    _TEST_SUCC(yama::parse_uint("0b0", false), mk<yama::uint_t>(0b0, 3));
    _TEST_SUCC(yama::parse_uint("0b1", false), mk<yama::uint_t>(0b1, 3));

    _TEST_SUCC(yama::parse_uint("0b0_0_1u", false), mk<yama::uint_t>(0b001, 7));
    _TEST_SUCC(yama::parse_uint("0b1_0_1u", false), mk<yama::uint_t>(0b101, 7));
    
    _TEST_SUCC(yama::parse_uint("0b0_0_1", false), mk<yama::uint_t>(0b001, 7));
    _TEST_SUCC(yama::parse_uint("0b1_0_1", false), mk<yama::uint_t>(0b101, 7));

    _TEST_SUCC(yama::parse_uint("0b101011u", false), mk<yama::uint_t>(0b101011, 8));
    _TEST_SUCC(yama::parse_uint("0b101011uaaa", false), mk<yama::uint_t>(0b101011, 8));
    _TEST_SUCC(yama::parse_uint("0b000101011u", false), mk<yama::uint_t>(0b101011, 11));
    _TEST_SUCC(yama::parse_uint("0b000101011uaaa", false), mk<yama::uint_t>(0b101011, 11));
    
    _TEST_SUCC(yama::parse_uint("0b101011", false), mk<yama::uint_t>(0b101011, 8));
    _TEST_SUCC(yama::parse_uint("0b101011aaa", false), mk<yama::uint_t>(0b101011, 8));
    _TEST_SUCC(yama::parse_uint("0b000101011", false), mk<yama::uint_t>(0b101011, 11));
    _TEST_SUCC(yama::parse_uint("0b000101011aaa", false), mk<yama::uint_t>(0b101011, 11));

    // failures

    _TEST_FAIL(yama::parse_uint("", false));
    _TEST_FAIL(yama::parse_uint(" ", false));
    _TEST_FAIL(yama::parse_uint("!@#", false));
    _TEST_FAIL(yama::parse_uint("abc", false));

    _TEST_FAIL(yama::parse_uint("-0u", false));
    _TEST_FAIL(yama::parse_uint("+0u", false));

    _TEST_FAIL(yama::parse_uint("_0u", false));
    _TEST_FAIL(yama::parse_uint("0_u", false));
    _TEST_FAIL(yama::parse_uint("0__0u", false));
    _TEST_FAIL(yama::parse_uint("0_u", false));
    //_TEST_FAIL(yama::parse_uint("0u_", false));

    //_TEST_FAIL(yama::parse_uint("0X0u", false));
    _TEST_FAIL(yama::parse_uint("0xu", false));
    _TEST_FAIL(yama::parse_uint("_0x0u", false));
    _TEST_FAIL(yama::parse_uint("0_x0u", false));
    _TEST_FAIL(yama::parse_uint("0x_0u", false));
    _TEST_FAIL(yama::parse_uint("0x0_u", false));
    _TEST_FAIL(yama::parse_uint("0x0__0u", false));
    _TEST_FAIL(yama::parse_uint("0x0_u", false));
    //_TEST_FAIL(yama::parse_uint("0x0u_", false));

    //_TEST_FAIL(yama::parse_uint("0B0u", false));
    _TEST_FAIL(yama::parse_uint("0bu", false));
    _TEST_FAIL(yama::parse_uint("_0b0u", false));
    _TEST_FAIL(yama::parse_uint("0_b0u", false));
    _TEST_FAIL(yama::parse_uint("0b_0u", false));
    _TEST_FAIL(yama::parse_uint("0b0_u", false));
    _TEST_FAIL(yama::parse_uint("0b0__0u", false));
    _TEST_FAIL(yama::parse_uint("0b0_u", false));
    //_TEST_FAIL(yama::parse_uint("0b0u_", false));
}

TEST(ScalarsTests, ParseUInt_Extremes) {
    // not gonna test underflow, as those shouldn't even parse

    // max uint_t == 18446744073709551615

    // impl must be able to handle max uint_t
    _TEST_SUCC(yama::parse_uint("18446744073709551615u"), mk<yama::uint_t>(18446744073709551615, 21));

    // overflow (if one above max uint_t)
    const auto a = yama::parse_uint("18446744073709551616u");
    EXPECT_TRUE(a);
    if (a) {
        // value of a->v is unspecified
        EXPECT_EQ(a->bytes, 21);
        EXPECT_TRUE(a->overflow);
        EXPECT_FALSE(a->underflow);
    }
}

TEST(ScalarsTests, ParseUInt_NoU_Extremes) {
    // not gonna test underflow, as those shouldn't even parse

    // max uint_t == 18446744073709551615

    // impl must be able to handle max uint_t
    _TEST_SUCC(yama::parse_uint("18446744073709551615u", false), mk<yama::uint_t>(18446744073709551615, 20));
    _TEST_SUCC(yama::parse_uint("18446744073709551615", false), mk<yama::uint_t>(18446744073709551615, 20));

    // overflow (if one above max uint_t)
    {
        const auto a = yama::parse_uint("18446744073709551616u", false);
        EXPECT_TRUE(a);
        if (a) {
            // value of a->v is unspecified
            EXPECT_EQ(a->bytes, 20);
            EXPECT_TRUE(a->overflow);
            EXPECT_FALSE(a->underflow);
        }
    }
    {
        const auto a = yama::parse_uint("18446744073709551616", false);
        EXPECT_TRUE(a);
        if (a) {
            // value of a->v is unspecified
            EXPECT_EQ(a->bytes, 20);
            EXPECT_TRUE(a->overflow);
            EXPECT_FALSE(a->underflow);
        }
    }
}

TEST(ScalarsTests, ParseFloat) {
    _TEST_SUCC(yama::parse_float("0.0"), mk<yama::float_t>(0.0, 3));
    _TEST_SUCC(yama::parse_float("1.0"), mk<yama::float_t>(1.0, 3));
    _TEST_SUCC(yama::parse_float("2.0"), mk<yama::float_t>(2.0, 3));
    _TEST_SUCC(yama::parse_float("3.0"), mk<yama::float_t>(3.0, 3));
    _TEST_SUCC(yama::parse_float("4.0"), mk<yama::float_t>(4.0, 3));
    _TEST_SUCC(yama::parse_float("5.0"), mk<yama::float_t>(5.0, 3));
    _TEST_SUCC(yama::parse_float("6.0"), mk<yama::float_t>(6.0, 3));
    _TEST_SUCC(yama::parse_float("7.0"), mk<yama::float_t>(7.0, 3));
    _TEST_SUCC(yama::parse_float("8.0"), mk<yama::float_t>(8.0, 3));
    _TEST_SUCC(yama::parse_float("9.0"), mk<yama::float_t>(9.0, 3));

    _TEST_SUCC(yama::parse_float("-0.0"), mk<yama::float_t>(-0.0, 4));
    _TEST_SUCC(yama::parse_float("-1.0"), mk<yama::float_t>(-1.0, 4));
    _TEST_SUCC(yama::parse_float("-2.0"), mk<yama::float_t>(-2.0, 4));
    _TEST_SUCC(yama::parse_float("-3.0"), mk<yama::float_t>(-3.0, 4));
    _TEST_SUCC(yama::parse_float("-4.0"), mk<yama::float_t>(-4.0, 4));
    _TEST_SUCC(yama::parse_float("-5.0"), mk<yama::float_t>(-5.0, 4));
    _TEST_SUCC(yama::parse_float("-6.0"), mk<yama::float_t>(-6.0, 4));
    _TEST_SUCC(yama::parse_float("-7.0"), mk<yama::float_t>(-7.0, 4));
    _TEST_SUCC(yama::parse_float("-8.0"), mk<yama::float_t>(-8.0, 4));
    _TEST_SUCC(yama::parse_float("-9.0"), mk<yama::float_t>(-9.0, 4));

    _TEST_SUCC(yama::parse_float("0.00"), mk<yama::float_t>(0.00, 4));
    _TEST_SUCC(yama::parse_float("0.01"), mk<yama::float_t>(0.01, 4));
    _TEST_SUCC(yama::parse_float("0.02"), mk<yama::float_t>(0.02, 4));
    _TEST_SUCC(yama::parse_float("0.03"), mk<yama::float_t>(0.03, 4));
    _TEST_SUCC(yama::parse_float("0.04"), mk<yama::float_t>(0.04, 4));
    _TEST_SUCC(yama::parse_float("0.05"), mk<yama::float_t>(0.05, 4));
    _TEST_SUCC(yama::parse_float("0.06"), mk<yama::float_t>(0.06, 4));
    _TEST_SUCC(yama::parse_float("0.07"), mk<yama::float_t>(0.07, 4));
    _TEST_SUCC(yama::parse_float("0.08"), mk<yama::float_t>(0.08, 4));
    _TEST_SUCC(yama::parse_float("0.09"), mk<yama::float_t>(0.09, 4));

    _TEST_SUCC(yama::parse_float("-0.00"), mk<yama::float_t>(-0.00, 5));
    _TEST_SUCC(yama::parse_float("-0.01"), mk<yama::float_t>(-0.01, 5));
    _TEST_SUCC(yama::parse_float("-0.02"), mk<yama::float_t>(-0.02, 5));
    _TEST_SUCC(yama::parse_float("-0.03"), mk<yama::float_t>(-0.03, 5));
    _TEST_SUCC(yama::parse_float("-0.04"), mk<yama::float_t>(-0.04, 5));
    _TEST_SUCC(yama::parse_float("-0.05"), mk<yama::float_t>(-0.05, 5));
    _TEST_SUCC(yama::parse_float("-0.06"), mk<yama::float_t>(-0.06, 5));
    _TEST_SUCC(yama::parse_float("-0.07"), mk<yama::float_t>(-0.07, 5));
    _TEST_SUCC(yama::parse_float("-0.08"), mk<yama::float_t>(-0.08, 5));
    _TEST_SUCC(yama::parse_float("-0.09"), mk<yama::float_t>(-0.09, 5));

    _TEST_SUCC(yama::parse_float(".00"), mk<yama::float_t>(0.00, 3));
    _TEST_SUCC(yama::parse_float(".01"), mk<yama::float_t>(0.01, 3));
    _TEST_SUCC(yama::parse_float(".02"), mk<yama::float_t>(0.02, 3));
    _TEST_SUCC(yama::parse_float(".03"), mk<yama::float_t>(0.03, 3));
    _TEST_SUCC(yama::parse_float(".04"), mk<yama::float_t>(0.04, 3));
    _TEST_SUCC(yama::parse_float(".05"), mk<yama::float_t>(0.05, 3));
    _TEST_SUCC(yama::parse_float(".06"), mk<yama::float_t>(0.06, 3));
    _TEST_SUCC(yama::parse_float(".07"), mk<yama::float_t>(0.07, 3));
    _TEST_SUCC(yama::parse_float(".08"), mk<yama::float_t>(0.08, 3));
    _TEST_SUCC(yama::parse_float(".09"), mk<yama::float_t>(0.09, 3));

    _TEST_SUCC(yama::parse_float("-.00"), mk<yama::float_t>(-0.00, 4));
    _TEST_SUCC(yama::parse_float("-.01"), mk<yama::float_t>(-0.01, 4));
    _TEST_SUCC(yama::parse_float("-.02"), mk<yama::float_t>(-0.02, 4));
    _TEST_SUCC(yama::parse_float("-.03"), mk<yama::float_t>(-0.03, 4));
    _TEST_SUCC(yama::parse_float("-.04"), mk<yama::float_t>(-0.04, 4));
    _TEST_SUCC(yama::parse_float("-.05"), mk<yama::float_t>(-0.05, 4));
    _TEST_SUCC(yama::parse_float("-.06"), mk<yama::float_t>(-0.06, 4));
    _TEST_SUCC(yama::parse_float("-.07"), mk<yama::float_t>(-0.07, 4));
    _TEST_SUCC(yama::parse_float("-.08"), mk<yama::float_t>(-0.08, 4));
    _TEST_SUCC(yama::parse_float("-.09"), mk<yama::float_t>(-0.09, 4));

    _TEST_SUCC(yama::parse_float("0_1.0_1_0"), mk<yama::float_t>(01.01, 9));
    _TEST_SUCC(yama::parse_float("1_1.0_1_0"), mk<yama::float_t>(11.01, 9));
    _TEST_SUCC(yama::parse_float("2_1.0_1_0"), mk<yama::float_t>(21.01, 9));
    _TEST_SUCC(yama::parse_float("3_1.0_1_0"), mk<yama::float_t>(31.01, 9));
    _TEST_SUCC(yama::parse_float("4_1.0_1_0"), mk<yama::float_t>(41.01, 9));
    _TEST_SUCC(yama::parse_float("5_1.0_1_0"), mk<yama::float_t>(51.01, 9));
    _TEST_SUCC(yama::parse_float("6_1.0_1_0"), mk<yama::float_t>(61.01, 9));
    _TEST_SUCC(yama::parse_float("7_1.0_1_0"), mk<yama::float_t>(71.01, 9));
    _TEST_SUCC(yama::parse_float("8_1.0_1_0"), mk<yama::float_t>(81.01, 9));
    _TEST_SUCC(yama::parse_float("9_1.0_1_0"), mk<yama::float_t>(91.01, 9));

    _TEST_SUCC(yama::parse_float("-0_1.0_1_0"), mk<yama::float_t>(-01.01, 10));
    _TEST_SUCC(yama::parse_float("-1_1.0_1_0"), mk<yama::float_t>(-11.01, 10));
    _TEST_SUCC(yama::parse_float("-2_1.0_1_0"), mk<yama::float_t>(-21.01, 10));
    _TEST_SUCC(yama::parse_float("-3_1.0_1_0"), mk<yama::float_t>(-31.01, 10));
    _TEST_SUCC(yama::parse_float("-4_1.0_1_0"), mk<yama::float_t>(-41.01, 10));
    _TEST_SUCC(yama::parse_float("-5_1.0_1_0"), mk<yama::float_t>(-51.01, 10));
    _TEST_SUCC(yama::parse_float("-6_1.0_1_0"), mk<yama::float_t>(-61.01, 10));
    _TEST_SUCC(yama::parse_float("-7_1.0_1_0"), mk<yama::float_t>(-71.01, 10));
    _TEST_SUCC(yama::parse_float("-8_1.0_1_0"), mk<yama::float_t>(-81.01, 10));
    _TEST_SUCC(yama::parse_float("-9_1.0_1_0"), mk<yama::float_t>(-91.01, 10));

    _TEST_SUCC(yama::parse_float("3.14159"), mk<yama::float_t>(3.14159, 7));
    _TEST_SUCC(yama::parse_float("-3.14159"), mk<yama::float_t>(-3.14159, 8));
    
    _TEST_SUCC(yama::parse_float("3141.59e-3"), mk<yama::float_t>(3.14159, 10));
    _TEST_SUCC(yama::parse_float("314.159e-2"), mk<yama::float_t>(3.14159, 10));
    _TEST_SUCC(yama::parse_float("31.4159e-1"), mk<yama::float_t>(3.14159, 10));
    _TEST_SUCC(yama::parse_float("3.14159e0"), mk<yama::float_t>(3.14159, 9));
    _TEST_SUCC(yama::parse_float("0.314159e1"), mk<yama::float_t>(3.14159, 10));
    _TEST_SUCC(yama::parse_float("0.0314159e2"), mk<yama::float_t>(3.14159, 11));
    _TEST_SUCC(yama::parse_float("0.00314159e3"), mk<yama::float_t>(3.14159, 12));
    _TEST_SUCC(yama::parse_float("3.14159e+0"), mk<yama::float_t>(3.14159, 10));
    _TEST_SUCC(yama::parse_float("0.314159e+1"), mk<yama::float_t>(3.14159, 11));
    _TEST_SUCC(yama::parse_float("0.0314159e+2"), mk<yama::float_t>(3.14159, 12));
    _TEST_SUCC(yama::parse_float("0.00314159e+3"), mk<yama::float_t>(3.14159, 13));
    
    _TEST_SUCC(yama::parse_float("3141.59e-00_0_3"), mk<yama::float_t>(3.14159, 15));
    _TEST_SUCC(yama::parse_float("314.159e-00_0_2"), mk<yama::float_t>(3.14159, 15));
    _TEST_SUCC(yama::parse_float("31.4159e-00_0_1"), mk<yama::float_t>(3.14159, 15));
    _TEST_SUCC(yama::parse_float("3.14159e00_0_0"), mk<yama::float_t>(3.14159, 14));
    _TEST_SUCC(yama::parse_float("0.314159e00_0_1"), mk<yama::float_t>(3.14159, 15));
    _TEST_SUCC(yama::parse_float("0.0314159e00_0_2"), mk<yama::float_t>(3.14159, 16));
    _TEST_SUCC(yama::parse_float("0.00314159e00_0_3"), mk<yama::float_t>(3.14159, 17));
    _TEST_SUCC(yama::parse_float("3.14159e+00_0_0"), mk<yama::float_t>(3.14159, 15));
    _TEST_SUCC(yama::parse_float("0.314159e+00_0_1"), mk<yama::float_t>(3.14159, 16));
    _TEST_SUCC(yama::parse_float("0.0314159e+00_0_2"), mk<yama::float_t>(3.14159, 17));
    _TEST_SUCC(yama::parse_float("0.00314159e+00_0_3"), mk<yama::float_t>(3.14159, 18));

    _TEST_SUCC(yama::parse_float("-3141.59e-00_0_3"), mk<yama::float_t>(-3.14159, 16));
    _TEST_SUCC(yama::parse_float("-314.159e-00_0_2"), mk<yama::float_t>(-3.14159, 16));
    _TEST_SUCC(yama::parse_float("-31.4159e-00_0_1"), mk<yama::float_t>(-3.14159, 16));
    _TEST_SUCC(yama::parse_float("-3.14159e00_0_0"), mk<yama::float_t>(-3.14159, 15));
    _TEST_SUCC(yama::parse_float("-0.314159e00_0_1"), mk<yama::float_t>(-3.14159, 16));
    _TEST_SUCC(yama::parse_float("-0.0314159e00_0_2"), mk<yama::float_t>(-3.14159, 17));
    _TEST_SUCC(yama::parse_float("-0.00314159e00_0_3"), mk<yama::float_t>(-3.14159, 18));
    _TEST_SUCC(yama::parse_float("-3.14159e+00_0_0"), mk<yama::float_t>(-3.14159, 16));
    _TEST_SUCC(yama::parse_float("-0.314159e+00_0_1"), mk<yama::float_t>(-3.14159, 17));
    _TEST_SUCC(yama::parse_float("-0.0314159e+00_0_2"), mk<yama::float_t>(-3.14159, 18));
    _TEST_SUCC(yama::parse_float("-0.00314159e+00_0_3"), mk<yama::float_t>(-3.14159, 19));

    // integer-like (legal outside Yama code)

    _TEST_SUCC(yama::parse_float("0"), mk<yama::float_t>(0.0, 1));
    _TEST_SUCC(yama::parse_float("1"), mk<yama::float_t>(1.0, 1));
    _TEST_SUCC(yama::parse_float("2"), mk<yama::float_t>(2.0, 1));
    _TEST_SUCC(yama::parse_float("3"), mk<yama::float_t>(3.0, 1));
    _TEST_SUCC(yama::parse_float("4"), mk<yama::float_t>(4.0, 1));
    _TEST_SUCC(yama::parse_float("5"), mk<yama::float_t>(5.0, 1));
    _TEST_SUCC(yama::parse_float("6"), mk<yama::float_t>(6.0, 1));
    _TEST_SUCC(yama::parse_float("7"), mk<yama::float_t>(7.0, 1));
    _TEST_SUCC(yama::parse_float("8"), mk<yama::float_t>(8.0, 1));
    _TEST_SUCC(yama::parse_float("9"), mk<yama::float_t>(9.0, 1));

    _TEST_SUCC(yama::parse_float("-0"), mk<yama::float_t>(-0.0, 2));
    _TEST_SUCC(yama::parse_float("-1"), mk<yama::float_t>(-1.0, 2));
    _TEST_SUCC(yama::parse_float("-2"), mk<yama::float_t>(-2.0, 2));
    _TEST_SUCC(yama::parse_float("-3"), mk<yama::float_t>(-3.0, 2));
    _TEST_SUCC(yama::parse_float("-4"), mk<yama::float_t>(-4.0, 2));
    _TEST_SUCC(yama::parse_float("-5"), mk<yama::float_t>(-5.0, 2));
    _TEST_SUCC(yama::parse_float("-6"), mk<yama::float_t>(-6.0, 2));
    _TEST_SUCC(yama::parse_float("-7"), mk<yama::float_t>(-7.0, 2));
    _TEST_SUCC(yama::parse_float("-8"), mk<yama::float_t>(-8.0, 2));
    _TEST_SUCC(yama::parse_float("-9"), mk<yama::float_t>(-9.0, 2));

    _TEST_SUCC(yama::parse_float("00_1_0_1"), mk<yama::float_t>(00101.0, 8));
    _TEST_SUCC(yama::parse_float("10_1_0_1"), mk<yama::float_t>(10101.0, 8));
    _TEST_SUCC(yama::parse_float("20_1_0_1"), mk<yama::float_t>(20101.0, 8));
    _TEST_SUCC(yama::parse_float("30_1_0_1"), mk<yama::float_t>(30101.0, 8));
    _TEST_SUCC(yama::parse_float("40_1_0_1"), mk<yama::float_t>(40101.0, 8));
    _TEST_SUCC(yama::parse_float("50_1_0_1"), mk<yama::float_t>(50101.0, 8));
    _TEST_SUCC(yama::parse_float("60_1_0_1"), mk<yama::float_t>(60101.0, 8));
    _TEST_SUCC(yama::parse_float("70_1_0_1"), mk<yama::float_t>(70101.0, 8));
    _TEST_SUCC(yama::parse_float("80_1_0_1"), mk<yama::float_t>(80101.0, 8));
    _TEST_SUCC(yama::parse_float("90_1_0_1"), mk<yama::float_t>(90101.0, 8));

    _TEST_SUCC(yama::parse_float("-00_1_0_1"), mk<yama::float_t>(-00101.0, 9));
    _TEST_SUCC(yama::parse_float("-10_1_0_1"), mk<yama::float_t>(-10101.0, 9));
    _TEST_SUCC(yama::parse_float("-20_1_0_1"), mk<yama::float_t>(-20101.0, 9));
    _TEST_SUCC(yama::parse_float("-30_1_0_1"), mk<yama::float_t>(-30101.0, 9));
    _TEST_SUCC(yama::parse_float("-40_1_0_1"), mk<yama::float_t>(-40101.0, 9));
    _TEST_SUCC(yama::parse_float("-50_1_0_1"), mk<yama::float_t>(-50101.0, 9));
    _TEST_SUCC(yama::parse_float("-60_1_0_1"), mk<yama::float_t>(-60101.0, 9));
    _TEST_SUCC(yama::parse_float("-70_1_0_1"), mk<yama::float_t>(-70101.0, 9));
    _TEST_SUCC(yama::parse_float("-80_1_0_1"), mk<yama::float_t>(-80101.0, 9));
    _TEST_SUCC(yama::parse_float("-90_1_0_1"), mk<yama::float_t>(-90101.0, 9));

    _TEST_SUCC(yama::parse_float("0000"), mk<yama::float_t>(0.0, 4));
    _TEST_SUCC(yama::parse_float("0001"), mk<yama::float_t>(1.0, 4));
    _TEST_SUCC(yama::parse_float("0002"), mk<yama::float_t>(2.0, 4));
    _TEST_SUCC(yama::parse_float("0003"), mk<yama::float_t>(3.0, 4));
    _TEST_SUCC(yama::parse_float("0004"), mk<yama::float_t>(4.0, 4));
    _TEST_SUCC(yama::parse_float("0005"), mk<yama::float_t>(5.0, 4));
    _TEST_SUCC(yama::parse_float("0006"), mk<yama::float_t>(6.0, 4));
    _TEST_SUCC(yama::parse_float("0007"), mk<yama::float_t>(7.0, 4));
    _TEST_SUCC(yama::parse_float("0008"), mk<yama::float_t>(8.0, 4));
    _TEST_SUCC(yama::parse_float("0009"), mk<yama::float_t>(9.0, 4));

    _TEST_SUCC(yama::parse_float("-0000"), mk<yama::float_t>(-0.0, 5));
    _TEST_SUCC(yama::parse_float("-0001"), mk<yama::float_t>(-1.0, 5));
    _TEST_SUCC(yama::parse_float("-0002"), mk<yama::float_t>(-2.0, 5));
    _TEST_SUCC(yama::parse_float("-0003"), mk<yama::float_t>(-3.0, 5));
    _TEST_SUCC(yama::parse_float("-0004"), mk<yama::float_t>(-4.0, 5));
    _TEST_SUCC(yama::parse_float("-0005"), mk<yama::float_t>(-5.0, 5));
    _TEST_SUCC(yama::parse_float("-0006"), mk<yama::float_t>(-6.0, 5));
    _TEST_SUCC(yama::parse_float("-0007"), mk<yama::float_t>(-7.0, 5));
    _TEST_SUCC(yama::parse_float("-0008"), mk<yama::float_t>(-8.0, 5));
    _TEST_SUCC(yama::parse_float("-0009"), mk<yama::float_t>(-9.0, 5));

    _TEST_SUCC(yama::parse_float("14e-2"), mk<yama::float_t>(0.14, 5));
    _TEST_SUCC(yama::parse_float("14e-1"), mk<yama::float_t>(1.4, 5));
    _TEST_SUCC(yama::parse_float("14e0"), mk<yama::float_t>(14.0, 4));
    _TEST_SUCC(yama::parse_float("14e1"), mk<yama::float_t>(140.0, 4));
    _TEST_SUCC(yama::parse_float("14e2"), mk<yama::float_t>(1400.0, 4));
    _TEST_SUCC(yama::parse_float("14e+0"), mk<yama::float_t>(14.0, 5));
    _TEST_SUCC(yama::parse_float("14e+1"), mk<yama::float_t>(140.0, 5));
    _TEST_SUCC(yama::parse_float("14e+2"), mk<yama::float_t>(1400.0, 5));

    // inf and nan

    _TEST_SUCC(yama::parse_float("infaaa"), mk<yama::float_t>(yama::inf, 3));
    _TEST_SUCC(yama::parse_float("-infaaa"), mk<yama::float_t>(-yama::inf, 4));

    EXPECT_TRUE(yama::parse_float("nanaaa"));
    if (auto a = yama::parse_float("nanaaa")) {
        EXPECT_EQ(a.value().bytes, 3);
        EXPECT_TRUE(std::isnan(a.value().v));
        //EXPECT_FALSE(std::signbit(a.value().v)); <- NaN values LOVE being arbitrarily +/-, regardless of parse_float input
    }

    EXPECT_TRUE(yama::parse_float("-nanaaa"));
    if (auto a = yama::parse_float("-nanaaa")) {
        EXPECT_EQ(a.value().bytes, 4);
        EXPECT_TRUE(std::isnan(a.value().v));
        //EXPECT_TRUE(std::signbit(a.value().v)); <- NaN values LOVE being arbitrarily +/-, regardless of parse_float input
    }

    // failures

    _TEST_FAIL(yama::parse_float(""));
    _TEST_FAIL(yama::parse_float(" "));
    _TEST_FAIL(yama::parse_float("!@#"));
    _TEST_FAIL(yama::parse_float("abc"));
    _TEST_FAIL(yama::parse_float("_"));

    _TEST_FAIL(yama::parse_float("-"));
    _TEST_FAIL(yama::parse_float("--0.0"));
    _TEST_FAIL(yama::parse_float("+0.0"));
    _TEST_FAIL(yama::parse_float("+.0"));
    _TEST_FAIL(yama::parse_float("+0"));

    _TEST_FAIL(yama::parse_float("_0.0"));
    _TEST_FAIL(yama::parse_float("0_.0"));
    _TEST_FAIL(yama::parse_float("0._0"));
    _TEST_FAIL(yama::parse_float("0.0_"));
    _TEST_FAIL(yama::parse_float("0__0.0"));
    _TEST_FAIL(yama::parse_float("0.0__0"));

    _TEST_FAIL(yama::parse_float("_.0"));
    _TEST_FAIL(yama::parse_float("._0"));
    _TEST_FAIL(yama::parse_float(".0_"));
    _TEST_FAIL(yama::parse_float(".0__0"));

    _TEST_FAIL(yama::parse_float("_0"));
    _TEST_FAIL(yama::parse_float("0_"));
    _TEST_FAIL(yama::parse_float("0__0"));

    _TEST_FAIL(yama::parse_float("_0e0"));
    _TEST_FAIL(yama::parse_float("0_e0"));
    _TEST_FAIL(yama::parse_float("0e_0"));
    _TEST_FAIL(yama::parse_float("0e0_"));
    _TEST_FAIL(yama::parse_float("0__0e0"));
    _TEST_FAIL(yama::parse_float("0e0__0"));

    _TEST_FAIL(yama::parse_float("_0e-0"));
    _TEST_FAIL(yama::parse_float("0_e-0"));
    _TEST_FAIL(yama::parse_float("0e_-0"));
    _TEST_FAIL(yama::parse_float("0e-_0"));
    _TEST_FAIL(yama::parse_float("0e-0_"));
    _TEST_FAIL(yama::parse_float("0__0e-0"));
    _TEST_FAIL(yama::parse_float("0e0__0"));

    _TEST_FAIL(yama::parse_float("_0e+0"));
    _TEST_FAIL(yama::parse_float("0_e+0"));
    _TEST_FAIL(yama::parse_float("0e_+0"));
    _TEST_FAIL(yama::parse_float("0e+_0"));
    _TEST_FAIL(yama::parse_float("0e+0_"));
    _TEST_FAIL(yama::parse_float("0__0e+0"));
    _TEST_FAIL(yama::parse_float("0e0__0"));
}

TEST(ScalarsTests, ParseFloat_Extremes) {
    // impl must be able to handle DBL_MAX as input
    //      * DBL_MAX == 1.7976931348623158e+308
    _TEST_SUCC(yama::parse_float("1.7976931348623158e308"), mk<yama::float_t>(DBL_MAX, 22));
    _TEST_SUCC(yama::parse_float("1.7976931348623158e+308"), mk<yama::float_t>(DBL_MAX, 23));

    // impl must be able to handle DBL_TRUE_MIN as input
    //      * DBL_TRUE_MIN == 4.9406564584124654e-324
    _TEST_SUCC(yama::parse_float("4.9406564584124654e-324"), mk<yama::float_t>(DBL_TRUE_MIN, 23));

    // impl must return +inf w/ overflow if input >DBL_MAX (w/ the amount big enough to not get rounded away)
    //      * first below has '...723158e...' instead of '...623158e...'
    //      * second below has 'e309' instead of 'e308'
    //          * anything w/ exponent >308 will overflow/underflow IEEE 754 float
    //      * third/fourth below are negative versions of first/second, respectively
    _TEST_SUCC(yama::parse_float("1.7976931348723158e308"), mk<yama::float_t>(yama::inf, 22, true));
    _TEST_SUCC(yama::parse_float("1.7976931348723158e+308"), mk<yama::float_t>(yama::inf, 23, true));
    _TEST_SUCC(yama::parse_float("1.0e309"), mk<yama::float_t>(yama::inf, 7, true));
    _TEST_SUCC(yama::parse_float("1.0e+309"), mk<yama::float_t>(yama::inf, 8, true));
    _TEST_SUCC(yama::parse_float("-1.7976931348723159e308"), mk<yama::float_t>(-yama::inf, 23, false, true));
    _TEST_SUCC(yama::parse_float("-1.7976931348723159e+308"), mk<yama::float_t>(-yama::inf, 24, false, true));
    _TEST_SUCC(yama::parse_float("-1.0e309"), mk<yama::float_t>(-yama::inf, 8, false, true));
    _TEST_SUCC(yama::parse_float("-1.0e+309"), mk<yama::float_t>(-yama::inf, 9, false, true));

    // TODO: I'm not entirely sure what should be done w/ values w/ exponent -324, but
    //       w/ mantissa <4.9406564584124654, as sometimes it seems to round to 0.0, and
    //       other times rounds to DBL_TRUE_MIN, and I'm not 100% sure what to make of it...

    // impl must ensure that any values w/ exponents below -324 round to 0.0
    _TEST_SUCC(yama::parse_float("4.9406564584124654e-325"), mk<yama::float_t>(0.0, 23));
    _TEST_SUCC(yama::parse_float("4.9406564584124654e-1325"), mk<yama::float_t>(0.0, 24));
    _TEST_SUCC(yama::parse_float("1.0e-325"), mk<yama::float_t>(0.0, 8));
    _TEST_SUCC(yama::parse_float("1.0e-1325"), mk<yama::float_t>(0.0, 9));
    _TEST_SUCC(yama::parse_float("0.0e-325"), mk<yama::float_t>(0.0, 8));
    _TEST_SUCC(yama::parse_float("0.0e-1325"), mk<yama::float_t>(0.0, 9));

    //constexpr yama::float_t aa = 1.0e-99999999999999999999999999999999999999999999999999999999999999999999999999999;

    // impl must be able to handle cases where digits of mantissa or exponent would overflow
    // or underflow during integer parsing
    _TEST_SUCC(yama::parse_float("99999999999999999999999999999999999999999999999999999999999999999999999999999.0"), mk<yama::float_t>(1.0e77, 79));
    _TEST_SUCC(yama::parse_float("0.99999999999999999999999999999999999999999999999999999999999999999999999999999"), mk<yama::float_t>(1.0, 79));
    _TEST_SUCC(yama::parse_float("-99999999999999999999999999999999999999999999999999999999999999999999999999999.0"), mk<yama::float_t>(-1.0e77, 80));
    _TEST_SUCC(yama::parse_float("-0.99999999999999999999999999999999999999999999999999999999999999999999999999999"), mk<yama::float_t>(-1.0, 80));
    _TEST_SUCC(yama::parse_float("1.0e99999999999999999999999999999999999999999999999999999999999999999999999999999"), mk<yama::float_t>(yama::inf, 81, true));
    _TEST_SUCC(yama::parse_float("1.0e+99999999999999999999999999999999999999999999999999999999999999999999999999999"), mk<yama::float_t>(yama::inf, 82, true));
    _TEST_SUCC(yama::parse_float("1.0e-99999999999999999999999999999999999999999999999999999999999999999999999999999"), mk<yama::float_t>(0.0, 82));
    _TEST_SUCC(yama::parse_float("-1.0e99999999999999999999999999999999999999999999999999999999999999999999999999999"), mk<yama::float_t>(-yama::inf, 82, false, true));
    _TEST_SUCC(yama::parse_float("-1.0e+99999999999999999999999999999999999999999999999999999999999999999999999999999"), mk<yama::float_t>(-yama::inf, 83, false, true));
    _TEST_SUCC(yama::parse_float("-1.0e-99999999999999999999999999999999999999999999999999999999999999999999999999999"), mk<yama::float_t>(0.0, 83));
}

TEST(ScalarsTests, ParseBool) {
    _TEST_SUCC(yama::parse_bool("true"), mk<yama::bool_t>(true, 4));
    _TEST_SUCC(yama::parse_bool("trueaaa"), mk<yama::bool_t>(true, 4));
    _TEST_SUCC(yama::parse_bool("false"), mk<yama::bool_t>(false, 5));
    _TEST_SUCC(yama::parse_bool("falseaaa"), mk<yama::bool_t>(false, 5));

    _TEST_FAIL(yama::parse_bool(""));
    _TEST_FAIL(yama::parse_bool(" "));
    _TEST_FAIL(yama::parse_bool("!@#"));
    _TEST_FAIL(yama::parse_bool("abc"));
    _TEST_FAIL(yama::parse_bool("123"));

    _TEST_FAIL(yama::parse_bool("True"));
    _TEST_FAIL(yama::parse_bool("TRUE"));
    _TEST_FAIL(yama::parse_bool("False"));
    _TEST_FAIL(yama::parse_bool("FALSE"));
}

TEST(ScalarsTests, ParseChar) {
    // ASCII & escape seqs

    for (yama::char_t c = 0; c <= 127; c++) {
        // ASCII char itself

        if (c != '\\') { // <- gotta skip '\\', as it requires escape seq to write
            _TEST_SUCC(yama::parse_char(std::format("{}", char(c))), mk<yama::char_t>(c, 1));
            _TEST_SUCC(yama::parse_char(std::format("{}aa", char(c))), mk<yama::char_t>(c, 1));
        }

        // escape seqs
        
        if (c == '\0') {
            _TEST_SUCC(yama::parse_char("\\0"), mk<yama::char_t>(c, 2));
            _TEST_SUCC(yama::parse_char("\\0aa"), mk<yama::char_t>(c, 2));
        }
        else if (c == '\a') {
            _TEST_SUCC(yama::parse_char("\\a"), mk<yama::char_t>(c, 2));
            _TEST_SUCC(yama::parse_char("\\aaa"), mk<yama::char_t>(c, 2));
        }
        else if (c == '\b') {
            _TEST_SUCC(yama::parse_char("\\b"), mk<yama::char_t>(c, 2));
            _TEST_SUCC(yama::parse_char("\\baa"), mk<yama::char_t>(c, 2));
        }
        else if (c == '\f') {
            _TEST_SUCC(yama::parse_char("\\f"), mk<yama::char_t>(c, 2));
            _TEST_SUCC(yama::parse_char("\\faa"), mk<yama::char_t>(c, 2));
        }
        else if (c == '\n') {
            _TEST_SUCC(yama::parse_char("\\n"), mk<yama::char_t>(c, 2));
            _TEST_SUCC(yama::parse_char("\\naa"), mk<yama::char_t>(c, 2));
        }
        else if (c == '\r') {
            _TEST_SUCC(yama::parse_char("\\r"), mk<yama::char_t>(c, 2));
            _TEST_SUCC(yama::parse_char("\\raa"), mk<yama::char_t>(c, 2));
        }
        else if (c == '\t') {
            _TEST_SUCC(yama::parse_char("\\t"), mk<yama::char_t>(c, 2));
            _TEST_SUCC(yama::parse_char("\\taa"), mk<yama::char_t>(c, 2));
        }
        else if (c == '\v') {
            _TEST_SUCC(yama::parse_char("\\v"), mk<yama::char_t>(c, 2));
            _TEST_SUCC(yama::parse_char("\\vaa"), mk<yama::char_t>(c, 2));
        }
        else if (c == '\'') {
            _TEST_SUCC(yama::parse_char("\\'"), mk<yama::char_t>(c, 2));
            _TEST_SUCC(yama::parse_char("\\'aa"), mk<yama::char_t>(c, 2));
        }
        else if (c == '\"') {
            _TEST_SUCC(yama::parse_char("\\\""), mk<yama::char_t>(c, 2));
            _TEST_SUCC(yama::parse_char("\\\"aa"), mk<yama::char_t>(c, 2));
        }
        else if (c == '\\') {
            _TEST_SUCC(yama::parse_char("\\\\"), mk<yama::char_t>(c, 2));
            _TEST_SUCC(yama::parse_char("\\\\aa"), mk<yama::char_t>(c, 2));
        }

        const auto b1_u = taul::hex_uppercase[c % 16];
        const auto b1_l = taul::hex_lowercase[c % 16];
        const auto b0_u = taul::hex_uppercase[c / 16];
        const auto b0_l = taul::hex_lowercase[c / 16];

        const std::string s0_u = std::format("\\x{}{}", b0_u, b1_u);
        const std::string s0_l = std::format("\\x{}{}", b0_l, b1_l);
        const std::string s1_u = std::format("\\u00{}{}", b0_u, b1_u);
        const std::string s1_l = std::format("\\u00{}{}", b0_l, b1_l);
        const std::string s2_u = std::format("\\U000000{}{}", b0_u, b1_u);
        const std::string s2_l = std::format("\\U000000{}{}", b0_l, b1_l);

        _TEST_SUCC(yama::parse_char(s0_u), mk<yama::char_t>(c, 4));
        _TEST_SUCC(yama::parse_char(s0_l), mk<yama::char_t>(c, 4));
        _TEST_SUCC(yama::parse_char(s1_u), mk<yama::char_t>(c, 6));
        _TEST_SUCC(yama::parse_char(s1_l), mk<yama::char_t>(c, 6));
        _TEST_SUCC(yama::parse_char(s2_u), mk<yama::char_t>(c, 10));
        _TEST_SUCC(yama::parse_char(s2_l), mk<yama::char_t>(c, 10));

        _TEST_SUCC(yama::parse_char(s0_u + "aa"), mk<yama::char_t>(c, 4));
        _TEST_SUCC(yama::parse_char(s0_l + "aa"), mk<yama::char_t>(c, 4));
        _TEST_SUCC(yama::parse_char(s1_u + "aa"), mk<yama::char_t>(c, 6));
        _TEST_SUCC(yama::parse_char(s1_l + "aa"), mk<yama::char_t>(c, 6));
        _TEST_SUCC(yama::parse_char(s2_u + "aa"), mk<yama::char_t>(c, 10));
        _TEST_SUCC(yama::parse_char(s2_l + "aa"), mk<yama::char_t>(c, 10));
    }

    // 8/16/32-bit hex escape seqs + mixed case letter digits

    _TEST_SUCC(yama::parse_char("\\xDb"), mk<yama::char_t>(U'\xdb', 4));
    _TEST_SUCC(yama::parse_char("\\xDbaa"), mk<yama::char_t>(U'\xdb', 4));
    _TEST_SUCC(yama::parse_char("\\xEa"), mk<yama::char_t>(U'\xea', 4));
    _TEST_SUCC(yama::parse_char("\\xEaaa"), mk<yama::char_t>(U'\xea', 4));
    
    _TEST_SUCC(yama::parse_char("\\ua1F4"), mk<yama::char_t>(U'\ua1f4', 6));
    _TEST_SUCC(yama::parse_char("\\ua1F4aa"), mk<yama::char_t>(U'\ua1f4', 6));
    _TEST_SUCC(yama::parse_char("\\uE83e"), mk<yama::char_t>(U'\ue83e', 6));
    _TEST_SUCC(yama::parse_char("\\uE83eaa"), mk<yama::char_t>(U'\ue83e', 6));

    _TEST_SUCC(yama::parse_char("\\U000DAaF4"), mk<yama::char_t>(U'\U000daaf4', 10));
    _TEST_SUCC(yama::parse_char("\\U000DAaF4aa"), mk<yama::char_t>(U'\U000daaf4', 10));
    _TEST_SUCC(yama::parse_char("\\U000AE83e"), mk<yama::char_t>(U'\U000ae83e', 10));
    _TEST_SUCC(yama::parse_char("\\U000AE83eaa"), mk<yama::char_t>(U'\U000ae83e', 10));

    // literalization

    _TEST_SUCC(yama::parse_char("\\y"), mk<yama::char_t>(U'y', 2));
    _TEST_SUCC(yama::parse_char("\\yaa"), mk<yama::char_t>(U'y', 2));
    _TEST_SUCC(yama::parse_char("\\4"), mk<yama::char_t>(U'4', 2));
    _TEST_SUCC(yama::parse_char("\\4aa"), mk<yama::char_t>(U'4', 2));
    _TEST_SUCC(yama::parse_char("\\&"), mk<yama::char_t>(U'&', 2));
    _TEST_SUCC(yama::parse_char("\\&aa"), mk<yama::char_t>(U'&', 2));
    _TEST_SUCC(yama::parse_char("\\ "), mk<yama::char_t>(U' ', 2));
    _TEST_SUCC(yama::parse_char("\\ aa"), mk<yama::char_t>(U' ', 2));

    // invalid 8/16/32-bit hex escape seqs literalize instead

    _TEST_SUCC(yama::parse_char("\\x8g"), mk<yama::char_t>(U'x', 2));
    _TEST_SUCC(yama::parse_char("\\x8gaa"), mk<yama::char_t>(U'x', 2));

    _TEST_SUCC(yama::parse_char("\\u80ag"), mk<yama::char_t>(U'u', 2));
    _TEST_SUCC(yama::parse_char("\\u80agaa"), mk<yama::char_t>(U'u', 2));

    _TEST_SUCC(yama::parse_char("\\U80aE37bg"), mk<yama::char_t>(U'U', 2));
    _TEST_SUCC(yama::parse_char("\\U80aE37bgaa"), mk<yama::char_t>(U'U', 2));

    // non-ASCII Unicode

    _TEST_SUCC(yama::parse_char("\\x80"), mk<yama::char_t>(U'\x80', 4));
    _TEST_SUCC(yama::parse_char("\\x80aa"), mk<yama::char_t>(U'\x80', 4));
    _TEST_SUCC(yama::parse_char("\\u0394"), mk<yama::char_t>(U'Δ', 6));
    _TEST_SUCC(yama::parse_char("\\u0394aa"), mk<yama::char_t>(U'Δ', 6));
    _TEST_SUCC(yama::parse_char("\\u9b42"), mk<yama::char_t>(U'魂', 6));
    _TEST_SUCC(yama::parse_char("\\u9b42aa"), mk<yama::char_t>(U'魂', 6));
    _TEST_SUCC(yama::parse_char("\\U0001f4a9"), mk<yama::char_t>(U'💩', 10));
    _TEST_SUCC(yama::parse_char("\\U0001f4a9aa"), mk<yama::char_t>(U'💩', 10));

    _TEST_SUCC(yama::parse_char(taul::utf8_s(u8"Δ")), mk<yama::char_t>(U'Δ', 2));
    _TEST_SUCC(yama::parse_char(taul::utf8_s(u8"Δaa")), mk<yama::char_t>(U'Δ', 2));
    _TEST_SUCC(yama::parse_char(taul::utf8_s(u8"魂")), mk<yama::char_t>(U'魂', 3));
    _TEST_SUCC(yama::parse_char(taul::utf8_s(u8"魂aa")), mk<yama::char_t>(U'魂', 3));
    _TEST_SUCC(yama::parse_char(taul::utf8_s(u8"💩")), mk<yama::char_t>(U'💩', 4));
    _TEST_SUCC(yama::parse_char(taul::utf8_s(u8"💩aa")), mk<yama::char_t>(U'💩', 4));
    
    // illegal Unicode

    // UTF-16 surrogate codepoint
    _TEST_SUCC(yama::parse_char("\\ud8a2"), mk<yama::char_t>((yama::char_t)0xd8a2, 6));
    _TEST_SUCC(yama::parse_char("\\ud8a2aa"), mk<yama::char_t>((yama::char_t)0xd8a2, 6));

    // codepoints outside Unicode codespace
    _TEST_SUCC(yama::parse_char("\\UbD0Aa1F4"), mk<yama::char_t>(yama::char_t(0xbd0aa1f4), 10));
    _TEST_SUCC(yama::parse_char("\\UbD0Aa1F4aa"), mk<yama::char_t>(yama::char_t(0xbd0aa1f4), 10));
    _TEST_SUCC(yama::parse_char("\\U6Af1E83e"), mk<yama::char_t>(yama::char_t(0x6af1e83e), 10));
    _TEST_SUCC(yama::parse_char("\\U6Af1E83eaa"), mk<yama::char_t>(yama::char_t(0x6af1e83e), 10));

    // failures

    _TEST_FAIL(yama::parse_char(""));
    _TEST_FAIL(yama::parse_char("\\"));
}

#undef _TEST_SUCC
#undef _TEST_FAIL

