

#include "errors.h"

#include "../internal/general.h"


const YmChar* ymFmtYmErrCode(YmErrCode code) {
    static_assert(YmErrCode_Num == 17);
    switch (code) {
    case YmErrCode_IllegalSpecifier:        return "IllegalSpecifier";
    case YmErrCode_IllegalConstraint:       return "IllegalConstraint";
    case YmErrCode_PathBindError:           return "PathBindError";
    case YmErrCode_TypeArgsError:           return "TypeArgsError";
    case YmErrCode_ParcelNotFound:          return "ParcelNotFound";
    case YmErrCode_TypeNotFound:            return "TypeNotFound";
    case YmErrCode_ParamNotFound:           return "ParamNotFound";
    case YmErrCode_NameConflict:            return "NameConflict";
    case YmErrCode_LimitReached:            return "LimitReached";
    case YmErrCode_ConcreteType:            return "ConcreteType";
    case YmErrCode_GenericType:             return "GenericType";
    case YmErrCode_MemberType:              return "MemberType";
    case YmErrCode_NonCallableType:         return "NonCallableType";
    case YmErrCode_TypeCannotHaveMembers:   return "TypeCannotHaveMembers";
    case YmErrCode_ProtocolType:            return "ProtocolType";
    case YmErrCode_NonProtocolType:         return "NonProtocolType";
    case YmErrCode_InternalError:           return "InternalError";
    default:                                return "???";
    }
}

void ymSetErrCallback(YmErrCallbackFn fn, void* user) {
    _ym::Global::setErrCallback(fn, user);
}

