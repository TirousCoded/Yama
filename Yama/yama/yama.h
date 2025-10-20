

#pragma once
#ifndef _YAMA_DOT_H
#define _YAMA_DOT_H 1


#include <stdint.h>
#include <uchar.h>


extern "C" {


#if defined(_WIN32)
#define YM_PLATFORM_WINDOWS 1
#elif defined(__APPLE__) || defined(__MACH__)
#define YM_PLATFORM_MAC 1
#elif defined(__linux__)
#define YM_PLATFORM_LINUX 1
#else
#error "Unknown platform!"
#endif

#if defined(DEBUG)
#define YM_DEBUG 1
#endif


#define YM_EXPAND_MACRO(x) x
#define YM_STRINGIFY_MACRO(x) #x


#if defined(__cplusplus)
#define YM_STATIC_ASSERT(x) static_assert((x))
#else
#define YM_STATIC_ASSERT(x)
#endif


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

    YM_STATIC_ASSERT(sizeof(YmFloat32) == sizeof(YmInt32));
    YM_STATIC_ASSERT(sizeof(YmFloat64) == sizeof(YmInt64));

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


#if defined(YM_DEBUG)
    /* Performs debug assertion of cond. */
    /* Disabled if compiled without YM_DEBUG. */
#define ymAssert(cond) _ymAssert(YmBool(cond) == YM_TRUE, YM_STRINGIFY_MACRO(cond), __FILE__, __LINE__);
#else
    /* Performs debug assertion of cond. */
    /* Disabled if compiled without YM_DEBUG. */
#define ymAssert(cond) ((void)0)
#endif

    /* Delimits end of code path which should not be reachable. */
    /* Disabled if compiled without YM_DEBUG. */
#define YM_DEADEND ymAssert(false)

    /* Performs assertion of cond. */
    /* Is not disabled if compiled without YM_DEBUG. */
#define ymVerify(cond) _ymVerify(YmBool(cond) == YM_TRUE, YM_STRINGIFY_MACRO(cond), __FILE__, __LINE__);


    /* TODO: When we add ARC, in the backend are we gonna have to have
    *		 seperate stuff for atomic vs. non-atomic? Or are allow ARC
    *		 resources gonna be non-atomic?
    * 
    *		 If all are gonna be non-atomic, we should write that down too.
    */

    /* TODO: Later expand below to include the concept of ARC references
    *		 including Python-style rules about borrowed/stolen refs, and
    *		 updating ymDrop description to detail about ARC resources.
    * 
    *		 As part of this we'll also need to explain how the system can
    *		 have *internal* refs to an ARC resource which can keep it alive
    *		 even when it's *public* ref count is zero.
    */

    /* Attempts to release Yama resource x. */
    /* Behaviour is undefined if x is invalid. */
#define ymDrop(x) _ymDrop((void*)x)


    struct YmDm;
    struct YmCtx;

    /* Creates a new Yama domain, returning a pointer to it. */
    struct YmDm* ymNewDm(void);

    /* Creates a new Yama context, bound to domain, returning a pointer to it. */
    /* Behaviour is undefined if domain is invalid. */
    struct YmCtx* ymNewCtx(struct YmDm* domain);


    /* Internals */

    void _ymAssert(YmBool cond, const YmChar* cond_txt, const YmChar* file, YmWord line);
    void _ymVerify(YmBool cond, const YmChar* cond_txt, const YmChar* file, YmWord line);
    void _ymDrop(void* x);
}


#endif

