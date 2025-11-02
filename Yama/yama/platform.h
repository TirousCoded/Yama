

#pragma once
#ifndef _PLATFORM_DOT_H
#define _PLATFORM_DOT_H 1


#if defined(_WIN32)
#define YM_PLATFORM_WINDOWS 1
#elif defined(__APPLE__) || defined(__MACH__)
#define YM_PLATFORM_MAC 1
#elif defined(__linux__)
#define YM_PLATFORM_LINUX 1
#else
#error "Unknown platform!"
#endif


#endif

