

#include "errors.h"

#include "../internal/general.h"
#include "../yama++/print.h"


const YmChar* ymFmtYmErrCode(YmErrCode code) {
    static_assert(YmErrCode_Num == 37);
    switch (code) {
    case YmErrCode_IllegalSpecifier:            return "IllegalSpecifier";
    case YmErrCode_IllegalConstraint:           return "IllegalConstraint";
    case YmErrCode_PathBindError:               return "PathBindError";
    case YmErrCode_TypeArgsError:               return "TypeArgsError";
    case YmErrCode_ParcelNotFound:              return "ParcelNotFound";
    case YmErrCode_TypeNotFound:                return "TypeNotFound";
    case YmErrCode_ParamNotFound:               return "ParamNotFound";
    case YmErrCode_CallSigNotFound:             return "CallSigNotFound";
    case YmErrCode_CallSigNotUserDefined:       return "CallSigNotUserDefined";
    case YmErrCode_IllegalName:                 return "IllegalName";
    case YmErrCode_NameConflict:                return "NameConflict";
    case YmErrCode_LimitReached:                return "LimitReached";
    case YmErrCode_TypeMismatch:                return "TypeMismatch";
    case YmErrCode_IllegalNameList:             return "IllegalNameList";
    case YmErrCode_ConcreteType:                return "ConcreteType";
    case YmErrCode_GenericType:                 return "GenericType";
    case YmErrCode_MemberType:                  return "MemberType";
    case YmErrCode_NonCallableType:             return "NonCallableType";
    case YmErrCode_TypeCannotHaveMembers:       return "TypeCannotHaveMembers";
    case YmErrCode_TypeCannotHaveTypeParams:    return "TypeCannotHaveTypeParams";
    case YmErrCode_NonStructType:               return "NonStructType";
    case YmErrCode_ProtocolType:                return "ProtocolType";
    case YmErrCode_NonProtocolType:             return "NonProtocolType";
    case YmErrCode_ProtocolMemberType:          return "ProtocolMemberType";
    case YmErrCode_ReadOnlyVarType:             return "ReadOnlyVarType";
    case YmErrCode_NonVarType:                  return "NonVarType";
    case YmErrCode_ReadOnlyPropertyType:        return "ReadOnlyPropertyType";
    case YmErrCode_PropertyType:                return "PropertyType";
    case YmErrCode_NonPropertyType:             return "NonPropertyType";
    case YmErrCode_PropertyAssignerType:        return "PropertyAssignerType";
    case YmErrCode_ArgNotFound:                 return "ArgNotFound";
    case YmErrCode_LocalNotFound:               return "LocalNotFound";
    case YmErrCode_CallProcedureError:          return "CallProcedureError";
    case YmErrCode_CallStackOverflow:           return "CallStackOverflow";
    case YmErrCode_NoDefaultValue:              return "NoDefaultValue";
    case YmErrCode_IllegalConversion:           return "IllegalConversion";
    case YmErrCode_InternalError:               return "InternalError";
    default:                                    return "???";
    }
}

void ymDefaultErrFn(YmErrCode code, const YmChar* msg, void* user) {
    if (msg) {
        // TODO: This doesn't output to stderr.
        ym::println("{}", msg);
    }
}

void ymSetErrCallback(YmErrCallbackFn fn, void* user) {
    _ym::Global::setErrCallback(fn, user);
}

