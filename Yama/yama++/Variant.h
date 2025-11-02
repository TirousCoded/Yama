

#pragma once


#include <variant>
#include <format>

#include "meta.h"
#include "hash.h"


namespace ym {


    // A wrapper of std::variant.
    // This exists in part to provide a more convenient interface.
    template<typename... Types>
        requires (sizeof...(Types) >= 1)
    class Variant final {
    public:
        using Underlying = std::variant<Types...>;
        static constexpr size_t size = std::variant_size_v<Underlying>;
        template<size_t I>
            requires validPackIndex<I, Types...>
        using Alt = std::variant_alternative_t<I, Underlying>;


        constexpr Variant(const std::variant<Types...>& x) : _state(x) {} // Implicit
        constexpr Variant(std::variant<Types...>&& x) noexcept : // Implicit
            _state(std::forward<decltype(_state)>(x)) {}

        constexpr Variant(TypeInPack<Types...> auto&& x) noexcept : // Implicit
            _state(std::forward<std::remove_reference_t<decltype(x)>>(x)) {}
        constexpr Variant& operator=(TypeInPack<Types...> auto&& x) noexcept { // Implicit
            _state = std::forward<std::remove_reference_t<decltype(x)>>(x);
            return *this;
        }

        Variant() = delete;
        constexpr Variant(const Variant&) = default;
        constexpr Variant(Variant&&) noexcept = default;
        constexpr ~Variant() noexcept = default;
        constexpr Variant& operator=(const Variant&) = default;
        constexpr Variant& operator=(Variant&&) noexcept = default;


        constexpr bool operator==(const Variant&) const noexcept = default;

        constexpr operator Underlying& () noexcept { return _state; } // Implicit
        constexpr operator const Underlying& () const noexcept { return _state; } // Implicit

        constexpr size_t index() const noexcept { return _state.index(); }
        constexpr bool valuelessByException() const noexcept { return _state.valueless_by_exception(); }

        // Returns if the variant holds T.
        template<TypeInPack<Types...> T>
        constexpr bool is() const noexcept { return std::holds_alternative<T>(_state); }

        // Returns the variant's value as Alt<I>.
        // Throws std::bad_variant_access on failure.
        template<size_t I>
            requires validPackIndex<I, Types...>
        constexpr Alt<I>& as() { return std::get<I>(_state); }
        // Returns the variant's value as Alt<I>.
        // Throws std::bad_variant_access on failure.
        template<size_t I>
            requires validPackIndex<I, Types...>
        constexpr const Alt<I>& as() const { return std::get<I>(_state); }
        // Returns the variant's value as T.
        // Throws std::bad_variant_access on failure.
        template<TypeInPack<Types...> T>
        constexpr T& as() { return std::get<T>(_state); }
        // Returns the variant's value as T.
        // Throws std::bad_variant_access on failure.
        template<TypeInPack<Types...> T>
        constexpr const T& as() const { return std::get<T>(_state); }

        // Returns a pointer to the variant's value as Alt<I>, or nullptr on failure.
        template<size_t I>
            requires validPackIndex<I, Types...>
        constexpr Alt<I>* tryAs() noexcept { return std::get_if<I>(&_state); }
        // Returns a pointer to the variant's value as Alt<I>, or nullptr on failure.
        template<size_t I>
            requires validPackIndex<I, Types...>
        constexpr const Alt<I>* tryAs() const noexcept { return std::get_if<I>(&_state); }
        // Returns a pointer to the variant's value as T, or nullptr on failure.
        template<TypeInPack<Types...> T>
        constexpr T* tryAs() noexcept { return std::get_if<T>(&_state); }
        // Returns a pointer to the variant's value as T, or nullptr on failure.
        template<TypeInPack<Types...> T>
        constexpr const T* tryAs() const noexcept { return std::get_if<T>(&_state); }

        // Unavailable if allOf<Hashable<Types>...> == false.
        inline size_t hash() const noexcept {
            return ym::hash(_state);
        }

        inline std::string fmt() const {
            return _fmt();
        }

