

#include "specifiers.h"

#include <taul/hashing.h>

#include "util.h"


std::optional<yama::internal::parsed_ip> yama::internal::parse_ip(const str& input) {
    auto [head, relative_path] = internal::split(input, '.', true);
    return parsed_ip{
        .head = head,
        .relative_path = relative_path,
    };
}

std::optional<yama::internal::parsed_uqn> yama::internal::parse_uqn(const str& input) {
    auto [owner, member] = internal::split_s(input, "::");
    return parsed_uqn{
        .name = input,
        .owner_name = owner,
        .member_name = member,
    };
}

std::optional<yama::internal::parsed_qn> yama::internal::parse_qn(const str& input) {
    auto [ip_s, uqn_s] = internal::split(input, ':');
    if (uqn_s.empty()) return std::nullopt;
    auto our_ip = parse_ip(ip_s);
    auto our_uqn = parse_uqn(uqn_s);
    if (!our_ip || !our_uqn) return std::nullopt;
    return parsed_qn{
        .ip = std::move(*our_ip),
        .uqn = std::move(*our_uqn),
    };
}

std::optional<yama::internal::parsed_fln> yama::internal::parse_fln(const str& input) {
    auto our_qn = parse_qn(input);
    if (!our_qn) return std::nullopt;
    return parsed_fln{
        .qn = std::move(*our_qn),
    };
}

bool yama::internal::valid_ip(const str& input) {
    return (bool)parse_ip(input);
}

bool yama::internal::valid_uqn(const str& input) {
    return (bool)parse_uqn(input);
}

bool yama::internal::valid_qn(const str& input) {
    return (bool)parse_qn(input);
}

bool yama::internal::valid_fln(const str& input) {
    return (bool)parse_fln(input);
}

std::optional<yama::str> yama::internal::extract_head_name(const str& x) {
    const auto [head, rest] = internal::split(x, ".:");
    return head;
}

std::optional<yama::internal::import_path> yama::internal::specifier_provider::ip(const env& e, const yama::str& x, bool& head_was_bad) {
    return import_path::parse(e, x, head_was_bad);
}

std::optional<yama::internal::import_path> yama::internal::specifier_provider::ip(const env& e, const yama::str& x) {
    return import_path::parse(e, x);
}

std::optional<yama::internal::unqualified_name> yama::internal::specifier_provider::uqn(const yama::str& x) {
    return unqualified_name::parse(x);
}

std::optional<yama::internal::qualified_name> yama::internal::specifier_provider::qn(const env& e, const yama::str& x, bool& head_was_bad) {
    return qualified_name::parse(e, x, head_was_bad);
}

std::optional<yama::internal::qualified_name> yama::internal::specifier_provider::qn(const env& e, const yama::str& x) {
    return qualified_name::parse(e, x);
}

std::optional<yama::internal::fullname> yama::internal::specifier_provider::fln(const env& e, const yama::str& x, bool& head_was_bad) {
    return fullname::parse(e, x, head_was_bad);
}

std::optional<yama::internal::fullname> yama::internal::specifier_provider::fln(const env& e, const yama::str& x) {
    return fullname::parse(e, x);
}

yama::parcel_id yama::internal::import_path::head() const noexcept {
    return _head;
}

const yama::str& yama::internal::import_path::relative_path() const noexcept {
    return _relative_path;
}

bool yama::internal::import_path::is_root() const noexcept {
    return relative_path().empty();
}

size_t yama::internal::import_path::hash() const noexcept {
    return taul::hash(head(), relative_path());
}

yama::str yama::internal::import_path::str() const noexcept {
    return yama::str(fmt());
}

yama::str yama::internal::import_path::str(const env& e) const noexcept {
    return yama::str(fmt(e));
}

std::string yama::internal::import_path::fmt() const noexcept {
    return std::format("{{ID={}}}{}", head(), relative_path());
}

std::string yama::internal::import_path::fmt(const env& e) const noexcept {
    const auto name = e.name(head());
    return name ? std::format("{}{}", *name, relative_path()) : fmt();
}

std::optional<yama::internal::import_path> yama::internal::import_path::parse(const env& e, const yama::str& x, bool& head_was_bad) {
    head_was_bad = false; // guarantee this gets set
    const auto parsed = parse_ip(x);
    if (!parsed) {
        return std::nullopt;
    }
    if (!e.exists(parsed->head)) {
        return std::nullopt;
    }
    const auto id = e.id(parsed->head);
    if (!id) {
        head_was_bad = true;
        return std::nullopt;
    }
    return import_path(*id, yama::str(parsed->relative_path));
}

std::optional<yama::internal::import_path> yama::internal::import_path::parse(const env& e, const yama::str& x) {
    bool head_was_bad{};
    return parse(e, x, head_was_bad);
}

