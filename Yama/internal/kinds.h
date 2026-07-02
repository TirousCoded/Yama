

#pragma once


#include "../yama/yama.h"


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

