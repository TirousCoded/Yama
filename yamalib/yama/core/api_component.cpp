

#include "api_component.h"


yama::api_component::api_component(std::shared_ptr<yama::debug> dbg)
    : _dbg(dbg) {}

std::shared_ptr<yama::debug> yama::api_component::get_debug() const noexcept {
    return _dbg;
}

std::shared_ptr<yama::debug> yama::api_component::dbg() const noexcept {
    return get_debug();
}

