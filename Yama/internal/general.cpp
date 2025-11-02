

#include "../yama/yama.h"

#if defined(YM_DEBUG) && defined(YM_PLATFORM_LINUX)
#include <signal.h>
#endif

#include "general.h"
#include "resources.h"


void _ym::debugbreak() noexcept {
#if defined(YM_DEBUG)
#if defined(YM_PLATFORM_WINDOWS)
    __debugbreak();
#elif defined(YM_PLATFORM_LINUX)
    raise(SIGTRAP);
#else
    crash(); // Fallback
#endif
#endif
}

void _ym::crash() noexcept {
    std::abort();
}

thread_local _ym::ErrCallbackInfo _ym::Global::_errCallbackInfo = {};

const std::regex _ym::Global::_legalPathPattern = std::regex("[^/:]+(/[^/:]+)*");

void _ym::Global::setErrCallback(YmErrCallbackFn fn, void* user) noexcept {
    _errCallbackInfo = ErrCallbackInfo{
        .fn = fn,
        .user = user,
    };
}

bool _ym::Global::pathIsLegal(std::string_view path) {
    return std::regex_match(path.begin(), path.end(), _legalPathPattern);
}

