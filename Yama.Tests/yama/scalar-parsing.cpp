

#include <gtest/gtest.h>
#include <taul/strings.h>
#include <yama/yama.h>

#define _YM_SUPPRESS_LOGGING 1
#include "scalar-parsing-helpers.h"


TEST(ScalarParsing, NullptrInput) {
	failure<YmInt>(nullptr);
	failure<YmUInt>(nullptr);
	failure<YmUInt>(nullptr, true);
	failure<YmFloat>(nullptr);
	failure<YmBool>(nullptr);
	failure<YmRune>(nullptr);
}

TEST(ScalarParsing, OutputDiscarded) {
	{
		size_t bytes = size_t(-1);
		EXPECT_EQ(ymParseInt("100", nullptr, &bytes), YmParseStatus_Success);
		EXPECT_EQ(bytes, 3);
	}
	{
		size_t bytes = size_t(-1);
		EXPECT_EQ(ymParseUInt("100u", nullptr, &bytes, false), YmParseStatus_Success);
		EXPECT_EQ(bytes, 4);
	}
	{
		size_t bytes = size_t(-1);
		EXPECT_EQ(ymParseFloat("3.14159", nullptr, &bytes), YmParseStatus_Success);
		EXPECT_EQ(bytes, 7);
	}
	{
		size_t bytes = size_t(-1);
		EXPECT_EQ(ymParseBool("true", nullptr, &bytes), YmParseStatus_Success);
		EXPECT_EQ(bytes, 4);
	}
	{
		size_t bytes = size_t(-1);
		EXPECT_EQ(ymParseRune("\\&", nullptr, &bytes), YmParseStatus_Success);
		EXPECT_EQ(bytes, 2);
	}
}

TEST(ScalarParsing, ByteCountDiscarded) {
	{
		YmInt output{};
		EXPECT_EQ(ymParseInt("100", &output, nullptr), YmParseStatus_Success);
		EXPECT_EQ(output, 100);
	}
	{
		YmUInt output{};
		EXPECT_EQ(ymParseUInt("100u", &output, nullptr, false), YmParseStatus_Success);
		EXPECT_EQ(output, 100);
	}
	{
		YmFloat output{};
		EXPECT_EQ(ymParseFloat("3.14159", &output, nullptr), YmParseStatus_Success);
		EXPECT_DOUBLE_EQ(output, 3.14159);
	}
	{
		YmBool output{};
		EXPECT_EQ(ymParseBool("true", &output, nullptr), YmParseStatus_Success);
		EXPECT_EQ(output, true);
	}
	{
		YmRune output{};
		EXPECT_EQ(ymParseRune("\\&", &output, nullptr), YmParseStatus_Success);
		EXPECT_EQ(output, U'&');
	}
}

