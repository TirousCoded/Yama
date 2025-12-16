

#include "errors.h"

#include "../internal/general.h"


const YmChar* ymFmtYmErrCode(YmErrCode code) {
    static_assert(YmErrCode_Num == 11);
    switch (code) {
    case YmErrCode_IllegalPath:             return "IllegalPath";
    case YmErrCode_IllegalFullname:         return "IllegalFullname";
    case YmErrCode_ParcelNotFound:          return "ParcelNotFound";
    case YmErrCode_ItemNotFound:            return "ItemNotFound";
    case YmErrCode_ParamNotFound:           return "ParamNotFound";
    case YmErrCode_ItemNameConflict:        return "ItemNameConflict";
    case YmErrCode_ParamNameConflict:       return "ParamNameConflict";
    case YmErrCode_MaxParamsLimit:          return "MaxParamsLimit";
    case YmErrCode_NonCallableItem:         return "NonCallableItem";
    case YmErrCode_ItemCannotHaveMembers:   return "ItemCannotHaveMembers";
    case YmErrCode_InternalError:           return "InternalError";
    default:                                return "???";
    }
}

void ymSetErrCallback(YmErrCallbackFn fn, void* user) {
    _ym::Global::setErrCallback(fn, user);
}

