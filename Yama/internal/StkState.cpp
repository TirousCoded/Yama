

#include "StkState.h"

#include "../yama++/Safe.h"
#include "YmObj.h"


_ym::StkState::StkState() {
    _pushUserPseudoCall();
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

_ym::CF& _ym::StkState::cf() noexcept {
    ymAssert(callStkHeight() >= 1);
    return _callStk.back();
}

const _ym::CF& _ym::StkState::cf() const noexcept {
    ymAssert(callStkHeight() >= 1);
    return _callStk.back();
}

void _ym::StkState::pushCF(YmType* fn, YmLocal returnTo, CallArgPackInfo argPack) {
    _callStk.push_back(CF{
        .fn = fn,
        .returnTo = returnTo,
        .localsOffset = globals(),
        .argPack = std::move(argPack),
        });
}

std::optional<_ym::CF> _ym::StkState::pullCF() {
    if (callStkHeight() == 0) {
        return std::nullopt;
    }
    pop(locals());
    CF result = std::move(_callStk.back());
    _callStk.pop_back();
    return ym::retopt(result);
}

void _ym::StkState::popAllCFs() {
    while ((bool)pullCF()) {}
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

std::optional<YmLocal> _ym::StkState::absLocal(YmLocal x) const noexcept {
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

std::optional<YmLocal> _ym::StkState::absLocalForRead(YmLocal x) const noexcept {
    if (x == YM_PUSH || x == YM_DISCARD) {
        return std::nullopt;
    }
    return absLocal(x);
}

bool _ym::StkState::writeIsInBounds(YmLocal where) const noexcept {
    return absLocal(where).has_value();
}

bool _ym::StkState::readIsInBounds(YmLocal where) const noexcept {
    return absLocalForRead(where).has_value();
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
    auto where = cf().argOffset(which).value();
    bool isCallObjOfProtoMethod = which == 0 && cf().fwdFromProto;
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
    setGlobal(cf().argOffset(which).value(), std::move(newArg));
    return true;
}

YmType* _ym::StkState::ref(YmRef reference) {
    return
        cf().fn
        ? cf().fn->ref(reference)
        : nullptr;
}

_ym::TempRef _ym::StkState::local(YmLocal where) {
    return global(cf().localOffset(absLocalForRead(where)));
}

_ym::TempRef _ym::StkState::stealLocal(YmLocal where, bool frontendRef) {
    return stealGlobal(cf().localOffset(absLocalForRead(where)), frontendRef);
}

void _ym::StkState::reset() {
    popAllCFs();
    _pushUserPseudoCall();
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

void _ym::StkState::popUntil(YmLocals n) {
    pop(locals() - std::min(n, locals()));
}

_ym::TempRef _ym::StkState::pull(bool frontendRef) noexcept {
    if (locals() == 0) {
        return nullptr;
    }
    auto result = _globalObjStk.back().steal(frontendRef);
    _globalObjStk.pop_back();
    return result;
}

bool _ym::StkState::copy(YmLocal from, YmLocal to) {
    return put(to, local(from));
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
    if (auto offset = cf().localOffset(absLocal(where))) {
        setGlobal(*offset, std::move(what));
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
    auto aLocal = absLocalForRead(a);
    auto bLocal = absLocalForRead(b);
    if (aLocal && bLocal) {
        std::swap(
            _globalObjStk[cf().localOffset(*aLocal)],
            _globalObjStk[cf().localOffset(*bLocal)]);
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

bool _ym::StkState::retObj(_ym::TempRef what) {
    if (!what) {
        return false;
    }
    if (isUser()) {
        return false;
    }
    cf().returnVal = std::move(what);
    return true;
}

void _ym::StkState::_pushUserPseudoCall() {
    ymAssert(callStkHeight() == 0);
    pushCF(nullptr, YmLocal{}, CallArgPackInfo{});
}

