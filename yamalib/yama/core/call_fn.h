

#pragma once


//


namespace yama {


    class context;
    class const_table;


    // call_fn defines the behaviour of a call, being provided a 
    // context to query/manipulate

    using call_fn = void(*)(context&, const_table);


    // special no-op call_fn function

    void noop_call_fn(context&, const_table);

    // special bytecode interpreter call_fn function

    void bcode_call_fn(context& ctx, const_table consts);
}

