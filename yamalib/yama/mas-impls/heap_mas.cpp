

#include "heap_mas.h"

#include <format>


yama::heap_mas::heap_mas(std::shared_ptr<debug> dbg) 
    : mas(dbg) {}

std::string yama::heap_mas::report() const {
    return std::format("memory in use: {} bytes", _mem_in_use);
}

yama::mas_allocator_info yama::heap_mas::get_info() noexcept {
    return mas_allocator_info{
        this,
        [](mas_allocator_info::client_ptr_t client_ptr, size_t bytes) -> void* {
            auto result = (void*)std::malloc(bytes);
            if (result) {
                auto self = (heap_mas*)client_ptr;
                YAMA_DEREF_SAFE(self) {
                    self->_mem_in_use += bytes;
                }
            }
            return result;
        },
        [](mas_allocator_info::client_ptr_t client_ptr, void* block, size_t bytes) noexcept -> void {
            auto self = (heap_mas*)client_ptr;
            YAMA_DEREF_SAFE(self) {
                self->_mem_in_use -= bytes;
            }
            std::free(block);
        },
        [](mas_allocator_info::client_ptr_t) noexcept -> size_t { return std::numeric_limits<size_t>::max(); },
    };
}

