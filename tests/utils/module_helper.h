

#pragma once


#include <yama/core/module.h>


// This is used to help build modules for unit tests w/out coupling test code to the
// Yama frontend (ie. so we can more easily change said frontend.)
class module_helper final {
public:
    module_helper() = default;


    yama::module result() const;

    void add_primitive(
        const yama::str& name,
        yama::const_table consts,
        yama::ptype ptype);
    void add_function(
        const yama::str& name,
        yama::const_table consts,
        yama::callsig callsig,
        size_t max_locals,
        yama::call_fn call_fn);
    void add_function(
        const yama::str& name,
        yama::const_table consts,
        yama::callsig callsig,
        size_t max_locals,
        yama::bc::code bcode);
    void add_method(
        const yama::str& owner_name,
        const yama::str& member_name,
        yama::const_table consts,
        yama::callsig callsig,
        size_t max_locals,
        yama::call_fn call_fn);
    void add_method(
        const yama::str& owner_name,
        const yama::str& member_name,
        yama::const_table consts,
        yama::callsig callsig,
        size_t max_locals,
        yama::bc::code bcode);
    void add_struct(
        const yama::str& name,
        yama::const_table consts);

    void bind_bcode(
        const yama::str& name,
        yama::bc::code bcode,
        yama::bc::syms bsyms = {});


private:
    yama::module _m;
};

