

#include "parcel.h"


yama::parcel::services::services(std::shared_ptr<debug> dbg)
    : api_component(dbg) {}

yama::parcel::parcel(std::shared_ptr<debug> dbg)
    : api_component(dbg) {}

bool yama::dep_reqs::exists(const str& dep_name) const noexcept {
    return reqs.contains(dep_name);
}

const yama::dep_reqs::metadata& yama::dep_reqs::lookup(const str& dep_name) const noexcept {
    YAMA_ASSERT(exists(dep_name));
    return reqs.at(dep_name);
}

yama::dep_reqs& yama::dep_reqs::add(str dep_name) {
    YAMA_ASSERT(!exists(dep_name));
    reqs.insert({ dep_name, metadata{} });
    return *this;
}

