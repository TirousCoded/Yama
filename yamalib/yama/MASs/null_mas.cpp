

#include "null_mas.h"


std::string yama::null_mas::report() const {
    return "";
}

yama::mas_allocator_info yama::null_mas::get_info() noexcept {
    return mas_allocator_info{
        nullptr,
        [](mas_allocator_info::info_ptr_t, size_t) -> void* { return nullptr; },
        [](mas_allocator_info::info_ptr_t, void*, size_t) noexcept -> void {},
        [](mas_allocator_info::info_ptr_t) noexcept -> size_t { return 0; },
    };
}
