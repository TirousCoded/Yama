

#pragma once


#include <atomic>
#include <concepts>
#include <format>
#include <iostream>
#include <regex>

#include "../yama/yama.h"
#include "../yama++/Safe.h"


namespace _ym {


    void debugbreak() noexcept;
    void crash() noexcept;


    // Destroyable is intended for the pattern of objects having static 'create'
    // and destroy methods which handle the dynamic alloc/init and dealloc/deinit,
    // respectively, of objects of that type.

    template<typename T>
    concept Destroyable =
        std::same_as<T, std::remove_cvref_t<T>> &&
        requires (ym::Safe<T> ptr)
    {
        { T::destroy(ptr) } noexcept;
    };
    template<Destroyable T>
    inline void destroy(ym::Safe<T> x) noexcept {
        (void)T::destroy(ym::Safe(x));
    }


    struct ErrCallbackInfo final {
        YmErrCallbackFn fn = nullptr;
        void* user = nullptr;
    };

    // Static class encapsulating Yama API process-wide and thread-local data/functionality.
    class Global final {
    public:
        Global() = delete;


        static void setErrCallback(YmErrCallbackFn fn, void* user) noexcept;
        static bool pathIsLegal(std::string_view path);

        template<typename... Args>
        static inline void raiseErr(YmErrCode code, std::format_string<Args...> fmt, Args&&... args) {
            if (!_errCallbackInfo.fn) {
                return;
            }
            std::string errmsg(std::format("[Yama] [{}] {}", ymFmtYmErrCode(code), std::format(fmt, std::forward<Args>(args)...)));
            _errCallbackInfo.fn(code, errmsg.c_str(), _errCallbackInfo.user);
        }


    private:
        thread_local static ErrCallbackInfo _errCallbackInfo;
        static const std::regex _legalPathPattern;
    };
}

