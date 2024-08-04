

#include "call_frame_mem.h"

#include "../core/context.h"


bool yama::internal::call_frame_mem_elem::good() const noexcept {
    return *this != call_frame_mem_elem{};
}

yama::object_ref& yama::internal::call_frame_mem_elem::view() const noexcept {
    YAMA_ASSERT(good());
    return view_unchecked();
}

yama::object_ref& yama::internal::call_frame_mem_elem::view_unchecked() const noexcept {
    return *(object_ref*)this;
}

yama::internal::call_frame_mem_elem yama::internal::call_frame_mem_elem::make(const object_ref& x) noexcept {
    static_assert(std::is_trivially_copyable_v<object_ref>);
    static_assert(sizeof(call_frame_mem_elem) == sizeof(object_ref));
    call_frame_mem_elem result{};
    std::memcpy((void*)&result, (const void*)&x, sizeof(x));
    return result;
}

