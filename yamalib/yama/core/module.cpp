

#include "module.h"

#include "asserts.h"
#include "../internals/specifiers.h"


uint16_t yama::module::count() const noexcept {
    return _try_get_s() ? _get_s().ir.count() : 0;
}

bool yama::module::exists(lid_t id) const noexcept {
    return _try_get_s() ? _get_s().ir.exists(id) : false;
}

bool yama::module::exists(const str& name) const noexcept {
    return _try_get_s() ? _get_s().ir.exists(name) : false;
}

std::optional<yama::str> yama::module::name(lid_t id) const noexcept {
    return _try_get_s() ? _get_s().ir.name(id) : std::nullopt;
}

std::optional<yama::lid_t> yama::module::id(const str& name) const noexcept {
    return _try_get_s() ? _get_s().ir.index(name) : std::nullopt;
}

template<typename T>
static bool module_equality_helper(const yama::module::item& a, const yama::module::item& b) noexcept {
    // Fail due to not the same.
    if (a.all_of<T>() != b.all_of<T>()) {
        return false;
    }
    // Skip if not present.
    if (a.none_of<T>()) {
        return true;
    }
    return a.one<T>() == b.one<T>();
}

bool yama::module::operator==(const module& other) const noexcept {
    if (count() != other.count()) {
        return false;
    }
    for (const auto item : view_items<>()) {
        if (const auto other_item = other.get_item<>(item.id())) {
            static_assert(internal::module_dmap_t::number == 6); // Reminder.
            const bool result =
                module_equality_helper<item_desc>(item, *other_item) &&
                module_equality_helper<owner_desc>(item, *other_item) &&
                module_equality_helper<member_desc>(item, *other_item) &&
                module_equality_helper<prim_desc>(item, *other_item) &&
                module_equality_helper<call_desc>(item, *other_item) &&
                module_equality_helper<bcode_desc>(item, *other_item);
            if (!result) {
                return false;
            }
        }
        else {
            return false;
        }
    }
    return true;
}

bool yama::module::add_primitive(
    const str& name,
    const_table consts,
    ptype ptype) {
    if (exists(name)) {
        return false;
    }
    const auto id = _load_s().ir.pull_index(name);
    if (!id) { // would exceed max ID count
        return false;
    }
    _load_s().dmap.bind<item_desc>(*id, name, kind::primitive, std::move(consts));
    _load_s().dmap.bind<prim_desc>(*id, ptype);
    _handle_if_members_already_exist(*id);
    return true;
}

bool yama::module::add_function(
    const str& name,
    const_table consts,
    callsig callsig,
    size_t max_locals,
    call_fn call_fn) {
    if (exists(name)) {
        return false;
    }
    const auto id = _load_s().ir.pull_index(name);
    if (!id) { // would exceed max ID count
        return false;
    }
    _load_s().dmap.bind<item_desc>(*id, name, kind::function, std::move(consts));
    _load_s().dmap.bind<call_desc>(*id, std::move(callsig), max_locals, call_fn);
    _handle_if_members_already_exist(*id);
    return true;
}

bool yama::module::add_method(
    const str& owner_name,
    const str& member_name,
    const_table consts,
    callsig callsig,
    size_t max_locals,
    call_fn call_fn) {
    const str name = _member_name(owner_name, member_name);
    if (exists(name)) {
        return false;
    }
    const auto id = _load_s().ir.pull_index(name);
    if (!id) { // would exceed max ID count
        return false;
    }
    _load_s().dmap.bind<item_desc>(*id, name, kind::method, std::move(consts));
    _load_s().dmap.bind<call_desc>(*id, std::move(callsig), max_locals, call_fn);
    _handle_if_owner_already_exists(*id, owner_name);
    return true;
}

bool yama::module::add_struct(
    const str& name,
    const_table consts) {
    if (exists(name)) {
        return false;
    }
    const auto id = _load_s().ir.pull_index(name);
    if (!id) { // would exceed max ID count
        return false;
    }
    _load_s().dmap.bind<item_desc>(*id, name, kind::struct0, std::move(consts));
    _handle_if_members_already_exist(*id);
    return true;
}

bool yama::module::bind_bcode(
    lid_t id,
    bc::code bcode,
    bc::syms bsyms) {
    if (!(exists(id) && none_of<bcode_desc>(id))) {
        return false;
    }
    _load_s().dmap.bind<bcode_desc>(id, std::move(bcode), std::move(bsyms));
    return true;
}

bool yama::module::bind_bcode(
    const str& name,
    bc::code bcode,
    bc::syms bsyms) {
    const auto _id = id(name);
    return _id ? bind_bcode(*_id, std::move(bcode), std::move(bsyms)) : false;
}

void yama::module::_handle_if_owner_already_exists(lid_t member, const str& owner_name) {
    const auto _id = id(owner_name);
    if (!_id) {
        return;
    }
    _load_s().dmap.bind<member_desc>(member, *_id);
    if (_load_s().dmap.none_of<owner_desc>(*_id)) {
        _load_s().dmap.bind<owner_desc>(*_id);
    }
    auto [owner] = _load_s().dmap.get<owner_desc>(*_id);
    owner.members.push_back(member);
}

void yama::module::_handle_if_members_already_exist(lid_t owner) {
    const str owner_name = name(owner).value();
    for (auto [id, item] : _load_s().dmap.view<item_desc>()) {
        if (const auto uqn = internal::parse_uqn(item.name); uqn && uqn->is_member() && uqn->owner_name == owner_name) {
            YAMA_ASSERT(_load_s().dmap.none_of<member_desc>(id));
            _load_s().dmap.bind<member_desc>(id, owner);
        }
    }
}

yama::str yama::module::_member_name(const str& owner, const str& member) {
    return str(std::format("{}::{}", owner, member));
}

std::unique_ptr<yama::module::_state> yama::module::_clone_s(const module& other) {
    return
        other._s
        ? std::make_unique<_state>(*other._s)
        : nullptr;
}

