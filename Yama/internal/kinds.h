

#pragma once


#include "../yama/yama.h"
#include "../yama++/meta.h"


namespace _ym {


    static_assert(YmKind_Num == 8);
    // Extends YmKind w/ new consts for types w/ irregular semantics.
    enum class KindEx : YmUInt8 {
        // Regular

        Struct = 0,
        Protocol,
        Fn,
        Var,
        VarAssigner,
        Method,
        Property,
        PropertyAssigner,

        // Special

        None,
        Int,
        UInt,
        Float,
        Bool,
        Rune,
        Type,

        MethodReq,

        StoredVarGet,
        StoredVarSet,
        StoredPropertyGet,
        StoredPropertySet,

        Num, // Enum Size
    };

    constexpr size_t KindExSize = (size_t)KindEx::Num;

    constexpr KindEx kindExOf(YmKind k) noexcept {
        return KindEx(k);
    }
    constexpr YmKind kindOf(KindEx x) noexcept {
        static_assert(KindExSize == 20);
        switch (x) {
        case KindEx::Struct:                return YmKind_Struct;
        case KindEx::Protocol:              return YmKind_Protocol;
        case KindEx::Fn:                    return YmKind_Fn;
        case KindEx::Var:                   return YmKind_Var;
        case KindEx::VarAssigner:           return YmKind_VarAssigner;
        case KindEx::Method:                return YmKind_Method;
        case KindEx::Property:              return YmKind_Property;
        case KindEx::PropertyAssigner:      return YmKind_PropertyAssigner;

        case KindEx::None:                  return YmKind_Struct;
        case KindEx::Int:                   return YmKind_Struct;
        case KindEx::UInt:                  return YmKind_Struct;
        case KindEx::Float:                 return YmKind_Struct;
        case KindEx::Bool:                  return YmKind_Struct;
        case KindEx::Rune:                  return YmKind_Struct;
        case KindEx::Type:                  return YmKind_Struct;

        case KindEx::MethodReq:             return YmKind_Method;

        case KindEx::StoredVarGet:          return YmKind_Var;
        case KindEx::StoredVarSet:          return YmKind_VarAssigner;
        case KindEx::StoredPropertyGet:     return YmKind_Property;
        case KindEx::StoredPropertySet:     return YmKind_PropertyAssigner;

        default:                            return YmKind{};
        }
    }
    template<YmKind MustBe>
    inline KindEx mustBe(KindEx x) noexcept {
        ymAssert(kindOf(x) == MustBe);
        return x;
    }

    using Slots = YmUInt16;

    // Statically known slot count for x, if any.
    constexpr std::optional<Slots> slotsOf(KindEx x) noexcept {
        static_assert(KindExSize == 20);
        switch (x) {
        case KindEx::Struct:                return std::nullopt; // One slot per stored property.
        case KindEx::Protocol:              return 2; // Slot #1 is boxed value, slot #2 is ptable ptr.
        case KindEx::Fn:                    return std::nullopt;
        case KindEx::Var:                   return std::nullopt;
        case KindEx::VarAssigner:           return std::nullopt;
        case KindEx::Method:                return std::nullopt;
        case KindEx::Property:              return std::nullopt;
        case KindEx::PropertyAssigner:      return std::nullopt;

        case KindEx::None:                  return 0;
        case KindEx::Int:                   return 1;
        case KindEx::UInt:                  return 1;
        case KindEx::Float:                 return 1;
        case KindEx::Bool:                  return 1;
        case KindEx::Rune:                  return 1;
        case KindEx::Type:                  return 1;

        case KindEx::MethodReq:             return std::nullopt;

        case KindEx::StoredVarGet:          return std::nullopt;
        case KindEx::StoredVarSet:          return std::nullopt;
        case KindEx::StoredPropertyGet:     return std::nullopt;
        case KindEx::StoredPropertySet:     return std::nullopt;

        default:                            return std::nullopt;
        }
    }

    // Given some slot count, this discerns if the type of x would be a ref
    // carrier type.
    constexpr bool isRefCarrier(KindEx x, Slots slots) noexcept {
        static_assert(KindExSize == 20);
        switch (x) {
        case KindEx::Struct:                return slots >= 1;
        case KindEx::Protocol:              return true;
        case KindEx::Fn:                    return false;
        case KindEx::Var:                   return false;
        case KindEx::VarAssigner:           return false;
        case KindEx::Method:                return false;
        case KindEx::Property:              return false;
        case KindEx::PropertyAssigner:      return false;

        case KindEx::None:                  return false;
        case KindEx::Int:                   return false;
        case KindEx::UInt:                  return false;
        case KindEx::Float:                 return false;
        case KindEx::Bool:                  return false;
        case KindEx::Rune:                  return false;
        case KindEx::Type:                  return false;

        case KindEx::MethodReq:             return false;

        case KindEx::StoredVarGet:          return false;
        case KindEx::StoredVarSet:          return false;
        case KindEx::StoredPropertyGet:     return false;
        case KindEx::StoredPropertySet:     return false;

        default:                            return false;
        }
    }

    // Given some slot count, and some visitor, this traverses each of the
    // ref slots types of x would have, if any.
    inline void forEachRefSlotIndexOf(
        KindEx x,
        Slots slots,
        ym::Callable<void, Slots> auto&& visitor) {
        static_assert(KindExSize == 20);
        if (x == KindEx::Struct) {
            for (Slots i = 0; i < slots; i++) {
                visitor(i); // Visit each stored property slot.
            }
        }
        else if (x == KindEx::Protocol) {
            ymAssert(slots == *slotsOf(x));
            visitor(0); // Visit boxed value.
        }
    }

    constexpr bool isRegular(KindEx x) noexcept {
        return size_t(x) < YmKind_Num;
    }
    constexpr bool isIrregular(KindEx x) noexcept {
        return !isRegular(x);
    }
    constexpr bool isPrimitive(KindEx x) noexcept {
        return
            x >= KindEx::None &&
            x <= KindEx::Type;
    }
    constexpr bool isGetter(KindEx x) noexcept {
        return
            kindOf(x) == YmKind_Var ||
            kindOf(x) == YmKind_Property;
    }
    constexpr bool isSetter(KindEx x) noexcept {
        return
            kindOf(x) == YmKind_VarAssigner ||
            kindOf(x) == YmKind_PropertyAssigner;
    }
    constexpr bool isVarLike(KindEx x) noexcept {
        return isGetter(x) || isSetter(x);
    }
    constexpr bool isProtocolReq(KindEx x) noexcept {
        // TODO: Update whenever we add new protocol req. types.
        return x == KindEx::MethodReq;
    }
}

