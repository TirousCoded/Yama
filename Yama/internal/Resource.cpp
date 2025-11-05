

#include "Resource.h"

#include "../yama++/print.h"
#include "general.h"
#include "resources.h"


_ym::Resource::Resource(RMode rmode) :
    rmode(rmode),
    _refcount(0) {
    _initAtomicRefsIfARC();
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
        // NOTE: I tried putting this ym::println in dtor, but got some issues w/ virtual debugName call.
        //          See https://stackoverflow.com/questions/12092933/calling-virtual-function-from-destructor.
        //          See https://www.artima.com/articles/never-call-virtual-functions-during-construction-or-destruction.
#if 0
        ym::println("[Yama] Cleanup: {} @ {}...", debugName(), (void*)this);
#endif
        destroy(); // Deinits *this, so be careful here.
    }
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
    ym::println("[Yama] _incrRefs: {} ({}) @ {} ({} refs)", debugName(), fmtRMode(rmode), (void*)this, refcount());
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
    ym::println("[Yama] _decrRefs: {} ({}) @ {} ({} refs)", debugName(), fmtRMode(rmode), (void*)this, refcount());
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

