

#include "type_info.h"

#include "asserts.h"
#include "kind-features.h"

#include "../internals/util.h"


yama::type_info::type_info(const type_info& other)
    : _info(other._info->clone()) {}

yama::type_info& yama::type_info::operator=(const type_info& other) {
    *this = std::move(type_info(other));
    return *this;
}

yama::str& yama::type_info::unqualified_name() noexcept {
    return _info->unqualified_name;
}

const yama::str& yama::type_info::unqualified_name() const noexcept {
    return _info->unqualified_name;
}

yama::const_table_info& yama::type_info::consts() noexcept {
    return _info->consts;
}

const yama::const_table_info& yama::type_info::consts() const noexcept {
    return _info->consts;
}

yama::kind yama::type_info::kind() const noexcept {
    return _info->get_kind();
}

std::optional<yama::ptype> yama::type_info::ptype() const noexcept {
    return _info->get_ptype();
}

const yama::callsig_info* yama::type_info::callsig() const noexcept {
    return _info->get_callsig();
}

size_t yama::type_info::max_locals() const noexcept {
    return _info->get_max_locals();
}

std::optional<yama::call_fn> yama::type_info::call_fn() const noexcept {
    return _info->get_call_fn();
}

const yama::bc::code* yama::type_info::bcode() const noexcept {
    return _info->get_bcode();
}

const yama::bc::syms* yama::type_info::bsyms() const noexcept {
    return _info->get_bsyms();
}

bool yama::type_info::uses_bcode() const noexcept {
    return _info->uses_bcode();
}

yama::str yama::type_info::owner_name() const noexcept {
    return internal::split_s(unqualified_name(), "::").first;
}

yama::str yama::type_info::member_name() const noexcept {
    return internal::split_s(unqualified_name(), "::").second;
}

bool yama::type_info::operator==(const yama::type_info& other) const noexcept {
    return
        unqualified_name() == other.unqualified_name() &&
        consts() == other.consts() &&
        kind() == other.kind() &&
        ptype() == other.ptype() &&
        ((bool)callsig() == (bool)other.callsig()) && (callsig() ? *callsig() == *other.callsig() : true) &&
        max_locals() == other.max_locals() &&
        call_fn() == other.call_fn() &&
        ((bool)bcode() == (bool)other.bcode()) && (bcode() ? *bcode() == *other.bcode() : true) &&
        ((bool)bsyms() == (bool)other.bsyms()) && (bsyms() ? *bsyms() == *other.bsyms() : true);
}

void yama::type_info::change_unqualified_name(str x) noexcept {
    _info->unqualified_name = std::move(x);
}

void yama::type_info::change_consts(const_table_info x) noexcept {
    _info->consts = std::move(x);
}

void yama::type_info::change_ptype(yama::ptype x) noexcept {
    _info->set_ptype(x);
}

void yama::type_info::change_callsig(callsig_info x) noexcept {
    _info->set_callsig(std::move(x));
}

void yama::type_info::change_call_fn(yama::call_fn x) noexcept {
    _info->set_call_fn(x);
}

void yama::type_info::change_max_locals(size_t x) noexcept {
    _info->set_max_locals(x);
}

void yama::type_info::change_bcode(bc::code x) noexcept {
    _info->set_bcode(std::move(x));
}

void yama::type_info::change_bsyms(bc::syms x) noexcept {
    _info->set_bsyms(std::move(x));
}

std::string yama::type_info::fmt(const char* tab) const {
    YAMA_ASSERT(tab);
    return _info->fmt(tab);
}

std::string yama::type_info::fmt_sym(size_t index) const {
    if (const auto syms = bsyms())  return syms->fmt_sym(index);
    else                            return internal::fmt_no_sym(index); // ensures valid result even if no bc::syms to use
}

yama::type_info::type_info(yama::res<yama::internal::base_info>&& info)
    : _info(std::forward<res<internal::base_info>>(info)) {}

yama::type_info yama::make_primitive(const str& unqualified_name, const_table_info consts, ptype ptype) {
    return internal::make_type_info<internal::primitive_info>(
        unqualified_name,
        std::move(consts),
        ptype);
}

yama::type_info yama::make_function(const str& unqualified_name, const_table_info consts, callsig_info callsig, size_t max_locals, yama::call_fn call_fn) {
    return internal::make_type_info<internal::function_info>(
        unqualified_name,
        std::move(consts),
        std::move(callsig),
        max_locals,
        call_fn);
}

yama::type_info yama::make_function(const str& unqualified_name, const_table_info consts, callsig_info callsig, size_t max_locals, bc::code bcode, bc::syms bsyms) {
    return internal::make_type_info<internal::function_info>(
        unqualified_name,
        std::move(consts),
        std::move(callsig),
        max_locals,
        std::move(bcode),
        std::move(bsyms));
}

yama::type_info yama::make_method(const str& unqualified_name, const_table_info consts, callsig_info callsig, size_t max_locals, yama::call_fn call_fn) {
    return internal::make_type_info<internal::method_info>(
        unqualified_name,
        std::move(consts),
        std::move(callsig),
        max_locals,
        call_fn);
}

