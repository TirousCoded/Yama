

#include "domain.h"


using namespace yama::string_literals;


yama::domain::domain(std::shared_ptr<debug> dbg)
    : api_component(dbg) {}

bool yama::domain::setup_domain() {
    yama::primitive_info _None_info{
        "None"_str,
        std::nullopt,
        {},
        ptype::none,
    };
    yama::primitive_info _Int_info{
        "Int"_str,
        std::nullopt,
        {},
        ptype::int0,
    };
    yama::primitive_info _UInt_info{
        "UInt"_str,
        std::nullopt,
        {},
        ptype::uint,
    };
    yama::primitive_info _Float_info{
        "Float"_str,
        std::nullopt,
        {},
        ptype::float0,
    };
    yama::primitive_info _Bool_info{
        "Bool"_str,
        std::nullopt,
        {},
        ptype::bool0,
    };
    yama::primitive_info _Char_info{
        "Char"_str,
        std::nullopt,
        {},
        ptype::char0,
    };

    if (!push(_None_info)) return false;
    if (!push(_Int_info)) return false;
    if (!push(_UInt_info)) return false;
    if (!push(_Float_info)) return false;
    if (!push(_Bool_info)) return false;
    if (!push(_Char_info)) return false;

    const auto _None = load("None"_str);
    const auto _Int = load("Int"_str);
    const auto _UInt = load("UInt"_str);
    const auto _Float = load("Float"_str);
    const auto _Bool = load("Bool"_str);
    const auto _Char = load("Char"_str);

    if (!_None) return false;
    if (!_Int) return false;
    if (!_UInt) return false;
    if (!_Float) return false;
    if (!_Bool) return false;
    if (!_Char) return false;

    _quick_access = std::make_optional(_quick_access_t{
        .none = _None.value(),
        .int0 = _Int.value(),
        .uint = _UInt.value(),
        .float0 = _Float.value(),
        .bool0 = _Bool.value(),
        .char0 = _Char.value(),
        });

    return true;
}

void yama::domain::fail_domain_setup() {
    throw domain_setup_error("domain setup failed!");
}

yama::type yama::domain::load_none() {
    YAMA_ASSERT(_quick_access);
    return _quick_access->none;
}

yama::type yama::domain::load_int() {
    YAMA_ASSERT(_quick_access);
    return _quick_access->int0;
}

yama::type yama::domain::load_uint() {
    YAMA_ASSERT(_quick_access);
    return _quick_access->uint;
}

yama::type yama::domain::load_float() {
    YAMA_ASSERT(_quick_access);
    return _quick_access->float0;
}

yama::type yama::domain::load_bool() {
    YAMA_ASSERT(_quick_access);
    return _quick_access->bool0;
}

yama::type yama::domain::load_char() {
    YAMA_ASSERT(_quick_access);
    return _quick_access->char0;
}
