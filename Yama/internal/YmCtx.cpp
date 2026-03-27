

#include "YmCtx.h"

#include "general.h"
#include "SpecSolver.h"
#include "YmObj.h"
#include "YmParcel.h"
#include "YmType.h"


YmCtx::YmCtx(ym::Safe<YmDm> domain) :
    domain(domain),
    loader(std::make_shared<_ym::CtxLoader>(domain->loader)) {
    _beginUserPseudoCall();
}

YmCtx::~YmCtx() noexcept {
    reset(); // Cleanup
    _endCall(); // End user pseudo-call.
}

std::shared_ptr<YmParcel> YmCtx::import(const std::string& path) {
    if (auto s = _ym::Spec::path(path)) {
        return loader->import(*s);
    }
    else {
        _ym::Global::raiseErr(
            YmErrCode_IllegalSpecifier,
            "Import failed; \"{}\" syntax error!",
            path);
        return nullptr;
    }
}

std::shared_ptr<YmType> YmCtx::load(const std::string& fullname) {
    if (auto s = _ym::Spec::type(fullname)) {
        return loader->load(*s);
    }
    else {
        _ym::Global::raiseErr(
            YmErrCode_IllegalSpecifier,
            "Load failed; \"{}\" syntax error!",
            fullname);
        return nullptr;
    }
}

YmObj* YmCtx::create(YmType& type) {
    auto al = mas.allocator<int>();
    ym::Safe result(_ym::ObjHAL::create(YmObj(*this, type), al));
    result->refs.addRef();
    ymAssert(result->refs.count() == 1);
    _objects.insert(result);
    return result;
}

YmRefCount YmCtx::secure(YmObj& obj) {
    return obj.refs.addRef();
}

YmRefCount YmCtx::release(YmObj& obj) {
    auto old = obj.refs.drop();
    if (old == 1) {
        _objects.erase(&obj);
        auto al = mas.allocator<int>();
        _ym::ObjHAL::destroy(obj, al);
    }
    return old;
}

void YmCtx::reset() {
    ymAssert(callStkHeight() == 1);
    // End current user pseudo-call.
    _endCall();
    // Copy _objects and iterate over the copy as otherwise we'd be modifying
    // objects as we iterate over it.
    auto objects = _objects;
    for (auto& object : objects) {
        while (release(*object) > 1) {}
    }
    ymAssert(_objects.empty());
    // Begin new user pseudo-call.
    _beginUserPseudoCall();
}

ym::Safe<YmObj> YmCtx::newNone() {
    return ym::Safe(create(loader->ldNone()));
}

ym::Safe<YmObj> YmCtx::newInt(YmInt v) {
    auto result = ym::Safe(create(loader->ldInt()));
    result->slot(0).i = v;
    return result;
}

ym::Safe<YmObj> YmCtx::newUInt(YmUInt v) {
    auto result = ym::Safe(create(loader->ldUInt()));
    result->slot(0).ui = v;
    return result;
}

ym::Safe<YmObj> YmCtx::newFloat(YmFloat v) {
    auto result = ym::Safe(create(loader->ldFloat()));
    result->slot(0).f = v;
    return result;
}

ym::Safe<YmObj> YmCtx::newBool(YmBool v) {
    auto result = ym::Safe(create(loader->ldBool()));
    result->slot(0).b = v;
    return result;
}

ym::Safe<YmObj> YmCtx::newRune(YmRune v) {
    auto result = ym::Safe(create(loader->ldRune()));
    result->slot(0).r = v;
    return result;
}

ym::Safe<YmObj> YmCtx::newType(ym::Safe<YmType> v) {
    auto result = ym::Safe(create(loader->ldType()));
    result->slot(0).type = v;
    return result;
}

YmCallStackHeight YmCtx::callStkHeight() const noexcept {
    return (YmCallStackHeight)_callStk.size();
}

