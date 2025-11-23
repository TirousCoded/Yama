

#pragma once


#include <atomic>
#include <concepts>
#include <format>
#include <iostream>
#include <optional>
#include <regex>

#include <taul/strings.h>

#include "../yama/yama.h"
#include "../yama++/meta.h"
#include "../yama++/Safe.h"


namespace _ym {


    void debugbreak() noexcept;
    void crash() noexcept;


    // By convention the Yama backend uses Num enum constants to specify the size of an enum.
    template<typename T>
    constexpr size_t enumSize = size_t(T::Num);


    struct ErrCallbackInfo final {
        YmErrCallbackFn fn = nullptr;
        void* user = nullptr;
    };

    // Static class encapsulating Yama API process-wide and thread-local data/functionality.
    class Global final {
    public:
        Global() = delete;


        static void setErrCallback(YmErrCallbackFn fn, void* user) noexcept;
        static bool pathIsLegal(std::string_view path);
        static bool fullnameIsLegal(std::string_view fullname);

        template<typename... Args>
        static inline void raiseErr(YmErrCode code, std::format_string<Args...> fmt, Args&&... args) {
            if (!_errCallbackInfo.fn) {
                return;
            }
            std::string errmsg(std::format("[Yama] [{}] {}", ymFmtYmErrCode(code), std::format(fmt, std::forward<Args>(args)...)));
            _errCallbackInfo.fn(code, errmsg.c_str(), _errCallbackInfo.user);
        }


        static void pIterStart(ym::Safe<YmCtx> ctx) noexcept;
        static void pIterStartFrom(ym::Safe<YmCtx> ctx, ym::Safe<YmParcel> parcel) noexcept;
        static void pIterAdvance(size_t n) noexcept;
        static YmParcel* pIterGet() noexcept;
        static bool pIterDone() noexcept;


    private:
        thread_local static ErrCallbackInfo _errCallbackInfo;
        static const std::regex _legalPathPattern, _legalFullnamePattern;
    };


    // TODO: But what if nothing after divider? maybe return empty optional instead of having
    //       failure be indicated by empty second string, which I worry may cause issues.

    // If no divider could be found, second string of pair will be empty.
    template<typename Char>
    inline std::pair<std::basic_string_view<Char>, std::basic_string_view<Char>> split(
        std::basic_string_view<Char> x,
        Char divider,
        bool includeDividerInSecond = false) noexcept {
        size_t i = 0;
        for (; i < x.length(); i++) {
            if (x[i] == divider) break;
        }
        return std::make_pair(
            x.substr(0, i),
            i < x.length() ? x.substr(includeDividerInSecond ? i : i + 1) : std::basic_string_view<Char>{});
    }

    // If no divider could be found, second string of pair will be empty.
    template<typename Char, std::convertible_to<std::basic_string_view<Char>> DividerSet>
    inline std::pair<std::basic_string_view<Char>, std::basic_string_view<Char>> split(
        std::basic_string_view<Char> x,
        const DividerSet& dividerSet,
        bool includeDividerInSecond = false) noexcept {
        size_t i = 0;
        for (; i < x.length(); i++) {
            if (taul::in_set(std::basic_string_view<Char>(dividerSet), x[i])) break;
        }
        return std::make_pair(
            x.substr(0, i),
            i < x.length() ? x.substr(includeDividerInSecond ? i : i + 1) : std::basic_string_view<Char>{});
    }

    // Splits along a string, rather than treating the string like a char set.
    // If no divider could be found, second string of pair will be empty.
    template<typename Char>
    constexpr std::pair<std::basic_string_view<Char>, std::basic_string_view<Char>> split_s(
        const std::convertible_to<std::basic_string_view<Char>> auto& x,
        const std::convertible_to<std::basic_string_view<Char>> auto& divider,
        bool includeDividerInSecond = false) noexcept {
        std::basic_string_view<Char> xx(x);
        std::basic_string_view<Char> d(divider);
        size_t i = 0;
        for (; i < xx.length(); i++) {
            if (xx.substr(i, d.length()) == d) break;
        }
        return std::make_pair(
            xx.substr(0, i),
            i < xx.length() ? xx.substr(includeDividerInSecond ? i : i + d.length()) : std::basic_string_view<Char>{});
    }


    // NOTE: Copied rangeOverlap impl from TAUL.

    // Returns if [aFirst, aLast) and [bFirst, bLast) overlap.
    // Returns false if the two ranges are 0 units apart, but not overlapping.
    // Ranges are exclusive: [aFirst, aLast) and [bFirst, bLast).
    template<typename T>
    inline bool rangeOverlap(T aFirst, T aLast, T bFirst, T bLast) noexcept {
        ymAssert(aFirst <= aLast);
        ymAssert(bFirst <= bLast);
        // Below is equiv to '!(aFirst >= bLast || aLast <= bFirst)'.
        return aFirst < bLast && bFirst < aLast;
    }

    // Returns if [aFirst, aLast) contains fully [bFirst, bLast).
    // Ranges are exclusive: [aFirst, aLast) and [bFirst, bLast).
    template<typename T>
    inline bool rangeContains(T aFirst, T aLast, T bFirst, T bLast) noexcept {
        ymAssert(aFirst <= aLast);
        ymAssert(bFirst <= bLast);
        return bFirst >= aFirst && bLast <= aLast;
    }
}

