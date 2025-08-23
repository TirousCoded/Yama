

#pragma once


#include <stdexcept>
#include <utility>
#include <memory>
#include <unordered_map>
#include <concepts>

#include "../core/concepts.h"


namespace yama::internal {


    template<typename T>
    concept id_type =
        std::copyable<T> &&
        std::movable<T> &&
        std::equality_comparable<T> &&
        hashable_type<T>;

    template<typename T>
    concept desc_type =
        std::copyable<T> &&
        std::movable<T> &&
        !std::is_const_v<T> &&
        !std::is_volatile_v<T> &&
        !std::is_reference_v<T>;


    template<desc_type... Ds>
    class desc_tuple final {
    public:
        desc_tuple() = default;
        inline desc_tuple(const desc_tuple& other)
            : _tuple(_clone_tuple(other._tuple)) {
        }
        desc_tuple(desc_tuple&&) noexcept = default;
        ~desc_tuple() noexcept = default;
        inline desc_tuple& operator=(const desc_tuple& other) {
            _tuple = _clone_tuple(other._tuple);
            return *this;
        }
        desc_tuple& operator=(desc_tuple&&) noexcept = default;


        // The size of descriptors Ds.
        static constexpr size_t number = sizeof...(Ds);

        // If types Ts are in descriptors Ds.
        template<desc_type... Ts>
        static constexpr bool supports = (internal::exists_in_pack_v<Ts, Ds...> && ...);


        template<desc_type... Ts>
            requires supports<Ts...>
        inline bool all_of() const noexcept {
            if constexpr (sizeof...(Ts) == 1) {
                return _get<Ts...>() != nullptr;
            }
            else {
                return (all_of<Ts>() && ...);
            }
        }
        template<desc_type... Ts>
            requires supports<Ts...>
        inline bool any_of() const noexcept {
            if constexpr (sizeof...(Ts) == 1) {
                return _get<Ts...>() != nullptr;
            }
            else {
                return (any_of<Ts>() || ...);
            }
        }
        template<desc_type... Ts>
            requires supports<Ts...>
        inline bool none_of() const noexcept {
            return !any_of<Ts...>();
        }

        template<bool Const, desc_type... Ts>
            requires supports<Ts...>
        inline auto try_get() const noexcept {
            return std::make_tuple(_get_ptr<Const, Ts>()...);
        }
        template<bool Const, desc_type... Ts>
            requires supports<Ts...>
        inline auto get() const {
            if (!all_of<Ts...>()) {
                throw std::out_of_range("Couldn't find bindings for all descriptor types!");
            }
            return std::tie(_get_ref<Const, Ts>()...);
        }

        template<desc_type T, typename... Args>
            requires supports<T>
        inline void bind(Args&&... args) {
            _get<T>() = std::make_unique<T>(std::forward<Args>(args)...);
        }
        template<desc_type... Ts>
            requires supports<Ts...>
        inline void unbind() noexcept {
            (_get<Ts>().reset(), ...);
        }


    private:
        using _tuple_t = std::tuple<std::unique_ptr<Ds>...>;


        mutable _tuple_t _tuple;


        template<desc_type T>
            requires supports<T>
        inline std::unique_ptr<T>& _get() const noexcept {
            return std::get<std::unique_ptr<T>>(_tuple);
        }
        template<bool Const, desc_type T>
            requires supports<T>
        inline auto _get_ptr() const noexcept {
            return const_cast<std::conditional_t<Const, const T*, T*>>(_get<T>().get());
        }
        // Throws if nullptr.
        template<bool Const, desc_type T>
            requires supports<T>
        inline auto& _get_ref() const {
            return deref_assert(_get_ptr<Const, T>());
        }

        static _tuple_t _clone_tuple(const _tuple_t& other) {
            return _clone_tuple_helper(other, std::make_index_sequence<sizeof...(Ds)>{});
        }
        template<size_t... I>
        static _tuple_t _clone_tuple_helper(const _tuple_t& other, std::index_sequence<I...>) {
            return std::make_tuple(_clone_field<I>(other)...);
        }
        template<size_t I>
        static std::tuple_element_t<I, _tuple_t> _clone_field(const _tuple_t& other) {
            using T = std::tuple_element_t<I, _tuple_t>;
            const auto& ours = std::get<I>(other);
            using U = std::remove_cvref_t<decltype(*ours)>;
            return
                ours != nullptr
                ? std::make_unique<U>(U(*ours))
                : nullptr;
        }
    };