TEST(ScalarParsing, Int) {
    // Decimal

    success<YmInt>("0", 0, 1);
    success<YmInt>("1", 1, 1);
    success<YmInt>("2", 2, 1);
    success<YmInt>("3", 3, 1);
    success<YmInt>("4", 4, 1);
    success<YmInt>("5", 5, 1);
    success<YmInt>("6", 6, 1);
    success<YmInt>("7", 7, 1);
    success<YmInt>("8", 8, 1);
    success<YmInt>("9", 9, 1);

    success<YmInt>("-0", 0, 2);
    success<YmInt>("-1", -1, 2);
    success<YmInt>("-2", -2, 2);
    success<YmInt>("-3", -3, 2);
    success<YmInt>("-4", -4, 2);
    success<YmInt>("-5", -5, 2);
    success<YmInt>("-6", -6, 2);
    success<YmInt>("-7", -7, 2);
    success<YmInt>("-8", -8, 2);
    success<YmInt>("-9", -9, 2);

    success<YmInt>("0_0_1", 1, 5);
    success<YmInt>("1_0_1", 101, 5);
    success<YmInt>("2_0_1", 201, 5);
    success<YmInt>("3_0_1", 301, 5);
    success<YmInt>("4_0_1", 401, 5);
    success<YmInt>("5_0_1", 501, 5);
    success<YmInt>("6_0_1", 601, 5);
    success<YmInt>("7_0_1", 701, 5);
    success<YmInt>("8_0_1", 801, 5);
    success<YmInt>("9_0_1", 901, 5);

    success<YmInt>("-0_0_1", -1, 6);
    success<YmInt>("-1_0_1", -101, 6);
    success<YmInt>("-2_0_1", -201, 6);
    success<YmInt>("-3_0_1", -301, 6);
    success<YmInt>("-4_0_1", -401, 6);
    success<YmInt>("-5_0_1", -501, 6);
    success<YmInt>("-6_0_1", -601, 6);
    success<YmInt>("-7_0_1", -701, 6);
    success<YmInt>("-8_0_1", -801, 6);
    success<YmInt>("-9_0_1", -901, 6);

    success<YmInt>("1230", 1230, 4);
    success<YmInt>("1230aa", 1230, 4);
    success<YmInt>("-1230", -1230, 5);
    success<YmInt>("-1230aa", -1230, 5);
    success<YmInt>("001230", 1230, 6);
    success<YmInt>("001230aa", 1230, 6);
    success<YmInt>("-001230", -1230, 7);
    success<YmInt>("-001230aa", -1230, 7);

    // Hexadecimal

    success<YmInt>("0x0", 0, 3);
    success<YmInt>("0x1", 1, 3);
    success<YmInt>("0x2", 2, 3);
    success<YmInt>("0x3", 3, 3);
    success<YmInt>("0x4", 4, 3);
    success<YmInt>("0x5", 5, 3);
    success<YmInt>("0x6", 6, 3);
    success<YmInt>("0x7", 7, 3);
    success<YmInt>("0x8", 8, 3);
    success<YmInt>("0x9", 9, 3);

    success<YmInt>("0xa", 10, 3);
    success<YmInt>("0xb", 11, 3);
    success<YmInt>("0xc", 12, 3);
    success<YmInt>("0xd", 13, 3);
    success<YmInt>("0xe", 14, 3);
    success<YmInt>("0xf", 15, 3);

    success<YmInt>("0xA", 10, 3);
    success<YmInt>("0xB", 11, 3);
    success<YmInt>("0xC", 12, 3);
    success<YmInt>("0xD", 13, 3);
    success<YmInt>("0xE", 14, 3);
    success<YmInt>("0xF", 15, 3);

    success<YmInt>("-0x0", -0, 4);
    success<YmInt>("-0x1", -1, 4);
    success<YmInt>("-0x2", -2, 4);
    success<YmInt>("-0x3", -3, 4);
    success<YmInt>("-0x4", -4, 4);
    success<YmInt>("-0x5", -5, 4);
    success<YmInt>("-0x6", -6, 4);
    success<YmInt>("-0x7", -7, 4);
    success<YmInt>("-0x8", -8, 4);
    success<YmInt>("-0x9", -9, 4);

    success<YmInt>("-0xa", -10, 4);
    success<YmInt>("-0xb", -11, 4);
    success<YmInt>("-0xc", -12, 4);
    success<YmInt>("-0xd", -13, 4);
    success<YmInt>("-0xe", -14, 4);
    success<YmInt>("-0xf", -15, 4);

    success<YmInt>("-0xA", -10, 4);
    success<YmInt>("-0xB", -11, 4);
    success<YmInt>("-0xC", -12, 4);
    success<YmInt>("-0xD", -13, 4);
    success<YmInt>("-0xE", -14, 4);
    success<YmInt>("-0xF", -15, 4);

    success<YmInt>("0x0_aE_3", 0x0ae3, 8);
    success<YmInt>("0x1_aE_3", 0x1ae3, 8);
    success<YmInt>("0x2_aE_3", 0x2ae3, 8);
    success<YmInt>("0x3_aE_3", 0x3ae3, 8);
    success<YmInt>("0x4_aE_3", 0x4ae3, 8);
    success<YmInt>("0x5_aE_3", 0x5ae3, 8);
    success<YmInt>("0x6_aE_3", 0x6ae3, 8);
    success<YmInt>("0x7_aE_3", 0x7ae3, 8);
    success<YmInt>("0x8_aE_3", 0x8ae3, 8);
    success<YmInt>("0x9_aE_3", 0x9ae3, 8);

    success<YmInt>("0xa_aE_3", 0xaae3, 8);
    success<YmInt>("0xb_aE_3", 0xbae3, 8);
    success<YmInt>("0xc_aE_3", 0xcae3, 8);
    success<YmInt>("0xd_aE_3", 0xdae3, 8);
    success<YmInt>("0xe_aE_3", 0xeae3, 8);
    success<YmInt>("0xf_aE_3", 0xfae3, 8);

    success<YmInt>("0xA_aE_3", 0xaae3, 8);
    success<YmInt>("0xB_aE_3", 0xbae3, 8);
    success<YmInt>("0xC_aE_3", 0xcae3, 8);
    success<YmInt>("0xD_aE_3", 0xdae3, 8);
    success<YmInt>("0xE_aE_3", 0xeae3, 8);
    success<YmInt>("0xF_aE_3", 0xfae3, 8);

    success<YmInt>("-0x0_aE_3", -0x0ae3, 9);
    success<YmInt>("-0x1_aE_3", -0x1ae3, 9);
    success<YmInt>("-0x2_aE_3", -0x2ae3, 9);
    success<YmInt>("-0x3_aE_3", -0x3ae3, 9);
    success<YmInt>("-0x4_aE_3", -0x4ae3, 9);
    success<YmInt>("-0x5_aE_3", -0x5ae3, 9);
    success<YmInt>("-0x6_aE_3", -0x6ae3, 9);
    success<YmInt>("-0x7_aE_3", -0x7ae3, 9);
    success<YmInt>("-0x8_aE_3", -0x8ae3, 9);
    success<YmInt>("-0x9_aE_3", -0x9ae3, 9);

    success<YmInt>("-0xa_aE_3", -0xaae3, 9);
    success<YmInt>("-0xb_aE_3", -0xbae3, 9);
    success<YmInt>("-0xc_aE_3", -0xcae3, 9);
    success<YmInt>("-0xd_aE_3", -0xdae3, 9);
    success<YmInt>("-0xe_aE_3", -0xeae3, 9);
    success<YmInt>("-0xf_aE_3", -0xfae3, 9);

    success<YmInt>("-0xA_aE_3", -0xaae3, 9);
    success<YmInt>("-0xB_aE_3", -0xbae3, 9);
    success<YmInt>("-0xC_aE_3", -0xcae3, 9);
    success<YmInt>("-0xD_aE_3", -0xdae3, 9);
    success<YmInt>("-0xE_aE_3", -0xeae3, 9);
    success<YmInt>("-0xF_aE_3", -0xfae3, 9);

    success<YmInt>("0x1aC23e4f", 0x1aC23e4f, 10);
    success<YmInt>("0x1aC23e4fggg", 0x1aC23e4f, 10);
    success<YmInt>("-0x1aC23e4f", -0x1aC23e4f, 11);
    success<YmInt>("-0x1aC23e4fggg", -0x1aC23e4f, 11);
    success<YmInt>("0x0001aC23e4f", 0x1aC23e4f, 13);
    success<YmInt>("0x0001aC23e4fggg", 0x1aC23e4f, 13);
    success<YmInt>("-0x0001aC23e4f", -0x1aC23e4f, 14);
    success<YmInt>("-0x0001aC23e4fggg", -0x1aC23e4f, 14);

    // Binary

    success<YmInt>("0b0", 0b0, 3);
    success<YmInt>("0b1", 0b1, 3);

    success<YmInt>("-0b0", -0b0, 4);
    success<YmInt>("-0b1", -0b1, 4);

    success<YmInt>("0b0_0_1", 0b001, 7);
    success<YmInt>("0b1_0_1", 0b101, 7);

    success<YmInt>("-0b0_0_1", -0b001, 8);
    success<YmInt>("-0b1_0_1", -0b101, 8);

    success<YmInt>("0b101011", 0b101011, 8);
    success<YmInt>("0b101011aaa", 0b101011, 8);
    success<YmInt>("-0b101011", -0b101011, 9);
    success<YmInt>("-0b101011aaa", -0b101011, 9);
    success<YmInt>("0b000101011", 0b101011, 11);
    success<YmInt>("0b000101011aaa", 0b101011, 11);
    success<YmInt>("-0b000101011", -0b101011, 12);
    success<YmInt>("-0b000101011aaa", -0b101011, 12);

    // Failures

    failure<YmInt>("");
    failure<YmInt>(" ");
    failure<YmInt>("!@#");
    failure<YmInt>("abc");

    failure<YmInt>("-");
    failure<YmInt>("+0");

    failure<YmInt>("_0");
    failure<YmInt>("0_");
    failure<YmInt>("0__0");

    //failure<YmInt>("0X0");
    failure<YmInt>("0x");
    failure<YmInt>("_0x0");
    failure<YmInt>("0_x0");
    failure<YmInt>("0x_0");
    failure<YmInt>("0x0_");
    failure<YmInt>("0x0__0");

    //failure<YmInt>("0B0");
    failure<YmInt>("0b");
    failure<YmInt>("_0b0");
    failure<YmInt>("0_b0");
    failure<YmInt>("0b_0");
    failure<YmInt>("0b0_");
    failure<YmInt>("0b0__0");
}

TEST(ScalarParsing, Int_Extremes) {
    // Max == 9223372036854775807
    // Min == -9223372036854775808

    // Must be able to handle max.
    success<YmInt>("9223372036854775807", 9223372036854775807, 19);

    // Must be able to handle min.
    success<YmInt>("-9223372036854775808", -(YmInt)9223372036854775808, 20);

    // Overflow, if one above max.
    overflow<YmInt>("9223372036854775808", 19);

    // Underflow, if one below min.
    underflow<YmInt>("-9223372036854775809", 20);
}

