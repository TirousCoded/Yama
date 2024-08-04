

#pragma once


//


namespace yama {


    class context;
    struct links_view;


    // call_fn defines the behaviour of a call, being provided a 
    // context to query/manipulate

    using call_fn = void(*)(context&, links_view);


    // no-op call_fn function

    void noop_call_fn(context&, links_view);
}

