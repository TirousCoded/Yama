

#pragma once


#include <format>

#include "const_type.h"


namespace yama {


    // IMPORTANT:
    //      this code is provided to allow for the end-user to define their own
    //      types, which they can then push to a yama::domain
    //
    //      this is part of the frontend, but being technical and niche, should
    //      avoid being liberally exposed on the interfaces of things like 
    //      yama::type, yama::callsig, etc.


    struct const_table_info;
    class const_table;


    // yama::callsig_info encapsulates a 'call signature' of a function-like type

    // yama::callsig_info encapsulate a list of parameter types, and a return 
    // type, w/ Yama functions being expected to use unit types to specify
    // situations where a function should return *nothing*

    // yama::callsig_info encapsulates type info in the form of constant table
    // indices, which specify types relative to some constant table

    // in Yama, methods are called as just regular functions but w/ their first
    // parameter being the object they're being called in relation to, w/ this
    // meaning that they use otherwise normal call signatures, w/ any kind of
    // syntax used to differentiate methods from other functions being beyond
    // the scope of call signature syntax/formatting


    struct callsig_info final {
        std::vector<const_t>    params; // the parameter types
        const_t                 ret;    // the return type


        // callsig_info is compared by value, comparing raw constant index values

        bool operator==(const callsig_info&) const noexcept = default;


        // behaviour is undefined if consts provided is inappropriate to use w/ *this

        std::string fmt(const const_table_info& consts) const;
        std::string fmt(const const_table& consts) const;
    };


    // TODO: make_callsig has not been unit tested

    inline callsig_info make_callsig(std::vector<const_t>&& params, const_t ret) {
        return callsig_info{ std::forward<decltype(params)&&>(params), ret };
    }
}