TEST(ScalarParsing, UInt) {
    // Decimal

    success<YmUInt>("0u", 0, 2);
    success<YmUInt>("1u", 1, 2);
    success<YmUInt>("2u", 2, 2);
    success<YmUInt>("3u", 3, 2);
    success<YmUInt>("4u", 4, 2);
    success<YmUInt>("5u", 5, 2);
    success<YmUInt>("6u", 6, 2);
    success<YmUInt>("7u", 7, 2);
    success<YmUInt>("8u", 8, 2);
    success<YmUInt>("9u", 9, 2);

    success<YmUInt>("0_0_1u", 1, 6);
    success<YmUInt>("1_0_1u", 101, 6);
    success<YmUInt>("2_0_1u", 201, 6);
    success<YmUInt>("3_0_1u", 301, 6);
    success<YmUInt>("4_0_1u", 401, 6);
    success<YmUInt>("5_0_1u", 501, 6);
    success<YmUInt>("6_0_1u", 601, 6);
    success<YmUInt>("7_0_1u", 701, 6);
    success<YmUInt>("8_0_1u", 801, 6);
    success<YmUInt>("9_0_1u", 901, 6);

    success<YmUInt>("1230u", 1230, 5);
    success<YmUInt>("1230uaa", 1230, 5);
    success<YmUInt>("001230u", 1230, 7);
    success<YmUInt>("001230uaa", 1230, 7);

    // Hexadecimal

    success<YmUInt>("0x0u", 0, 4);
    success<YmUInt>("0x1u", 1, 4);
    success<YmUInt>("0x2u", 2, 4);
    success<YmUInt>("0x3u", 3, 4);
    success<YmUInt>("0x4u", 4, 4);
    success<YmUInt>("0x5u", 5, 4);
    success<YmUInt>("0x6u", 6, 4);
    success<YmUInt>("0x7u", 7, 4);
    success<YmUInt>("0x8u", 8, 4);
    success<YmUInt>("0x9u", 9, 4);

    success<YmUInt>("0xau", 10, 4);
    success<YmUInt>("0xbu", 11, 4);
    success<YmUInt>("0xcu", 12, 4);
    success<YmUInt>("0xdu", 13, 4);
    success<YmUInt>("0xeu", 14, 4);
    success<YmUInt>("0xfu", 15, 4);

    success<YmUInt>("0xAu", 10, 4);
    success<YmUInt>("0xBu", 11, 4);
    success<YmUInt>("0xCu", 12, 4);
    success<YmUInt>("0xDu", 13, 4);
    success<YmUInt>("0xEu", 14, 4);
    success<YmUInt>("0xFu", 15, 4);

    success<YmUInt>("0x0_aE_3u", 0x0ae3, 9);
    success<YmUInt>("0x1_aE_3u", 0x1ae3, 9);
    success<YmUInt>("0x2_aE_3u", 0x2ae3, 9);
    success<YmUInt>("0x3_aE_3u", 0x3ae3, 9);
    success<YmUInt>("0x4_aE_3u", 0x4ae3, 9);
    success<YmUInt>("0x5_aE_3u", 0x5ae3, 9);
    success<YmUInt>("0x6_aE_3u", 0x6ae3, 9);
    success<YmUInt>("0x7_aE_3u", 0x7ae3, 9);
    success<YmUInt>("0x8_aE_3u", 0x8ae3, 9);
    success<YmUInt>("0x9_aE_3u", 0x9ae3, 9);

    success<YmUInt>("0xa_aE_3u", 0xaae3, 9);
    success<YmUInt>("0xb_aE_3u", 0xbae3, 9);
    success<YmUInt>("0xc_aE_3u", 0xcae3, 9);
    success<YmUInt>("0xd_aE_3u", 0xdae3, 9);
    success<YmUInt>("0xe_aE_3u", 0xeae3, 9);
    success<YmUInt>("0xf_aE_3u", 0xfae3, 9);

    success<YmUInt>("0xA_aE_3u", 0xaae3, 9);
    success<YmUInt>("0xB_aE_3u", 0xbae3, 9);
    success<YmUInt>("0xC_aE_3u", 0xcae3, 9);
    success<YmUInt>("0xD_aE_3u", 0xdae3, 9);
    success<YmUInt>("0xE_aE_3u", 0xeae3, 9);
    success<YmUInt>("0xF_aE_3u", 0xfae3, 9);

    success<YmUInt>("0x1aC23e4fu", 0x1aC23e4f, 11);
    success<YmUInt>("0x1aC23e4fuggg", 0x1aC23e4f, 11);
    success<YmUInt>("0x0001aC23e4fu", 0x1aC23e4f, 14);
    success<YmUInt>("0x0001aC23e4fuggg", 0x1aC23e4f, 14);

    // Binary

    success<YmUInt>("0b0u", 0b0, 4);
    success<YmUInt>("0b1u", 0b1, 4);

    success<YmUInt>("0b0_0_1u", 0b001, 8);
    success<YmUInt>("0b1_0_1u", 0b101, 8);

    success<YmUInt>("0b101011u", 0b101011, 9);
    success<YmUInt>("0b101011uaaa", 0b101011, 9);
    success<YmUInt>("0b000101011u", 0b101011, 12);
    success<YmUInt>("0b000101011uaaa", 0b101011, 12);

    // Failures

    failure<YmUInt>("");
    failure<YmUInt>(" ");
    failure<YmUInt>("!@#");
    failure<YmUInt>("abc");

    failure<YmUInt>("-0u");
    failure<YmUInt>("+0u");

    failure<YmUInt>("_0u");
    failure<YmUInt>("0_u");
    failure<YmUInt>("0__0u");
    failure<YmUInt>("0_u");
    //failure<YmUInt>("0u_");

    failure<YmUInt>("0X0u");
    failure<YmUInt>("0xu");
    failure<YmUInt>("_0x0u");
    failure<YmUInt>("0_x0u");
    failure<YmUInt>("0x_0u");
    failure<YmUInt>("0x0_u");
    failure<YmUInt>("0x0__0u");
    failure<YmUInt>("0x0_u");
    //failure<YmUInt>("0x0u_");

    failure<YmUInt>("0B0u");
    failure<YmUInt>("0bu");
    failure<YmUInt>("_0b0u");
    failure<YmUInt>("0_b0u");
    failure<YmUInt>("0b_0u");
    failure<YmUInt>("0b0_u");
    failure<YmUInt>("0b0__0u");
    failure<YmUInt>("0b0_u");
    //failure<YmUInt>("0b0u_");
}

