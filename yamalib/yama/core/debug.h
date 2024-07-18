

#pragma once


#include <cstdint>
#include <string>
#include <format>
#include <memory>

#include "macros.h"


namespace yama {


    // Yama uses 'debug layers' to report diagnostics to the end-user

    // debug layers are placed *in between* API components and the end-user,
    // and they moniter interactions between them, as well as outputting
    // diagnostic logs generally

    // debug layers filter what to output w/ 'categories', which specify
    // what they should, and shouldn't, be sensitive to

    // quality-of-life macros are provided to help perform outputs to the
    // debug layer in a way that helps avoid unneeded code being executed
    // when a debug layer is not sensitive to a given category


    // enum of debug layer categories

    enum class debug_cat : uint32_t {
        // this enum is intended to define bit flags

        none            = 0,
        all             = uint32_t(-1),

        general         = 1 << 0,           // general is for all non-specific debug logs
        type_instant    = 1 << 1,           // type instantiation debug logs
    };


    // these are used to summarize usage of yama::debug_cat

    constexpr auto none_c = debug_cat::none;
    constexpr auto all_c = debug_cat::all;

    constexpr auto general_c = debug_cat::general;
    constexpr auto type_instant_c = debug_cat::type_instant;
}

constexpr yama::debug_cat operator|(yama::debug_cat lhs, yama::debug_cat rhs) noexcept {
    return yama::debug_cat(uint32_t(lhs) | uint32_t(rhs));
}

namespace yama {


    // check returns if cat contains the flags of expect

    constexpr bool check(debug_cat cat, debug_cat expect) noexcept {
        return debug_cat(uint32_t(cat) & uint32_t(expect)) == expect;
    }

    // TODO: maybe update the below later to be properly comprehensive

    // NOTE: some quick-n'-dirty (and notably non-comprehensive) static_assert-based
    //       unit tests of yama::check

    static_assert(check(none_c, none_c));
    static_assert(check(all_c, none_c));
    static_assert(check(all_c, all_c));

    static_assert(!check(none_c, all_c));

    static_assert(check(all_c, general_c));


    // the base class of Yama debug layers

    class debug : public std::enable_shared_from_this<debug> {
    public:

        // categories to filter by

        debug_cat cats;


        debug(debug_cat cats);

        virtual ~debug() noexcept = default;


        // has_cat returns if debug output will occur w/ cat

        bool has_cat(debug_cat cat) const noexcept;


        // log performs a debug log w/ category cat

        // no log is performed if has_cat(cat) == false

        template<typename... Args>
        inline void log(debug_cat cat, std::format_string<Args...> fmt, Args&&... args) {
            if (!has_cat(cat)) return;
            do_log(std::format(fmt, std::forward<Args&&>(args)...));
        }


    protected:

        virtual void do_log(const std::string& msg) = 0;
    };
}


// quality-of-life helper macros

// these expect debug_layer_ptr to be some kind of pointer

#define YAMA_LOG(debug_layer_ptr, cat, fmt, ...) \
YAMA_COND( \
((debug_layer_ptr) && (debug_layer_ptr)->has_cat((cat))), \
(debug_layer_ptr)->log((cat), (fmt), __VA_ARGS__))

