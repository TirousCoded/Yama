

#pragma once
#ifndef _ERRORS_DOT_H
#define _ERRORS_DOT_H 1


#include "basics.h"


extern "C" {


    typedef enum : YmUInt16 {
        YmErrCode_IllegalPath = 0, /* Failure due to an illegal import path. */

        YmErrCode_Num, /* Enum size. Not a valid error code. */
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

    /* TODO: ymSetErrCallback hasn't been unit tested. */

    /* Sets the error callback function, on a thread-local basis. */
    /* fn is the callback function. */
    /* user is a pointer to external data to be exposed to the callback. */
    /* Error callbacks are disabled if fn == YM_NIL. */
    /* Behaviour is undefined if fn is invalid. */
    void ymSetErrCallback(YmErrCallbackFn fn, void* user);
}


#endif

