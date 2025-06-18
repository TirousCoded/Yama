

#include "object_ref.h"


yama::int_t yama::object_ref::as_int() const noexcept {
    YAMA_ASSERT(t.ptype() == ptype::int0);
    return v.i;
}

yama::uint_t yama::object_ref::as_uint() const noexcept {
    YAMA_ASSERT(t.ptype() == ptype::uint);
    return v.ui;
}

yama::float_t yama::object_ref::as_float() const noexcept {
    YAMA_ASSERT(t.ptype() == ptype::float0);
    return v.f;
}

yama::bool_t yama::object_ref::as_bool() const noexcept {
    YAMA_ASSERT(t.ptype() == ptype::bool0);
    return v.b;
}

yama::char_t yama::object_ref::as_char() const noexcept {
    YAMA_ASSERT(t.ptype() == ptype::char0);
    return v.c;
}

yama::type yama::object_ref::as_type() const noexcept {
    YAMA_ASSERT(t.ptype() == ptype::type);
    return v.t;
}

bool yama::object_ref::operator==(const object_ref& other) const noexcept {
    if (t != other.t)                                       return false;
    if (t.kind() != other.t.kind())                         return false;
    if (t.ptype() != other.t.ptype())                       return false;
    if (t.kind() == kind::primitive) {
        // don't need to check w/ ptype::none, as v doesn't matter for it
        if (t.ptype() == ptype::int0 && v.i != other.v.i)   return false;
        if (t.ptype() == ptype::uint && v.ui != other.v.ui) return false;
        if (t.ptype() == ptype::float0 && v.f != other.v.f) return false;
        if (t.ptype() == ptype::bool0 && v.b != other.v.b)  return false;
        if (t.ptype() == ptype::char0 && v.c != other.v.c)  return false;
        if (t.ptype() == ptype::type && v.t != other.v.t)   return false;
    }
    return true;
}

std::string yama::object_ref::fmt() const {
    static_assert(kinds == 4);
    std::string result{};
    if (t.kind() == kind::primitive) {
        const ptype x = t.ptype().value();
        if (x == ptype::none)               result = std::format("{} (n/a)", t.fullname());
        else if (x == ptype::int0)          result = std::format("{} ({})", t.fullname(), as_int());
        else if (x == ptype::uint)          result = std::format("{} ({})", t.fullname(), as_uint());
        else if (x == ptype::float0)        result = std::format("{} ({})", t.fullname(), as_float());
        else if (x == ptype::bool0)         result = std::format("{} ({})", t.fullname(), as_bool());
        else if (x == ptype::char0)         result = std::format("{} ({})", t.fullname(), taul::fmt_unicode(as_char()));
        else if (x == ptype::type)          result = std::format("{} ({})", t.fullname(), as_type());
        else                                YAMA_DEADEND;
    }
    else if (t.kind() == kind::function)    result = std::format("{} (n/a)", t.fullname());
    else if (t.kind() == kind::method)      result = std::format("{} (n/a)", t.fullname());
    else if (t.kind() == kind::struct0)     result = std::format("{} (n/a)", t.fullname());
    else                                    YAMA_DEADEND;
    return result;
}


