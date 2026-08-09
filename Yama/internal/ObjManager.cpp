

#include "ObjManager.h"

#include "YmObj.h"


#define _TRACE_RC 0
#define _TRACE_GC 0
#define _DEBUGBREAK_AT_DESTROYS 0

#if _TRACE_RC || _TRACE_GC
#include "../yama++/print.h"
#endif


_ym::ObjManager::ObjManager(YmCtx* ctx, GetGlobalObj getGlobalObj) :
    _ctx(ym::Safe(ctx)),
    _getGlobalObj(std::move(getGlobalObj)) {
}

size_t _ym::ObjManager::count() const noexcept {
    return _allocatedObjs.size();
}

bool _ym::ObjManager::exists(YmObj& obj) const noexcept {
    return _allocatedObjs.contains(&obj);
}

void _ym::ObjManager::setObjDestroyCallback(YmObjDestroyCallbackFn fn, void* user) noexcept {
    _objDestroyCallback = fn;
    _objDestroyCallbackUser = user;
}

void _ym::ObjManager::reset() {
    _destroyAll();
    _froots.untrackAll();
    _gcReset();
}

_ym::TempRef _ym::ObjManager::create(YmType& type, bool frontend) {
    auto al = _mas.allocator<int>();
    auto obj = ym::Safe(_ym::ObjHAL::create(YmObj(*_ctx, type), al));
#if _TRACE_RC
    ym::println("-- Create {}: {}", type.fullname(), (void*)&*obj);
#endif
    _allocatedObjs.insert(obj);
    auto result = _ym::TempRef::take(obj, frontend, true);
    // All ref slots should be nullptr, such that the obj is
    // properly 'memory initialized'.
    result->forEachRefSlotIndex([&result](_ym::Slots index) {
        ymAssert(!(bool)result->refSlot(index));
        });
    _gcAcknowledgeNewObj(*result);
    return result;
}

YmRefCount _ym::ObjManager::secure(YmObj& obj, bool frontend) {
#if _TRACE_RC
    ym::println("-- Secure {} ({}): {} -> {}",
        (void*)&obj,
        frontend ? "frontend" : "backend",
        obj.refs.total(),
        obj.refs.total() + 1);
#endif
    if (frontend && obj.refs.count(true) == 0) {
        _froots.track(obj);
    }
    return obj.refs.secure(frontend);
}

YmRefCount _ym::ObjManager::release(YmObj& obj, bool frontend) {
#if _TRACE_RC
    ym::println("-- Release {} ({}): {} -> {}",
        (void*)&obj,
        frontend ? "frontend" : "backend",
        obj.refs.total(),
        obj.refs.total() - 1);
#endif
    if (frontend && obj.refs.count(true) == 1) {
        _froots.untrack(obj);
    }
    auto old = obj.refs.release(frontend);
    if (old == 1) {
        _destroy(obj);
    }
    return old;
}

_ym::TempRef _ym::ObjManager::newNone(bool frontendRef) {
    return create(_ctx->ldNone(), frontendRef);
}

_ym::TempRef _ym::ObjManager::newInt(YmInt v, bool frontendRef) {
    auto result = create(_ctx->ldInt(), frontendRef);
    result->slot(0).i = v;
    return result;
}

_ym::TempRef _ym::ObjManager::newUInt(YmUInt v, bool frontendRef) {
    auto result = create(_ctx->ldUInt(), frontendRef);
    result->slot(0).ui = v;
    return result;
}

_ym::TempRef _ym::ObjManager::newFloat(YmFloat v, bool frontendRef) {
    auto result = create(_ctx->ldFloat(), frontendRef);
    result->slot(0).f = v;
    return result;
}

_ym::TempRef _ym::ObjManager::newBool(YmBool v, bool frontendRef) {
    auto result = create(_ctx->ldBool(), frontendRef);
    result->slot(0).b = v;
    return result;
}

