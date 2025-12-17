

#pragma once


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
            return ymParcel_Path(*this);
        }
    };
}

