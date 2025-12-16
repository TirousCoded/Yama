

#pragma once


#include <string>

#include "../yama/yama.h"


namespace ym {


    // TODO: These fns haven't been unit tested yet.

    std::string fmt(YmInt x, bool uppercaseHex = true, YmIntFmt fmt = YmIntFmt_Dec);
    std::string fmt(YmUInt x, bool uppercaseHex = true, YmIntFmt fmt = YmIntFmt_Dec);
    std::string fmt(YmFloat x);
    std::string fmt(YmBool x);
    std::string fmt(YmRune x, bool uppercaseHex = true, bool escapeQuotes = true, bool escapeDblQuotes = true, bool escapeBackslashes = true);
}

