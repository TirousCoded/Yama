

#include "const_table.h"

#include "type.h"


yama::const_table::const_table(const yama::type& x) noexcept
    : _mem(internal::get_type_mem(x)) {}

size_t yama::const_table::size() const noexcept {
    return _mem->length();
}

bool yama::const_table::is_stub(const_t x) const noexcept {
    return
        x < size()
        ? _mem.elems()[x].holds_stub()
        : true;
}

std::optional<yama::const_type> yama::const_table::const_type(const_t x) const noexcept {
    return _mem->info->consts().const_type(x);
}

std::optional<yama::type> yama::const_table::type(const_t x) const noexcept {
    static_assert(const_types == 9);
    if (auto r = get<primitive_type_const>(x))      return r;
    else if (auto r = get<function_type_const>(x))  return r;
    else if (auto r = get<method_type_const>(x))    return r;
    else if (auto r = get<struct_type_const>(x))    return r;
    else                                            return std::nullopt;
}

std::string yama::const_table::fmt_const(const_t x) const {
    std::string a{};
    static_assert(const_types == 9);
    if (x >= size())                                return std::format("<illegal({})>", x); // out-of-bounds
    else if (auto r = get<int_const>(x))            a = fmt_int(*r);
    else if (auto r = get<uint_const>(x))           a = fmt_uint(*r);
    else if (auto r = get<float_const>(x))          a = fmt_float(*r);
    else if (auto r = get<bool_const>(x))           a = fmt_bool(*r);
    else if (auto r = get<char_const>(x))           a = fmt_char(*r);
    else if (auto r = get<primitive_type_const>(x)) a = r->fmt();
    else if (auto r = get<function_type_const>(x))  a = r->fmt();
    else if (auto r = get<method_type_const>(x))    a = r->fmt();
    else if (auto r = get<struct_type_const>(x))    a = r->fmt();
    else                                            a = "<stub>";
    return std::format("({}) {}", const_type(x).value(), a);
}

std::string yama::const_table::fmt_type_const(const_t x) const {
    if (x >= size())        return std::format("<illegal({})>", x);
    else if (is_stub(x))    return std::format("<stub({})>", x);
    else                    return type(x).value().fmt();
}

std::string yama::const_table::fmt(const char* tab) const {
    YAMA_ASSERT(tab);
    std::string result{};
    result += std::format("const_table ({} consts)", size());
    for (const_t i = 0; i < size(); i++) {
        result += std::format("\n{}[{}] {}", tab, i, fmt_const(i));
    }
    return result;
}

