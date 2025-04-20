

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

std::optional<std::span<const yama::str>> yama::type_info::param_names() const noexcept {
    return
        std::holds_alternative<function_info>(info)
        ? std::make_optional(std::span(std::get<function_info>(info).param_names))
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

size_t yama::type_info::max_locals() const noexcept {
    return
        std::holds_alternative<function_info>(info)
        ? std::get<function_info>(info).max_locals
        : 0;
}

const yama::bc::code* yama::type_info::bcode() const noexcept {
    return
        std::holds_alternative<function_info>(info)
        ? &(std::get<function_info>(info).bcode)
        : nullptr;
}

const yama::bc::syms* yama::type_info::bsyms() const noexcept {
    return
        std::holds_alternative<function_info>(info)
        ? &(std::get<function_info>(info).bsyms)
        : nullptr;
}

bool yama::type_info::uses_bcode() const noexcept {
    return
        std::holds_alternative<function_info>(info)
        ? std::get<function_info>(info).uses_bcode()
        : false;
}

std::string yama::primitive_info::fmt(const char* tab) const {
    YAMA_ASSERT(tab);
    std::string result{};
    result += "primitive_info";
    result += std::format("\n{}ptype: {}", tab, ptype);
    return result;
}

bool yama::function_info::uses_bcode() const noexcept {
    return call_fn == bcode_call_fn;
}

std::string yama::function_info::fmt(const const_table_info& consts, const char* tab) const {
    YAMA_ASSERT(tab);
    auto _fmt_param_names =
        [&]() -> std::string {
        std::string result{};
        for (size_t i = 0; i < param_names.size(); i++) {
            if (i >= 1) result += ", ";
            result += param_names[i];
        }
        return result;
        };
    std::string result{};
    result += "function_info";
    result += std::format("\n{}param names : {}", tab, _fmt_param_names());
    result += std::format("\n{}callsig     : {}", tab, callsig.fmt(consts));
    result += std::format("\n{}call_fn     : {}", tab, uses_bcode() ? "bcode" : "C++");
    result += std::format("\n{}max_locals  : {}", tab, max_locals);
    result += std::format("\n{}", bcode.fmt_disassembly(tab));
    return result;
}

std::string yama::type_info::fmt(const char* tab) const {
    YAMA_ASSERT(tab);
    std::string result{};
    result += "type_info";
    result += std::format("\n{}unqualified-name: {}", tab, unqualified_name);
    if (kind() == kind::primitive) {
        result += std::format("\n{}", std::get<primitive_info>(info).fmt(tab));
    }
    else if (kind() == kind::function) {
        result += std::format("\n{}", std::get<function_info>(info).fmt(consts, tab));
    }
    else YAMA_DEADEND;
    result += std::format("\n{}", consts.fmt(tab));
    return result;
}

std::string yama::type_info::fmt_sym(size_t index) const {
    if (const auto syms = bsyms()) {
        return syms->fmt_sym(index);
    }
    else {
        // ensure valid fmt string even if we don't have bc::syms to use
        return internal::fmt_no_sym(index);
    }
}

