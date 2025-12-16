

#include <format>

#include <gtest/gtest.h>
#include <taul/strings.h>
#include <taul/unicode.h>
#include <yama/yama.h>


TEST(ScalarMeasuring, Int) {
    // Decimal (>=0)
    for (YmInt i = 0; i < 10; i++) {
        EXPECT_EQ(ymMeasureInt(i, YmIntFmt_Dec), 1) << "i == " << i;
    }
    EXPECT_EQ(ymMeasureInt(12, YmIntFmt_Dec), 2);
    EXPECT_EQ(ymMeasureInt(123, YmIntFmt_Dec), 3);
    EXPECT_EQ(ymMeasureInt(1230, YmIntFmt_Dec), 4);
    EXPECT_EQ(ymMeasureInt(12301, YmIntFmt_Dec), 5);
    EXPECT_EQ(ymMeasureInt(123012, YmIntFmt_Dec), 6);
    EXPECT_EQ(ymMeasureInt(123012301, YmIntFmt_Dec), 9);
    EXPECT_EQ(ymMeasureInt(123012301230, YmIntFmt_Dec), 12);
    EXPECT_EQ(ymMeasureInt(123012301230123, YmIntFmt_Dec), 15);
    EXPECT_EQ(ymMeasureInt(123012301230123012, YmIntFmt_Dec), 18);
    EXPECT_EQ(ymMeasureInt(12301230123012301230, YmIntFmt_Dec), 20);

    // Decimal (<0)
    for (YmInt i = -9; i <= -1; i++) {
        EXPECT_EQ(ymMeasureInt(i, YmIntFmt_Dec), 2) << "i == " << i;
    }
    EXPECT_EQ(ymMeasureInt(-12, YmIntFmt_Dec), 3);
    EXPECT_EQ(ymMeasureInt(-123, YmIntFmt_Dec), 4);
    EXPECT_EQ(ymMeasureInt(-1230, YmIntFmt_Dec), 5);
    EXPECT_EQ(ymMeasureInt(-12301, YmIntFmt_Dec), 6);
    EXPECT_EQ(ymMeasureInt(-123012, YmIntFmt_Dec), 7);
    EXPECT_EQ(ymMeasureInt(-123012301, YmIntFmt_Dec), 10);
    EXPECT_EQ(ymMeasureInt(-123012301230, YmIntFmt_Dec), 13);
    EXPECT_EQ(ymMeasureInt(-123012301230123, YmIntFmt_Dec), 16);
    EXPECT_EQ(ymMeasureInt(-123012301230123012, YmIntFmt_Dec), 19);
    EXPECT_EQ(ymMeasureInt(-1230123012301230123, YmIntFmt_Dec), 20);

    // Hexadecimal (>=0)
    for (YmInt i = 0; i < 16; i++) {
        EXPECT_EQ(ymMeasureInt(i, YmIntFmt_Hex), 3) << "i == " << i;
    }
    EXPECT_EQ(ymMeasureInt(0x1f, YmIntFmt_Hex), 4);
    EXPECT_EQ(ymMeasureInt(0x1f3, YmIntFmt_Hex), 5);
    EXPECT_EQ(ymMeasureInt(0x1f3a, YmIntFmt_Hex), 6);
    EXPECT_EQ(ymMeasureInt(0x1f3a1a4, YmIntFmt_Hex), 9);
    EXPECT_EQ(ymMeasureInt(0x1f3a1a41a4, YmIntFmt_Hex), 12);
    EXPECT_EQ(ymMeasureInt(0x1f3a1a41a41a4, YmIntFmt_Hex), 15);
    EXPECT_EQ(ymMeasureInt(0x1f3a1a41a41a41a4, YmIntFmt_Hex), 18);

    // Hexadecimal (<0)
    for (YmInt i = -15; i <= -1; i++) {
        EXPECT_EQ(ymMeasureInt(i, YmIntFmt_Hex), 4) << "i == " << i;
    }
    EXPECT_EQ(ymMeasureInt(-0x1f, YmIntFmt_Hex), 5);
    EXPECT_EQ(ymMeasureInt(-0x1f3, YmIntFmt_Hex), 6);
    EXPECT_EQ(ymMeasureInt(-0x1f3a, YmIntFmt_Hex), 7);
    EXPECT_EQ(ymMeasureInt(-0x1f3a1a4, YmIntFmt_Hex), 10);
    EXPECT_EQ(ymMeasureInt(-0x1f3a1a41a4, YmIntFmt_Hex), 13);
    EXPECT_EQ(ymMeasureInt(-0x1f3a1a41a41a4, YmIntFmt_Hex), 16);
    EXPECT_EQ(ymMeasureInt(-0x1f3a1a41a41a41a4, YmIntFmt_Hex), 19);

    // Binary (>=0)
    for (YmInt i = 0; i < 2; i++) {
        EXPECT_EQ(ymMeasureInt(i, YmIntFmt_Bin), 3) << "i == " << i;
    }
    EXPECT_EQ(ymMeasureInt(0b10, YmIntFmt_Bin), 4);
    EXPECT_EQ(ymMeasureInt(0b101, YmIntFmt_Bin), 5);
    EXPECT_EQ(ymMeasureInt(0b1010, YmIntFmt_Bin), 6);
    EXPECT_EQ(ymMeasureInt(0b10101, YmIntFmt_Bin), 7);
    EXPECT_EQ(ymMeasureInt(0b101010, YmIntFmt_Bin), 8);
    EXPECT_EQ(ymMeasureInt(0b1010101, YmIntFmt_Bin), 9);
    EXPECT_EQ(ymMeasureInt(0b1010101010, YmIntFmt_Bin), 12);
    EXPECT_EQ(ymMeasureInt(0b1010101010101, YmIntFmt_Bin), 15);
    EXPECT_EQ(ymMeasureInt(0b1010101010101010, YmIntFmt_Bin), 18);
    EXPECT_EQ(ymMeasureInt(0b1010101010101010101, YmIntFmt_Bin), 21);
    EXPECT_EQ(ymMeasureInt(0b1010101010101010101010, YmIntFmt_Bin), 24);
    EXPECT_EQ(ymMeasureInt(0b1010101010101010101010101, YmIntFmt_Bin), 27);
    EXPECT_EQ(ymMeasureInt(0b1010101010101010101010101010, YmIntFmt_Bin), 30);
    EXPECT_EQ(ymMeasureInt(0b1010101010101010101010101010101, YmIntFmt_Bin), 33);
    EXPECT_EQ(ymMeasureInt(0b1010101010101010101010101010101010, YmIntFmt_Bin), 36);
    EXPECT_EQ(ymMeasureInt(0b1010101010101010101010101010101010101, YmIntFmt_Bin), 39);
    EXPECT_EQ(ymMeasureInt(0b1010101010101010101010101010101010101010, YmIntFmt_Bin), 42);
    EXPECT_EQ(ymMeasureInt(0b1010101010101010101010101010101010101010101, YmIntFmt_Bin), 45);
    EXPECT_EQ(ymMeasureInt(0b1010101010101010101010101010101010101010101010, YmIntFmt_Bin), 48);
    EXPECT_EQ(ymMeasureInt(0b1010101010101010101010101010101010101010101010101, YmIntFmt_Bin), 51);
    EXPECT_EQ(ymMeasureInt(0b1010101010101010101010101010101010101010101010101010, YmIntFmt_Bin), 54);
    EXPECT_EQ(ymMeasureInt(0b1010101010101010101010101010101010101010101010101010101, YmIntFmt_Bin), 57);
    EXPECT_EQ(ymMeasureInt(0b1010101010101010101010101010101010101010101010101010101010, YmIntFmt_Bin), 60);
    EXPECT_EQ(ymMeasureInt(0b1010101010101010101010101010101010101010101010101010101010101, YmIntFmt_Bin), 63);
    EXPECT_EQ(ymMeasureInt(0b1010101010101010101010101010101010101010101010101010101010101010, YmIntFmt_Bin), 66);

    // Binary (<0)
    EXPECT_EQ(ymMeasureInt(-0b1, YmIntFmt_Bin), 4);
    EXPECT_EQ(ymMeasureInt(-0b10, YmIntFmt_Bin), 5);
    EXPECT_EQ(ymMeasureInt(-0b101, YmIntFmt_Bin), 6);
    EXPECT_EQ(ymMeasureInt(-0b1010, YmIntFmt_Bin), 7);
    EXPECT_EQ(ymMeasureInt(-0b10101, YmIntFmt_Bin), 8);
    EXPECT_EQ(ymMeasureInt(-0b101010, YmIntFmt_Bin), 9);
    EXPECT_EQ(ymMeasureInt(-0b1010101, YmIntFmt_Bin), 10);
    EXPECT_EQ(ymMeasureInt(-0b1010101010, YmIntFmt_Bin), 13);
    EXPECT_EQ(ymMeasureInt(-0b1010101010101, YmIntFmt_Bin), 16);
    EXPECT_EQ(ymMeasureInt(-0b1010101010101010, YmIntFmt_Bin), 19);
    EXPECT_EQ(ymMeasureInt(-0b1010101010101010101, YmIntFmt_Bin), 22);
    EXPECT_EQ(ymMeasureInt(-0b1010101010101010101010, YmIntFmt_Bin), 25);
    EXPECT_EQ(ymMeasureInt(-0b1010101010101010101010101, YmIntFmt_Bin), 28);
    EXPECT_EQ(ymMeasureInt(-0b1010101010101010101010101010, YmIntFmt_Bin), 31);
    EXPECT_EQ(ymMeasureInt(-0b1010101010101010101010101010101, YmIntFmt_Bin), 34);
    EXPECT_EQ(ymMeasureInt(-0b1010101010101010101010101010101010, YmIntFmt_Bin), 37);
    EXPECT_EQ(ymMeasureInt(-0b1010101010101010101010101010101010101, YmIntFmt_Bin), 40);
    EXPECT_EQ(ymMeasureInt(-0b1010101010101010101010101010101010101010, YmIntFmt_Bin), 43);
    EXPECT_EQ(ymMeasureInt(-0b1010101010101010101010101010101010101010101, YmIntFmt_Bin), 46);
    EXPECT_EQ(ymMeasureInt(-0b1010101010101010101010101010101010101010101010, YmIntFmt_Bin), 49);
    EXPECT_EQ(ymMeasureInt(-0b1010101010101010101010101010101010101010101010101, YmIntFmt_Bin), 52);
    EXPECT_EQ(ymMeasureInt(-0b1010101010101010101010101010101010101010101010101010, YmIntFmt_Bin), 55);
    EXPECT_EQ(ymMeasureInt(-0b1010101010101010101010101010101010101010101010101010101, YmIntFmt_Bin), 58);
    EXPECT_EQ(ymMeasureInt(-0b1010101010101010101010101010101010101010101010101010101010, YmIntFmt_Bin), 61);
    EXPECT_EQ(ymMeasureInt(-0b1010101010101010101010101010101010101010101010101010101010101, YmIntFmt_Bin), 64);
    EXPECT_EQ(ymMeasureInt(-0b101010101010101010101010101010101010101010101010101010101010101, YmIntFmt_Bin), 66);
}

