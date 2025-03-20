

#pragma once


#include <format>

#include "../core/asserts.h"
#include "../core/debug.h"

#include "ast.h"


namespace yama::internal {


    // TODO: when needed, add support for things like warnings

    class error_reporter final {
    public:
        inline error_reporter(
            std::shared_ptr<debug> dbg,
            const taul::source_code& src)
            : _dbg(std::move(dbg)),
            _src(&src) {}


        inline bool is_fatal() const noexcept { return _err; }

        inline void fatal() { _err = true; }


        template<typename... Args>
        inline void error(
            const ast_node& where,
            dsignal dsig,
            std::format_string<Args...> fmt,
            Args&&... args);
        

    private:
        std::shared_ptr<debug> _dbg;
        const taul::source_code* _src;

        bool _err = false;
    };

    template<typename... Args>
    inline void error_reporter::error(const ast_node& where, dsignal dsig, std::format_string<Args...> fmt, Args&&... args) {
        YAMA_RAISE(_dbg, dsig);
        YAMA_LOG(
            _dbg, compile_error_c,
            "error: {} {}",
            yama::deref_assert(_src).location_at(where.low_pos()),
            std::format(fmt, std::forward<Args&&>(args)...));
        fatal();
    }
}

