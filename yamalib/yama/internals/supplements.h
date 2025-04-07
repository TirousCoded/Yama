

#pragma once


#include "../core/module_info.h"


namespace yama::internal {


    // make_supplements generates a module of useful fns which make Yama a lot
    // more usable given its current spartan state

    // these fn impls are temporary, w/ plans to replace them w/ a more proper
    // standard library later

    module_info make_supplements();
}

