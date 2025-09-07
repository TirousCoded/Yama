

#include "item_instance.h"


yama::internal::item_mem yama::internal::get_item_mem(const internal::item_instance& x) noexcept {
    return x._mem;
}

yama::internal::item_instance::item_instance(str fullname, mid_t mid, module::item info)
    : _mem(_create_mem(fullname, mid, info)) {}

yama::internal::item_instance::item_instance(str new_fullname, const item_instance& other)
    : _mem(_create_mem(new_fullname, other)) {}

yama::internal::item_instance::~item_instance() noexcept {
    _destroy_mem(_mem); // RAII cleanup.
}

bool yama::internal::item_instance::complete() const noexcept {
    return _mem->stubs == 0;
}

const yama::str& yama::internal::item_instance::fullname() const noexcept {
    return _mem->fullname;
}

yama::internal::item_mem yama::internal::item_instance::_create_mem(str fullname, mid_t mid, module::item info) {
    internal::item_mem_header header{
        .fullname = fullname,
        .len = info.consts().size(),
        .stubs = info.consts().size(),
        .info = info,
        .mid = mid,
        .kind = info.kind(),
        .ptype = info.ptype(),
        .cf = info.call_fn(),
        .max_locals = info.max_locals(),
    };
    return internal::item_mem::create(std::allocator<void>{}, std::move(header));
}

yama::internal::item_mem yama::internal::item_instance::_create_mem(str new_fullname, const item_instance& other) {
    auto result = internal::item_mem::clone(std::allocator<void>{}, other._mem);
    result->fullname = new_fullname;
    return result;
}

void yama::internal::item_instance::_destroy_mem(internal::item_mem mem) noexcept {
    internal::item_mem::destroy(std::allocator<void>{}, mem);
}

