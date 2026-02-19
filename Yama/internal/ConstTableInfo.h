

#pragma once


#include <array>
#include <vector>
#include <optional>

#include "../yama/yama.h"
#include "../yama++/scalar.h"
#include "../yama++/Variant.h"

#include "general.h"
#include "Spec.h"


namespace _ym {

    
    enum class ConstType : YmUInt8 {
        Int = 0,
        UInt,
        Float,
        Bool,
        Rune,

        Ref,

        Num, // Enum size. Not a constant type.
    };
    constexpr size_t ConstTypes = (size_t)ConstType::Num;

    inline const YmChar* fmt(ConstType x) {
        static constexpr std::array<const YmChar*, ConstTypes> names{
            "Int",
            "UInt",
            "Float",
            "Bool",
            "Rune",

            "Ref",
        };
        return
            size_t(x) < ConstTypes
            ? names[size_t(x)]
            : "???";
    }


    struct RefInfo final {
        Spec sym;


        bool operator==(const RefInfo&) const noexcept = default;
    };
    using ConstInfo = ym::Variant<
        YmInt,
        YmUInt,
        YmFloat,
        YmBool,
        YmRune,

        RefInfo
    >;
    static_assert(ConstInfo::size == ConstTypes);

    inline ConstType constTypeOf(const ConstInfo& x) noexcept {
        return ConstType(x.index());
    }
    inline bool isVal(const ConstInfo& x) noexcept {
        return constTypeOf(x) != ConstType::Ref;
    }
    inline bool isRef(const ConstInfo& x) noexcept {
        return !isVal(x);
    }

    inline std::string fmt(const ConstInfo& x) {
        static_assert(ConstInfo::size == ConstTypes);
        auto t = constTypeOf(x);
        switch (t) {
        case ConstType::Int:        return std::format("{} ({})", fmt(t), ym::fmt(x.as<YmInt>()));
        case ConstType::UInt:       return std::format("{} ({})", fmt(t), ym::fmt(x.as<YmUInt>()));
        case ConstType::Float:      return std::format("{} ({})", fmt(t), ym::fmt(x.as<YmFloat>()));
        case ConstType::Bool:       return std::format("{} ({})", fmt(t), ym::fmt(x.as<YmBool>()));
        case ConstType::Rune:       return std::format("{} ({})", fmt(t), ym::fmt(x.as<YmRune>()));
        case ConstType::Ref:        return std::format("{} ({})", fmt(t), x.as<RefInfo>().sym);
        default:                    return std::format("{} (n/a)", fmt(t));
        }
    }


    using ConstIndex = size_t;

	class ConstTableInfo final {
	public:
		ConstTableInfo() = default;


        inline size_t size() const noexcept {
            return _consts.size();
        }

        inline const ConstInfo& at(ConstIndex x) const noexcept {
            ymAssert(x < _consts.size());
            return _consts[x];
        }
        inline const ConstInfo& operator[](ConstIndex x) const noexcept {
            return at(x);
        }

        // Returns if constant symbol at x is a value constant.
        inline bool isVal(ConstIndex x) const noexcept {
            return _ym::isVal(at(x));
        }
        // Returns if constant symbol at x is a reference constant.
        inline bool isRef(ConstIndex x) const noexcept {
            return _ym::isRef(at(x));
        }

        // Returns the index of constant symbol x, if any.
        // Fails if returned index would need to be >= sizeLimit.
        inline std::optional<ConstIndex> fetch(const ConstInfo& x, size_t sizeLimit = size_t(-1)) const noexcept {
            for (ConstIndex i = 0; i < std::min(_consts.size(), sizeLimit); i++) {
                if (_consts[i] == x) {
                    return i;
                }
            }
            return std::nullopt;
        }
        template<typename T>
        inline auto fetchVal(const T& x, size_t sizeLimit = size_t(-1)) const noexcept {
            return fetch(ConstInfo(x), sizeLimit);
        }
        inline auto fetchRef(Spec symbol, size_t sizeLimit = size_t(-1)) const noexcept {
            return fetch(ConstInfo(RefInfo{ .sym = std::move(symbol) }), sizeLimit);
        }

        // Returns the index of constant symbol x, adding it if not already present.
        // Fails if returned index would need to be >= sizeLimit.
        inline std::optional<ConstIndex> pull(ConstInfo x, size_t sizeLimit = size_t(-1)) {
            if (auto result = fetch(x)) {
                return result;
            }
            if (size() >= sizeLimit) {
                return std::nullopt;
            }
            _consts.push_back(std::move(x));
            return _consts.size() - 1;
        }
        template<typename T>
        inline auto pullVal(const T& x, size_t sizeLimit = size_t(-1)) {
            return pull(ConstInfo(x), sizeLimit);
        }
        inline auto pullRef(Spec symbol, size_t sizeLimit = size_t(-1)) {
            return pull(ConstInfo(RefInfo{ .sym = std::move(symbol) }), sizeLimit);
        }


    private:
        std::vector<ConstInfo> _consts;
	};
}

