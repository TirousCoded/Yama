

#include "env.h"



const yama::internal::env& yama::internal::env_instance::get() const noexcept {
    return _env;
}

void yama::internal::env_instance::add(const yama::str& name, yama::parcel_id id) {
    YAMA_ASSERT(!_env.exists(name));
    YAMA_ASSERT(!_env.exists(id));
    _env._data->name_to_id.insert({ name, id });
    _env._data->id_to_name.insert({ id, name });
}

yama::internal::env::env()
    : _data(yama::make_res<_data_t>()) {}

size_t yama::internal::env::count() const noexcept {
    return _data->name_to_id.size();
}

bool yama::internal::env::exists(const str& name) const noexcept {
    return _data->name_to_id.contains(name);
}

bool yama::internal::env::exists(parcel_id id) const noexcept {
    return _data->id_to_name.contains(id);
}

std::optional<yama::parcel_id> yama::internal::env::id(const str& name) const noexcept {
    const auto it = _data->name_to_id.find(name);
    return
        it != _data->name_to_id.end()
        ? std::make_optional(it->second)
        : std::nullopt;
}

std::optional<yama::str> yama::internal::env::name(parcel_id id) const noexcept {
    const auto it = _data->id_to_name.find(id);
    return
        it != _data->id_to_name.end()
        ? std::make_optional(it->second)
        : std::nullopt;
}

std::string yama::internal::env::fmt(const char* tab) const {
    YAMA_ASSERT(tab);
    std::string result{};
    result += std::format("env ({} mappings)\n", _data->id_to_name.size());
    for (const auto& [key, value] : _data->id_to_name) {
        result += std::format("{}{{ID={}}} {}\n", tab, key, value);
    }
    return result;
}