TEST(ScalarParsing, UInt_IgnoreU) {
    // decimal

    success<YmUInt>("0u", 0, 1, true);
    success<YmUInt>("1u", 1, 1, true);
    success<YmUInt>("2u", 2, 1, true);
    success<YmUInt>("3u", 3, 1, true);
    success<YmUInt>("4u", 4, 1, true);
    success<YmUInt>("5u", 5, 1, true);
    success<YmUInt>("6u", 6, 1, true);
    success<YmUInt>("7u", 7, 1, true);
    success<YmUInt>("8u", 8, 1, true);
    success<YmUInt>("9u", 9, 1, true);

    success<YmUInt>("0", 0, 1, true);
    success<YmUInt>("1", 1, 1, true);
    success<YmUInt>("2", 2, 1, true);
    success<YmUInt>("3", 3, 1, true);
    success<YmUInt>("4", 4, 1, true);
    success<YmUInt>("5", 5, 1, true);
    success<YmUInt>("6", 6, 1, true);
    success<YmUInt>("7", 7, 1, true);
    success<YmUInt>("8", 8, 1, true);
    success<YmUInt>("9", 9, 1, true);

    success<YmUInt>("0_0_1u", 1, 5, true);
    success<YmUInt>("1_0_1u", 101, 5, true);
    success<YmUInt>("2_0_1u", 201, 5, true);
    success<YmUInt>("3_0_1u", 301, 5, true);
    success<YmUInt>("4_0_1u", 401, 5, true);
    success<YmUInt>("5_0_1u", 501, 5, true);
    success<YmUInt>("6_0_1u", 601, 5, true);
    success<YmUInt>("7_0_1u", 701, 5, true);
    success<YmUInt>("8_0_1u", 801, 5, true);
    success<YmUInt>("9_0_1u", 901, 5, true);

    success<YmUInt>("0_0_1", 1, 5, true);
    success<YmUInt>("1_0_1", 101, 5, true);
    success<YmUInt>("2_0_1", 201, 5, true);
    success<YmUInt>("3_0_1", 301, 5, true);
    success<YmUInt>("4_0_1", 401, 5, true);
    success<YmUInt>("5_0_1", 501, 5, true);
    success<YmUInt>("6_0_1", 601, 5, true);
    success<YmUInt>("7_0_1", 701, 5, true);
    success<YmUInt>("8_0_1", 801, 5, true);
    success<YmUInt>("9_0_1", 901, 5, true);

    success<YmUInt>("1230u", 1230, 4, true);
    success<YmUInt>("1230uaa", 1230, 4, true);
    success<YmUInt>("001230u", 1230, 6, true);
    success<YmUInt>("001230uaa", 1230, 6, true);

    success<YmUInt>("1230", 1230, 4, true);
    success<YmUInt>("1230aa", 1230, 4, true);
    success<YmUInt>("001230", 1230, 6, true);
    success<YmUInt>("001230aa", 1230, 6, true);

    // hexadecimal

    success<YmUInt>("0x0u", 0, 3, true);
    success<YmUInt>("0x1u", 1, 3, true);
    success<YmUInt>("0x2u", 2, 3, true);
    success<YmUInt>("0x3u", 3, 3, true);
    success<YmUInt>("0x4u", 4, 3, true);
    success<YmUInt>("0x5u", 5, 3, true);
    success<YmUInt>("0x6u", 6, 3, true);
    success<YmUInt>("0x7u", 7, 3, true);
    success<YmUInt>("0x8u", 8, 3, true);
    success<YmUInt>("0x9u", 9, 3, true);

    success<YmUInt>("0x0", 0, 3, true);
    success<YmUInt>("0x1", 1, 3, true);
    success<YmUInt>("0x2", 2, 3, true);
    success<YmUInt>("0x3", 3, 3, true);
    success<YmUInt>("0x4", 4, 3, true);
    success<YmUInt>("0x5", 5, 3, true);
    success<YmUInt>("0x6", 6, 3, true);
    success<YmUInt>("0x7", 7, 3, true);
    success<YmUInt>("0x8", 8, 3, true);
    success<YmUInt>("0x9", 9, 3, true);

    success<YmUInt>("0xau", 10, 3, true);
    success<YmUInt>("0xbu", 11, 3, true);
    success<YmUInt>("0xcu", 12, 3, true);
    success<YmUInt>("0xdu", 13, 3, true);
    success<YmUInt>("0xeu", 14, 3, true);
    success<YmUInt>("0xfu", 15, 3, true);

    success<YmUInt>("0xa", 10, 3, true);
    success<YmUInt>("0xb", 11, 3, true);
    success<YmUInt>("0xc", 12, 3, true);
    success<YmUInt>("0xd", 13, 3, true);
    success<YmUInt>("0xe", 14, 3, true);
    success<YmUInt>("0xf", 15, 3, true);

    success<YmUInt>("0xAu", 10, 3, true);
    success<YmUInt>("0xBu", 11, 3, true);
    success<YmUInt>("0xCu", 12, 3, true);
    success<YmUInt>("0xDu", 13, 3, true);
    success<YmUInt>("0xEu", 14, 3, true);
    success<YmUInt>("0xFu", 15, 3, true);

    success<YmUInt>("0xA", 10, 3, true);
    success<YmUInt>("0xB", 11, 3, true);
    success<YmUInt>("0xC", 12, 3, true);
    success<YmUInt>("0xD", 13, 3, true);
    success<YmUInt>("0xE", 14, 3, true);
    success<YmUInt>("0xF", 15, 3, true);

    success<YmUInt>("0x0_aE_3u", 0x0ae3, 8, true);
    success<YmUInt>("0x1_aE_3u", 0x1ae3, 8, true);
    success<YmUInt>("0x2_aE_3u", 0x2ae3, 8, true);
    success<YmUInt>("0x3_aE_3u", 0x3ae3, 8, true);
    success<YmUInt>("0x4_aE_3u", 0x4ae3, 8, true);
    success<YmUInt>("0x5_aE_3u", 0x5ae3, 8, true);
    success<YmUInt>("0x6_aE_3u", 0x6ae3, 8, true);
    success<YmUInt>("0x7_aE_3u", 0x7ae3, 8, true);
    success<YmUInt>("0x8_aE_3u", 0x8ae3, 8, true);
    success<YmUInt>("0x9_aE_3u", 0x9ae3, 8, true);

    success<YmUInt>("0x0_aE_3", 0x0ae3, 8, true);
    success<YmUInt>("0x1_aE_3", 0x1ae3, 8, true);
    success<YmUInt>("0x2_aE_3", 0x2ae3, 8, true);
    success<YmUInt>("0x3_aE_3", 0x3ae3, 8, true);
    success<YmUInt>("0x4_aE_3", 0x4ae3, 8, true);
    success<YmUInt>("0x5_aE_3", 0x5ae3, 8, true);
    success<YmUInt>("0x6_aE_3", 0x6ae3, 8, true);
    success<YmUInt>("0x7_aE_3", 0x7ae3, 8, true);
    success<YmUInt>("0x8_aE_3", 0x8ae3, 8, true);
    success<YmUInt>("0x9_aE_3", 0x9ae3, 8, true);

    success<YmUInt>("0xa_aE_3u", 0xaae3, 8, true);
    success<YmUInt>("0xb_aE_3u", 0xbae3, 8, true);
    success<YmUInt>("0xc_aE_3u", 0xcae3, 8, true);
    success<YmUInt>("0xd_aE_3u", 0xdae3, 8, true);
    success<YmUInt>("0xe_aE_3u", 0xeae3, 8, true);
    success<YmUInt>("0xf_aE_3u", 0xfae3, 8, true);

    success<YmUInt>("0xa_aE_3", 0xaae3, 8, true);
    success<YmUInt>("0xb_aE_3", 0xbae3, 8, true);
    success<YmUInt>("0xc_aE_3", 0xcae3, 8, true);
    success<YmUInt>("0xd_aE_3", 0xdae3, 8, true);
    success<YmUInt>("0xe_aE_3", 0xeae3, 8, true);
    success<YmUInt>("0xf_aE_3", 0xfae3, 8, true);

    success<YmUInt>("0xA_aE_3u", 0xaae3, 8, true);
    success<YmUInt>("0xB_aE_3u", 0xbae3, 8, true);
    success<YmUInt>("0xC_aE_3u", 0xcae3, 8, true);
    success<YmUInt>("0xD_aE_3u", 0xdae3, 8, true);
    success<YmUInt>("0xE_aE_3u", 0xeae3, 8, true);
    success<YmUInt>("0xF_aE_3u", 0xfae3, 8, true);

    success<YmUInt>("0xA_aE_3", 0xaae3, 8, true);
    success<YmUInt>("0xB_aE_3", 0xbae3, 8, true);
    success<YmUInt>("0xC_aE_3", 0xcae3, 8, true);
    success<YmUInt>("0xD_aE_3", 0xdae3, 8, true);
    success<YmUInt>("0xE_aE_3", 0xeae3, 8, true);
    success<YmUInt>("0xF_aE_3", 0xfae3, 8, true);

    success<YmUInt>("0x1aC23e4fu", 0x1aC23e4f, 10, true);
    success<YmUInt>("0x1aC23e4fuggg", 0x1aC23e4f, 10, true);
    success<YmUInt>("0x0001aC23e4fu", 0x1aC23e4f, 13, true);
    success<YmUInt>("0x0001aC23e4fuggg", 0x1aC23e4f, 13, true);

    success<YmUInt>("0x1aC23e4f", 0x1aC23e4f, 10, true);
    success<YmUInt>("0x1aC23e4fggg", 0x1aC23e4f, 10, true);
    success<YmUInt>("0x0001aC23e4f", 0x1aC23e4f, 13, true);
    success<YmUInt>("0x0001aC23e4fggg", 0x1aC23e4f, 13, true);

    // binary

    success<YmUInt>("0b0u", 0b0, 3, true);
    success<YmUInt>("0b1u", 0b1, 3, true);

    success<YmUInt>("0b0", 0b0, 3, true);
    success<YmUInt>("0b1", 0b1, 3, true);

    success<YmUInt>("0b0_0_1u", 0b001, 7, true);
    success<YmUInt>("0b1_0_1u", 0b101, 7, true);

    success<YmUInt>("0b0_0_1", 0b001, 7, true);
    success<YmUInt>("0b1_0_1", 0b101, 7, true);

    success<YmUInt>("0b101011u", 0b101011, 8, true);
    success<YmUInt>("0b101011uaaa", 0b101011, 8, true);
    success<YmUInt>("0b000101011u", 0b101011, 11, true);
    success<YmUInt>("0b000101011uaaa", 0b101011, 11, true);

    success<YmUInt>("0b101011", 0b101011, 8, true);
    success<YmUInt>("0b101011aaa", 0b101011, 8, true);
    success<YmUInt>("0b000101011", 0b101011, 11, true);
    success<YmUInt>("0b000101011aaa", 0b101011, 11, true);

    // failures

    failure<YmUInt>("", true);
    failure<YmUInt>(" ", true);
    failure<YmUInt>("!@#", true);
    failure<YmUInt>("abc", true);

    failure<YmUInt>("-0u", true);
    failure<YmUInt>("+0u", true);

    failure<YmUInt>("_0u", true);
    failure<YmUInt>("0_u", true);
    failure<YmUInt>("0__0u", true);
    failure<YmUInt>("0_u", true);
    //failure<YmUInt>("0u_", true);

    //failure<YmUInt>("0X0u", true);
    failure<YmUInt>("0xu", true);
    failure<YmUInt>("_0x0u", true);
    failure<YmUInt>("0_x0u", true);
    failure<YmUInt>("0x_0u", true);
    failure<YmUInt>("0x0_u", true);
    failure<YmUInt>("0x0__0u", true);
    failure<YmUInt>("0x0_u", true);
    //failure<YmUInt>("0x0u_", true);

    //failure<YmUInt>("0B0u", true);
    failure<YmUInt>("0bu", true);
    failure<YmUInt>("_0b0u", true);
    failure<YmUInt>("0_b0u", true);
    failure<YmUInt>("0b_0u", true);
    failure<YmUInt>("0b0_u", true);
    failure<YmUInt>("0b0__0u", true);
    failure<YmUInt>("0b0_u", true);
    //failure<YmUInt>("0b0u_", true);
}

