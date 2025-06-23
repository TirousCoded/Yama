

#pragma once


#include <utility>
#include <ranges>
#include <type_traits>
#include <array>
#include <unordered_map>

#include "../core/asserts.h"
#include "../core/general.h"
#include "../core/concepts.h"


namespace yama {


    namespace internal {
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


        // TODO: maybe replace w/ something more *official* later

        template<typename T>
        concept reasonably_formattable =
            requires(T v)
        {
            { std::format("{}", v) } -> std::convertible_to<std::string>;
        };
    }


    // TODO: attrs is for later on when we revise our system to be 'attribute orientated'

    template<typename T>
    concept attrs_key_type =
        std::copyable<T> &&
        std::movable<T> &&
        std::equality_comparable<T> &&
        hashable_type<T>;

    template<typename T>
    concept attr_type =
        std::movable<T> &&
        !std::is_const_v<T> &&
        !std::is_volatile_v<T> &&
        !std::is_reference_v<T>;

    template<attrs_key_type Key, attr_type... Attrs>
    class attrs final {
    public:
        template<typename Attr>
        using iterator = std::unordered_map<Key, Attr>::iterator;
        template<typename Attr>
        using const_iterator = std::unordered_map<Key, Attr>::const_iterator;

        template<typename It>
        class traverser final : public std::ranges::view_interface<traverser<It>> {
        public:
            // internal, do not use
            inline traverser(It first, It last)
                : _first(first),
                _last(last) {}

            traverser() = delete;
            traverser(const traverser&) = default;
            traverser(traverser&&) noexcept = default;
            ~traverser() noexcept = default;
            traverser& operator=(const traverser&) = default;
            traverser& operator=(traverser&&) noexcept = default;


            constexpr const It& begin() const noexcept { return _first; }
            constexpr const It& end() const noexcept { return _last; }


        private:
            It _first, _last;
        };


        // If the attr types in Types are bindable in this yama::attrs.
        template<attr_type... Types>
        static constexpr bool supports = (internal::exists_in_pack_v<std::remove_cvref_t<Types>, Attrs...> && ...);


        attrs() = default;
        attrs(const attrs&) = default;
        attrs(attrs&&) noexcept = default;
        ~attrs() noexcept = default;
        attrs& operator=(const attrs&) = default;
        attrs& operator=(attrs&&) noexcept = default;


        // Returns the sum of the number of attrs bound for each attr type in Types.
        template<attr_type... Types>
            requires supports<Types...>
        inline size_t count() const noexcept {
            return internal::pack_sum<size_t>((_map<Types>().size())...);
        }
        // Returns the sum of the number of attrs bound to keys in ks for each attr type in Types.
        template<attr_type... Types>
            requires supports<Types...>
        inline size_t count_for(const std::convertible_to<Key> auto&... ks) const noexcept {
            return internal::pack_sum<size_t>((_count_for_key<Types...>(ks))...);
        }

        // Returns the total number of attrs bound.
        inline size_t total() const noexcept {
            return count<Attrs...>();
        }
        // Returns the total number of attrs bound to keys in ks.
        inline size_t total_for(const std::convertible_to<Key> auto&... ks) const noexcept {
            return count_for<Attrs...>(ks...);
        }

        // Returns if each key in ks has attrs bound for each attr type in Types.
        template<attr_type... Types>
            requires supports<Types...>
        inline bool have(const std::convertible_to<Key> auto&... ks) const noexcept {
            return (_have_for_key<Types...>(ks) && ...);
        }

        // Returns a non-owning view of attrs bound to k which are in Types.
        // Throws std::out_of_range if have<Types...>(k) == false.
        template<attr_type... Types>
            requires supports<Types...>
        inline std::tuple<std::remove_cvref_t<Types>&...> view(const std::convertible_to<Key> auto& k) {
            return std::tie<std::remove_cvref_t<Types>&...>((_map<Types>().at(k))...);
        }
        // Returns a non-owning view of attrs bound to k which are in Types.
        // Throws std::out_of_range if have<Types...>(k) == false.
        template<attr_type... Types>
            requires supports<Types...>
        inline std::tuple<const std::remove_cvref_t<Types>&...> view(const std::convertible_to<Key> auto& k) const {
            return std::tie<const std::remove_cvref_t<Types>&...>((_map<Types>().at(k))...);
        }

