

#pragma once


#include "YmType.h"


namespace _ym {


    class Loader;


    struct BuiltinsCache final {
        ym::Safe<YmType> none;
        ym::Safe<YmType> int0;
        ym::Safe<YmType> uint;
        ym::Safe<YmType> float0;
        ym::Safe<YmType> bool0;
        ym::Safe<YmType> rune;
        ym::Safe<YmType> type;


        // Performs the loads.
        static BuiltinsCache mk(Loader& ldr);
    };

}

