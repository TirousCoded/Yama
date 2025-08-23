

#pragma once


#include "macros.h"


namespace yama {


    using module_id = uint16_t;
    using mid_t = module_id;

    using local_id = uint16_t;
    using lid_t = local_id;

    struct global_id final {
        lid_t lid = {}; // ID of the item.
        mid_t mid = {}; // ID of the module the item exists within.


        bool operator==(const global_id&) const noexcept = default;

        size_t hash() const noexcept;
        //std::string fmt() const noexcept;
    };

    using gid_t = global_id;
}

YAMA_SETUP_HASH(yama::global_id, x.hash());
//YAMA_SETUP_FORMAT(yama::global_id, x.fmt());