        // Returns begin iterator for attrs of Type.
        template<attr_type Type>
            requires supports<Type>
        inline iterator<Type> begin() noexcept {
            return _map<Type>().begin();
        }
        // Returns begin iterator for attrs of Type.
        template<attr_type Type>
            requires supports<Type>
        inline const_iterator<Type> cbegin() const noexcept {
            return _map<Type>().cbegin();
        }
        // Returns begin iterator for attrs of Type.
        template<attr_type Type>
            requires supports<Type>
        inline const_iterator<Type> begin() const noexcept {
            return cbegin<Type>();
        }
        // Returns end iterator for attrs of Type.
        template<attr_type Type>
            requires supports<Type>
        inline iterator<Type> end() noexcept {
            return _map<Type>().end();
        }
        // Returns end iterator for attrs of Type.
        template<attr_type Type>
            requires supports<Type>
        inline const_iterator<Type> cend() const noexcept {
            return _map<Type>().cend();
        }
        // Returns end iterator for attrs of Type.
        template<attr_type Type>
            requires supports<Type>
        inline const_iterator<Type> end() const noexcept {
            return cend<Type>();
        }

        // Returns an object which is suitable for traversing the attrs of Type.
        template<attr_type Type>
            requires supports<Type>
        inline traverser<iterator<Type>> traverse() noexcept {
            return traverser(begin<Type>(), end<Type>());
        }
        // Returns an object which is suitable for traversing the attrs of Type.
        template<attr_type Type>
            requires supports<Type>
        inline traverser<const_iterator<Type>> ctraverse() const noexcept {
            return traverser(cbegin<Type>(), cend<Type>());
        }
        // Returns an object which is suitable for traversing the attrs of Type.
        template<attr_type Type>
            requires supports<Type>
        inline traverser<const_iterator<Type>> traverse() const noexcept {
            return ctraverse<Type>();
        }

        // Binds new_attrs to k, overwriting existing bindings.
        template<attr_type... Types>
            requires supports<Types...>
        inline void bind(const std::convertible_to<Key> auto& k, Types&&... new_attrs) {
            (_map<Types>().insert_or_assign(Key(k), std::forward<Types>(new_attrs)), ...);
        }
        // Unbinds attrs bound to keys in ks which are in Types.
        template<attr_type... Types>
            requires supports<Types...>
        inline void unbind(const std::convertible_to<Key> auto&... ks) {
            (_unbind_for_key<Types...>(ks), ...);
        }
        // Unbinds all attrs bound to keys in ks.
        inline void unbind_all(const std::convertible_to<Key> auto&... ks) {
            unbind<Attrs...>(ks...);
        }
        // Unbinds all bound attrs which are in Types.
        template<attr_type... Types>
            requires supports<Types...>
        inline void reset() noexcept {
            (_map<Types>().clear(), ...);
        }
        // Unbinds all bound attrs.
        inline void reset_all() noexcept {
            reset<Attrs...>();
        }


        // Returns a formatted string of the contents of the attrs of Type.
        template<attr_type Type>
            requires supports<Type> && internal::reasonably_formattable<Key> && internal::reasonably_formattable<Type>
        inline std::string fmt() const {
            std::string result{};
            result += std::format("attributes ({})", count<Type>());
            for (const auto& [key, value] : traverse<Type>()) {
                // try to keep pos of key and value parts inline w/ 4 char tabs
                result += std::format("\n    {: <16}: {}", key, value);
            }
            return result;
        }


    private:
        template<attr_type T>
        static constexpr size_t _indexof = internal::index_in_pack_v<T, Attrs...>;


        std::tuple<std::unordered_map<Key, Attrs>...> _maps;


        template<attr_type Attr>
        inline std::unordered_map<Key, Attr>& _map() noexcept {
            return std::get<_indexof<Attr>>(_maps);
        }
        template<attr_type Attr>
        inline const std::unordered_map<Key, Attr>& _map() const noexcept {
            return std::get<_indexof<Attr>>(_maps);
        }
        template<attr_type... Types>
            requires supports<Types...>
        inline size_t _count_for_key(const std::convertible_to<Key> auto& k) const noexcept {
            return internal::pack_sum<size_t>(((size_t)_map<Types>().contains(k))...);
        }
        template<attr_type... Types>
            requires supports<Types...>
        inline bool _have_for_key(const std::convertible_to<Key> auto& k) const noexcept {
            return (_map<Types>().contains(k) && ...);
        }
        template<attr_type... Types>
            requires supports<Types...>
        inline void _unbind_for_key(const std::convertible_to<Key> auto& k) {
            ((void)_map<Types>().erase(k), ...);
        }
    };
}

