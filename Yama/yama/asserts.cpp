

#include "asserts.h"

#include "basics.h"
#include "../yama++/print.h"
#include "../internal/general.h"


void _ymAssert(YmBool cond, const YmChar* cond_txt, const YmChar* file, YmWord line) {
    if (cond == YM_TRUE) return;
    ym::println("Yama Assert Failed:\ncond: {}\nfile: {}\nline: {}", cond_txt, file, line);
    _ym::debugbreak();
}

void _ymVerify(YmBool cond, const YmChar* cond_txt, const YmChar* file, YmWord line) {
    if (cond == YM_TRUE) return;
    ym::println("Yama Assert Failed:\ncond: {}\nfile: {}\nline: {}", cond_txt, file, line);
    // Using crash as debugbreak won't work in release mode.
    _ym::crash();
}