    template<id_type ID, desc_type... Ds>
    class desc_map final {
    public:
        using tuple_t = desc_tuple<Ds...>;

        
    private:
        using _descs_map_t = std::unordered_map<ID, tuple_t>;
        using _descs_iter_t = _descs_map_t::iterator;


    public:
        template<bool Const, desc_type... Ts>
        class view_iterator final {
        public:
            using value_type = std::tuple<ID, std::conditional_t<Const, const Ts&, Ts&>...>;
            using pointer = void;
            using reference = value_type;
            using difference_type = ptrdiff_t;
            using iterator_category = std::forward_iterator_tag;


            view_iterator() = default;
            view_iterator(const view_iterator&) = default;
            view_iterator(view_iterator&&) noexcept = default;
            ~view_iterator() noexcept = default;
            view_iterator& operator=(const view_iterator&) = default;
            view_iterator& operator=(view_iterator&&) noexcept = default;


            inline view_iterator& operator++() noexcept {
                if (_it != _end) {
                    ++_it;
                    _advance_until_valid_item_or_end();
                }
                return *this;
            }
            inline view_iterator operator++(int) noexcept {
                auto old = *this;
                ++*this;
                return old;
            }
            // Throws std::out_of_range if past-the-end.
            inline value_type operator*() const {
                if (_it == _end) {
                    throw std::out_of_range("Cannot dereference past-the-end iterator!");
                }
                return std::tuple_cat(std::make_tuple(_it->first), _it->second.get<Const, Ts...>());
            }

            bool operator==(const view_iterator&) const noexcept = default;


        private:
            friend class desc_map;


            _descs_iter_t _it, _end;


            // Inits a view iterator to the first item w/ all descriptors in Ds, if any.
            inline view_iterator(_descs_iter_t begin, _descs_iter_t end)
                : _it(begin),
                _end(end) {
                _advance_until_valid_item_or_end(); // Advance until first valid item, if any.
            }

            // Inits a view iterator to the specified past-the-end iterator.
            inline explicit view_iterator(_descs_iter_t end)
                : _it(end),
                _end(end) {
            }


            // Advances the iterator until either it reaches item w/ all descriptors in Ds, or past-the-end.
            // If current state satisfies condition, no advancing occurs.
            inline void _advance_until_valid_item_or_end() noexcept {
                while (_it != _end && !_it->second.all_of<Ts...>()) {
                    std::advance(_it, 1);
                }
            }
        };


        desc_map() = default;
        desc_map(const desc_map&) = default;
        desc_map(desc_map&&) noexcept = default;
        ~desc_map() noexcept = default;
        desc_map& operator=(const desc_map&) = default;
        desc_map& operator=(desc_map&&) noexcept = default;


        // The size of descriptors Ds.
        static constexpr size_t number = tuple_t::number;

        // If types Ts are in descriptors Ds.
        template<desc_type... Ts>
        static constexpr bool supports = tuple_t::template supports<Ts...>;


        // Returns number of items.
        inline uint16_t count() const noexcept {
            return (uint16_t)_data.size();
        }
        // Returns number of items with all descriptors Ts.
        template<desc_type... Ts>
            requires supports<Ts...>
        inline uint16_t count_with() const noexcept {
            if constexpr (sizeof...(Ts) == 0) {
                return count(); // Avoid iterating over EVERYTHING.
            }
            else {
                uint16_t n = 0;
                for (auto& [key, value] : _data) {
                    if (!value.all_of<Ts...>()) continue;
                    n++;
                }
                return n;
            }
        }

        // Returns if item exists.
        inline bool exists(ID id) const noexcept {
            return _data.contains(id);
        }

