

#pragma once


// general-purpose helper macros


// YAMA_COND performs expr if cond == true, w/ the macro acting
// as a void returning expression

#define YAMA_COND(cond, expr) \
(void)((cond) && ((void)(expr), true))

