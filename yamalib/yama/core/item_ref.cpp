

#include "item_ref.h"

#include "kind-features.h"
#include "callsig_ref.h"
#include "const_table_ref.h"

#include "../internals/item_instance.h"


yama::internal::item_mem yama::internal::get_item_mem(item_ref x) noexcept {
    return x._mem;
}

yama::item_ref yama::internal::create_type(const item_instance& x) noexcept {
    return item_ref(x);
}

yama::module::item yama::item_ref::info() const noexcept {
    return _mem->info;
}

yama::gid_t yama::item_ref::id() const noexcept {
    return gid_t{ .lid = info().id(), .mid = _mem->mid };
}

yama::str yama::item_ref::fullname() const noexcept {
    return _mem->fullname;
}

yama::kind yama::item_ref::kind() const noexcept {
    return _mem->kind;
}

yama::const_table_ref yama::item_ref::consts() const noexcept {
    return *this;
}

std::optional<yama::ptype> yama::item_ref::ptype() const noexcept {
    return _mem->ptype;
}

std::optional<yama::callsig_ref> yama::item_ref::callsig() const noexcept {
    if (const auto call = info().try_one<call_desc>()) {
        return yama::callsig_ref(call->callsig, consts());
    }
    return std::nullopt;
}

std::string yama::item_ref::fmt() const {
    return fullname().fmt();
}

yama::item_ref::item_ref(const internal::item_instance& instance) noexcept
    : _mem(instance._mem) {}

