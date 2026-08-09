

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
#include "obj-ref-helpers.h"
#include "FRootTracker.h"


struct YmObj final {
public:
    struct Slot final {
        // NOTE: 'ref' dtor won't automatically drop (how could it, the union wouldn't
        //       know to call the dtor anyway), and so ref slot dropping (aka. deinit)
        //       must be done MANUALLY!
        //          * See YmObj::dropAllRefSlots.
        union {
            YmInt i = 0;
            YmUInt ui;
            YmFloat f;
            YmBool b;
            YmRune r;
            YmType* type;
            _ym::LiteInternalRef ref;
            const ym::Safe<YmType>* ptable;
        };
    };


    // refs is not managed internally by this class.
    _ym::DualRefCounter refs;

    ym::Safe<YmCtx> ctx;
    ym::Safe<YmType> type;
    _ym::FRootID froot = _ym::NO_FROOT;
    _ym::GCCycleID lastSurvivedCycle = _ym::GCNoCycle;


    YmObj(YmCtx& ctx, YmType& type);


    // NOTE: See kinds.h for info about static slot layouts.

    _ym::Slots slots() const noexcept;
    inline _ym::Slots size() const noexcept { return slots(); }
    Slot& slot(_ym::Slots index) noexcept;
    const Slot& slot(_ym::Slots index) const noexcept;

    // NOTE: Due to dangers involving move-assigning LiteInternalRef, we'll restrict
    //       end-user in terms of how they assign/drop ref slots, for safety.

    const _ym::LiteInternalRef& refSlot(_ym::Slots index) const noexcept;
    void dropRefSlot(_ym::Slots index) noexcept;
    void dropAllRefSlots() noexcept;
    void assignRefSlot(_ym::Slots index, _ym::TempRef value) noexcept;
    _ym::TempRef stealRefSlot(_ym::Slots index, bool frontendRef) noexcept;

    inline void forEachRefSlotIndex(ym::Callable<void, _ym::Slots> auto&& visitor) const {
        type->forEachRefSlotIndex(std::forward<decltype(visitor)>(visitor));
    }

    std::optional<YmInt> toInt() const noexcept;
    std::optional<YmUInt> toUInt() const noexcept;
    std::optional<YmFloat> toFloat() const noexcept;
    std::optional<YmBool> toBool() const noexcept;
    std::optional<YmRune> toRune() const noexcept;
    YmType* toType() const noexcept;


    // TODO: What does 'ptable != nullptr' below mean? I don't remember, lol.

    // Sets up a boxed value.
    // Notice that ptable != nullptr is a valid ptable ptr in this context (ie. for yama:Any.)
    void box(_ym::TempRef value, const ym::Safe<YmType>* ptable) noexcept;

    // Returns borrowed ref to the boxed value.
    _ym::TempRef boxed() const noexcept;

    // Queries the ptable if this is a protocol value.
    // Notice that nullptr is a valid ptable ptr in this context (ie. for yama:Any.)
    const ym::Safe<YmType>* ptable() const noexcept;
};

namespace _ym {
    using ObjHAL = HAL<YmObj, YmObj::Slot>;
}

