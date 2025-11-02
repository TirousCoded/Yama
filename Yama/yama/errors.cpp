

#include "errors.h"

#include "../internal/general.h"


const YmChar* ymFmtYmErrCode(YmErrCode code) {
    static_assert(YmErrCode_Num == 1);
    switch (code) {
    case YmErrCode_IllegalPath: return "IllegalPath";
    default:                    return "???";
    }
}

void ymSetErrCallback(YmErrCallbackFn fn, void* user) {
    _ym::Global::setErrCallback(fn, user);
}

