

#include "GC.h"

#include "ObjManager.h"
#include "YmObj.h"


#define _TRACE_GC false

#if _TRACE_GC
#include "../yama++/print.h"
#endif


_ym::GC::GC(ObjManager& om, GCEvent events) noexcept :
    _om(&om),
    _events(events) {
}

_ym::ObjManager& _ym::GC::om() const noexcept {
    return *_om;
}

void _ym::GC::reset() noexcept {
    onReset();
}

void _ym::GC::ack(YmObj& obj) {
    if (_has(GCEvent::Ack)) {
        onAck(obj);
    }
}

void _ym::GC::collect() {
    if (_has(GCEvent::Collect)) {
        onCollect();
    }
}

void _ym::GC::onReset() noexcept {
}

void _ym::GC::onAck(YmObj& obj) {
}

void _ym::GC::onCollect() {
}

bool _ym::GC::_has(GCEvent e) const noexcept {
    return checkGCEvent(_events, e);
}

_ym::StopTheWorldGC::StopTheWorldGC(ObjManager& om) noexcept :
    GC(om, GCEvent::Ack | GCEvent::Collect) {
}

void _ym::StopTheWorldGC::onReset() noexcept {
    _gcReset();
}

void _ym::StopTheWorldGC::onAck(YmObj& obj) {
    _gcAcknowledgeNewObj(obj);
}

void _ym::StopTheWorldGC::onCollect() {
    _gcCollect(nullptr);
}

void _ym::StopTheWorldGC::_gcReset() {
    _gcThreshold = _gcThresholdInitial;
}

void _ym::StopTheWorldGC::_gcAcknowledgeNewObj(YmObj& obj) {
    if (om().count() == _gcThreshold) {
        _gcCollect(&obj);
    }
}

void _ym::StopTheWorldGC::_gcCollect(YmObj* triggerObj) {
    _gcBeginCycle();
    _gcMarkPhase(triggerObj);
    _gcSweepPhase();
    _gcEndCycle();
}

bool _ym::StopTheWorldGC::_gcIsReachable(YmObj& obj) {
    // NOTE: Using '==' instead of '<' on the off chance cycle ID overflow needs to
    //       be accounted for.
    //          * This could let us make the ID 8-bit.
    return obj.lastSurvivedCycle == _gcCurrentCycle;
}

void _ym::StopTheWorldGC::_gcBeginCycle() {
    _gcCurrentCycle++;
#if _TRACE_GC
    ym::println("-- GC Collection Cycle (ID={})", _gcCurrentCycle);
#endif
}

void _ym::StopTheWorldGC::_gcMarkPhase(YmObj* triggerObj) {
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
    om().forEachRoot([this](YmObj& root) {
        _gcMark(root);
        });
}

void _ym::StopTheWorldGC::_gcMark(YmObj& obj) {
    if (_gcIsReachable(obj)) {
        return;
    }
#if _TRACE_GC
    ym::println("-- GC Mark: {} @ {}", obj.type->fullname(), (void*)&obj);
#endif
    _gcMarkObjCycleID(obj);
    _gcMarkOutgoingRefs(obj);
}

void _ym::StopTheWorldGC::_gcMarkObjCycleID(YmObj& obj) noexcept {
    obj.lastSurvivedCycle = _gcCurrentCycle;
}

void _ym::StopTheWorldGC::_gcMarkOutgoingRefs(YmObj& obj) {
    ymAssert(_gcIsReachable(obj));
    obj.forEachRefSlotIndex([this, &obj](_ym::Slots index) {
        _gcMark(*obj.refSlot(index));
        });
}

void _ym::StopTheWorldGC::_gcSweepPhase() {
#if _TRACE_GC
    ym::println("-- GC Sweeping");
#endif
    // TODO: This copy and us looping over ALL YmObj* is suboptimal.
    // Gotta copy objects(), as we're gonna be constantly modifying it
    // while we loop, which means our iters would become invalidated.
    auto objs = om().objects();
    for (auto& obj : objs) {
        if (_gcIsReachable(*obj)) continue;
#if _TRACE_GC
        ym::println("-- GC Sweep: {} @ {}", obj->type->fullname(), (void*)&obj);
#endif
        om().reportDestroy(*obj);
    }
    // Perform a second loop for the deallocs, as reportDestroy breaks easily
    // if we try to merge the loops together.
    for (auto& obj : objs) {
        if (_gcIsReachable(*obj)) continue;
        _gcDropOutgoingRefsToReachableObjs(*obj);
        om().deallocObj(*obj);
    }
}

void _ym::StopTheWorldGC::_gcDropOutgoingRefsToReachableObjs(YmObj& obj) {
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

void _ym::StopTheWorldGC::_gcEndCycle() {
    _gcUpdateThreshold();
}

void _ym::StopTheWorldGC::_gcUpdateThreshold() noexcept {
    _gcThreshold = size_t(double(std::max(om().count(), _gcThresholdInitial)) * _gcThresholdGrowthFactor);
}

