

#include "BuiltinsCache.h"

#include "Loader.h"


_ym::BuiltinsCache _ym::BuiltinsCache::mk(Loader& ldr) {
    return BuiltinsCache{
        .none = ym::deref(ldr.load(Spec::typeFast("yama:None"))),
        .int0 = ym::deref(ldr.load(Spec::typeFast("yama:Int"))),
        .uint = ym::deref(ldr.load(Spec::typeFast("yama:UInt"))),
        .float0 = ym::deref(ldr.load(Spec::typeFast("yama:Float"))),
        .bool0 = ym::deref(ldr.load(Spec::typeFast("yama:Bool"))),
        .rune = ym::deref(ldr.load(Spec::typeFast("yama:Rune"))),
        .type = ym::deref(ldr.load(Spec::typeFast("yama:Type"))),
    };
}

