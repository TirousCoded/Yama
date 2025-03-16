

#include "install_batch.h"

#include <taul/hashing.h>


size_t yama::dep_mapping_name::hash() const noexcept {
    return taul::hash(install_name, dep_name);
}

std::string yama::dep_mapping_name::fmt() const {
    return std::format("{}/{}", install_name, dep_name);
}

yama::install_batch& yama::install_batch::install(str install_name, res<parcel> x) {
    installs.insert(std::pair{ install_name, std::move(x) });
    return *this;
}

yama::install_batch& yama::install_batch::map_dep(str install_name, str dep_name, str mapped_to) {
    dep_mappings[{ install_name, dep_name }] = mapped_to;
    return *this;
}

