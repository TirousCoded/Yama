

#pragma once


//


namespace yama {


    class context;


    // the command API defines a low-level interface used to query/manipulate
    // the state of a context

    // this API is to be strictly procedural, w/ data values being transacted
    // by command API method calls, w/ these data values being intended to be
    // lightweight and more often than not opaque

    // the idea is that bytecode will essentially be a more-or-less 1-to-1
    // wrapper over the command API interface, extending it w/ things like
    // control flow, which the command API won't cover

    // likewise, the command API will also be used to impl more object-orientated
    // high-level objects and methods for the end-user of the Yama API to be
    // able to interact w/ contexts via

    // IMPORTANT:
    //      the command API should always ensure memory safety, and should 
    //      ensure that any state corruption that arises from its improper
    //      usage is restricted to the logical state of the Yama state
    //      encapsulated by it, but w/ the *virtual machine* itself ALWAYS
    //      being in a valid state, not compromising the external system
    //
    //      of course, things like C++ functions called by the command API
    //      which are defined w/ code from outside the virtual machine could
    //      corrupt system state in other ways, but this form of state corruption
    //      is beyond the scope of what the command API impl can reasonably be
    //      expected to be able to prevent


    struct command_api final {
        context* _ctx; // internal, do not use


        // TODO
    };
}

