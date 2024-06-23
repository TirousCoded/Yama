

#pragma once


#include <memory>

#include "debug.h"


namespace yama {


    // api_component is the base class of all classes which describe 
    // Yama 'API components', which are macroscopic classes in the Yama
    // API frontend into which a yama::debug may be optionally injected

    class api_component {
    public:

        api_component(std::shared_ptr<debug> dbg = nullptr);

        virtual ~api_component() noexcept = default;


        // IMPORTANT:
        //      debug layer association is immutable as otherwise an API component who's 
        //      association can be modified isn't as straightforward to use w/ regards
        //      to propagating this association down to subcomponents

        // get_debug returns the debug layer associated w/ this API component, if any

        // dbg is get_debug, but w/ a shorter name

        std::shared_ptr<debug> get_debug() const noexcept;
        std::shared_ptr<debug> dbg() const noexcept;


    private:

        const std::shared_ptr<debug> _dbg;
    };
}

