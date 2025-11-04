

#include "Resource.h"

#include "../yama++/print.h"
#include "general.h"
#include "resources.h"


_ym::Resource::Resource(RType rtype) :
    rtype(rtype),
    rmode(rmodeOf(rtype)),
    _refcount(0) {
    _initAtomicRefsIfARC();
}

_ym::Resource::~Resource() noexcept {
#if 0
    ym::println("[Yama] Cleanup: {} @ {}...", fmtRType(rtype), (void*)this);
#endif
}

bool _ym::Resource::usesARC() const noexcept {
    return rmode == RMode::ARC;
}

YmRefCount _ym::Resource::refcount() const noexcept {
    return
        // TODO: I'm really not 100% sure about std::memory_order_relaxed here, but since
        //       ymRefCount is not used in the actual incr/decr of ref. count, I think that
        //       a looser memory order should be okay (and a bit faster!)
        usesARC()
        ? _refcountA.load(std::memory_order_relaxed)
        : _refcount;
}

void _ym::Resource::addRef() const noexcept {
    _incrRefs();
}

void _ym::Resource::drop() const noexcept {
    if (_decrRefs()) {
        Resource::destroy(ym::Safe(this));
    }
}

void _ym::Resource::destroy(ym::Safe<const Resource> x) noexcept {
    _destroyHelper(x, std::make_index_sequence<enumSize<RType>>{});
}

void _ym::Resource::_initAtomicRefsIfARC() noexcept {
    if (usesARC()) {
        // NOTE: By assigning the atomic field of our union, we make it the
        //       'active member', and thus safe to access (ie. w/out UB.)
        _refcountA = 0;
    }
}

void _ym::Resource::_incrRefs() const noexcept {
#if 0
    ym::println("[Yama] _incrRefs: {} ({}) @ {} ({} refs)", fmtRType(rtype), fmtRMode(rmode), (void*)this, refcount());
#endif
    if (usesARC()) {
        const auto old = _refcountA.fetch_add(1, std::memory_order_relaxed);
        ymAssert(old < std::numeric_limits<decltype(_refcount)>::max());
    }
    else {
        ymAssert(_refcount < std::numeric_limits<decltype(_refcount)>::max());
        _refcount++;
    }
}

bool _ym::Resource::_decrRefs() const noexcept {
#if 0
    ym::println("[Yama] _decrRefs: {} ({}) @ {} ({} refs)", fmtRType(rtype), fmtRMode(rmode), (void*)this, refcount());
#endif
    if (usesARC()) {
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

