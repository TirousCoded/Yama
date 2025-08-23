

#include "yama_parcel.h"

#include "supplements.h"


using namespace yama::string_literals;


std::optional<yama::import_result> yama::internal::yama_parcel::import(const yama::str& relative_path) {
    if (relative_path == "")            return _get_root_modinf();
    else if (relative_path == ".util")  return _get_util_modinf(); // TODO: temporary
    else                                return std::nullopt;
}

yama::res<yama::module> yama::internal::yama_parcel::_get_root_modinf() {
    if (!_root_modinf) {
        yama::module our_root_modinf{};
        our_root_modinf.add_primitive("None"_str, {}, yama::ptype::none);
        our_root_modinf.add_primitive("Int"_str, {}, yama::ptype::int0);
        our_root_modinf.add_primitive("UInt"_str, {}, yama::ptype::uint);
        our_root_modinf.add_primitive("Float"_str, {}, yama::ptype::float0);
        our_root_modinf.add_primitive("Bool"_str, {}, yama::ptype::bool0);
        our_root_modinf.add_primitive("Char"_str, {}, yama::ptype::char0);
        our_root_modinf.add_primitive("Type"_str, {}, yama::ptype::type);
        _root_modinf = std::make_shared<module>(std::move(our_root_modinf));
    }
    return res(_root_modinf);
}

yama::res<yama::module> yama::internal::yama_parcel::_get_util_modinf() {
    if (!_util_modinf) {
        _util_modinf = std::make_shared<module>(std::move(make_supplements()));
    }
    return res(_util_modinf);
}

