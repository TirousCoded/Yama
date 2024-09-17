

#pragma once


//


namespace yama {


    class context;


    // call_fn defines the behaviour of a call, being provided a 
    // context to query/manipulate

    using call_fn = void(*)(context&);


    void noop_call_fn(context&);    // special no-op call_fn function
    void bcode_call_fn(context&);   // special bytecode interpreter call_fn function
}

