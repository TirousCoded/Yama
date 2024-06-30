

#pragma once


#include <format>

#include "link_index.h"


namespace yama {


    // yama::callsig encapsulates a 'call signature' of a function-like type

    // yama::callsig encapsulate a list of argument types, and a return 
    // type, w/ Yama functions being expected to use unit types to specify
    // situations where a function should return *nothing*

    // yama::callsig encapsulates type info in the form of link indices, which
    // specify types relative to some link (symbol) table

    // in Yama, methods are called as just regular functions but w/ their first
    // argument being the object they're being called in relation to, w/ this
    // meaning that they use otherwise normal call signatures, w/ any kind of
    // syntax used to differentiate methods from other functions being beyond
    // the scope of call signature syntax/formatting

    struct callsig final {
        std::vector<link_index> args;   // the argument types
        link_index              ret;    // the return type


        // TODO: operator== has not been unit tested

        inline bool operator==(const callsig& other) const noexcept {
            return 
                args == other.args && 
                ret == other.ret;
        }


        template<link_formatter Formatter>
        inline std::string fmt(Formatter fmt) const {
            std::string args_s{};
            bool first = true;
            for (const auto& I : args) {
                if (!first) {
                    args_s += ", ";
                }
                args_s += fmt_link(I, fmt);
                first = false;
            }
            std::string ret_s = fmt_link(ret, fmt);
            return std::format("fn({}) -> {}", args_s, ret_s);
        }
    };


    // TODO: make_callsig has not been unit tested

    inline callsig make_callsig(std::vector<link_index>&& args, link_index ret) {
        return callsig{ std::forward<decltype(args)&&>(args), ret};
    }
}

