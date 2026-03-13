

#include "../yama/yama.h"

#if defined(YM_DEBUG) && defined(YM_PLATFORM_LINUX)
#include <signal.h>
#endif

#include "../yama++/Safe.h"
#include "general.h"
#include "frontend-resources.h"


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

thread_local _ym::ErrCallbackInfo _ym::Global::_errCallbackInfo = ErrCallbackInfo{
    .fn = ymDefaultErrFn,
    .user = nullptr,
};

const std::regex _ym::Global::_legalPathPattern = std::regex("[^/:]+(/[^/:]+)*");
const std::regex _ym::Global::_legalFullnamePattern = std::regex("[^/:]+(/[^/:]+)*:[^/:]+(::[^/:]+)?");

void _ym::Global::setErrCallback(YmErrCallbackFn fn, void* user) noexcept {
    _errCallbackInfo = ErrCallbackInfo{
        .fn = fn,
        .user = user,
    };
}

bool _ym::Global::pathIsLegal(std::string_view path) {
    return std::regex_match(path.begin(), path.end(), _legalPathPattern);
}

bool _ym::Global::fullnameIsLegal(std::string_view fullname) {
    return std::regex_match(fullname.begin(), fullname.end(), _legalFullnamePattern);
}

bool _ym::Global::refSymIsLegal(std::string_view refSym) {
    return
        refSym == "Self" ||
        fullnameIsLegal(refSym);
}

_ym::CallBhvrCallbackInfo _ym::CallBhvrCallbackInfo::mk(YmCallBhvrCallbackFn fn, void* user) noexcept {
    ym::assertSafe(fn);
    return CallBhvrCallbackInfo{ .fn = fn, .user = user };
}
