

#include "StkState.h"

#include "../yama++/Safe.h"
#include "YmObj.h"


_ym::StkState::StkState(YmCtx* ctx) :
    _ctx(ym::Safe(ctx)) {
    _beginUserPseudoCall();
}

YmCallStackHeight _ym::StkState::callStkHeight() const noexcept {
    return (YmCallStackHeight)_callStk.size();
}

std::string _ym::StkState::fmtCallStk(YmCallStackHeight skip) const {
    std::string result{};
    result += std::format("Yama Stack Trace ({} frames)", callStkHeight() - std::min(skip, callStkHeight()));
    YmCallStackHeight number = callStkHeight() - 1;
    const auto begin = std::next(_callStk.crbegin(), std::min(skip, callStkHeight()));
    const auto end = _callStk.crend();
    for (auto it = begin; it != end; std::advance(it, 1)) {
        const auto& frame = *it;
        if (number >= 10) {
            result +=
                frame.fn
                ? std::format("\n    [{}] {}", number, frame.fn->fullname())
                : std::format("\n    [{}] <user>", number);
        }
        else {
            result +=
                frame.fn
                ? std::format("\n    [0{}] {}", number, frame.fn->fullname())
                : std::format("\n    [0{}] <user>", number);
        }
        // Check for symbol info, and if so, add it to result.
        if (frame.fn) {
            // TODO: Add when we add bcode.
        }
        number--;
    }
    return result;
}

bool _ym::StkState::isUser() const noexcept {
    return callStkHeight() == 1;
}

YmUInt32 _ym::StkState::globals() const noexcept {
    ymAssert(_globalObjStk.size() <= size_t(YmUInt32(-1)));
    return YmUInt32(_globalObjStk.size());
}

YmUInt16 _ym::StkState::args() const noexcept {
    ymAssert(!_callStk.empty());
    return _callStk.back().args();
}

YmLocals _ym::StkState::locals() const noexcept {
    ymAssert(!_callStk.empty());
    return YmLocals(globals()) - _callStk.back().localsOffset;
}

_ym::TempRef _ym::StkState::global(YmUInt32 index) {
    return
        index < globals()
        ? _globalObjStk[index].borrow()
        : nullptr;
}

_ym::TempRef _ym::StkState::global(std::optional<YmUInt32> index) {
    return global(index.value_or(YmUInt32(-1)));
}

_ym::TempRef _ym::StkState::stealGlobal(YmUInt32 index, bool frontendRef) {
    return
        index < globals()
        ? _globalObjStk[index].steal(frontendRef)
        : nullptr;
}

_ym::TempRef _ym::StkState::stealGlobal(std::optional<YmUInt32> index, bool frontendRef) {
    return stealGlobal(index.value_or(YmUInt32(-1)), frontendRef);
}

void _ym::StkState::setGlobal(YmUInt32 index, TempRef v) {
    ymAssert(index < globals());
    _globalObjStk[index] = std::move(v);
}

_ym::TempRef _ym::StkState::arg(YmUInt16 which) {
    if (which >= args()) {
        return nullptr;
    }
    // If current call is one forwarded from protocol method call, then that means that
    // the first arg is the call object, which'll be a boxed value. In this circumstance,
    // we specially need to return the unboxed call object, as that's the object the
    // forwarded-to method call actually expects to be its call object.
    auto where = _cf().argOffset(which).value();
    bool isCallObjOfProtoMethod = which == 0 && _cf().fwdFromProto;
    return
        isCallObjOfProtoMethod
        ? global(where)->boxed()
        : global(where);
}

bool _ym::StkState::setArg(YmUInt16 which, _ym::TempRef newArg) {
    if (!newArg) {
        return false;
    }
    if (isUser()) {
        return false;
    }
    if (which >= args()) {
        _ym::Global::raiseErr(
            YmErrCode_ArgNotFound,
            "Set arg failed; arg index {} out-of-bounds!",
            which);
        return false;
    }
    setGlobal(_cf().argOffset(which).value(), std::move(newArg));
    return true;
}

YmType* _ym::StkState::ref(YmRef reference) {
    return
        _cf().fn
        ? _cf().fn->ref(reference)
        : nullptr;
}

_ym::TempRef _ym::StkState::local(YmLocal where) {
    return global(_cf().localOffset(_absIndexForRead(where)));
}

_ym::TempRef _ym::StkState::stealLocal(YmLocal where, bool frontendRef) {
    return stealGlobal(_cf().localOffset(_absIndexForRead(where)), frontendRef);
}

bool _ym::StkState::checkLocalIsInBounds(YmLocal where, std::string_view msgPrefix) const {
    if (!_absIndex(where)) {
        _ym::Global::raiseErr(
            YmErrCode_LocalNotFound,
            "{}; local object index {} out-of-bounds!",
            (std::string)msgPrefix,
            where);
        return false;
    }
    return true;
}

void _ym::StkState::reset() {
    while (callStkHeight() >= 1) {
        _endCall();
    }
    _beginUserPseudoCall();
}

