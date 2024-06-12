

#pragma once


#include <memory>

#include "debug.h"


namespace yama {


    // api_component is the base class of all classes which describe 
    // Yama 'API components', which are macroscopic classes in the Yama
    // API frontend into which a yama::debug may be optionally injected

    class api_component {
    public:

        // this is immutable as otherwise an API component who's debug association
        // can be modified would need to deal w/ the nuances of then having to
        // also ensure that its subcomponents get updated

        const std::shared_ptr<debug> dbg;


        api_component(std::shared_ptr<debug> dbg = nullptr);

        virtual ~api_component() noexcept = default;
    };
}

