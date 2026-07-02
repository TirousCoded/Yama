

#pragma once
#ifndef _ERRORS_DOT_H
#define _ERRORS_DOT_H 1


#include "scalars.h"


extern "C" {


    /* TODO: Maybe replace later w/ a string-based system, instead of using an enum.
    * 
    *        This has a number of advantages:
    *           - We needn't suffer a large recompile since we can avoid needing to update
    *             this frontend header file.
    *           - It'll probably be easier to push error code to backend if need-be.
    *           - We can segregate errors w/ otherwise similar names by prefixing them,
    *             for example w/ names like 'Compile.OutOfBounds'.
    *           - We could allow end-user to enable/disable error msgs for specific errors
    *             via specifying string prefixes (eg. 'Compile' to enable/disable above.)
    */

    typedef enum : YmUInt16 {
        YmErrCode_IllegalSpecifier = 0,     /* Illegal specifier (in some import/load context.) */
        YmErrCode_IllegalConstraint,        /* Illegal type parameter constraint. */
        YmErrCode_PathBindError,            /* Path binding error. */
        YmErrCode_TypeArgsError,            /* Type arguments error. */
        YmErrCode_ParcelNotFound,           /* Parcel not found. */
        YmErrCode_TypeNotFound,             /* Type not found. */
        YmErrCode_ParamNotFound,            /* Parameter not found. */
        YmErrCode_CallSigNotFound,          /* Call signature not found. */
        YmErrCode_CallSigNotUserDefined,    /* Call signature is not user-defined. */
        YmErrCode_IllegalName,              /* Illegal name. */
        YmErrCode_NameConflict,             /* Name conflict detected. */
        YmErrCode_LimitReached,             /* Known limit reached. */
        YmErrCode_TypeMismatch,             /* Type not expected. */
        YmErrCode_IllegalNameList,          /* Illegal named arg list. */
        YmErrCode_ConcreteType,             /* Type is concrete (ie. not generic.) */
        YmErrCode_GenericType,              /* Type is generic (ie. not concrete.) */
        YmErrCode_MemberType,               /* Type is a member. */
        YmErrCode_NonCallableType,          /* Type is non-callable. */
        YmErrCode_TypeCannotHaveMembers,    /* Type cannot have members. */
        YmErrCode_TypeCannotHaveTypeParams, /* Type cannot have type parameters. */
        YmErrCode_NonStructType,            /* Type is a non-struct. */
        YmErrCode_ProtocolType,             /* Type is a protocol. */
        YmErrCode_NonProtocolType,          /* Type is a non-protocol. */
        YmErrCode_ProtocolMemberType,       /* Type is member of protocol type. */
        YmErrCode_ReadOnlyVarType,          /* Type is a read-only var. */
        YmErrCode_NonVarType,               /* Type is a non-var. */
        YmErrCode_ReadOnlyPropertyType,     /* Type is a read-only property. */
        YmErrCode_PropertyType,             /* Type is a property. */
        YmErrCode_NonPropertyType,          /* Type is a non-property. */
        YmErrCode_PropertyAssignerType,     /* Type is a property assigner. */
        YmErrCode_ArgNotFound,              /* Arg object index out-of-bounds. */
        YmErrCode_LocalNotFound,            /* Local object stack index out-of-bounds. */
        YmErrCode_CallProcedureError,       /* Call procedure error. */
        YmErrCode_CallStackOverflow,        /* Call stack overflow. */
        YmErrCode_NoDefaultValue,           /* No default value. */
        YmErrCode_IllegalConversion,        /* Illegal conversion. */
        YmErrCode_InternalError,            /* Internal Error */

        YmErrCode_Num,                      /* Enum size. Not a valid error code. */
    } YmErrCode;

    /* TODO: ymFmtYmErrCode hasn't been unit tested. */

    /* Returns a string representation of error code, or "???" if code is invalid. */
    /* The memory of the returned string is static and is valid for the lifetime of the process. */
    const YmChar* ymFmtYmErrCode(YmErrCode code);


    /* A callback function used to report errors and error-like messages. */
    /* code is the error code. */
    /* msg is the string error message. */
    /* The memory of msg is deallocated internally at the end of the callback. */
    /* user is a pointer used to expose callback function to external data. */
    typedef void (*YmErrCallbackFn)(
        YmErrCode code,
        const YmChar* msg,
        void* user);

    /* The default YmErrCallbackFn used by Yama. */
    void ymDefaultErrFn(
        YmErrCode code,
        const YmChar* msg,
        void* user);

    /* TODO: ymSetErrCallback hasn't been unit tested. */

    /* Sets the error callback function, on a thread-local basis. */
    /* fn is the callback function. */
    /* user is a pointer to external data to be exposed to the callback. */
    /* Error callbacks are disabled if fn == YM_NIL. */
    /* Behaviour is undefined if fn is invalid. */
    void ymSetErrCallback(YmErrCallbackFn fn, void* user);
}


#endif

