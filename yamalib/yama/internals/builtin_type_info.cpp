

#include "builtin_type_info.h"


using namespace yama::string_literals;


yama::module_info yama::internal::get_builtin_type_info() {
    return
        module_factory()
        .add_primitive_type("None"_str, {}, yama::ptype::none)
        .add_primitive_type("Int"_str, {}, yama::ptype::int0)
        .add_primitive_type("UInt"_str, {}, yama::ptype::uint)
        .add_primitive_type("Float"_str, {}, yama::ptype::float0)
        .add_primitive_type("Bool"_str, {}, yama::ptype::bool0)
        .add_primitive_type("Char"_str, {}, yama::ptype::char0)
        .done();
}

