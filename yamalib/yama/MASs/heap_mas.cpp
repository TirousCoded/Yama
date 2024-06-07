

#include "heap_mas.h"

#include <format>


std::string yama::heap_mas::report() const {
    return std::format("memory in use: {} bytes", _mem_in_use);
}

yama::mas_allocator_info yama::heap_mas::get_info() noexcept {
    return mas_allocator_info{
        (void*)this,
        [](mas_allocator_info::info_ptr_t info_ptr, size_t bytes) -> void* {
            auto result = (void*)std::malloc(bytes);
            if (result) {
                auto self = (heap_mas*)info_ptr;
                YAMA_DEREF_SAFE(self) {
                    self->_mem_in_use += bytes;
                }
            }
            return result;
        },
        [](mas_allocator_info::info_ptr_t info_ptr, void* block, size_t bytes) noexcept -> void {
            auto self = (heap_mas*)info_ptr;
            YAMA_DEREF_SAFE(self) {
                self->_mem_in_use -= bytes;
            }
            std::free(block);
        },
        [](mas_allocator_info::info_ptr_t) noexcept -> size_t { return std::numeric_limits<size_t>::max(); },
    };
}

