

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

#if defined(__GNUC__)
#define YM_PLATFORM_GCC 1
#elif defined(__clang__)
#define YM_PLATFORM_CLANG 1
#elif defined(_MSC_VER)
#define YM_PLATFORM_MSVC 1
#else
#error "Unknown compiler!"
#endif

#if defined(YM_PLATFORM_GCC) || defined(YM_PLATFORM_CLANG)
#define YM_FUNCSIG __PRETTY_FUNCTION__
#elif defined(YM_PLATFORM_MSVC)
#define YM_FUNCSIG __FUNCSIG__
#endif


#endif

