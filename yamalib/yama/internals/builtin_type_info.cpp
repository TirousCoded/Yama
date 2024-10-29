

#include "builtin_type_info.h"


using namespace yama::string_literals;


yama::internal::builtin_type_info yama::internal::get_builtin_type_info() noexcept {
    return builtin_type_info{
        type_info{
            .fullname = "None"_str,
            .consts = {},
            .info = primitive_info{
                .ptype = ptype::none,
            },
        },
        type_info{
            .fullname = "Int"_str,
            .consts = {},
            .info = primitive_info{
                .ptype = ptype::int0,
            },
        },
        type_info{
            .fullname = "UInt"_str,
            .consts = {},
            .info = primitive_info{
                .ptype = ptype::uint,
            },
        },
        type_info{
            .fullname = "Float"_str,
            .consts = {},
            .info = primitive_info{
                .ptype = ptype::float0,
            },
        },
        type_info{
            .fullname = "Bool"_str,
            .consts = {},
            .info = primitive_info{
                .ptype = ptype::bool0,
            },
        },
        type_info{
            .fullname = "Char"_str,
            .consts = {},
            .info = primitive_info{
                .ptype = ptype::char0,
            },
        },
    };
}