_ym::TempRef _ym::StkState::pull(bool frontendRef) noexcept {
    if (locals() == 0) {
        return nullptr;
    }
    auto result = _globalObjStk.back().steal(frontendRef);
    _globalObjStk.pop_back();
    return result;
}

void _ym::StkState::pop(YmLocals n) {
    if (n < 0) {
        return;
    }
    if (n > locals()) {
        n = locals();
    }
    // Can't use resize here as C++ doesn't know at compile-time if
    // it'll grow/shrink vector, and growing can't happen due to ym::Safe
    // not having a null value.
    _globalObjStk.erase(std::prev(_globalObjStk.end(), n), _globalObjStk.end());
}

bool _ym::StkState::put(YmLocal where, _ym::TempRef what) {
    if (!what) {
        return false;
    }
    if (where == YM_DISCARD) {
        what.dropAsOk();
        return true;
    }
    if (where == YM_PUSH) {
        _globalObjStk.push_back(std::move(what));
        return true;
    }
    if (auto whereAbs = _absIndex(where)) {
        setGlobal(_cf().localOffset(*whereAbs), std::move(what));
        return true;
    }
    else {
        _ym::Global::raiseErr(
            YmErrCode_LocalNotFound,
            "Put failed; local object index {} out-of-bounds!",
            where);
        return false;
    }
}

bool _ym::StkState::swap(YmLocal a, YmLocal b) {
    auto aLocal = _absIndexForRead(a);
    auto bLocal = _absIndexForRead(b);
    if (aLocal && bLocal) {
        std::swap(
            _globalObjStk[_cf().localOffset(*aLocal)],
            _globalObjStk[_cf().localOffset(*bLocal)]);
        return true;
    }
    else {
        if (!aLocal) {
            _ym::Global::raiseErr(
                YmErrCode_LocalNotFound,
                "Swap failed; local object index {} out-of-bounds!",
                a);
        }
        if (!bLocal) {
            _ym::Global::raiseErr(
                YmErrCode_LocalNotFound,
                "Swap failed; local object index {} out-of-bounds!",
                b);
        }
        return false;
    }
}

bool _ym::StkState::call(YmType* fn, YmUInt16 argsN, std::string_view argNames, YmLocal returnTo) {
    if (_beginCall(fn, argsN, argNames, returnTo)) {
        _dispatchCall(fn);
        return _endCall();
    }
    return false;
}

bool _ym::StkState::retObj(_ym::TempRef what) {
    if (!what) {
        return false;
    }
    if (isUser()) {
        return false;
    }
    ymAssert(callStkHeight() >= 1);
    _cf().returnValue = std::move(what);
    return true;
}

_ym::StkState::_CallFrame& _ym::StkState::_cf() noexcept {
    return _callStk.back();
}

const _ym::StkState::_CallFrame& _ym::StkState::_cf() const noexcept {
    return _callStk.back();
}

void _ym::StkState::_beginUserPseudoCall() {
    ymAssert(_callStk.empty());
    _callStk.push_back(_CallFrame{
        .fn = nullptr,
        .returnTo = YmLocal{},
        .localsOffset = 0,
        });
}