std::string YmCtx::fmtCallStk(YmCallStackHeight skip) const {
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

bool YmCtx::isUser() const noexcept {
    return _callStk.size() == 1;
}

YmUInt16 YmCtx::args() const noexcept {
    ymAssert(!_callStk.empty());
    return _callStk.back().args;
}

YmLocals YmCtx::locals() const noexcept {
    ymAssert(!_callStk.empty());
    return YmLocals(_globalObjStk.size()) - _callStk.back().localsOffset;
}

YmObj* YmCtx::arg(YmUInt16 which, YmRefPolicy returnPolicy) {
    auto result =
        which < args()
        ? _globalObjStk[_callStk.back().argsOffset + which].get()
        : nullptr;
    if (result && returnPolicy == YM_TAKE) {
        secure(*result);
    }
    return result;
}

YmType* YmCtx::ref(YmRef reference) {
    return
        _callStk.back().fn
        ? _callStk.back().fn->ref(reference)
        : nullptr;
}

YmObj* YmCtx::local(YmLocal where, YmRefPolicy returnPolicy) {
    auto result =
        where < locals()
        ? _globalObjStk[_callStk.back().localsOffset + where].get()
        : nullptr;
    if (result && returnPolicy == YM_TAKE) {
        secure(*result);
    }
    return result;
}

YmObj* YmCtx::pop() noexcept {
    if (auto result = local(locals() - 1)) {
        _globalObjStk.pop_back();
        return result;
    }
    else {
        return nullptr;
    }
}

void YmCtx::popN(YmLocals n) {
    if (n > locals()) {
        n = locals();
    }
    for (YmUInt16 i = 0; i < n; i++) {
        release(ym::deref(pop()));
    }
}

bool YmCtx::put(YmLocal where, YmObj& what, YmRefPolicy whatPolicy) {
    if (where == YM_DISCARD) {
        if (whatPolicy == YM_TAKE) {
            release(what);
        }
        return true;
    }
    if (where == YM_NEWTOP) {
        _globalObjStk.push_back(what);
        if (whatPolicy == YM_BORROW) {
            secure(what);
        }
        return true;
    }
    if (where >= locals()) {
        _ym::Global::raiseErr(
            YmErrCode_LocalNotFound,
            "Put failed; local object index {} out-of-bounds!",
            where);
        return false;
    }
    release(ym::deref(local(where)));
    _globalObjStk[_callStk.back().localsOffset + where] = what;
    if (whatPolicy == YM_BORROW) {
        secure(what);
    }
    return true;
}

bool YmCtx::call(YmType& fn, YmUInt16 argsN, YmLocal returnTo) {
    if (_beginCall(fn, argsN, returnTo)) {
        auto& callBhvrInfo = fn.info->callBehaviour.value();
        callBhvrInfo.fn(this, callBhvrInfo.user);
        return _endCall();
    }
    return false;
}

bool YmCtx::ret(YmObj* what, YmRefPolicy whatPolicy) {
    if (isUser()) {
        return false;
    }
    if (!what) {
        return false;
    }
    ymAssert(!_callStk.empty());
    if (_callStk.back().returnValue) {
        release(*_callStk.back().returnValue);
    }
    _callStk.back().returnValue = what;
    if (what && whatPolicy == YM_BORROW) {
        secure(*what);
    }
    return true;
}

void YmCtx::_beginUserPseudoCall() {
    ymAssert(_callStk.empty());
    _callStk.push_back(_CallFrame{
        .fn = nullptr,
        .args = 0,
        .returnTo = YmLocal{},
        .argsOffset = 0,
        .localsOffset = 0,
        });
}

bool YmCtx::_beginCall(YmType& fn, YmUInt16 args, YmLocal returnTo) {
    ymAssert(!_callStk.empty());
    if (returnTo != YM_NEWTOP && returnTo != YM_DISCARD && returnTo >= locals()) {
        _ym::Global::raiseErr(
            YmErrCode_LocalNotFound,
            "Call to {} failed; local object index {} out-of-bounds!",
            fn.fullname(),
            returnTo);
        return false;
    }
    if (!ymKind_IsCallable(fn.kind())) {
        _ym::Global::raiseErr(
            YmErrCode_NonCallableType,
            "Call to {} failed; {} is non-callable!",
            fn.fullname(),
            fn.fullname());
        return false;
    }
    if (args != fn.params()) {
        _ym::Global::raiseErr(
            YmErrCode_CallProcedureError,
            "Call to {} failed; {} args provided, but fn takes {} params!",
            fn.fullname(),
            args,
            fn.params());
        return false;
    }
    if (YmLocals(args) > locals()) {
        _ym::Global::raiseErr(
            YmErrCode_LocalNotFound,
            "Call to {} failed; {} args provided, but local object stack height is {}!",
            fn.fullname(),
            args,
            locals());
        return false;
    }
    for (YmParamIndex i = 0; i < fn.params(); i++) {
        auto t = ym::deref(local(locals() - args + i)).type;
        if (t != fn.paramType(i)) {
            _ym::Global::raiseErr(
                YmErrCode_CallProcedureError,
                "Call to {} failed; arg #{} (for param {}) is {}, but expected {}!",
                fn.fullname(),
                i + 1,
                fn.paramName(i),
                t->fullname(),
                fn.paramType(i)->fullname());
            return false;
        }
    }
    if (callStkHeight() == YM_MAX_CALL_STACK_HEIGHT) {
        _ym::Global::raiseErr(
            YmErrCode_CallStackOverflow,
            "Call to {} failed; call stack overflow!",
            fn.fullname());
        return false;
    }
    _callStk.push_back(_CallFrame{
        .fn = &fn,
        .args = args,
        .returnTo = returnTo,
        .argsOffset = YmLocal(_globalObjStk.size()) - YmLocal(args),
        .localsOffset = YmLocal(_globalObjStk.size()),
        });
    return true;
}

bool YmCtx::_endCall() noexcept {
    ymAssert(!_callStk.empty());
    _CallFrame cf = _callStk.back();
    popN(locals());
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
        return false;
    }
    else if (cf.returnValue->type != cf.fn->returnType()) {
        _ym::Global::raiseErr(
            YmErrCode_CallProcedureError,
            "Call to {} failed; returned {}, but expected {}!",
            cf.fn->fullname(),
            cf.returnValue->type->fullname(),
            cf.fn->returnType()->fullname());
        release(*cf.returnValue);
        return false;
    }
    else {
        popN(cf.args);
        return put(cf.returnTo, *cf.returnValue);
    }
}

