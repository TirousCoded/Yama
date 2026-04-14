

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
            YmType* type;
            YmObj* ref;
            const ym::Safe<YmType>* ptable;
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


    // Governs object cleanup behaviour.
    void cleanup() noexcept;


    // Returns the number of slots this object has.
    inline size_t size() const noexcept {
        if (type->kind() == YmKind_Protocol) {
            return 2; // Slot #1 is boxed value, slot #2 is ptable ptr.
        }
        // TODO: Doesn't this part seem a bit costly? So many checks...
        else if (isInt() || isUInt() || isFloat() || isBool() || isRune() || isType()) {
            return 1;
        }
        else return 0;
    }

    Slot& slot(size_t index) noexcept;
    const Slot& slot(size_t index) const noexcept;

    bool isProtocol() const noexcept;

    // NOTE: Certain internal code expects is*** and to*** methods to operate
    //       such that they require the object's type to be SPECIFICALLY the
    //       primitive type in question.
    //
    //       To this end, when we add in things like frontend being able to
    //       marshal an object into a primitive as part of querying its value
    //       as a specific primitive type, the below methods should NOT be
    //       where we impl this.

    bool isNone() const noexcept;
    bool isInt() const noexcept;
    bool isUInt() const noexcept;
    bool isFloat() const noexcept;
    bool isBool() const noexcept;
    bool isRune() const noexcept;
    bool isType() const noexcept;

    std::optional<YmInt> toInt() const noexcept;
    std::optional<YmUInt> toUInt() const noexcept;
    std::optional<YmFloat> toFloat() const noexcept;
    std::optional<YmBool> toBool() const noexcept;
    std::optional<YmRune> toRune() const noexcept;
    YmType* toType() const noexcept;


    // Sets up a boxed value.
    // Does not do anything to ref count of value.
    // A ref count incr of value is stolen by the protocol value.
    // Notice that ptable != nullptr is a valid ptable ptr in this context (ie. for yama:Any.)
    void box(ym::Safe<YmObj> value, const ym::Safe<YmType>* ptable) noexcept;

    // Queries the boxed value if this is a protocol value.
    // Does not do anything to ref count of boxed value.
    YmObj* boxed() const noexcept;

    // Queries the ptable if this is a protocol value.
    // Notice that nullptr is a valid ptable ptr in this context (ie. for yama:Any.)
    const ym::Safe<YmType>* ptable() const noexcept;
};

namespace _ym {
    using ObjHAL = HAL<YmObj, YmObj::Slot>;
}

