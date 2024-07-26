

#include "domain.h"


using namespace yama::string_literals;


yama::domain::domain(std::shared_ptr<debug> dbg)
    : api_component(dbg) {}

void yama::domain::push_builtins() {
    yama::primitive_info _Int{
        "Int"_str,
        std::nullopt,
        {},
        ptype::int0,
    };
    yama::primitive_info _UInt{
        "UInt"_str,
        std::nullopt,
        {},
        ptype::uint,
    };
    yama::primitive_info _Float{
        "Float"_str,
        std::nullopt,
        {},
        ptype::float0,
    };
    yama::primitive_info _Bool{
        "Bool"_str,
        std::nullopt,
        {},
        ptype::bool0,
    };
    yama::primitive_info _Char{
        "Char"_str,
        std::nullopt,
        {},
        ptype::char0,
    };
    yama::primitive_info _None{
        "None"_str,
        std::nullopt,
        {},
        ptype::unit,
    };

    const bool success =
        push(_Int) &&
        push(_UInt) &&
        push(_Float) &&
        push(_Bool) &&
        push(_Char) &&
        push(_None);
}

