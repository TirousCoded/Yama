

#include "../yama/yama.h"

#if defined(YM_DEBUG) && defined(YM_PLATFORM_LINUX)
#include <signal.h>
#endif

#include "general.h"


void _ym::debugbreak() noexcept {
#if defined(YM_DEBUG)
#if defined(YM_PLATFORM_WINDOWS)
    __debugbreak();
#elif defined(YM_PLATFORM_LINUX)
    raise(SIGTRAP);
#else
    std::abort(); // Fallback
#endif
#endif
}

_ym::Resource::Resource(RType rtype) :
    _rtype(rtype) {}

_ym::RType _ym::Resource::rtype() const noexcept {
    return _rtype;
}

