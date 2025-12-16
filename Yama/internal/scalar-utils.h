

#pragma once


#include <span>

#include <taul/encoding.h>
#include <taul/strings.h>
#include <taul/unicode.h>

#include "../yama/scalars.h"


namespace _ym {


    constexpr const YmChar* hexDigits(bool uppercase) noexcept {
        return
            uppercase
            ? taul::hex_uppercase
            : taul::hex_lowercase;
    }

    template<size_t N>
    inline std::array<char, N> resolveHex(YmRune x, bool uppercase) noexcept {
        // This bit below takes x and decomposes it into an array of hex digits h,
        // modifying x in the process.
        std::array<YmChar, N> h{};
        for (auto it = h.rbegin(); it != h.rend(); std::advance(it, 1)) {
            size_t current = x % 16;
            *it = hexDigits(uppercase)[current];
            x = YmRune((size_t(x) - current) / 16);
        }
        return h;
    }

    size_t measureIntGeneric(YmInt x, YmUInt base, size_t prefix_len);
    size_t measureUIntGeneric(YmUInt x, YmUInt base, size_t prefix_len, bool u_suffix);

    void fmtIntGeneric(std::span<YmChar> buff, YmInt x, YmUInt base, std::string_view base_s, std::string_view prefix);
    void fmtUIntGeneric(std::span<YmChar> buff, YmUInt x, YmUInt base, std::string_view base_s, std::string_view prefix, bool u_suffix);

    struct ScalarParser final {
        std::string_view input;
        size_t offset = 0; // Current parse offset into input, in bytes.
        taul::decoder<YmChar> decoder; // Current decoder state.


        // 'check' methods assert what next codepoint should be, and do not consume 
        // it if it matches.

        std::optional<taul::decode_result> checkAny(); // Expect not nothing.
        std::optional<taul::decode_result> check(YmChar v); // Expect v.
        std::optional<taul::decode_result> checkSet(std::u32string_view vs); // Expect something from vs.

        // 'expect' methods assert what next codepoint should be, and consume it
        // if it matches.

        std::optional<taul::decode_result> expectAny(); // Expect not nothing.
        std::optional<taul::decode_result> expect(YmChar v); // Expect v.
        std::optional<taul::decode_result> expectSet(std::u32string_view vs); // Expect something from vs.


        static ScalarParser make(std::string_view input);
    };


    template<typename T>
    struct ScalarParseResult final {
        YmParseStatus status = YmParseStatus_Failure;
        T output = {};
        size_t bytes = size_t(-1);
    };

    ScalarParseResult<YmInt> parseInt(std::string_view input);
    ScalarParseResult<YmUInt> parseUInt(std::string_view input, bool ignoreU);
    ScalarParseResult<YmFloat> parseFloat(std::string_view input);
    ScalarParseResult<YmBool> parseBool(std::string_view input);
    ScalarParseResult<YmRune> parseRune(std::string_view input);
}