bool _ym::StkState::_beginCall(YmType* fn, YmUInt16 args, std::string_view argNames, YmLocal returnTo) {
    if (!fn) {
        return false;
    }
    auto& _fn = ym::deref(fn);
    ymAssert(!_callStk.empty());
    if (!_fn.isCallable()) {
        _ym::Global::raiseErr(
            YmErrCode_NonCallableType,
            "Call to {} failed; {} is non-callable!",
            _fn.fullname(),
            _fn.fullname());
        return false;
    }
    if (!_absIndex(returnTo)) {
        _ym::Global::raiseErr(
            YmErrCode_LocalNotFound,
            "Call to {} failed; local object index {} out-of-bounds!",
            _fn.fullname(),
            returnTo);
        return false;
    }
    if (YmLocals(args) > locals()) {
        _ym::Global::raiseErr(
            YmErrCode_LocalNotFound,
            "Call to {} failed; {} args provided, but local object stack height is {}!",
            _fn.fullname(),
            args,
            locals());
        return false;
    }
    if (callStkHeight() == YM_MAX_CALL_STACK_HEIGHT) {
        _ym::Global::raiseErr(
            YmErrCode_CallStackOverflow,
            "Call to {} failed; call stack overflow!",
            _fn.fullname());
        return false;
    }
    _ym::ArgPackInfo argPack(_fn);
    for (const auto& it : argNames | std::views::split(',')) {
        std::string_view argName(it.begin(), it.end());
        // TODO: Optimize out this std::string heap alloc.
        if (auto tparam = _fn.param((std::string)argName)) {
            if (tparam->isPositional()) {
                _ym::Global::raiseErr(
                    YmErrCode_IllegalNameList,
                    "Call to {} failed; {} is a positional param, not a named one!",
                    _fn.fullname(),
                    tparam->name());
                return false;
            }
            if (!argPack.specifyNextNamedArg(tparam->index())) {
                _ym::Global::raiseErr(
                    YmErrCode_IllegalNameList,
                    "Call to {} failed; named param {} specified multiple times!",
                    _fn.fullname(),
                    tparam->name());
                return false;
            }
        }
        else {
            _ym::Global::raiseErr(
                YmErrCode_IllegalNameList,
                "Call to {} failed; unknown named param \"{}\"!",
                _fn.fullname(),
                (std::string)argName);
            return false;
        }
    }
    argPack.done(); // Don't forget!
    if (args != argPack.specifiedArgs()) {
        _ym::Global::raiseErr(
            YmErrCode_CallProcedureError,
            "Call to {} failed; {} args provided, but expected {}! ({} positional + {} named)",
            _fn.fullname(),
            args,
            argPack.specifiedArgs(),
            argPack.positionalArgs(),
            argPack.namedArgs());
        return false;
    }
    for (YmParamIndex param = 0; param < argPack.paramCount(); param++) {
        // Quietly skip unspecified named args.
        if (auto argOffset = argPack.argOffset(param, true)) {
            auto ref = local(locals() - args + *argOffset);
            if (auto p = _fn.param(param); !p->type().sameAs(ref->type)) {
                _ym::Global::raiseErr(
                    YmErrCode_TypeMismatch,
                    "Call to {} failed; arg #{} (for {} param {}) is {}, but expected {}!",
                    _fn.fullname(),
                    *argOffset + 1,
                    p->isPositional() ? "positional" : "named",
                    p->name(),
                    ref->type->fullname(),
                    p->type().fullname());
                return false;
            }
        }
    }
    // Append arg pack w/ dummy objects, then push call frame, and return.
    for (YmParams i = 0; i < argPack.dummies(); i++) {
        put(YM_PUSH, _ctx->newNone(false));
    }
    _callStk.push_back(_CallFrame{
        .fn = &_fn,
        .argPack = std::move(argPack),
        .returnTo = returnTo,
        .localsOffset = globals(),
        });
    return true;
}

bool _ym::StkState::_endCall() noexcept {
    ymAssert(!_callStk.empty());
    _CallFrame cf = std::move(_callStk.back());
    pop(locals());
    _callStk.pop_back();
    if (!cf.fn) {
        // This is user pseudo-call.
        return true;
    }
    else if (!cf.returnValue) {
        _ym::Global::raiseErr(
            YmErrCode_CallProcedureError,
            "Call to {} failed; didn't bind a return value!",
            cf.fn->fullname());
        pop(cf.dummies());
        return false;
    }
    else if (cf.returnValue->type != cf.fn->returnType()) {
        _ym::Global::raiseErr(
            YmErrCode_CallProcedureError,
            "Call to {} failed; returned {}, but expected {}!",
            cf.fn->fullname(),
            cf.returnValue->type->fullname(),
            cf.fn->returnType()->fullname());
        pop(cf.dummies());
        return false;
    }
    else {
        pop(cf.args());
        return put(cf.returnTo, cf.returnValue.steal(false));
    }
}

void _ym::StkState::_dispatchCall(YmType* fn) {
    if (!fn) {
        return;
    }
    auto& _fn = ym::deref(fn);
    if (_fn.isMethodReq()) { // Protocol Method Dispatch
        // NOTE: Prior to changing fwdFromProto, arg(0) shouldn't see through boxing.
        auto callobj = arg(0);
        auto ptableInd = (uintptr_t)ym::deref(_fn.info->callBehaviour()).user;
        auto forwardedTo = callobj->ptable()[ptableInd];
        _cf().fn = forwardedTo;
        _cf().fwdFromProto = true;
        // If indirectly called method has named params, we gotta add proper number
        // of dummies to _cf().argPack, and we gotta push dummy objects for each.
        if (auto named = forwardedTo->namedParams(); named >= 1) {
            for (YmParams i = 0; i < named; i++) {
                put(YM_PUSH, _ctx->newNone(false));
            }
            _cf().argPack.addDummies(named);
            _cf().localsOffset += named;
        }
    }
    auto& callBhvrInfo = ym::deref(_cf().fn->info->callBehaviour());
    callBhvrInfo.fn(_ctx, fn, callBhvrInfo.user);
}

std::optional<YmLocal> _ym::StkState::_absIndex(YmLocal x) const noexcept {
    if (x == YM_PUSH || x == YM_DISCARD) {
        return x;
    }
    if (x >= locals()) {
        return std::nullopt;
    }
    // The 'x < 0' part is to avoid the potential overflow related edge cases.
    if (x < 0 && locals() + x < 0) {
        return std::nullopt;
    }
    return
        x >= 0
        ? x
        : locals() + x;
}

std::optional<YmLocal> _ym::StkState::_absIndexForRead(YmLocal x) const noexcept {
    if (x == YM_PUSH || x == YM_DISCARD) {
        return std::nullopt;
    }
    return _absIndex(x);
}

