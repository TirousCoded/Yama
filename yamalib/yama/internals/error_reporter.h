

#pragma once


#include <format>

#include "../core/asserts.h"
#include "../core/debug.h"

#include "safeptr.h"
#include "ast.h"


namespace yama::internal {


    class translation_unit;


    // TODO: when needed, add support for things like warnings

    class error_reporter final {
    public:
        safeptr<translation_unit> tu;


        error_reporter(translation_unit& tu);


        bool good() const noexcept;
        bool is_fatal() const noexcept;

        void fatal();


        template<typename... Args>
        inline void error(
            taul::source_pos where,
            dsignal dsig,
            std::format_string<Args...> fmt,
            Args&&... args);
        
        template<typename... Args>
        inline void error(
            const ast_node& where,
            dsignal dsig,
            std::format_string<Args...> fmt,
            Args&&... args);
        

    private:
        bool _err = false;


        std::shared_ptr<debug> _dbg();
        taul::source_location _loc_at(taul::source_pos where);
    };

    template<typename... Args>
    inline void error_reporter::error(taul::source_pos where, dsignal dsig, std::format_string<Args...> fmt, Args&&... args) {
        YAMA_RAISE(_dbg(), dsig);
        YAMA_LOG(
            _dbg(), compile_error_c,
            "error: {} {}",
            _loc_at(where),
            std::format(fmt, std::forward<Args>(args)...));
        fatal();
    }

    template<typename... Args>
    inline void error_reporter::error(const ast_node& where, dsignal dsig, std::format_string<Args...> fmt, Args&&... args) {
        error(where.low_pos(), dsig, fmt, std::forward<Args>(args)...);
    }
}

