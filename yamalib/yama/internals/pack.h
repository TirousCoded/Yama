

#pragma once


//


namespace yama::internal {


    template<typename T, typename... Pack, size_t... I>
        requires (sizeof...(Pack) == sizeof...(I))
    constexpr size_t index_in_pack_helper(std::index_sequence<I...>) noexcept {
        size_t result{};
        bool success = ((result = I, std::is_same_v<T, Pack>) || ...);
        return success ? result : sizeof...(Pack);
    }

    // TODO: I really like index_in_pack, so maybe bring it to the frontend at some point.

    // index_in_pack discerns the index in Pack where T is first located, or sizeof...(Pack) if
    // T is not in Pack.

    template<typename T, typename... Pack>
    struct index_in_pack final {
        static constexpr size_t value = index_in_pack_helper<T, Pack...>(std::index_sequence_for<Pack...>{});
    };
    template<typename T, typename... Pack>
    constexpr size_t index_in_pack_v = index_in_pack<T, Pack...>::value;

    // unit tests
    static_assert(index_in_pack_v<int, int, float, char> == 0);
    static_assert(index_in_pack_v<int, float, int, char> == 1);
    static_assert(index_in_pack_v<int, float, char, int> == 2);
    static_assert(index_in_pack_v<int, float, int, char, int> == 1); // stop at first match
    static_assert(index_in_pack_v<int, float, bool, char> == 3); // fail
    static_assert(index_in_pack_v<int> == 0); // fail


    // exists_in_pack discerns if T exists within Pack.

    template<typename T, typename... Pack>
    struct exists_in_pack final {
        static constexpr bool value = index_in_pack_v<T, Pack...> != sizeof...(Pack);
    };
    template<typename T, typename... Pack>
    constexpr bool exists_in_pack_v = exists_in_pack<T, Pack...>::value;

    // unit tests
    static_assert(exists_in_pack_v<int, int, float, char> == true);
    static_assert(exists_in_pack_v<int, int, float, char, int> == true); // multiple
    static_assert(exists_in_pack_v<int, bool, float, char> == false); // fail
    static_assert(exists_in_pack_v<int> == false); // fail


    // pack_sum discerns the sum of the contents of Pack, w/ it being able to
    // handle when sizeof...(Pack) == 0 (unlike constant folding.)

    template<typename Int>
    constexpr Int pack_sum() noexcept {
        return Int(0);
    }
    template<typename Int, std::convertible_to<Int>... Args>
    constexpr Int pack_sum(Args&&... args) noexcept {
        return (Int(std::forward<Args>(args)) + ...);
    }

    // unit tests
    static_assert(pack_sum<size_t>() == 0);
    static_assert(pack_sum<size_t>(1, 4, 3) == 8);
}

