

#include "parcel.h"

#include "domain.h"


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

const yama::str& yama::parcel_services::install_name() const noexcept {
    return _install_name;
}

std::shared_ptr<const yama::module_info> yama::parcel_services::compile(const taul::source_code& src) {
    return deref_assert(_client).do_ps_compile(src, install_name());
}

yama::parcel_services::parcel_services(domain& client, const str& install_name)
    : _client(&client),
    _install_name(install_name) {}

yama::parcel::parcel(std::shared_ptr<debug> dbg)
    : api_component(dbg) {}

