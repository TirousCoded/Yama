

#include "Resource.h"

#include "../yama++/print.h"
#include "general.h"
#include "resources.h"


_ym::Resource::Resource(YmRType rtype) :
    _rtype(rtype),
    _refcount(0) {
    _initAtomicRefsIfARC();
}

YmRType _ym::Resource::rtype() const noexcept {
    return _rtype;
}

YmRefCount _ym::Resource::refcount() const noexcept {
    return
        usesARC(rtype())
        // TODO: I'm really not 100% sure about std::memory_order_relaxed here, but since
        //       ymRefCount is not used in the actual incr/decr of ref. count, I think that
        //       a looser memory order should be okay (and a bit faster!)
        ? _refcountA.load(std::memory_order_relaxed)
        : _refcount;
}

void _ym::Resource::addRef() noexcept {
    _incrRefs();
}

void _ym::Resource::drop() noexcept {
    if (_decrRefs()) {
        Resource::destroy(ym::Safe(this));
    }
}

void _ym::Resource::destroy(ym::Safe<Resource> x) noexcept {
#if 0
    ym::println("-- Resource::destroy({})", ymFmtYmRType(x->rtype()));
#endif
    static_assert(YmRType_Num == 4);
    switch (x->rtype()) {
    case YmRType_Dm:
    {
        _ym::destroy(x.downcastInto<YmDm>());
    }
    break;
    case YmRType_Ctx:
    {
        _ym::destroy(x.downcastInto<YmCtx>());
    }
    break;
    case YmRType_ParcelDef:
    {
        _ym::destroy(x.downcastInto<YmParcelDef>());
    }
    break;
    case YmRType_Parcel:
    {
        _ym::destroy(x.downcastInto<YmParcel>());
    }
    break;
    default: YM_DEADEND; break;
    }
}

void _ym::Resource::_initAtomicRefsIfARC() noexcept {
    if (usesARC(rtype())) {
        // NOTE: By assigning the atomic field of our union, we make it the
        //       'active member', and thus safe to access (ie. w/out UB.)
        _refcountA = 0;
    }
}

void _ym::Resource::_incrRefs() noexcept {
    if (usesARC(rtype())) {
        const auto old = _refcountA.fetch_add(1, std::memory_order_relaxed);
        ymAssert(old < std::numeric_limits<decltype(_refcount)>::max());
    }
    else {
        ymAssert(_refcount < std::numeric_limits<decltype(_refcount)>::max());
        _refcount++;
    }
}

bool _ym::Resource::_decrRefs() noexcept {
    if (usesARC(rtype())) {
        const auto old = _refcountA.fetch_sub(1, std::memory_order_acq_rel);
        ymAssert(old >= 1);
        return old == 1;
    }
    else {
        ymAssert(_refcount >= 1);
        _refcount--;
        return _refcount == 0;
    }
}