TEST(ScalarParsing, UInt_Extremes) {
    // NOTE: Not gonna test underflow, as those shouldn't even parse.

    // Max == 18446744073709551615

    // Must be able to handle max.
    success<YmUInt>("18446744073709551615u", 18446744073709551615, 21);

    // Overflow, if one above max.
    overflow<YmUInt>("18446744073709551616u", 21);
}

TEST(ScalarParsing, UInt_IgnoreU_Extremes) {
    // NOTE: Not gonna test underflow, as those shouldn't even parse.

    // Max == 18446744073709551615

    // Must be able to handle max.
    success<YmUInt>("18446744073709551615u", 18446744073709551615, 20, true); // Should ignore 'u'.
    success<YmUInt>("18446744073709551615", 18446744073709551615, 20, true);

    // Overflow, if one above max.
    overflow<YmUInt>("18446744073709551616u", 20, true); // Should ignore 'u'.
    overflow<YmUInt>("18446744073709551616", 20, true);
}

TEST(ScalarParsing, Float) {
    success<YmFloat>("0.0", 0.0, 3);
    success<YmFloat>("1.0", 1.0, 3);
    success<YmFloat>("2.0", 2.0, 3);
    success<YmFloat>("3.0", 3.0, 3);
    success<YmFloat>("4.0", 4.0, 3);
    success<YmFloat>("5.0", 5.0, 3);
    success<YmFloat>("6.0", 6.0, 3);
    success<YmFloat>("7.0", 7.0, 3);
    success<YmFloat>("8.0", 8.0, 3);
    success<YmFloat>("9.0", 9.0, 3);

    success<YmFloat>("-0.0", -0.0, 4);
    success<YmFloat>("-1.0", -1.0, 4);
    success<YmFloat>("-2.0", -2.0, 4);
    success<YmFloat>("-3.0", -3.0, 4);
    success<YmFloat>("-4.0", -4.0, 4);
    success<YmFloat>("-5.0", -5.0, 4);
    success<YmFloat>("-6.0", -6.0, 4);
    success<YmFloat>("-7.0", -7.0, 4);
    success<YmFloat>("-8.0", -8.0, 4);
    success<YmFloat>("-9.0", -9.0, 4);

    success<YmFloat>("0.00", 0.00, 4);
    success<YmFloat>("0.01", 0.01, 4);
    success<YmFloat>("0.02", 0.02, 4);
    success<YmFloat>("0.03", 0.03, 4);
    success<YmFloat>("0.04", 0.04, 4);
    success<YmFloat>("0.05", 0.05, 4);
    success<YmFloat>("0.06", 0.06, 4);
    success<YmFloat>("0.07", 0.07, 4);
    success<YmFloat>("0.08", 0.08, 4);
    success<YmFloat>("0.09", 0.09, 4);

    success<YmFloat>("-0.00", -0.00, 5);
    success<YmFloat>("-0.01", -0.01, 5);
    success<YmFloat>("-0.02", -0.02, 5);
    success<YmFloat>("-0.03", -0.03, 5);
    success<YmFloat>("-0.04", -0.04, 5);
    success<YmFloat>("-0.05", -0.05, 5);
    success<YmFloat>("-0.06", -0.06, 5);
    success<YmFloat>("-0.07", -0.07, 5);
    success<YmFloat>("-0.08", -0.08, 5);
    success<YmFloat>("-0.09", -0.09, 5);

    success<YmFloat>(".00", 0.00, 3);
    success<YmFloat>(".01", 0.01, 3);
    success<YmFloat>(".02", 0.02, 3);
    success<YmFloat>(".03", 0.03, 3);
    success<YmFloat>(".04", 0.04, 3);
    success<YmFloat>(".05", 0.05, 3);
    success<YmFloat>(".06", 0.06, 3);
    success<YmFloat>(".07", 0.07, 3);
    success<YmFloat>(".08", 0.08, 3);
    success<YmFloat>(".09", 0.09, 3);

    success<YmFloat>("-.00", -0.00, 4);
    success<YmFloat>("-.01", -0.01, 4);
    success<YmFloat>("-.02", -0.02, 4);
    success<YmFloat>("-.03", -0.03, 4);
    success<YmFloat>("-.04", -0.04, 4);
    success<YmFloat>("-.05", -0.05, 4);
    success<YmFloat>("-.06", -0.06, 4);
    success<YmFloat>("-.07", -0.07, 4);
    success<YmFloat>("-.08", -0.08, 4);
    success<YmFloat>("-.09", -0.09, 4);

    success<YmFloat>("0_1.0_1_0", 01.01, 9);
    success<YmFloat>("1_1.0_1_0", 11.01, 9);
    success<YmFloat>("2_1.0_1_0", 21.01, 9);
    success<YmFloat>("3_1.0_1_0", 31.01, 9);
    success<YmFloat>("4_1.0_1_0", 41.01, 9);
    success<YmFloat>("5_1.0_1_0", 51.01, 9);
    success<YmFloat>("6_1.0_1_0", 61.01, 9);
    success<YmFloat>("7_1.0_1_0", 71.01, 9);
    success<YmFloat>("8_1.0_1_0", 81.01, 9);
    success<YmFloat>("9_1.0_1_0", 91.01, 9);

    success<YmFloat>("-0_1.0_1_0", -01.01, 10);
    success<YmFloat>("-1_1.0_1_0", -11.01, 10);
    success<YmFloat>("-2_1.0_1_0", -21.01, 10);
    success<YmFloat>("-3_1.0_1_0", -31.01, 10);
    success<YmFloat>("-4_1.0_1_0", -41.01, 10);
    success<YmFloat>("-5_1.0_1_0", -51.01, 10);
    success<YmFloat>("-6_1.0_1_0", -61.01, 10);
    success<YmFloat>("-7_1.0_1_0", -71.01, 10);
    success<YmFloat>("-8_1.0_1_0", -81.01, 10);
    success<YmFloat>("-9_1.0_1_0", -91.01, 10);

    success<YmFloat>("3.14159", 3.14159, 7);
    success<YmFloat>("-3.14159", -3.14159, 8);

    success<YmFloat>("3141.59e-3", 3.14159, 10);
    success<YmFloat>("314.159e-2", 3.14159, 10);
    success<YmFloat>("31.4159e-1", 3.14159, 10);
    success<YmFloat>("3.14159e0", 3.14159, 9);
    success<YmFloat>("0.314159e1", 3.14159, 10);
    success<YmFloat>("0.0314159e2", 3.14159, 11);
    success<YmFloat>("0.00314159e3", 3.14159, 12);
    success<YmFloat>("3.14159e+0", 3.14159, 10);
    success<YmFloat>("0.314159e+1", 3.14159, 11);
    success<YmFloat>("0.0314159e+2", 3.14159, 12);
    success<YmFloat>("0.00314159e+3", 3.14159, 13);

    success<YmFloat>("3141.59e-00_0_3", 3.14159, 15);
    success<YmFloat>("314.159e-00_0_2", 3.14159, 15);
    success<YmFloat>("31.4159e-00_0_1", 3.14159, 15);
    success<YmFloat>("3.14159e00_0_0", 3.14159, 14);
    success<YmFloat>("0.314159e00_0_1", 3.14159, 15);
    success<YmFloat>("0.0314159e00_0_2", 3.14159, 16);
    success<YmFloat>("0.00314159e00_0_3", 3.14159, 17);
    success<YmFloat>("3.14159e+00_0_0", 3.14159, 15);
    success<YmFloat>("0.314159e+00_0_1", 3.14159, 16);
    success<YmFloat>("0.0314159e+00_0_2", 3.14159, 17);
    success<YmFloat>("0.00314159e+00_0_3", 3.14159, 18);

    success<YmFloat>("-3141.59e-00_0_3", -3.14159, 16);
    success<YmFloat>("-314.159e-00_0_2", -3.14159, 16);
    success<YmFloat>("-31.4159e-00_0_1", -3.14159, 16);
    success<YmFloat>("-3.14159e00_0_0", -3.14159, 15);
    success<YmFloat>("-0.314159e00_0_1", -3.14159, 16);
    success<YmFloat>("-0.0314159e00_0_2", -3.14159, 17);
    success<YmFloat>("-0.00314159e00_0_3", -3.14159, 18);
    success<YmFloat>("-3.14159e+00_0_0", -3.14159, 16);
    success<YmFloat>("-0.314159e+00_0_1", -3.14159, 17);
    success<YmFloat>("-0.0314159e+00_0_2", -3.14159, 18);
    success<YmFloat>("-0.00314159e+00_0_3", -3.14159, 19);

    // Integer-Like (ie. Legal outside Yama code.)

    success<YmFloat>("0", 0.0, 1);
    success<YmFloat>("1", 1.0, 1);
    success<YmFloat>("2", 2.0, 1);
    success<YmFloat>("3", 3.0, 1);
    success<YmFloat>("4", 4.0, 1);
    success<YmFloat>("5", 5.0, 1);
    success<YmFloat>("6", 6.0, 1);
    success<YmFloat>("7", 7.0, 1);
    success<YmFloat>("8", 8.0, 1);
    success<YmFloat>("9", 9.0, 1);

    success<YmFloat>("-0", -0.0, 2);
    success<YmFloat>("-1", -1.0, 2);
    success<YmFloat>("-2", -2.0, 2);
    success<YmFloat>("-3", -3.0, 2);
    success<YmFloat>("-4", -4.0, 2);
    success<YmFloat>("-5", -5.0, 2);
    success<YmFloat>("-6", -6.0, 2);
    success<YmFloat>("-7", -7.0, 2);
    success<YmFloat>("-8", -8.0, 2);
    success<YmFloat>("-9", -9.0, 2);

    success<YmFloat>("00_1_0_1", 00101.0, 8);
    success<YmFloat>("10_1_0_1", 10101.0, 8);
    success<YmFloat>("20_1_0_1", 20101.0, 8);
    success<YmFloat>("30_1_0_1", 30101.0, 8);
    success<YmFloat>("40_1_0_1", 40101.0, 8);
    success<YmFloat>("50_1_0_1", 50101.0, 8);
    success<YmFloat>("60_1_0_1", 60101.0, 8);
    success<YmFloat>("70_1_0_1", 70101.0, 8);
    success<YmFloat>("80_1_0_1", 80101.0, 8);
    success<YmFloat>("90_1_0_1", 90101.0, 8);

    success<YmFloat>("-00_1_0_1", -00101.0, 9);
    success<YmFloat>("-10_1_0_1", -10101.0, 9);
    success<YmFloat>("-20_1_0_1", -20101.0, 9);
    success<YmFloat>("-30_1_0_1", -30101.0, 9);
    success<YmFloat>("-40_1_0_1", -40101.0, 9);
    success<YmFloat>("-50_1_0_1", -50101.0, 9);
    success<YmFloat>("-60_1_0_1", -60101.0, 9);
    success<YmFloat>("-70_1_0_1", -70101.0, 9);
    success<YmFloat>("-80_1_0_1", -80101.0, 9);
    success<YmFloat>("-90_1_0_1", -90101.0, 9);

    success<YmFloat>("0000", 0.0, 4);
    success<YmFloat>("0001", 1.0, 4);
    success<YmFloat>("0002", 2.0, 4);
    success<YmFloat>("0003", 3.0, 4);
    success<YmFloat>("0004", 4.0, 4);
    success<YmFloat>("0005", 5.0, 4);
    success<YmFloat>("0006", 6.0, 4);
    success<YmFloat>("0007", 7.0, 4);
    success<YmFloat>("0008", 8.0, 4);
    success<YmFloat>("0009", 9.0, 4);

    success<YmFloat>("-0000", -0.0, 5);
    success<YmFloat>("-0001", -1.0, 5);
    success<YmFloat>("-0002", -2.0, 5);
    success<YmFloat>("-0003", -3.0, 5);
    success<YmFloat>("-0004", -4.0, 5);
    success<YmFloat>("-0005", -5.0, 5);
    success<YmFloat>("-0006", -6.0, 5);
    success<YmFloat>("-0007", -7.0, 5);
    success<YmFloat>("-0008", -8.0, 5);
    success<YmFloat>("-0009", -9.0, 5);

    success<YmFloat>("14e-2", 0.14, 5);
    success<YmFloat>("14e-1", 1.4, 5);
    success<YmFloat>("14e0", 14.0, 4);
    success<YmFloat>("14e1", 140.0, 4);
    success<YmFloat>("14e2", 1400.0, 4);
    success<YmFloat>("14e+0", 14.0, 5);
    success<YmFloat>("14e+1", 140.0, 5);
    success<YmFloat>("14e+2", 1400.0, 5);

    // Infinities/NaNs

    success<YmFloat>("infaaa", YM_INF, 3);
    success<YmFloat>("-infaaa", -YM_INF, 4);

    // TODO: Migrated this over from old tests, but NaN support hasn't been added yet,
    //       so this code hasn't been refactored yet, and is just commented out.

    //EXPECT_TRUE(yama::parse_float("nanaaa"));
    //if (auto a = yama::parse_float("nanaaa")) {
    //    EXPECT_EQ(a.value().bytes, 3);
    //    EXPECT_TRUE(std::isnan(a.value().v));
    //    //EXPECT_FALSE(std::signbit(a.value().v)); <- NaN values LOVE being arbitrarily +/-, regardless of parse_float input
    //}
    //
    //EXPECT_TRUE(yama::parse_float("-nanaaa"));
    //if (auto a = yama::parse_float("-nanaaa")) {
    //    EXPECT_EQ(a.value().bytes, 4);
    //    EXPECT_TRUE(std::isnan(a.value().v));
    //    //EXPECT_TRUE(std::signbit(a.value().v)); <- NaN values LOVE being arbitrarily +/-, regardless of parse_float input
    //}

    // Failures

    failure<YmFloat>("");
    failure<YmFloat>(" ");
    failure<YmFloat>("!@#");
    failure<YmFloat>("abc");
    failure<YmFloat>("_");

    failure<YmFloat>("-");
    failure<YmFloat>("--0.0");
    failure<YmFloat>("+0.0");
    failure<YmFloat>("+.0");
    failure<YmFloat>("+0");

    failure<YmFloat>("_0.0");
    failure<YmFloat>("0_.0");
    failure<YmFloat>("0._0");
    failure<YmFloat>("0.0_");
    failure<YmFloat>("0__0.0");
    failure<YmFloat>("0.0__0");

    failure<YmFloat>("_.0");
    failure<YmFloat>("._0");
    failure<YmFloat>(".0_");
    failure<YmFloat>(".0__0");

    failure<YmFloat>("_0");
    failure<YmFloat>("0_");
    failure<YmFloat>("0__0");

    failure<YmFloat>("_0e0");
    failure<YmFloat>("0_e0");
    failure<YmFloat>("0e_0");
    failure<YmFloat>("0e0_");
    failure<YmFloat>("0__0e0");
    failure<YmFloat>("0e0__0");

    failure<YmFloat>("_0e-0");
    failure<YmFloat>("0_e-0");
    failure<YmFloat>("0e_-0");
    failure<YmFloat>("0e-_0");
    failure<YmFloat>("0e-0_");
    failure<YmFloat>("0__0e-0");
    failure<YmFloat>("0e0__0");

    failure<YmFloat>("_0e+0");
    failure<YmFloat>("0_e+0");
    failure<YmFloat>("0e_+0");
    failure<YmFloat>("0e+_0");
    failure<YmFloat>("0e+0_");
    failure<YmFloat>("0__0e+0");
    failure<YmFloat>("0e0__0");
}