        template<TypeInPack<Types...> T, typename... Args>
        constexpr T& emplace(Args&&... args) { return _state.emplace<T>(std::forward<Args>(args)...); }
        template<TypeInPack<Types...> T, typename U, typename... Args>
        constexpr T& emplace(std::initializer_list<U> il, Args&&... args) { return _state.emplace<T>(il, std::forward<Args>(args)...); }
        template<size_t I, typename... Args>
            requires validPackIndex<I, Types...>
        constexpr Alt<I>& emplace(Args&&... args) { return _state.emplace<I>(std::forward<Args>(args)...); }
        template<size_t I, typename U, typename... Args>
            requires validPackIndex<I, Types...>
        constexpr Alt<I>& emplace(std::initializer_list<U> il, Args&&... args) { return _state.emplace<I>(il, std::forward<Args>(args)...); }


        // Initialize a variant by type.
        template<TypeInPack<Types...> T, typename... Args>
        static constexpr Variant<Types...> byType(Args&&... args) {
            return Variant(Underlying(std::in_place_type_t<T>{}, std::forward<Args>(args)...));
        }
        // Initialize a variant by type.
        template<TypeInPack<Types...> T, typename U, typename... Args>
        static constexpr Variant<Types...> byType(std::initializer_list<U> il, Args&&... args) {
            return Variant(Underlying(std::in_place_type_t<T>{}, il, std::forward<Args>(args)...));
        }

        // Initialize a variant by index.
        template<size_t I, typename... Args>
            requires validPackIndex<I, Types...>
        static constexpr Variant<Types...> byIndex(Args&&... args) {
            return Variant(Underlying(std::in_place_index_t<I>{}, std::forward<Args>(args)...));
        }
        // Initialize a variant by index.
        template<size_t I, typename U, typename... Args>
            requires validPackIndex<I, Types...>
        static constexpr Variant<Types...> byIndex(std::initializer_list<U> il, Args&&... args) {
            return Variant(Underlying(std::in_place_index_t<I>{}, il, std::forward<Args>(args)...));
        }


    private:
        std::variant<Types...> _state;


        template<typename TAlt, bool HasFormatterImpl>
        class _AltFmtHelper {};
        
        template<Formattable<char> TAlt>
        class _AltFmtHelper<TAlt, true> {
        public:
            static inline std::string fmt(const TAlt& x) {
                return std::format("{}", x);
            }
        };
        
        template<typename TAlt>
        class _AltFmtHelper<TAlt, false> {
        public:
            static inline std::string fmt(const TAlt&) {
                return _fmtFail;
            }
        };
        
        template<typename TAlt>
        static inline std::string _fmtAlt(const TAlt& x) {
            return _AltFmtHelper<TAlt, Formattable<TAlt, char>>::fmt(x);
        }
        template<size_t I>
            requires validPackIndex<I, Types...>
        inline std::string _fmtAltAtIndex() const {
            return _fmtAlt(as<I>());
        }
        template<size_t I>
            requires validPackIndex<I, Types...>
        inline void _attemptFmtForAltAtIndex(std::string& output) const {
            if (index() == I) {
                output = _fmtAltAtIndex<I>();
            }
        }
        template<size_t... Is>
        inline void _attemptFmtForEachAlt(std::string& output, std::index_sequence<Is...>) const {
            (_attemptFmtForAltAtIndex<Is>(output), ...);
        }
        inline std::string _fmt() const {
            if (valuelessByException()) {
                return _fmtFail;
            }
            std::string result{};
            _attemptFmtForEachAlt(result, std::make_index_sequence<size>{});
            return result;
        }

        static constexpr auto _fmtFail = "n/a";
    };
}

template<ym::Hashable... Types>
struct std::hash<ym::Variant<Types...>> {
    inline size_t operator()(const ym::Variant<Types...>& x) const noexcept {
        return x.hash();
    }
};

template<typename... Types>
struct std::formatter<ym::Variant<Types...>> : std::formatter<std::string> {
    auto format(const ym::Variant<Types...>& x, format_context& ctx) const {
        return formatter<string>::format(x.fmt(), ctx);
    }
};
namespace std {
    template<typename... Types>
    inline std::ostream& operator<<(std::ostream& stream, const ym::Variant<Types...>& x) {
        return stream << x.fmt();
    }
}

