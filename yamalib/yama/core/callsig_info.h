

#pragma once


#include <format>

#include "link_index.h"


namespace yama {


    struct linksym;


    // yama::callsig_info encapsulates a 'call signature' of a function-like type

    // yama::callsig_info encapsulate a list of parameter types, and a return 
    // type, w/ Yama functions being expected to use unit types to specify
    // situations where a function should return *nothing*

    // yama::callsig_info encapsulates type info in the form of link indices, which
    // specify types relative to some link (symbol) table

    // in Yama, methods are called as just regular functions but w/ their first
    // parameter being the object they're being called in relation to, w/ this
    // meaning that they use otherwise normal call signatures, w/ any kind of
    // syntax used to differentiate methods from other functions being beyond
    // the scope of call signature syntax/formatting


    struct callsig_info final {
        std::vector<link_index> params; // the parameter types
        link_index              ret;    // the return type


        // verify_indices returns if the callsig_info contains no 
        // indices out-of-bounds of linksyms

        bool verify_indices(std::span<const linksym> linksyms) const noexcept;


        // callsig_info is compared by value, comparing raw link index values

        bool operator==(const callsig_info& other) const noexcept;


        std::string fmt(std::span<const linksym> linksyms) const;


    private:

        std::string _fmt_index(std::span<const linksym> linksyms, link_index index) const;
    };


    // TODO: make_callsig_info has not been unit tested

    inline callsig_info make_callsig_info(std::vector<link_index>&& params, link_index ret) {
        return callsig_info{ std::forward<decltype(params)&&>(params), ret };
    }
}

