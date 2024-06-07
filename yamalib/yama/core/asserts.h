

#pragma once


#include <taul/asserts.h>


// YAMA_ASSERT is here so later we can make assert behaviour configurable

#define YAMA_ASSERT(cond) TAUL_ASSERT(cond)

// YAMA_DEADEND is for asserting that a region of code should not be reached at runtime

#define YAMA_DEADEND YAMA_ASSERT(false)

// YAMA_DEREF_SAFE is a if-statement-like macro used to summarize the checking of pointer 
// safety, calling YAMA_DEADEND if the safety check fails

#define YAMA_DEREF_SAFE(cond) if (!(cond)) YAMA_DEADEND; else

