

#include "module_helper.h"

#include <gtest/gtest.h>


yama::module module_helper::result() const {
    return _m;
}

void module_helper::add_primitive(
    const yama::str& name,
    yama::const_table consts,
    yama::ptype ptype) {
    ASSERT_TRUE(_m.add_primitive(name, std::move(consts), ptype));
}

void module_helper::add_function(
    const yama::str& name,
    yama::const_table consts,
    yama::callsig callsig,
    size_t max_locals,
    yama::call_fn call_fn) {
    ASSERT_TRUE(_m.add_function(name, std::move(consts), std::move(callsig), max_locals, call_fn));
}

void module_helper::add_function(
    const yama::str& name,
    yama::const_table consts,
    yama::callsig callsig,
    size_t max_locals,
    yama::bc::code bcode) {
    ASSERT_TRUE(_m.add_function(name, std::move(consts), std::move(callsig), max_locals, yama::bcode_call_fn));
    ASSERT_TRUE(_m.bind_bcode(name, std::move(bcode)));
}

void module_helper::add_method(
    const yama::str& owner_name,
    const yama::str& member_name,
    yama::const_table consts,
    yama::callsig callsig,
    size_t max_locals,
    yama::call_fn call_fn) {
    ASSERT_TRUE(_m.add_method(owner_name, member_name, std::move(consts), std::move(callsig), max_locals, call_fn));
}

void module_helper::add_method(
    const yama::str& owner_name,
    const yama::str& member_name,
    yama::const_table consts,
    yama::callsig callsig,
    size_t max_locals,
    yama::bc::code bcode) {
    ASSERT_TRUE(_m.add_method(owner_name, member_name, std::move(consts), std::move(callsig), max_locals, yama::bcode_call_fn));
    ASSERT_TRUE(_m.bind_bcode(owner_name + "::" + member_name, std::move(bcode)));
}

void module_helper::add_struct(
    const yama::str& name,
    yama::const_table consts) {
    ASSERT_TRUE(_m.add_struct(name, std::move(consts)));
}

void module_helper::bind_bcode(
    const yama::str& name,
    yama::bc::code bcode,
    yama::bc::syms bsyms) {
    ASSERT_TRUE(_m.bind_bcode(name, std::move(bcode), std::move(bsyms)));
}

