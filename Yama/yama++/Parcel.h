

#pragma once


#include <format>
#include <iostream>

#include "Handle.h"


namespace ym {


    // TODO: All of the below haven't been unit tested.

    // A handle wrapping a YmParcel.
    class Parcel final : public Handle<YmParcel> {
    public:
        inline explicit Parcel(Safe<YmParcel> x) noexcept :
            Handle(x) {
        }


        inline std::string_view path() const noexcept {
            return ymParcel_Path(get());
        }
        inline std::string fmt() const {
            return std::string(path());
        }
    };
}

template<>
struct std::formatter<ym::Parcel> : std::formatter<std::string> {
    auto format(const ym::Parcel& x, format_context& ctx) const {
        return formatter<string>::format(x.fmt(), ctx);
    }
};
namespace std {
    inline std::ostream& operator<<(std::ostream& stream, const ym::Parcel& x) {
        return stream << x.fmt();
    }
}