TEST(ScalarMeasuring, UInt) {
    // Decimal
    for (YmInt i = 0; i < 10; i++) {
        EXPECT_EQ(ymMeasureUInt(i, YmIntFmt_Dec), 2) << "i == " << i;
    }
    EXPECT_EQ(ymMeasureUInt(12, YmIntFmt_Dec), 3);
    EXPECT_EQ(ymMeasureUInt(123, YmIntFmt_Dec), 4);
    EXPECT_EQ(ymMeasureUInt(1230, YmIntFmt_Dec), 5);
    EXPECT_EQ(ymMeasureUInt(12301, YmIntFmt_Dec), 6);
    EXPECT_EQ(ymMeasureUInt(123012, YmIntFmt_Dec), 7);
    EXPECT_EQ(ymMeasureUInt(123012301, YmIntFmt_Dec), 10);
    EXPECT_EQ(ymMeasureUInt(123012301230, YmIntFmt_Dec), 13);
    EXPECT_EQ(ymMeasureUInt(123012301230123, YmIntFmt_Dec), 16);
    EXPECT_EQ(ymMeasureUInt(123012301230123012, YmIntFmt_Dec), 19);
    EXPECT_EQ(ymMeasureUInt(12301230123012301230, YmIntFmt_Dec), 21);

    // Hexadecimal
    for (YmInt i = 0; i < 16; i++) {
        EXPECT_EQ(ymMeasureUInt(i, YmIntFmt_Hex), 4) << "i == " << i;
    }
    EXPECT_EQ(ymMeasureUInt(0x1f, YmIntFmt_Hex), 5);
    EXPECT_EQ(ymMeasureUInt(0x1f3, YmIntFmt_Hex), 6);
    EXPECT_EQ(ymMeasureUInt(0x1f3a, YmIntFmt_Hex), 7);
    EXPECT_EQ(ymMeasureUInt(0x1f3a1a4, YmIntFmt_Hex), 10);
    EXPECT_EQ(ymMeasureUInt(0x1f3a1a41a4, YmIntFmt_Hex), 13);
    EXPECT_EQ(ymMeasureUInt(0x1f3a1a41a41a4, YmIntFmt_Hex), 16);
    EXPECT_EQ(ymMeasureUInt(0x1f3a1a41a41a41a4, YmIntFmt_Hex), 19);

    // Binary
    for (YmInt i = 0; i < 2; i++) {
        EXPECT_EQ(ymMeasureUInt(i, YmIntFmt_Bin), 4) << "i == " << i;
    }
    EXPECT_EQ(ymMeasureUInt(0b10, YmIntFmt_Bin), 5);
    EXPECT_EQ(ymMeasureUInt(0b101, YmIntFmt_Bin), 6);
    EXPECT_EQ(ymMeasureUInt(0b1010, YmIntFmt_Bin), 7);
    EXPECT_EQ(ymMeasureUInt(0b10101, YmIntFmt_Bin), 8);
    EXPECT_EQ(ymMeasureUInt(0b101010, YmIntFmt_Bin), 9);
    EXPECT_EQ(ymMeasureUInt(0b1010101, YmIntFmt_Bin), 10);
    EXPECT_EQ(ymMeasureUInt(0b1010101010, YmIntFmt_Bin), 13);
    EXPECT_EQ(ymMeasureUInt(0b1010101010101, YmIntFmt_Bin), 16);
    EXPECT_EQ(ymMeasureUInt(0b1010101010101010, YmIntFmt_Bin), 19);
    EXPECT_EQ(ymMeasureUInt(0b1010101010101010101, YmIntFmt_Bin), 22);
    EXPECT_EQ(ymMeasureUInt(0b1010101010101010101010, YmIntFmt_Bin), 25);
    EXPECT_EQ(ymMeasureUInt(0b1010101010101010101010101, YmIntFmt_Bin), 28);
    EXPECT_EQ(ymMeasureUInt(0b1010101010101010101010101010, YmIntFmt_Bin), 31);
    EXPECT_EQ(ymMeasureUInt(0b1010101010101010101010101010101, YmIntFmt_Bin), 34);
    EXPECT_EQ(ymMeasureUInt(0b1010101010101010101010101010101010, YmIntFmt_Bin), 37);
    EXPECT_EQ(ymMeasureUInt(0b1010101010101010101010101010101010101, YmIntFmt_Bin), 40);
    EXPECT_EQ(ymMeasureUInt(0b1010101010101010101010101010101010101010, YmIntFmt_Bin), 43);
    EXPECT_EQ(ymMeasureUInt(0b1010101010101010101010101010101010101010101, YmIntFmt_Bin), 46);
    EXPECT_EQ(ymMeasureUInt(0b1010101010101010101010101010101010101010101010, YmIntFmt_Bin), 49);
    EXPECT_EQ(ymMeasureUInt(0b1010101010101010101010101010101010101010101010101, YmIntFmt_Bin), 52);
    EXPECT_EQ(ymMeasureUInt(0b1010101010101010101010101010101010101010101010101010, YmIntFmt_Bin), 55);
    EXPECT_EQ(ymMeasureUInt(0b1010101010101010101010101010101010101010101010101010101, YmIntFmt_Bin), 58);
    EXPECT_EQ(ymMeasureUInt(0b1010101010101010101010101010101010101010101010101010101010, YmIntFmt_Bin), 61);
    EXPECT_EQ(ymMeasureUInt(0b1010101010101010101010101010101010101010101010101010101010101, YmIntFmt_Bin), 64);
    EXPECT_EQ(ymMeasureUInt(0b1010101010101010101010101010101010101010101010101010101010101010, YmIntFmt_Bin), 67);
}