        // Returns if id has all descriptors in Ts.
        template<desc_type... Ts>
            requires supports<Ts...>
        inline bool all_of(ID id) const noexcept {
            if constexpr (sizeof...(Ts) == 0) {
                return true;
            }
            else {
                const auto it = _data.find(id);
                return
                    it != _data.end()
                    ? it->second.all_of<Ts...>()
                    : false;
            }
        }
        // Returns if id has at least one descriptor in Ts.
        template<desc_type... Ts>
            requires supports<Ts...>
        inline bool any_of(ID id) const noexcept {
            if constexpr (sizeof...(Ts) == 0) {
                return false;
            }
            else {
                const auto it = _data.find(id);
                return
                    it != _data.end()
                    ? it->second.any_of<Ts...>()
                    : false;
            }
        }
        // Returns if id has no descriptors in Ts.
        template<desc_type... Ts>
            requires supports<Ts...>
        inline bool none_of(ID id) const noexcept {
            return !any_of<Ts...>(id);
        }

        // Returns pointers to descriptors Ts for id.
        // Pointers will be nullptr for descriptors not bound to id.
        template<desc_type... Ts>
            requires supports<Ts...>
        inline std::tuple<Ts*...> try_get(ID id) noexcept {
            const auto it = _data.find(id);
            return
                it != _data.end()
                ? it->second.try_get<false, Ts...>()
                : std::make_tuple((Ts*)nullptr...);
        }
        // Returns pointers to descriptors Ts for id.
        // Pointers will be nullptr for descriptors not bound to id.
        template<desc_type... Ts>
            requires supports<Ts...>
        inline std::tuple<const Ts*...> try_get(ID id) const noexcept {
            const auto it = _data.find(id);
            return
                it != _data.end()
                ? it->second.try_get<true, Ts...>()
                : std::make_tuple((const Ts*)nullptr...);
        }

        // Returns references to descriptors Ts for id.
        // Throws std::out_of_range if all_of<Ts...>(id) == false.
        template<desc_type... Ts>
            requires supports<Ts...>
        inline std::tuple<Ts&...> get(ID id) {
            auto it = _data.find(id);
            if (it == _data.end()) {
                throw std::out_of_range("Couldn't find bindings for all descriptor types!");
            }
            return it->second.get<false, Ts...>();
        }
        // Returns references to descriptors Ts for id.
        // Throws std::out_of_range if all_of<Ts...>(id) == false.
        template<desc_type... Ts>
            requires supports<Ts...>
        inline std::tuple<const Ts&...> get(ID id) const {
            auto it = _data.find(id);
            if (it == _data.end()) {
                throw std::out_of_range("Couldn't find bindings for all descriptor types!");
            }
            return it->second.get<true, Ts...>();
        }


        // Returns a view over all items with descriptors Ts, if any.
        template<desc_type... Ts>
            requires supports<Ts...>
        inline auto view() noexcept {
            using Iter = view_iterator<false, Ts...>;
            return std::ranges::subrange<Iter>(Iter(_data.begin(), _data.end()), Iter(_data.end()));
        }
        // Returns a view over all items with descriptors Ts, if any.
        template<desc_type... Ts>
            requires supports<Ts...>
        inline auto view() const noexcept {
            using Iter = view_iterator<true, Ts...>;
            return std::ranges::subrange<Iter>(Iter(_data.begin(), _data.end()), Iter(_data.end()));
        }

        // Returns a empty via w/out need of a desc_map object to exist in relation to.
        template<bool Const, desc_type... Ts>
            requires supports<Ts...>
        static inline auto empty_view() noexcept {
            return std::ranges::subrange<view_iterator<Const, Ts...>>{};
        }


        // Returns the desc_tuple containing the underlying data for the item under id, if any.
        // This info about desc_tuple is for Yama impl code only, and may not be exposed in the frontend.
        inline const tuple_t* peek_tuple(ID id) const noexcept {
            auto it = _data.find(id);
            return
                it != _data.end()
                ? &it->second
                : nullptr;
        }


        // Binds descriptor.
        template<desc_type T, typename... Args>
            requires supports<T>
        inline void bind(ID id, Args&&... args) {
            _data[id].bind<T>(std::forward<Args>(args)...);
        }
        // Unbinds descriptors Ts from id.
        template<desc_type... Ts>
            requires supports<Ts...>
        inline void unbind(ID id) noexcept {
            if (const auto it = _data.find(id); it != _data.end()) {
                it->second.unbind<Ts...>();
            }
        }
        // Unbinds descriptors Ts from all items.
        template<desc_type... Ts>
            requires supports<Ts...>
        inline void unbind_every() noexcept {
            for (auto& [key, value] : _data) {
                value.unbind<Ts...>();
            }
        }


    private:
        mutable _descs_map_t _data;
    };
}

