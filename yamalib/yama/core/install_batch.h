

#pragma once


#include <unordered_map>

#include "general.h"
#include "res.h"
#include "parcel.h"


namespace yama {


    struct dep_mapping_name final {
        str install_name, dep_name;


        bool operator==(const dep_mapping_name&) const noexcept = default;

        size_t hash() const noexcept;
        std::string fmt() const;
    };
}

YAMA_SETUP_HASH(yama::dep_mapping_name, x.hash());
YAMA_SETUP_FORMAT(yama::dep_mapping_name, x.fmt());

namespace yama {


    struct install_batch final {
        // installs maps install names to parcels to install

        std::unordered_map<str, res<parcel>> installs;

        // maps dep mapping names to the install names of the parcels mapped to

        std::unordered_map<dep_mapping_name, str> dep_mappings;


        // TODO: these have not been unit tested

        install_batch& install(str install_name, res<parcel> x);
        install_batch& map_dep(str install_name, str dep_name, str mapped_to);
    };
}

