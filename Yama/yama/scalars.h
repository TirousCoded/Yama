

#pragma once
#ifndef _BASICS_DOT_H
#define _BASICS_DOT_H 1


#include <limits>
#include <stdint.h>
#include <uchar.h>


extern "C" {


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


#define YM_INF (YmFloat(HUGE_VAL))
#define YM_TRUE (YmBool(1))
#define YM_FALSE (YmBool(0))
#define YM_NIL (NULL)


    typedef enum : YmUInt8 {
        YmIntFmt_Dec, /* Decimal */
        YmIntFmt_Hex, /* Hexadecimal */
        YmIntFmt_Bin, /* Binary */
    } YmIntFmt;

    /* Returns the amount of bytes needed to store a formatted string of x. */
    /* Return value does not count a null-terminator. */
    size_t ymMeasureInt(YmInt x, YmIntFmt fmt);

    /* Returns the amount of bytes needed to store a formatted string of x. */
    /* Return value does not count a null-terminator. */
    size_t ymMeasureUInt(YmUInt x, YmIntFmt fmt);
    
    /* Returns the amount of bytes needed to store a formatted string of x. */
    /* Return value does not count a null-terminator. */
    size_t ymMeasureFloat(YmFloat x);
    
    /* Returns the amount of bytes needed to store a formatted string of x. */
    /* Return value does not count a null-terminator. */
    size_t ymMeasureBool(YmBool x);
    
    /* Returns the amount of bytes needed to store a formatted string of x. */
    /* Return value does not count a null-terminator. */
    size_t ymMeasureRune(YmRune x, YmBool escapeQuotes, YmBool escapeDblQuotes, YmBool escapeBackslashes);

    /* Returns a formatted string representation of x. */
    /* If writeTo == YM_NIL, the returned string memory must be cleaned up by the end-user via free. */
    /* If writeTo != YM_NIL, the formatted string will be written to writeTo, and writeTo will be returned. */
    /* Behaviour is undefined if writeTo != YM_NIL, but the writeTo buffer isn't large enough to contain the formatted string. */
    const YmChar* ymFmtInt(YmInt x, YmBool uppercaseHex, YmIntFmt fmt, YmChar* writeTo);

    /* Returns a formatted string representation of x. */
    /* If writeTo == YM_NIL, the returned string memory must be cleaned up by the end-user via free. */
    /* If writeTo != YM_NIL, the formatted string will be written to writeTo, and writeTo will be returned. */
    /* Behaviour is undefined if writeTo != YM_NIL, but the writeTo buffer isn't large enough to contain the formatted string. */
    const YmChar* ymFmtUInt(YmUInt x, YmBool uppercaseHex, YmIntFmt fmt, YmChar* writeTo);

    /* Returns a formatted string representation of x. */
    /* If writeTo == YM_NIL, the returned string memory must be cleaned up by the end-user via free. */
    /* If writeTo != YM_NIL, the formatted string will be written to writeTo, and writeTo will be returned. */
    /* Behaviour is undefined if writeTo != YM_NIL, but the writeTo buffer isn't large enough to contain the formatted string. */
    const YmChar* ymFmtFloat(YmFloat x, YmChar* writeTo);

    /* Returns a formatted string representation of x. */
    /* The memory of the returned string is static and is valid for the lifetime of the process. */
    const YmChar* ymFmtBool(YmBool x);

    /* Returns a formatted string representation of x. */
    /* If writeTo == YM_NIL, the returned string memory must be cleaned up by the end-user via free. */
    /* If writeTo != YM_NIL, the formatted string will be written to writeTo, and writeTo will be returned. */
    /* Behaviour is undefined if writeTo != YM_NIL, but the writeTo buffer isn't large enough to contain the formatted string. */
    const YmChar* ymFmtRune(YmRune x, YmBool uppercaseHex, YmBool escapeQuotes, YmBool escapeDblQuotes, YmBool escapeBackslashes, YmChar* writeTo);


    typedef enum : YmUInt8 {
        YmParseStatus_Success, /* Success */
        YmParseStatus_Failure, /* Failure */
        YmParseStatus_Overflow, /* Success, but int/float value overflowed. */
        YmParseStatus_Underflow, /* Success, but int/float value underflowed. */
    } YmParseStatus;

    /* Parses a Yama int from input, returning parse status. */
    /* If present, the parsed value will be written to *output. */
    /* If present, the number of bytes parsed will be written to *bytes. */
    /* Parsed value is undefined in case of overflow/underflow. */
    /* Additional text may exist following a valid parsed value. */
    YmParseStatus ymParseInt(const YmChar* input, YmInt* output, size_t* bytes);

    /* Parses a Yama uint from input, returning parse status. */
    /* ignoreU dictates if requiring the final 'u' is disabled, in which case suffixial 'u' characters will be ignored. */
    /* If present, the parsed value will be written to *output. */
    /* If present, the number of bytes parsed will be written to *bytes. */
    /* Parsed value is undefined in case of overflow/underflow. */
    /* Additional text may exist following a valid parsed value. */
    YmParseStatus ymParseUInt(const YmChar* input, YmUInt* output, size_t* bytes, YmBool ignoreU);

    /* TODO: Parsing supports infinities, but doesn't support NaNs.
    *        When we do add this, be sure to finish refactoring that part of our unit tests.
    */

    /* Parses a Yama float from input, returning parse status. */
    /* If present, the parsed value will be written to *output. */
    /* If present, the number of bytes parsed will be written to *bytes. */
    /* Parsed value is YM_INF/-YM_INF in case of overflow/underflow. */
    /* Additional text may exist following a valid parsed value. */
    YmParseStatus ymParseFloat(const YmChar* input, YmFloat* output, size_t* bytes);

    /* Parses a Yama bool from input, returning parse status. */
    /* If present, the parsed value will be written to *output. */
    /* If present, the number of bytes parsed will be written to *bytes. */
    /* Additional text may exist following a valid parsed value. */
    YmParseStatus ymParseBool(const YmChar* input, YmBool* output, size_t* bytes);

    /* Parses a Yama rune from input, returning parse status. */
    /* If present, the parsed value will be written to *output. */
    /* If present, the number of bytes parsed will be written to *bytes. */
    /* Additional text may exist following a valid parsed value. */
    YmParseStatus ymParseRune(const YmChar* input, YmRune* output, size_t* bytes);
}


#endif