_ym::TempRef _ym::ObjManager::newRune(YmRune v, bool frontendRef) {
    auto result = create(_ctx->ldRune(), frontendRef);
    result->slot(0).r = uint2rune((YmUInt)v);
    return result;
}

_ym::TempRef _ym::ObjManager::newType(YmType& v, bool frontendRef) {
    auto result = create(_ctx->ldType(), frontendRef);
    result->slot(0).type = &v;
    return result;
}

_ym::TempRef _ym::ObjManager::newDefault(YmType* type, bool frontendRef) {
    if (!type) {
        return nullptr;
    }
    auto& _type = ym::deref(type);
    static_assert(YmKind_Num == 8);
    if (_type.sameAs(_ctx->ldNone()))       return newNone(frontendRef);
    else if (_type.sameAs(_ctx->ldInt()))   return newInt(0, frontendRef);
    else if (_type.sameAs(_ctx->ldUInt()))  return newUInt(0, frontendRef);
    else if (_type.sameAs(_ctx->ldFloat())) return newFloat(0.0, frontendRef);
    else if (_type.sameAs(_ctx->ldBool()))  return newBool(YM_FALSE, frontendRef);
    else if (_type.sameAs(_ctx->ldRune()))  return newRune(U'\0', frontendRef);
    else if (_type.sameAs(_ctx->ldType()))  return newType(_ctx->ldNone(), frontendRef);
    else if (_type.isStruct() && _type.hasDefaultValue()) {
        // TODO: Add ctor calls + handle panics.
        return create(_type, frontendRef);
    }
    else {
        ymAssert(!_type.hasDefaultValue());
        _ym::Global::raiseErr(
            YmErrCode_NoDefaultValue,
            "{} has no default value!",
            _type.fullname());
        return nullptr;
    }
}

void _ym::ObjManager::gcCollect() {
    _gcCollect(nullptr);
}

void _ym::ObjManager::_destroy(YmObj& obj) {
#if _TRACE_RC
    ym::println("-- Destroy {}", (void*)&obj);
#endif
#if _DEBUGBREAK_AT_DESTROYS
    __debugbreak();
#endif
    _reportDestroy(obj);
    _deinitObj(obj, true);
    _deallocObj(obj);
}

void _ym::ObjManager::_destroyAll() {
#if _TRACE_RC
    ym::println("-- Destroy All");
#endif
    for (auto& obj : _allocatedObjs) {
        _reportDestroy(*obj);
    }
    for (auto& obj : _allocatedObjs) {
        _deinitObj(*obj, false);
    }
    // Gotta iter differently here as we're gonna be constantly
    // modifying _objects.
    while (!_allocatedObjs.empty()) {
        _deallocObj(**_allocatedObjs.begin());
    }
}

void _ym::ObjManager::_reportDestroy(YmObj& obj) {
    if (_objDestroyCallback) {
        _objDestroyCallback(&obj, _objDestroyCallbackUser);
    }
}

void _ym::ObjManager::_deinitObj(YmObj& obj, bool releaseOutgoingRefs) {
    if (releaseOutgoingRefs) {
        obj.dropAllRefSlots(); // Can't forget!
    }
    _froots.untrack(obj);
}

void _ym::ObjManager::_deallocObj(YmObj& obj) noexcept {
    _allocatedObjs.erase(&obj);
    auto al = _mas.allocator<int>();
    _ym::ObjHAL::destroy(obj, al);
}

void _ym::ObjManager::_gcReset() {
    _gcThreshold = _gcThresholdInitial;
}

void _ym::ObjManager::_gcAcknowledgeNewObj(YmObj& obj) {
    if (count() == _gcThreshold) {
        _gcCollect(&obj);
    }
}

void _ym::ObjManager::_gcCollect(YmObj* triggerObj) {
    _gcBeginCycle();
    _gcMarkPhase(triggerObj);
    _gcSweepPhase();
    _gcEndCycle();
}

