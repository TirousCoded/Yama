

#include "errors.h"

#include "../internal/general.h"


const YmChar* ymFmtYmErrCode(YmErrCode code) {
    static_assert(YmErrCode_Num == 6);
    switch (code) {
    case YmErrCode_IllegalPath:         return "IllegalPath";
    case YmErrCode_IllegalFullname:     return "IllegalFullname";
    case YmErrCode_ParcelNotFound:      return "ParcelNotFound";
    case YmErrCode_ItemNotFound:        return "ItemNotFound";
    case YmErrCode_ItemNameConflict:    return "ItemNameConflict";
    case YmErrCode_MaxConstsLimit:      return "MaxConstsLimit";
    default:                            return "???";
    }
}

void ymSetErrCallback(YmErrCallbackFn fn, void* user) {
    _ym::Global::setErrCallback(fn, user);
}

