

#pragma once


//


namespace yama::qs {


    // TODO: this is here for *completeness*, but doesn't have much going for it at the moment...

    // specializations require:
    //      using qtypes = ...;
    //          * an alias of the QTypes template argument
    //          * this defines the enum of different provider types for this query system
    //          * query systems are defined/differentiated by their QTypes enum type

    template<typename QTypes>
    struct system_traits {};
}