TEST(ScalarParsing, Float_Extremes) {
    // Must be able to handle DBL_MAX as input.
    //      * DBL_MAX == 1.7976931348623158e+308
    success<YmFloat>("1.7976931348623158e308", DBL_MAX, 22);
    success<YmFloat>("1.7976931348623158e+308", DBL_MAX, 23);

    // Must be able to handle DBL_TRUE_MIN as input.
    //      * DBL_TRUE_MIN == 4.9406564584124654e-324
    success<YmFloat>("4.9406564584124654e-324", DBL_TRUE_MIN, 23);

    // Must return +inf w/ overflow if input >DBL_MAX (w/ the amount big enough to not get rounded away.)
    //      * First below has '...723158e...' instead of '...623158e...'.
    //      * Second below has 'e309' instead of 'e308'.
    //          * Anything w/ exponent >308 will overflow/underflow IEEE 754 float.
    //      * Third/fourth below are negative versions of first/second, respectively.
    overflow<YmFloat>("1.7976931348723158e308", 22);
    overflow<YmFloat>("1.7976931348723158e+308", 23);
    overflow<YmFloat>("1.0e309", 7);
    overflow<YmFloat>("1.0e+309", 8);
    underflow<YmFloat>("-1.7976931348723159e308", 23);
    underflow<YmFloat>("-1.7976931348723159e+308", 24);
    underflow<YmFloat>("-1.0e309", 8);
    underflow<YmFloat>("-1.0e+309", 9);

    // TODO: I'm not entirely sure what should be done w/ values w/ exponent -324, but
    //       w/ mantissa <4.9406564584124654, as sometimes it seems to round to 0.0, and
    //       other times rounds to DBL_TRUE_MIN, and I'm not 100% sure what to make of it...

    // Must ensure that any values w/ exponents below -324 round to 0.0.
    success<YmFloat>("4.9406564584124654e-325", 0.0, 23);
    success<YmFloat>("4.9406564584124654e-1325", 0.0, 24);
    success<YmFloat>("1.0e-325", 0.0, 8);
    success<YmFloat>("1.0e-1325", 0.0, 9);
    success<YmFloat>("0.0e-325", 0.0, 8);
    success<YmFloat>("0.0e-1325", 0.0, 9);

    // Must be able to handle cases where digits of mantissa or exponent would overflow
    // or underflow during integer parsing.
    success<YmFloat>("99999999999999999999999999999999999999999999999999999999999999999999999999999.0", 1.0e77, 79);
    success<YmFloat>("0.99999999999999999999999999999999999999999999999999999999999999999999999999999", 1.0, 79);
    success<YmFloat>("-99999999999999999999999999999999999999999999999999999999999999999999999999999.0", -1.0e77, 80);
    success<YmFloat>("-0.99999999999999999999999999999999999999999999999999999999999999999999999999999", -1.0, 80);
    overflow<YmFloat>("1.0e99999999999999999999999999999999999999999999999999999999999999999999999999999", 81);
    overflow<YmFloat>("1.0e+99999999999999999999999999999999999999999999999999999999999999999999999999999", 82);
    success<YmFloat>("1.0e-99999999999999999999999999999999999999999999999999999999999999999999999999999", 0.0, 82);
    underflow<YmFloat>("-1.0e99999999999999999999999999999999999999999999999999999999999999999999999999999", 82);
    underflow<YmFloat>("-1.0e+99999999999999999999999999999999999999999999999999999999999999999999999999999", 83);
    success<YmFloat>("-1.0e-99999999999999999999999999999999999999999999999999999999999999999999999999999", 0.0, 83);
}