yama::internal::import_path::import_path(parcel_id head, yama::str&& relative_path)
    : _head(head),
    _relative_path(std::forward<yama::str>(relative_path)) {}

yama::internal::unqualified_name::unqualified_name(const yama::str& name)
    : _name(name) {}

yama::internal::unqualified_name::unqualified_name(const yama::str& owner_name, const yama::str& member_name)
    : unqualified_name(yama::str(std::format("{}::{}", owner_name, member_name))) {}

yama::str yama::internal::unqualified_name::owner_name() const noexcept {
    return split_s(_name, "::").first;
}

yama::str yama::internal::unqualified_name::member_name() const noexcept {
    return split_s(_name, "::").second;
}

bool yama::internal::unqualified_name::is_member() const noexcept {
    return !member_name().empty();
}

size_t yama::internal::unqualified_name::hash() const noexcept {
    return taul::hash(_name);
}

yama::str yama::internal::unqualified_name::str() const noexcept {
    return _name;
}

yama::str yama::internal::unqualified_name::str(const env& e) const noexcept {
    return str();
}

std::string yama::internal::unqualified_name::fmt() const noexcept {
    return std::string(_name);
}

std::string yama::internal::unqualified_name::fmt(const env& e) const noexcept {
    return fmt();
}

std::optional<yama::internal::unqualified_name> yama::internal::unqualified_name::parse(const yama::str& x) {
    return unqualified_name(x);
}

yama::internal::qualified_name::qualified_name(internal::import_path ip, internal::unqualified_name uqn)
    : _ip(std::move(ip)),
    _uqn(std::move(uqn)) {}

const yama::internal::import_path& yama::internal::qualified_name::ip() const noexcept {
    return _ip;
}

const yama::internal::unqualified_name& yama::internal::qualified_name::uqn() const noexcept {
    return _uqn;
}

size_t yama::internal::qualified_name::hash() const noexcept {
    return taul::hash(ip(), uqn());
}

yama::str yama::internal::qualified_name::str() const noexcept {
    return yama::str(fmt());
}

yama::str yama::internal::qualified_name::str(const env& e) const noexcept {
    return yama::str(fmt(e));
}

std::string yama::internal::qualified_name::fmt() const noexcept {
    return std::format("{}:{}", ip(), uqn());
}

std::string yama::internal::qualified_name::fmt(const env& e) const noexcept {
    return std::format("{}:{}", ip().fmt(e), uqn().fmt(e));
}

std::optional<yama::internal::qualified_name> yama::internal::qualified_name::parse(const env& e, const yama::str& x, bool& head_was_bad) {
    head_was_bad = false; // guarantee this gets set
    // TODO: replace below w/ parse_qn
    auto [import_path_s, unqualified_name_s] = internal::split(x, ':');
    if (unqualified_name_s.empty()) return std::nullopt;
    auto our_import_path = yama::internal::import_path::parse(e, import_path_s, head_was_bad);
    if (!our_import_path) return std::nullopt;
    return yama::internal::qualified_name(std::move(*our_import_path), std::move(unqualified_name_s));
}

std::optional<yama::internal::qualified_name> yama::internal::qualified_name::parse(const env& e, const yama::str& x) {
    bool head_was_bad{};
    return parse(e, x, head_was_bad);
}

yama::internal::fullname::fullname(internal::qualified_name qn)
    : _qn(std::move(qn)) {}

const yama::internal::import_path& yama::internal::fullname::ip() const noexcept {
    return qn().ip();
}

const yama::internal::unqualified_name& yama::internal::fullname::uqn() const noexcept {
    return qn().uqn();
}

const yama::internal::qualified_name& yama::internal::fullname::qn() const noexcept {
    return _qn;
}

size_t yama::internal::fullname::hash() const noexcept {
    return taul::hash(qn());
}

yama::str yama::internal::fullname::str() const noexcept {
    return qn().str();
}

yama::str yama::internal::fullname::str(const env& e) const noexcept {
    return qn().str(e);
}

std::string yama::internal::fullname::fmt() const noexcept {
    return qn().fmt();
}

std::string yama::internal::fullname::fmt(const env& e) const noexcept {
    return qn().fmt(e);
}

std::optional<yama::internal::fullname> yama::internal::fullname::parse(const env& e, const yama::str& x, bool& head_was_bad) {
    head_was_bad = false; // guarantee this gets set
    // TODO: replace below w/ parse_fln
    auto our_qualified_name = yama::internal::qualified_name::parse(e, x, head_was_bad);
    if (!our_qualified_name) return std::nullopt;
    return fullname(std::move(*our_qualified_name));
}

std::optional<yama::internal::fullname> yama::internal::fullname::parse(const env& e, const yama::str& x) {
    bool head_was_bad{};
    return parse(e, x, head_was_bad);
}

