

#pragma once
#ifndef _ASSERTS_DOT_H
#define _ASSERTS_DOT_H 1


#include "config.h"
#include "macros.h"
#include "basics.h"


extern "C" {


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


    /* -- Internals -- */
    void _ymAssert(YmBool cond, const YmChar* cond_txt, const YmChar* file, YmWord line);
    void _ymVerify(YmBool cond, const YmChar* cond_txt, const YmChar* file, YmWord line);
}


#endif

