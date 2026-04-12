

#pragma once


#include <format>
#include <iostream>

#include "Handle.h"
#include "resources.h"
#include "Type.h"


namespace ym {


    // TODO: All of the below haven't been unit tested.

    // A RAII handle wrapping a YmObj.
    class Object final : public Handle<YmObj> {
    public:
        // Increments resource's ref count if secure == true.
        inline explicit Object(Safe<YmObj> resource, bool secure) noexcept :
            Handle(resource, secure) {
        }


        inline Type type() const noexcept {
            return Type(Safe(ymObj_Type(get())));
        }
        inline std::string fmt() const {
            // TODO: Figure out how to remove this extra round of heap alloc.
            auto temp = ymObj_Fmt(get());
            assertSafe(temp);
            std::string result(temp);
            // TODO: This cleanup won't occur if any above throws.
            std::free((void*)temp);
            return result;
        }

        inline std::optional<YmInt> toInt() const noexcept {
            YmBool success{};
            auto result = ymObj_ToInt(get(), &success);
            return
                success == YM_TRUE
                ? std::make_optional(result)
                : std::nullopt;
        }
        inline std::optional<YmUInt> toUInt() const noexcept {
            YmBool success{};
            auto result = ymObj_ToUInt(get(), &success);
            return
                success == YM_TRUE
                ? std::make_optional(result)
                : std::nullopt;
        }
        inline std::optional<YmFloat> toFloat() const noexcept {
            YmBool success{};
            auto result = ymObj_ToFloat(get(), &success);
            return
                success == YM_TRUE
                ? std::make_optional(result)
                : std::nullopt;
        }
        inline std::optional<YmBool> toBool() const noexcept {
            YmBool success{};
            auto result = ymObj_ToBool(get(), &success);
            return
                success == YM_TRUE
                ? std::make_optional(result)
                : std::nullopt;
        }
        inline std::optional<YmRune> toRune() const noexcept {
            YmBool success{};
            auto result = ymObj_ToRune(get(), &success);
            return
                success == YM_TRUE
                ? std::make_optional(result)
                : std::nullopt;
        }
        inline std::optional<Type> toType() const noexcept {
            YmBool success{};
            auto result = ymObj_ToType(get(), &success);
            return
                success == YM_TRUE
                ? std::make_optional(Type(Safe(result)))
                : std::nullopt;
        }
    };
}

template<>
struct std::formatter<ym::Object> : std::formatter<std::string> {
    auto format(const ym::Object& x, format_context& ctx) const {
        return formatter<string>::format(x.fmt(), ctx);
    }
};
namespace std {
    inline std::ostream& operator<<(std::ostream& stream, const ym::Object& x) {
        return stream << x.fmt();
    }
}