TEST(ScalarParsing, Bool) {
    success<YmBool>("true", YM_TRUE, 4);
    success<YmBool>("trueaaa", YM_TRUE, 4);
    success<YmBool>("false", YM_FALSE, 5);
    success<YmBool>("falseaaa", YM_FALSE, 5);

    failure<YmBool>("");
    failure<YmBool>(" ");
    failure<YmBool>("!@#");
    failure<YmBool>("abc");
    failure<YmBool>("123");

    failure<YmBool>("True");
    failure<YmBool>("TRUE");
    failure<YmBool>("False");
    failure<YmBool>("FALSE");
}

TEST(ScalarParsing, Rune) {
    // ASCII & Escape Seqs
    for (YmRune c = 0; c <= 127; c++) {
        // ASCII Chars Themselves
        if (c != '\\' && c != '\0') { // Skip '\\' and '\0' as it requires escape seq to write.
            std::string s0 = std::format("{}", char(c));
            std::string s1 = std::format("{}aa", char(c));
            success<YmRune>(s0.c_str(), c, 1);
            success<YmRune>(s1.c_str(), c, 1);
        }
        // Escape Seqs
        if (c == '\0') {
            success<YmRune>("\\0", c, 2);
            success<YmRune>("\\0aa", c, 2);
        }
        else if (c == '\a') {
            success<YmRune>("\\a", c, 2);
            success<YmRune>("\\aaa", c, 2);
        }
        else if (c == '\b') {
            success<YmRune>("\\b", c, 2);
            success<YmRune>("\\baa", c, 2);
        }
        else if (c == '\f') {
            success<YmRune>("\\f", c, 2);
            success<YmRune>("\\faa", c, 2);
        }
        else if (c == '\n') {
            success<YmRune>("\\n", c, 2);
            success<YmRune>("\\naa", c, 2);
        }
        else if (c == '\r') {
            success<YmRune>("\\r", c, 2);
            success<YmRune>("\\raa", c, 2);
        }
        else if (c == '\t') {
            success<YmRune>("\\t", c, 2);
            success<YmRune>("\\taa", c, 2);
        }
        else if (c == '\v') {
            success<YmRune>("\\v", c, 2);
            success<YmRune>("\\vaa", c, 2);
        }
        else if (c == '\'') {
            success<YmRune>("\\'", c, 2);
            success<YmRune>("\\'aa", c, 2);
        }
        else if (c == '\"') {
            success<YmRune>("\\\"", c, 2);
            success<YmRune>("\\\"aa", c, 2);
        }
        else if (c == '\\') {
            success<YmRune>("\\\\", c, 2);
            success<YmRune>("\\\\aa", c, 2);
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

        const std::string s0_u_aa = std::format("\\x{}{}aa", b0_u, b1_u);
        const std::string s0_l_aa = std::format("\\x{}{}aa", b0_l, b1_l);
        const std::string s1_u_aa = std::format("\\u00{}{}aa", b0_u, b1_u);
        const std::string s1_l_aa = std::format("\\u00{}{}aa", b0_l, b1_l);
        const std::string s2_u_aa = std::format("\\U000000{}{}aa", b0_u, b1_u);
        const std::string s2_l_aa = std::format("\\U000000{}{}aa", b0_l, b1_l);

        success<YmRune>(s0_u.c_str(), c, 4);
        success<YmRune>(s0_l.c_str(), c, 4);
        success<YmRune>(s1_u.c_str(), c, 6);
        success<YmRune>(s1_l.c_str(), c, 6);
        success<YmRune>(s2_u.c_str(), c, 10);
        success<YmRune>(s2_l.c_str(), c, 10);

        success<YmRune>(s0_u_aa.c_str(), c, 4);
        success<YmRune>(s0_l_aa.c_str(), c, 4);
        success<YmRune>(s1_u_aa.c_str(), c, 6);
        success<YmRune>(s1_l_aa.c_str(), c, 6);
        success<YmRune>(s2_u_aa.c_str(), c, 10);
        success<YmRune>(s2_l_aa.c_str(), c, 10);
    }

    // 8/16/32-bit hex escape seqs + mixed case letter digits.

    success<YmRune>("\\xDb", U'\xdb', 4);
    success<YmRune>("\\xDbaa", U'\xdb', 4);
    success<YmRune>("\\xEa", U'\xea', 4);
    success<YmRune>("\\xEaaa", U'\xea', 4);

    success<YmRune>("\\ua1F4", U'\ua1f4', 6);
    success<YmRune>("\\ua1F4aa", U'\ua1f4', 6);
    success<YmRune>("\\uE83e", U'\ue83e', 6);
    success<YmRune>("\\uE83eaa", U'\ue83e', 6);

    success<YmRune>("\\U000DAaF4", U'\U000daaf4', 10);
    success<YmRune>("\\U000DAaF4aa", U'\U000daaf4', 10);
    success<YmRune>("\\U000AE83e", U'\U000ae83e', 10);
    success<YmRune>("\\U000AE83eaa", U'\U000ae83e', 10);

    // Literalization

    success<YmRune>("\\y", U'y', 2);
    success<YmRune>("\\yaa", U'y', 2);
    success<YmRune>("\\4", U'4', 2);
    success<YmRune>("\\4aa", U'4', 2);
    success<YmRune>("\\&", U'&', 2);
    success<YmRune>("\\&aa", U'&', 2);
    success<YmRune>("\\ ", U' ', 2);
    success<YmRune>("\\ aa", U' ', 2);

    // Invalid 8/16/32-bit hex escape seqs literalize instead.

    success<YmRune>("\\x8g", U'x', 2);
    success<YmRune>("\\x8gaa", U'x', 2);

    success<YmRune>("\\u80ag", U'u', 2);
    success<YmRune>("\\u80agaa", U'u', 2);

    success<YmRune>("\\U80aE37bg", U'U', 2);
    success<YmRune>("\\U80aE37bgaa", U'U', 2);

    // Non-ASCII Unicode

    success<YmRune>("\\x80", U'\x80', 4);
    success<YmRune>("\\x80aa", U'\x80', 4);
    success<YmRune>("\\u0394", U'Δ', 6);
    success<YmRune>("\\u0394aa", U'Δ', 6);
    success<YmRune>("\\u9b42", U'魂', 6);
    success<YmRune>("\\u9b42aa", U'魂', 6);
    success<YmRune>("\\U0001f4a9", U'💩', 10);
    success<YmRune>("\\U0001f4a9aa", U'💩', 10);

    auto utf8_1 = taul::utf8_s(u8"Δ");
    auto utf8_2 = taul::utf8_s(u8"Δaa");
    auto utf8_3 = taul::utf8_s(u8"魂");
    auto utf8_4 = taul::utf8_s(u8"魂aa");
    auto utf8_5 = taul::utf8_s(u8"💩");
    auto utf8_6 = taul::utf8_s(u8"💩aa");

    success<YmRune>(utf8_1.c_str(), U'Δ', 2);
    success<YmRune>(utf8_2.c_str(), U'Δ', 2);
    success<YmRune>(utf8_3.c_str(), U'魂', 3);
    success<YmRune>(utf8_4.c_str(), U'魂', 3);
    success<YmRune>(utf8_5.c_str(), U'💩', 4);
    success<YmRune>(utf8_6.c_str(), U'💩', 4);

    // Illegal Unicode

    // UTF-16 Surrogate Codepoint
    success<YmRune>("\\ud8a2", (YmRune)0xd8a2, 6);
    success<YmRune>("\\ud8a2aa", (YmRune)0xd8a2, 6);

    // Codepoints outside Unicode codespace.
    success<YmRune>("\\UbD0Aa1F4", YmRune(0xbd0aa1f4), 10);
    success<YmRune>("\\UbD0Aa1F4aa", YmRune(0xbd0aa1f4), 10);
    success<YmRune>("\\U6Af1E83e", YmRune(0x6af1e83e), 10);
    success<YmRune>("\\U6Af1E83eaa", YmRune(0x6af1e83e), 10);

    // Failures

    failure<YmRune>("");
    failure<YmRune>("\\");
}