TEST(ScalarMeasuring, Float) {
    // TODO: It's really hard to properly unit test ymMeasureFloat, as it basically
    //       just wraps std::formatted_size, and its behaviour is hard for us to
    //		 predict, since for any given float value, there's multiple equally
    //		 valid ways of expressing it.
    //
    //       Maybe try at some point to write better tests for this.

    EXPECT_EQ(ymMeasureFloat(0.0), std::formatted_size("{}", 0.0));
    EXPECT_EQ(ymMeasureFloat(110.0), std::formatted_size("{}", 110.0));
    EXPECT_EQ(ymMeasureFloat(3.14159), std::formatted_size("{}", 3.14159));
    EXPECT_EQ(ymMeasureFloat(YM_INF), std::formatted_size("{}", YM_INF));
}

TEST(ScalarMeasuring, Bool) {
    EXPECT_EQ(ymMeasureBool(true), 4);
    EXPECT_EQ(ymMeasureBool(false), 5);
}

TEST(ScalarMeasuring, Rune) {
    for (YmRune c = 0; c <= 127; c++) {
        const auto escapeSeqChars = std::u32string_view(U"\0\a\b\f\n\r\t\v\'\"\\", 12);
        if (taul::in_set(escapeSeqChars, c)) {
            EXPECT_EQ(ymMeasureRune(c, true, true, true), 2) // \X
                << "c == " << taul::fmt_unicode(c);
        }
        else if (taul::is_visible_ascii(c)) {
            EXPECT_EQ(ymMeasureRune(c, true, true, true), 1) // X
                << "c == " << taul::fmt_unicode(c);
        }
        else {
            EXPECT_EQ(ymMeasureRune(c, true, true, true), 4) // \xXX
                << "c == " << taul::fmt_unicode(c);
        }
    }

    EXPECT_EQ(ymMeasureRune(U'\'', true, bool{}, bool{}), 2); // \'
    EXPECT_EQ(ymMeasureRune(U'\'', false, bool{}, bool{}), 1); // '
    EXPECT_EQ(ymMeasureRune(U'\"', bool{}, true, bool{}), 2); // \"
    EXPECT_EQ(ymMeasureRune(U'\"', bool{}, false, bool{}), 1); // "
    EXPECT_EQ(ymMeasureRune(U'\\', bool{}, bool{}, true), 2); /* \\ */
    EXPECT_EQ(ymMeasureRune(U'\\', bool{}, bool{}, false), 1); /* \ */
    
    EXPECT_EQ(ymMeasureRune(U'\x80', true, true, true), 4); // \x80, 1 byte escape seq
    EXPECT_EQ(ymMeasureRune(U'Δ', true, true, true), 6); // \u0394, 2 byte escape seq
    EXPECT_EQ(ymMeasureRune(U'魂', true, true, true), 6); // \u9b42, 2 byte escape seq
    EXPECT_EQ(ymMeasureRune(U'💩', true, true, true), 10); // \U0001f4a9, 4 byte escape seq

    EXPECT_EQ(ymMeasureRune(YmRune(0xd8a2), true, true, true), 6); // \ud8a2, UTF-16 Surrogate Value
    EXPECT_EQ(ymMeasureRune(YmRune(0x110000), true, true, true), 6); // \ufffd (aka. �, 3 byte in UTF-8), first value after Unicode codespace.
    //EXPECT_EQ(ymMeasureRune(YmRune(0x110000), true, true, true), 3); // �, 3 byte in UTF-8, first value after Unicode codespace.
}

