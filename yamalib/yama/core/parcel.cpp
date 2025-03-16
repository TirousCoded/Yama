

#include "parcel.h"


bool yama::parcel_metadata::is_dep_name(const str& name) const noexcept {
    return dep_names.contains(name);
}

bool yama::parcel_metadata::is_self_or_dep_name(const str& name) const noexcept {
    return name == self_name || is_dep_name(name);
}

void yama::parcel_metadata::add_dep_name(const str& name) {
    if (is_self_or_dep_name(name)) return;
    dep_names.insert(name);
}

yama::import_result::import_result(module_info&& x)
    : import_result(make_res<module_info>(std::forward<module_info>(x))) {}

yama::import_result::import_result(res<module_info>&& x)
    : _data(std::in_place_type<res<module_info>>, std::forward<res<module_info>>(x)) {}

yama::import_result::import_result(taul::source_code&& x)
    : _data(std::in_place_type<taul::source_code>, std::forward<taul::source_code>(x)) {}

bool yama::import_result::holds_module() const noexcept {
    return std::holds_alternative<res<module_info>>(_data);
}

bool yama::import_result::holds_source() const noexcept {
    return std::holds_alternative<taul::source_code>(_data);
}

yama::res<yama::module_info>& yama::import_result::get_module() {
    YAMA_ASSERT(holds_module());
    return std::get<res<module_info>>(_data);
}

const yama::res<yama::module_info>& yama::import_result::get_module() const {
    YAMA_ASSERT(holds_module());
    return std::get<res<module_info>>(_data);
}

taul::source_code& yama::import_result::get_source() {
    YAMA_ASSERT(holds_source());
    return std::get<taul::source_code>(_data);
}

const taul::source_code& yama::import_result::get_source() const {
    YAMA_ASSERT(holds_source());
    return std::get<taul::source_code>(_data);
}

yama::parcel::parcel(std::shared_ptr<debug> dbg)
    : api_component(dbg),
    _id(_acquire_id()) {}

yama::parcel_id yama::parcel::id() const noexcept {
    return _id;
}

std::atomic<yama::parcel_id> yama::parcel::_next_id = 0;

yama::parcel_id yama::parcel::_acquire_id() noexcept {
    return _next_id.fetch_add(1);
}

yama::null_parcel::null_parcel()
    : parcel() {}

