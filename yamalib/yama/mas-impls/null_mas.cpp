

#include "null_mas.h"


yama::null_mas::null_mas(std::shared_ptr<debug> dbg) 
    : mas(dbg) {}

std::string yama::null_mas::report() const {
    return "";
}

yama::mas_allocator_info yama::null_mas::get_info() noexcept {
    return mas_allocator_info{
        this,
        [](mas_allocator_info::client_ptr_t, size_t) -> void* { return nullptr; },
        [](mas_allocator_info::client_ptr_t, void*, size_t) noexcept -> void {},
        [](mas_allocator_info::client_ptr_t) noexcept -> size_t { return 0; },
    };
}
