

#include "type_instance.h"


#define _DUMP_LOG 0


yama::internal::type_mem yama::internal::get_type_mem(const internal::type_instance& x) noexcept {
    return x._mem;
}

yama::internal::type_instance::type_instance(str fullname, res<type_info> info)
    : _mem(_create_mem(fullname, info)) {}

yama::internal::type_instance::type_instance(str new_fullname, const type_instance& other)
    : _mem(_create_mem(new_fullname, other)) {}

yama::internal::type_instance::~type_instance() noexcept {
#if _DUMP_LOG
    std::cerr << std::format("~type_instance @ {}\n", (void*)this);
#endif
    _destroy_mem(_mem); // RAII cleanup of _mem
}

bool yama::internal::type_instance::complete() const noexcept {
    return _mem->stubs == 0;
}

const yama::str& yama::internal::type_instance::fullname() const noexcept {
    return _mem->fullname;
}

yama::internal::type_mem yama::internal::type_instance::_create_mem(str fullname, res<type_info> info) {
    internal::type_mem_header header{
        .fullname = fullname,
        .len = info->consts().size(),
        .stubs = info->consts().size(),
        .info = info,
        .kind = info->kind(),
        .ptype = info->ptype(),
    };
    return internal::type_mem::create(std::allocator<void>{}, std::move(header));
}

yama::internal::type_mem yama::internal::type_instance::_create_mem(str new_fullname, const type_instance& other) {
    auto result = internal::type_mem::clone(std::allocator<void>{}, other._mem);
    result->fullname = new_fullname;
    return result;
}

void yama::internal::type_instance::_destroy_mem(internal::type_mem mem) noexcept {
    internal::type_mem::destroy(std::allocator<void>{}, mem);
}

