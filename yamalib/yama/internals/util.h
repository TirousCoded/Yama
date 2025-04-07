

#pragma once


#include <string_view>

#include "../core/asserts.h"
#include "../core/general.h"
#include "../core/scalars.h"


namespace yama::internal {


    // TODO: but what if nothing after divider? maybe return empty optional instead of having
    //       failure be indicated by empty second string, which I worry may cause issues

    // if no divider could be found, second string of pair will be empty

    template<typename Char>
    inline std::pair<std::basic_string_view<Char>, std::basic_string_view<Char>> split(
        std::basic_string_view<Char> x,
        Char divider,
        bool include_divider_in_second = false) noexcept {
        size_t i = 0;
        for (; i < x.length(); i++) {
            if (x[i] == divider) break;
        }
        return std::make_pair(
            x.substr(0, i),
            i < x.length() ? x.substr(include_divider_in_second ? i : i + 1) : std::basic_string_view<Char>{});
    }
    
    template<typename Char, std::convertible_to<std::basic_string_view<Char>> DividerSet>
    inline std::pair<std::basic_string_view<Char>, std::basic_string_view<Char>> split(
        std::basic_string_view<Char> x,
        const DividerSet& divider_set,
        bool include_divider_in_second = false) noexcept {
        size_t i = 0;
        for (; i < x.length(); i++) {
            if (taul::in_set(std::basic_string_view<Char>(divider_set), x[i])) break;
        }
        return std::make_pair(
            x.substr(0, i),
            i < x.length() ? x.substr(include_divider_in_second ? i : i + 1) : std::basic_string_view<Char>{});
    }
    
    template<typename Char>
    inline std::pair<taul::basic_str<Char>, taul::basic_str<Char>> split(
        taul::basic_str<Char> x,
        Char divider,
        bool include_divider_in_second = false) noexcept {
        size_t i = 0;
        for (; i < x.length(); i++) {
            if (x[i] == divider) break;
        }
        return std::make_pair(
            x.substr(0, i),
            i < x.length() ? x.substr(include_divider_in_second ? i : i + 1) : taul::basic_str<Char>{});
    }
    
    template<typename Char, std::convertible_to<std::basic_string_view<Char>> DividerSet>
    inline std::pair<taul::basic_str<Char>, taul::basic_str<Char>> split(
        taul::basic_str<Char> x,
        const DividerSet& divider_set,
        bool include_divider_in_second = false) noexcept {
        size_t i = 0;
        for (; i < x.length(); i++) {
            if (taul::in_set(std::basic_string_view<Char>(divider_set), x[i])) break;
        }
        return std::make_pair(
            x.substr(0, i),
            i < x.length() ? x.substr(include_divider_in_second ? i : i + 1) : taul::basic_str<Char>{});
    }


    // NOTE: copied range_overlap from TAUL

    // below are exclusive ranges [a_first, a_last) and [b_first, b_last)

    // range_overlap returns false if the two ranges are 0 units apart, but not overlapping

    template<typename T>
    inline bool range_overlap(T a_first, T a_last, T b_first, T b_last) noexcept {
        YAMA_ASSERT(a_first <= a_last);
        YAMA_ASSERT(b_first <= b_last);
        //return !(a_first >= b_last || a_last <= b_first); <- helps me understand what below does
        return a_first < b_last && b_first < a_last;
    }

    // range_contains returns if [a_first, a_last) *contains fully* [b_first, b_last)

    template<typename T>
    inline bool range_contains(T a_first, T a_last, T b_first, T b_last) noexcept {
        YAMA_ASSERT(a_first <= a_last);
        YAMA_ASSERT(b_first <= b_last);
        return b_first >= a_first && b_last <= a_last;
    }


    // TODO: replace w/ frontend version later

    std::string fmt_string_literal(const str& x, char_t err = U'?');


    inline std::string fmt_tabs(size_t tabs, const char* tab) {
        YAMA_ASSERT(tab);
        std::string result{};
        result.reserve(tabs * strlen(tab));
        for (size_t i = 0; i < tabs; i++) {
            result += tab;
        }
        return result;
    }
}

