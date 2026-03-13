

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
        // Does not increment the ref count of resource.
        inline explicit Object(Safe<YmObj> resource) noexcept :
            Handle(resource) {
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

        inline YmInt toInt() const noexcept { return ymObj_ToInt(get()); }
        inline YmUInt toUInt() const noexcept { return ymObj_ToUInt(get()); }
        inline YmFloat toFloat() const noexcept { return ymObj_ToFloat(get()); }
        inline YmBool toBool() const noexcept { return ymObj_ToBool(get()); }
        inline YmRune toRune() const noexcept { return ymObj_ToRune(get()); }
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

