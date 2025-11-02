

#pragma once
#ifndef _BASICS_DOT_H
#define _BASICS_DOT_H 1


#include <stdint.h>
#include <uchar.h>


extern "C" {


    typedef size_t YmWord;

    typedef int8_t YmInt8;
    typedef int16_t YmInt16;
    typedef int32_t YmInt32;
    typedef int64_t YmInt64;

    typedef uint8_t YmUInt8;
    typedef uint16_t YmUInt16;
    typedef uint32_t YmUInt32;
    typedef uint64_t YmUInt64;

    typedef float YmFloat32;
    typedef double YmFloat64;

#ifdef __cplusplus
    static_assert(sizeof(YmFloat32) == sizeof(YmInt32));
    static_assert(sizeof(YmFloat64) == sizeof(YmInt64));
#endif

    typedef YmInt64 YmInt;
    typedef YmUInt64 YmUInt;
    typedef YmFloat64 YmFloat;

    /* TODO: Look into things like _Bool later. */

    typedef YmInt8 YmBool;

    typedef char YmChar;
    typedef wchar_t YmWChar;
    typedef char8_t YmChar8;
    typedef char16_t YmChar16;
    typedef char32_t YmChar32;
    typedef YmChar32 YmRune;


#define YM_TRUE (1)
#define YM_FALSE (0)
#define YM_NIL (NULL)
}


#endif

