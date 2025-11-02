

#pragma once


#include <type_traits>
#include <utility>
#include <format>


namespace ym {


    // NOTE: These exist to make use of folding exprs more readable.

    // If all values in Bs are true.
    // True if sizeof...(Bs) == 0.
    template<bool... Bs>
    constexpr bool allOf = (Bs && ...);

    // If at least one value in Bs is true.
    // False if sizeof...(Bs) == 0.
    template<bool... Bs>
    constexpr bool anyOf = (Bs || ...);

    // If no values in Bs are true.
    // True if sizeof...(Bs) == 0.
    template<bool... Bs>
    constexpr bool noneOf = !anyOf<Bs...>;


    // Models callable types T with signature Returns(Args...).
    template<typename T, typename Returns, typename... Args>
    concept Callable =
        requires (std::remove_reference_t<T> f, Args&&... args)
    {
        { f(std::forward<Args>(args)...) } -> std::convertible_to<Returns>;
    };


    // Models hashable types T.
    template<typename T>
    concept Hashable =
        requires (std::remove_reference_t<T> v)
    {
        { std::hash<T>{}(v) } noexcept -> std::convertible_to<size_t>;
    };


    namespace detail {
        // NOTE: Stole this impl from the MSVC C++23 impl of std::formattable.

        template<typename CharT>
        struct PhonyFmtIterFor {
            using difference_type = ptrdiff_t;

            // These member functions are never defined:
            CharT& operator*() const;
            PhonyFmtIterFor& operator++();
            PhonyFmtIterFor operator++(int);
        };
        template<class T, class FmtCtx, class Formatter = FmtCtx::template formatter_type<std::remove_const_t<T>>>
        concept FormattableWith =
            std::semiregular<Formatter> &&
            requires(Formatter& f, const Formatter& cf, T&& t, FmtCtx fc,
                std::basic_format_parse_context<typename FmtCtx::char_type> pc)
        {
            { f.parse(pc) } -> std::same_as<typename decltype(pc)::iterator>;
            { cf.format(t, fc) } -> std::same_as<typename FmtCtx::iterator>;
        };
    }

    // Models formattable types T.
    template<class T, class CharT>
    concept Formattable =
        detail::FormattableWith<std::remove_reference_t<T>, std::basic_format_context<detail::PhonyFmtIterFor<CharT>, CharT>>;


    template<typename T>
    concept BasicLocakable =
        requires (std::remove_reference_t<T> v)
    {
        v.lock();
        v.unlock();
    };

    template<typename T>
    concept Lockable =
        BasicLocakable<T> &&
        requires (std::remove_reference_t<T> v)
    {
        { v.try_lock() } -> std::convertible_to<bool>;
    };


    namespace detail {
        template<typename T, typename... Pack, size_t... I>
            requires (sizeof...(Pack) == sizeof...(I))
        constexpr size_t indexInPackHelper(std::index_sequence<I...>) noexcept {
            size_t result{};
            bool success = ((result = I, std::is_same_v<T, Pack>) || ...);
            return success ? result : sizeof...(Pack);
        }
    }

    // The index in Pack where T is first located, or sizeof...(Pack) if T is not in Pack.
    template<typename T, typename... Pack>
    constexpr size_t indexInPack = detail::indexInPackHelper<T, Pack...>(std::index_sequence_for<Pack...>{});

    // Unit Tests
    static_assert(indexInPack<int, int, float, char> == 0);
    static_assert(indexInPack<int, float, int, char> == 1);
    static_assert(indexInPack<int, float, char, int> == 2);
    static_assert(indexInPack<int, float, int, char, int> == 1); // Stop at first match.
    static_assert(indexInPack<int, float, bool, char> == 3); // Fail
    static_assert(indexInPack<int> == 0); // Fail


    // If index I is valid index in Pack.
    template<size_t I, typename... Pack>
    constexpr bool validPackIndex = I < sizeof...(Pack);

    // Unit Tests
    static_assert(validPackIndex<0> == false); // Empty
    static_assert(validPackIndex<0, int> == true);
    static_assert(validPackIndex<1, int> == false);
    static_assert(validPackIndex<0, int, float, char> == true);
    static_assert(validPackIndex<1, int, float, char> == true);
    static_assert(validPackIndex<2, int, float, char> == true);
    static_assert(validPackIndex<3, int, float, char> == false);


    // Models types T which exist in Pack.
    template<typename T, typename... Pack>
    concept TypeInPack = validPackIndex<indexInPack<T, Pack...>, Pack...>;

    // Unit Tests
    static_assert(TypeInPack<int, int, float, char> == true);
    static_assert(TypeInPack<int, int, float, char, int> == true); // Multiple
    static_assert(TypeInPack<int, bool, float, char> == false); // Fail
    static_assert(TypeInPack<int> == false); // Fail


    // Returns the sum of the contents of Pack.
    // Unlike constant folding, packSum can handle sizeof...(Pack) == 0.
    template<typename Int>
    constexpr Int packSum() noexcept {
        return Int(0);
    }
    // Returns the sum of the contents of Pack.
    // Unlike constant folding, packSum can handle sizeof...(Pack) == 0.
    template<typename Int, std::convertible_to<Int>... Args>
    constexpr Int packSum(Args&&... args) noexcept {
        return (Int(std::forward<Args>(args)) + ...);
    }

    // Unit Tests
    static_assert(packSum<size_t>() == 0);
    static_assert(packSum<size_t>(1, 4, 3) == 8);
}