yama::type_info yama::make_method(const str& unqualified_name, const_table_info consts, callsig_info callsig, size_t max_locals, bc::code bcode, bc::syms bsyms) {
    return internal::make_type_info<internal::method_info>(
        unqualified_name,
        std::move(consts),
        std::move(callsig),
        max_locals,
        std::move(bcode),
        std::move(bsyms));
}

yama::type_info yama::make_struct(const str& unqualified_name, const_table_info consts) {
    return internal::make_type_info<internal::struct_info>(
        unqualified_name,
        std::move(consts));
}

yama::internal::base_info::base_info(const str& unqualified_name, const_table_info consts)
    : unqualified_name(unqualified_name),
    consts(std::move(consts)) {}

std::string yama::internal::base_info::fmt(const char* tab) const {
    YAMA_ASSERT(tab);
    std::string result{};
    result += "type_info";
    result += std::format("\n{}unqualified-name: {}", tab, unqualified_name);
    result += fmt_extension(consts, tab);
    result += std::format("\n{}", consts.fmt(tab));
    return result;
}

yama::internal::primitive_info::primitive_info(const str& unqualified_name, const_table_info consts, yama::ptype ptype)
    : base_info(unqualified_name, std::move(consts)),
    ptype(ptype) {}

yama::res<yama::internal::base_info> yama::internal::primitive_info::clone() const {
    return make_res<primitive_info>(unqualified_name, consts, ptype);
}

std::string yama::internal::primitive_info::fmt_extension(const const_table_info& consts, const char* tab) const {
    YAMA_ASSERT(tab);
    std::string result{};
    result += std::format("\n{}ptype: {}", tab, ptype);
    return result;
}

yama::internal::callable_type_info::callable_type_info(const str& unqualified_name, const_table_info consts, callsig_info callsig, size_t max_locals, yama::call_fn call_fn)
    : base_info(unqualified_name, std::move(consts)),
    callsig(std::move(callsig)),
    max_locals(max_locals),
    call_fn(call_fn) {}

yama::internal::callable_type_info::callable_type_info(const str& unqualified_name, const_table_info consts, callsig_info callsig, size_t max_locals, bc::code bcode, bc::syms bsyms)
    : base_info(unqualified_name, std::move(consts)),
    callsig(std::move(callsig)),
    max_locals(max_locals),
    call_fn(bcode_call_fn),
    bcode(std::move(bcode)),
    bsyms(std::move(bsyms)) {}

std::string yama::internal::callable_type_info::fmt_extension(const const_table_info& consts, const char* tab) const {
    YAMA_ASSERT(tab);
    std::string result{};
    result += std::format("\n{}callsig     : {}", tab, callsig.fmt(consts));
    result += std::format("\n{}call_fn     : {}", tab, uses_bcode() ? "bcode" : "C++");
    result += std::format("\n{}max_locals  : {}", tab, max_locals);
    result += std::format("\n{}", bcode.fmt_disassembly(tab));
    return result;
}

yama::internal::function_info::function_info(const str& unqualified_name, const_table_info consts, callsig_info callsig, size_t max_locals, yama::call_fn call_fn)
    : callable_type_info(unqualified_name, std::move(consts), std::move(callsig), max_locals, call_fn) {}

yama::internal::function_info::function_info(const str& unqualified_name, const_table_info consts, callsig_info callsig, size_t max_locals, bc::code bcode, bc::syms bsyms)
    : callable_type_info(unqualified_name, std::move(consts), std::move(callsig), max_locals, std::move(bcode), std::move(bsyms)) {}

yama::res<yama::internal::base_info> yama::internal::function_info::clone() const {
    auto result = make_res<function_info>(unqualified_name, consts, callsig, max_locals, bcode, bsyms);
    // gotta assign this one manually
    result->call_fn = call_fn;
    return result;
}

yama::internal::method_info::method_info(const str& unqualified_name, const_table_info consts, callsig_info callsig, size_t max_locals, yama::call_fn call_fn)
    : callable_type_info(unqualified_name, std::move(consts), std::move(callsig), max_locals, call_fn) {}

yama::internal::method_info::method_info(const str& unqualified_name, const_table_info consts, callsig_info callsig, size_t max_locals, bc::code bcode, bc::syms bsyms)
    : callable_type_info(unqualified_name, std::move(consts), std::move(callsig), max_locals, std::move(bcode), std::move(bsyms)) {}

yama::res<yama::internal::base_info> yama::internal::method_info::clone() const {
    auto result = make_res<method_info>(unqualified_name, consts, callsig, max_locals, bcode, bsyms);
    // gotta assign this one manually
    result->call_fn = call_fn;
    return result;
}

yama::internal::struct_info::struct_info(const str& unqualified_name, const_table_info consts)
    : base_info(unqualified_name, std::move(consts)) {}

yama::res<yama::internal::base_info> yama::internal::struct_info::clone() const {
    return make_res<struct_info>(unqualified_name, consts);
}

std::string yama::internal::struct_info::fmt_extension(const const_table_info& consts, const char* tab) const {
    YAMA_ASSERT(tab);
    return std::string{};
}