bool _ym::ObjManager::_gcIsReachable(YmObj& obj) {
    // NOTE: Using '==' instead of '<' on the off chance cycle ID overflow needs to
    //       be accounted for.
    //          * This could let us make the ID 8-bit.
    return obj.lastSurvivedCycle == _gcCurrentCycle;
}

void _ym::ObjManager::_gcBeginCycle() {
    _gcCurrentCycle++;
#if _TRACE_GC
    ym::println("-- GC Collection Cycle (ID={})", _gcCurrentCycle);
#endif
}

void _ym::ObjManager::_gcMarkPhase(YmObj* triggerObj) {
#if _TRACE_GC
    ym::println("-- GC Marking");
#endif
    if (triggerObj) {
        // As it's possible for this to trigger while obj hasn't been given a
        // frontend ref, nor pushed to obj stack, we'll just let obj survive
        // the collection cycle no matter what, so that other parts of our code
        // needn't worry about GC nuances.
        _gcMark(*triggerObj);
    }
    forEachRoot([this](YmObj& root) {
        _gcMark(root);
        });
}

void _ym::ObjManager::_gcMark(YmObj& obj) {
    if (_gcIsReachable(obj)) {
        return;
    }
#if _TRACE_GC
    ym::println("-- GC Mark: {} @ {}", obj.type->fullname(), (void*)&obj);
#endif
    _gcMarkObjCycleID(obj);
    _gcMarkOutgoingRefs(obj);
}

void _ym::ObjManager::_gcMarkObjCycleID(YmObj& obj) noexcept {
    obj.lastSurvivedCycle = _gcCurrentCycle;
}

void _ym::ObjManager::_gcMarkOutgoingRefs(YmObj& obj) {
    ymAssert(_gcIsReachable(obj));
    obj.forEachRefSlotIndex([this, &obj](_ym::Slots index) {
        _gcMark(*obj.refSlot(index));
        });
}

void _ym::ObjManager::_gcSweepPhase() {
#if _TRACE_GC
    ym::println("-- GC Sweeping");
#endif
    // TODO: This copy and us looping over ALL YmObj* is suboptimal.
    // Gotta copy _allocatedObjs, as we're gonna be constantly modifying it
    // while we loop, which means our iters would become invalidated.
    auto objs = _allocatedObjs;
    for (auto& obj : objs) {
        if (_gcIsReachable(*obj)) continue;
#if _TRACE_GC
        ym::println("-- GC Sweep: {} @ {}", obj->type->fullname(), (void*)&obj);
#endif
        _reportDestroy(*obj);
    }
    // Perform a second loop for the deallocs, as _reportDestroy breaks easily
    // if we try to merge the loops together.
    for (auto& obj : objs) {
        if (_gcIsReachable(*obj)) continue;
        _gcDropOutgoingRefsToReachableObjs(*obj);
        _deallocObj(*obj);
    }
}

void _ym::ObjManager::_gcDropOutgoingRefsToReachableObjs(YmObj& obj) {
    // Release refs to reachable objs, but NOT refs to unreachable objs,
    // as we don't want _destroy called on those objs.
    obj.forEachRefSlotIndex([this, &obj](_ym::Slots index) {
        // If an outgoing ref is REACHABLE, that means that there MUST be AT LEAST
        // two refs to it. To this end, decr of said refs CANNOT cause these reachable
        // outgoing ref objs' ref counts to reach 0.
        auto ref = obj.refSlot(index).borrow();
        if (_gcIsReachable(*ref)) {
            ymAssert(ref->refs.total() >= 2);
            obj.dropRefSlot(index);
        }
        });
}

void _ym::ObjManager::_gcEndCycle() {
    _gcUpdateThreshold();
}

void _ym::ObjManager::_gcUpdateThreshold() noexcept {
    _gcThreshold = size_t(double(std::max(count(), _gcThresholdInitial)) * _gcThresholdGrowthFactor);
}

