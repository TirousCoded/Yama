

#include "errors.h"

#include "../internal/general.h"


const YmChar* ymFmtYmErrCode(YmErrCode code) {
    static_assert(YmErrCode_Num == 17);
    switch (code) {
    case YmErrCode_IllegalSpecifier:        return "IllegalSpecifier";
    case YmErrCode_IllegalConstraint:       return "IllegalConstraint";
    case YmErrCode_PathBindError:           return "PathBindError";
    case YmErrCode_ItemArgsError:           return "ItemArgsError";
    case YmErrCode_ParcelNotFound:          return "ParcelNotFound";
    case YmErrCode_ItemNotFound:            return "ItemNotFound";
    case YmErrCode_ParamNotFound:           return "ParamNotFound";
    case YmErrCode_NameConflict:            return "NameConflict";
    case YmErrCode_LimitReached:            return "LimitReached";
    case YmErrCode_ConcreteItem:            return "ConcreteItem";
    case YmErrCode_GenericItem:             return "GenericItem";
    case YmErrCode_MemberItem:              return "MemberItem";
    case YmErrCode_NonCallableItem:         return "NonCallableItem";
    case YmErrCode_ItemCannotHaveMembers:   return "ItemCannotHaveMembers";
    case YmErrCode_ProtocolItem:            return "ProtocolItem";
    case YmErrCode_NonProtocolItem:         return "NonProtocolItem";
    case YmErrCode_InternalError:           return "InternalError";
    default:                                return "???";
    }
}

void ymSetErrCallback(YmErrCallbackFn fn, void* user) {
    _ym::Global::setErrCallback(fn, user);
}

