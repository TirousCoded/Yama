

#include "type_info.h"

#include "asserts.h"
#include "kind-features.h"


yama::kind yama::type_info::kind() const noexcept {
    static_assert(kinds == 2);
    yama::kind result{};
    if (std::holds_alternative<primitive_info>(info))       result = kind::primitive;
    else if (std::holds_alternative<function_info>(info))   result = kind::function;
    else                                                    YAMA_DEADEND;
    return result;
}

std::optional<yama::ptype> yama::type_info::ptype() const noexcept {
    return
        std::holds_alternative<primitive_info>(info)
        ? std::make_optional(std::get<primitive_info>(info).ptype)
        : std::nullopt;
}

const yama::callsig_info* yama::type_info::callsig() const noexcept {
    return
        std::holds_alternative<function_info>(info)
        ? &(std::get<function_info>(info).callsig)
        : nullptr;
}

std::optional<yama::call_fn> yama::type_info::call_fn() const noexcept {
    return
        std::holds_alternative<function_info>(info)
        ? std::make_optional(std::get<function_info>(info).call_fn)
        : std::nullopt;
}

size_t yama::type_info::locals() const noexcept {
    return
        std::holds_alternative<function_info>(info)
        ? std::get<function_info>(info).locals
        : 0;
}

const yama::bc::code* yama::type_info::bcode() const noexcept {
    return
        std::holds_alternative<function_info>(info)
        ? &(std::get<function_info>(info).bcode)
        : nullptr;
}

