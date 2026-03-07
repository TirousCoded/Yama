

#pragma once


#ifdef _YM_FORBID_INCLUDE_IN_YAMA_DOT_H
#error Not allowed to expose this header file to header file yama.h!
#endif


#include <optional>

#include "../yama/yama.h"
#include "../yama++/Safe.h"
#include "HAL.h"
#include "RefCounter.h"
#include "YmCtx.h"


struct YmObj final {
public:
    struct Slot final {
        union {
            YmInt i = 0;
            YmUInt ui;
            YmFloat f;
            YmBool b;
            YmRune r;
            YmObj* ref;
        };
    };


    // refs is not managed internally by this class.
    _ym::RefCounter<YmRefCount> refs;

    // TODO: I don't 100% like having YmObj carry a 'ctx' field, and I kinda wanna make ymObj_***
    //       frontend fns instead be passed a YmCtx* explicitly instead.
    //
    //       However, in order to do this we'll need to do a major revision to our ym::*** frontend,
    //       in particular ym::Handle, ym::RefCountedRes, and ym::Scoped would all need to be revised.
    //          * Template specializations?

    ym::Safe<YmCtx> ctx;
    ym::Safe<YmType> type;


    YmObj(YmCtx& ctx, YmType& type);


    // Returns the number of slots this object has.
    constexpr size_t size() const noexcept {
        return isNone() ? 0 : 1;
    }

    Slot& slot(size_t index) noexcept;
    const Slot& slot(size_t index) const noexcept;

    bool isNone() const noexcept;
    bool isInt() const noexcept;
    bool isUInt() const noexcept;
    bool isFloat() const noexcept;
    bool isBool() const noexcept;
    bool isRune() const noexcept;

    std::optional<YmInt> toInt() const noexcept;
    std::optional<YmUInt> toUInt() const noexcept;
    std::optional<YmFloat> toFloat() const noexcept;
    std::optional<YmBool> toBool() const noexcept;
    std::optional<YmRune> toRune() const noexcept;
};

namespace _ym {
    using ObjHAL = HAL<YmObj, YmObj::Slot>;
}

