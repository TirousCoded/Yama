

#pragma once


#include <cstdint>
#include <limits>
#include <string_view>
#include <optional>

#include <taul/unicode.h>
#include <taul/encoding.h>


namespace yama {


    // the below five types are used to represent the values of primitive
    // scalar values found in the Yama language

    using int_t     = int64_t;
    using uint_t    = uint64_t;
    using float_t   = double;
    using bool_t    = bool;
    using char_t    = taul::unicode_t;

    static_assert(sizeof(int_t) == sizeof(int64_t)); // make sure range is correct
    static_assert(sizeof(uint_t) == sizeof(int64_t)); // make sure range is correct
    static_assert(sizeof(float_t) == sizeof(int64_t)); // make sure double precision IEEE 754
    static_assert(sizeof(bool_t) == sizeof(int8_t));
    static_assert(sizeof(char_t) == sizeof(int32_t)); // make sure range is correct


    constexpr float_t inf = std::numeric_limits<float_t>::infinity();


    enum class int_fmt : uint8_t {
        dec,    // decimal
        hex,    // hexadecimal
        bin,    // binary
    };


    std::string fmt_int_dec(int_t x);
    std::string fmt_int_hex(int_t x, bool uppercase_hex = false);
    std::string fmt_int_bin(int_t x);
    std::string fmt_int(int_t x, int_fmt fmt = int_fmt::dec, bool uppercase_hex = false);

    std::string fmt_uint_dec(uint_t x);
    std::string fmt_uint_hex(uint_t x, bool uppercase_hex = false);
    std::string fmt_uint_bin(uint_t x);
    std::string fmt_uint(uint_t x, int_fmt fmt = int_fmt::dec, bool uppercase_hex = false);

    std::string fmt_float(float_t x);
    std::string fmt_bool(bool_t x);

    // fmt_char will, for characters that are not visible ASCII or which lack
    // escape seqs dedicated to them, format such characters via the use of hex escape seqs

    // fmt_char will also uses hex escape seqs in order to be able to output a formatted 
    // string containing potentially illegal Unicode codepoints

    // fmt_char will return "?" if x is not in the Unicode codespace

    // if escape_quotes == true, '\'' will be escaped
    // if escape_dbl_quotes == true, '\"' will be escaped
    // if escape_backslashes == true, '\\' will be escaped

    std::string fmt_char(
        char_t x,
        bool uppercase_hex = false,
        bool escape_quotes = true,
        bool escape_dbl_quotes = true,
        bool escape_backslashes = true);


    template<typename T>
    struct parsed final {
        T       v;                      // the value parsed
        size_t  bytes;                  // the number of bytes consumed by the parse
        bool    overflow    = false;    // error if int/float overflows
        bool    underflow   = false;    // error if int/float underflows


        bool operator==(const parsed<T>&) const noexcept = default;
    };

    using parsed_int = parsed<int_t>;
    using parsed_uint = parsed<uint_t>;
    using parsed_float = parsed<float_t>;
    using parsed_bool = parsed<bool_t>;
    using parsed_char = parsed<char_t>;


    // below parse_# methods return std::nullopt if x could not be parsed

    // below parse_# methods allow for extra text following a valid int/uint/float/bool/char

    // parse_int returns an unspecified int value if overflow/underflow, w/ overflow/underflow
    // field being set to true
    
    // parse_uint returns an unspecified uint value if overflow/underflow, w/ overflow/underflow
    // field being set to true

    // parse_float returns inf/-inf in cases where value described overflows/underflows float range,
    // w/ overflow/underflow field being set to true
    
    std::optional<parsed_int> parse_int(std::string_view x);
    std::optional<parsed_uint> parse_uint(std::string_view x);
    std::optional<parsed_float> parse_float(std::string_view x);
    std::optional<parsed_bool> parse_bool(std::string_view x);
    std::optional<parsed_char> parse_char(std::string_view x);


    namespace internal {
        const char* get_hex_digits(bool uppercase_hex) noexcept;
        template<size_t N>
        inline std::array<char, N> resolve_hex(char_t x, bool uppercase_hex) noexcept;

        std::string fmt_int_generic(int_t x, uint_t base, const char* base_s, const char* prefix);
        std::string fmt_uint_generic(uint_t x, uint_t base, const char* base_s, const char* prefix, bool u_suffix);


        struct parse_ctx final {
            std::string_view input;
            size_t offset = 0; // current parse offset into input, in bytes
            taul::decoder<char> decoder; // current decoder state


            // 'check' methods assert what next codepoint should be, and do not consume 
            // it if it matches

            std::optional<taul::decode_result> check_any(); // expect not nothing
            std::optional<taul::decode_result> check(char_t v); // expect v
            std::optional<taul::decode_result> check_set(std::u32string_view vs); // expect something from vs

            // 'expect' methods assert what next codepoint should be, and consume it
            // if it matches

            std::optional<taul::decode_result> expect_any(); // expect not nothing
            std::optional<taul::decode_result> expect(char_t v); // expect v
            std::optional<taul::decode_result> expect_set(std::u32string_view vs); // expect something from vs


            static parse_ctx make(std::string_view input);
        };
    }
}


template<size_t N>
std::array<char, N> yama::internal::resolve_hex(char_t x, bool uppercase_hex) noexcept {
    // this bit below takes x and decomposes it into an array of hex digits h,
    // modifying x in the process
    std::array<char, N> h{};
    for (auto it = h.rbegin(); it != h.rend(); std::advance(it, 1)) {
        size_t current = x % 16;
        *it = get_hex_digits(uppercase_hex)[current];
        x = char_t((size_t(x) - current) / 16);
    }
    return h;
}

