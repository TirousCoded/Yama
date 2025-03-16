

#include "yama_parcel.h"


using namespace yama::string_literals;


std::optional<yama::import_result> yama::internal::yama_parcel::import(const yama::str& relative_path) {
    if (relative_path != "") return std::nullopt; // currently only have root module
    return _get_root_modinf();
}

yama::res<yama::module_info> yama::internal::yama_parcel::_get_root_modinf() {
    if (!_root_modinf) {
        auto our_root_modinf =
            module_factory()
            .add_primitive_type("None"_str, {}, yama::ptype::none)
            .add_primitive_type("Int"_str, {}, yama::ptype::int0)
            .add_primitive_type("UInt"_str, {}, yama::ptype::uint)
            .add_primitive_type("Float"_str, {}, yama::ptype::float0)
            .add_primitive_type("Bool"_str, {}, yama::ptype::bool0)
            .add_primitive_type("Char"_str, {}, yama::ptype::char0)
            .done();
        _root_modinf = std::make_shared<module_info>(std::move(our_root_modinf));
    }
    return res(_root_modinf);
}

