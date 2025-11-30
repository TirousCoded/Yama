

#include "ErrCounter.h"

#include <yama++/print.h>


YmWord ErrCounter::count(YmErrCode code) const noexcept {
    ymAssert(code < YmErrCode_Num);
    return _counts[code];
}

YmWord ErrCounter::operator[](YmErrCode code) const noexcept {
    return count(code);
}

void ErrCounter::report(YmErrCode code, const YmChar* msg) {
    ymAssert(code < YmErrCode_Num);
    ymAssert(msg != nullptr);
    if (!_suppressLog) {
        ym::println("{}", msg);
    }
    _counts[code]++;
}

void ErrCounter::reset() noexcept {
    _counts.fill(0);
}

void ErrCounter::setSuppressLog(bool suppress) noexcept {
    _suppressLog = suppress;
}

void ErrCounter::setupCallbackForThisThread() {
    ymSetErrCallback(
        [](YmErrCode code, const YmChar* msg, void* user) {
            ymAssert(user != nullptr);
            ((ErrCounter*)user)->report(code, msg);
        },
        (void*)this);
}

