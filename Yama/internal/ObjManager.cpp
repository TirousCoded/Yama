

#include "ObjManager.h"

#include "YmObj.h"


#define _TRACE_RC false
#define _DEBUGBREAK_AT_DESTROYS false

#if _TRACE_RC
#include "../yama++/print.h"
#endif


_ym::ObjManager::ObjManager(YmCtx* ctx, GetGlobalObj getGlobalObj, std::unique_ptr<GC> gc) :
    _ctx(ym::Safe(ctx)),
    _getGlobalObj(std::move(getGlobalObj)),
    _gc(std::move(gc)) {
    ymAssert((bool)_gc);
}

size_t _ym::ObjManager::count() const noexcept {
    return _allocatedObjs.size();
}

void _ym::ObjManager::setObjDestroyCallback(YmObjDestroyCallbackFn fn, void* user) noexcept {
    _objDestroyCallback = fn;
    _objDestroyCallbackUser = user;
}

void _ym::ObjManager::reset() {
    _destroyAll();
    _froots.untrackAll();
    _gc->reset();
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
    _gc->ack(*result);
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
    _gc->collect();
}

const std::unordered_set<YmObj*>& _ym::ObjManager::objects() const noexcept {
    return _allocatedObjs;
}

void _ym::ObjManager::reportDestroy(YmObj& obj) {
    if (_objDestroyCallback) {
        _objDestroyCallback(&obj, _objDestroyCallbackUser);
    }
}

void _ym::ObjManager::deinitObj(YmObj& obj, bool releaseOutgoingRefs) {
    if (releaseOutgoingRefs) {
        obj.dropAllRefSlots(); // Can't forget!
    }
    _froots.untrack(obj);
}

void _ym::ObjManager::deallocObj(YmObj& obj) noexcept {
    // Calling untrack here in case deinitObj doesn't get called.
    _froots.untrack(obj);
    _allocatedObjs.erase(&obj);
    auto al = _mas.allocator<int>();
    _ym::ObjHAL::destroy(obj, al);
}

void _ym::ObjManager::_destroy(YmObj& obj) {
#if _TRACE_RC
    ym::println("-- Destroy {}", (void*)&obj);
#endif
#if _DEBUGBREAK_AT_DESTROYS
    __debugbreak();
#endif
    reportDestroy(obj);
    deinitObj(obj, true);
    deallocObj(obj);
}

void _ym::ObjManager::_destroyAll() {
#if _TRACE_RC
    ym::println("-- Destroy All");
#endif
    for (auto& obj : _allocatedObjs) {
        reportDestroy(*obj);
    }
    for (auto& obj : _allocatedObjs) {
        deinitObj(*obj, false);
    }
    // Gotta iter differently here as we're gonna be constantly
    // modifying _objects.
    while (!_allocatedObjs.empty()) {
        deallocObj(**_allocatedObjs.begin());
    }
}

