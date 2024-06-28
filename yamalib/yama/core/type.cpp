

#include "type.h"


size_t yama::type::refs_view::size() const noexcept {
    return _mem->length();
}

std::optional<yama::type> yama::type::refs_view::ref(size_t index) const noexcept {
    if (index >= _mem.elems().size()) return std::nullopt;
    auto value = _mem.elems()[index];
    return
        value
        ? std::make_optional(type(internal::type_mem::from_anon_ref(value)))
        : std::nullopt;
}

std::optional<yama::type> yama::type::refs_view::operator[](size_t index) const noexcept {
    return ref(index);
}

yama::type::type(const type_instance& instance) noexcept 
    : _mem(instance._mem) {}

yama::type::type(type&& other) noexcept 
    : _mem(std::move(other._mem)) {}

yama::type& yama::type::operator=(type&& other) noexcept {
    if (this == &other) return *this;
    _mem = std::move(other._mem);
    return *this;
}

bool yama::type::complete() const noexcept {
    return _mem->stubs == 0;
}

yama::str yama::type::fullname() const noexcept {
    return _mem->fullname;
}

yama::kind yama::type::kind() const noexcept {
    return _mem->kind;
}

yama::type::refs_view yama::type::refs() const noexcept {
    return refs_view{ _mem };
}

std::span<const yama::str> yama::type::refsyms() const noexcept {
    return _mem->data.refsyms();
}

bool yama::type::operator==(const type& other) const noexcept {
    return _mem == other._mem;
}

yama::type::type(internal::type_mem mem) noexcept 
    : _mem(std::move(mem)) {}

yama::type_instance::type_instance(str fullname, const type_data& data) 
    : _mem(_create_mem(std::move(fullname), data)) {}

yama::type_instance::type_instance(str new_fullname, const type_instance& other) 
    : _mem(_create_mem(std::move(new_fullname), other)) {}

yama::type_instance::~type_instance() noexcept {
    _destroy_mem(_mem); // RAII cleanup of _mem
}

void yama::type_instance::put_ref(size_t index, type x) noexcept {
    if (index >= _mem.elems().size()) return;
    // decr stubs if we're assigning to a stub
    if (!_mem.elems()[index]) _mem->stubs--;
    _mem.elems()[index] = x._mem.anon_ref();
}

yama::internal::type_mem yama::type_instance::_create_mem(str fullname, const type_data& data) {
    internal::type_mem_header header{
        .fullname = fullname,
        .data = data,
        .refs = data.refsyms().size(),
        .kind = data.kind(),
        .stubs = data.refsyms().size(),
    };
    return internal::type_mem::create(std::allocator<void>(), std::move(header));
}

yama::internal::type_mem yama::type_instance::_create_mem(str new_fullname, const type_instance& other) {
    auto result = internal::type_mem::clone(std::allocator<void>(), other._mem);
    result->fullname = std::move(new_fullname);
    return result;
}

void yama::type_instance::_destroy_mem(internal::type_mem mem) noexcept {
    internal::type_mem::destroy(std::allocator<void>(), mem);
}

