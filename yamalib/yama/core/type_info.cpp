

#include "type_info.h"

#include "asserts.h"
#include "kind-features.h"


yama::kind yama::type_info::kind() const noexcept {
    static_assert(kinds == 3);
    yama::kind result{};
    if (std::holds_alternative<primitive_info>(info))       result = kind::primitive;
    else if (std::holds_alternative<function_info>(info))   result = kind::function;
    else if (std::holds_alternative<struct_info>(info))     result = kind::struct0;
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

yama::primitive_info yama::primitive_info::create(yama::ptype ptype) {
    return primitive_info{
        .ptype = ptype,
    };
}

bool yama::function_info::uses_bcode() const noexcept {
    return call_fn == bcode_call_fn;
}

std::string yama::function_info::fmt(const const_table_info& consts, const char* tab) const {
    YAMA_ASSERT(tab);
    std::string result{};
    result += "function_info";
    result += std::format("\n{}callsig     : {}", tab, callsig.fmt(consts));
    result += std::format("\n{}call_fn     : {}", tab, uses_bcode() ? "bcode" : "C++");
    result += std::format("\n{}max_locals  : {}", tab, max_locals);
    result += std::format("\n{}", bcode.fmt_disassembly(tab));
    return result;
}

yama::function_info yama::function_info::create(callsig_info callsig, size_t max_locals, yama::call_fn call_fn) {
    return function_info{
        .callsig = std::move(callsig),
        .call_fn = call_fn,
        .max_locals = max_locals,
    };
}

yama::function_info yama::function_info::create(callsig_info callsig, size_t max_locals, bc::code bcode, bc::syms bsyms) {
    return function_info{
        .callsig = std::move(callsig),
        .call_fn = bcode_call_fn,
        .max_locals = max_locals,
        .bcode = std::move(bcode),
        .bsyms = std::move(bsyms),
    };
}

std::string yama::struct_info::fmt(const char* tab) const {
    YAMA_ASSERT(tab);
    std::string result{};
    result += "struct_info";
    return result;
}

yama::struct_info yama::struct_info::create() {
    return struct_info{};
}

std::string yama::type_info::fmt(const char* tab) const {
    YAMA_ASSERT(tab);
    std::string result{};
    result += "type_info";
    result += std::format("\n{}unqualified-name: {}", tab, unqualified_name);
    if (kind() == kind::primitive)      result += std::format("\n{}", std::get<primitive_info>(info).fmt(tab));
    else if (kind() == kind::function)  result += std::format("\n{}", std::get<function_info>(info).fmt(consts, tab));
    else if (kind() == kind::struct0)   result += std::format("\n{}", std::get<struct_info>(info).fmt(tab));
    else                                YAMA_DEADEND;
    result += std::format("\n{}", consts.fmt(tab));
    return result;
}

std::string yama::type_info::fmt_sym(size_t index) const {
    if (const auto syms = bsyms())  return syms->fmt_sym(index);
    else                            return internal::fmt_no_sym(index); // ensures valid result even if no bc::syms to use
}

yama::type_info yama::type_info::create(const str& unqualified_name, const_table_info consts, info_t info) {
    return type_info{
        .unqualified_name = unqualified_name,
        .consts = std::move(consts),
        .info = std::move(info),
    };
}

yama::type_info yama::make_primitive(const str& unqualified_name, const_table_info consts, ptype ptype) {
    return type_info::create(
        unqualified_name, std::move(consts),
        primitive_info::create(ptype));
}

yama::type_info yama::make_function(const str& unqualified_name, const_table_info consts, callsig_info callsig, size_t max_locals, yama::call_fn call_fn) {
    return type_info::create(
        unqualified_name, std::move(consts),
        function_info::create(std::move(callsig), max_locals, call_fn));
}

yama::type_info yama::make_function(const str& unqualified_name, const_table_info consts, callsig_info callsig, size_t max_locals, bc::code bcode, bc::syms bsyms) {
    return type_info::create(
        unqualified_name, std::move(consts),
        function_info::create(std::move(callsig), max_locals, std::move(bcode), std::move(bsyms)));
}

yama::type_info yama::make_struct(const str& unqualified_name, const_table_info consts) {
    return type_info::create(
        unqualified_name, std::move(consts),
        struct_info::create());
}
