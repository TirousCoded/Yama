

#pragma once


#include "command_api.h"


namespace yama {


    // call_fn defines the behaviour of a call, being provided a 
    // command API object to query/manipulate context state

    using call_fn = void(*)(command_api&);


    // no-op call_fn function

    void noop_call_fn(command_api&);
}

