

#pragma once


#include "../yama/asserts.h"
#include "meta.h"


namespace ym {


    template<Dereferenceable T>
    inline auto&& deref(T&& x) noexcept {
        ymAssert((bool)x);
        return *x;
    }
}

