

#include "specifiers.h"

#include <taul/hashing.h>

#include "util.h"


yama::internal::fullname::fullname(yama::internal::qualified_name&& qualified_name)
    : _qualified_name(std::forward<yama::internal::qualified_name>(qualified_name)) {}

std::optional<yama::internal::parsed_import_path> yama::internal::parse_import_path(const str& input) {
    auto [head, relative_path] = internal::split(input, '.', true);
    return parsed_import_path{
        .head = head,
        .relative_path = relative_path,
    };
}

std::optional<yama::internal::parsed_qualified_name> yama::internal::parse_qualified_name(const str& input) {
    auto [import_path_s, unqualified_name] = internal::split(input, ':');
    if (unqualified_name.empty()) return std::nullopt;
    auto our_import_path = parse_import_path(import_path_s);
    if (!our_import_path) return std::nullopt;
    return parsed_qualified_name{
        .import_path = std::move(*our_import_path),
        .unqualified_name = unqualified_name,
    };
}

std::optional<yama::internal::parsed_fullname> yama::internal::parse_fullname(const str& input) {
    auto our_qualified_name = parse_qualified_name(input);
    if (!our_qualified_name) return std::nullopt;
    return parsed_fullname{
        .qualified_name = std::move(*our_qualified_name),
    };
}

bool yama::internal::valid_import_path(const str& input) {
    return (bool)parse_import_path(input);
}

bool yama::internal::valid_qualified_name(const str& input) {
    return (bool)parse_qualified_name(input);
}

bool yama::internal::valid_fullname(const str& input) {
    return (bool)parse_fullname(input);
}

std::optional<yama::str> yama::internal::extract_head_name(const str& x) {
    const auto [head, rest] = internal::split(x, ".:");
    return head;
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
    const auto parsed = parse_import_path(x);
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

const yama::internal::import_path& yama::internal::qualified_name::import_path() const noexcept {
    return _import_path;
}

const yama::str& yama::internal::qualified_name::unqualified_name() const noexcept {
    return _unqualified_name;
}

yama::parcel_id yama::internal::qualified_name::head() const noexcept {
    return import_path().head();
}

const yama::str& yama::internal::qualified_name::relative_path() const noexcept {
    return import_path().relative_path();
}

bool yama::internal::qualified_name::is_root() const noexcept {
    return import_path().is_root();
}

size_t yama::internal::qualified_name::hash() const noexcept {
    return taul::hash(import_path(), unqualified_name());
}

yama::str yama::internal::qualified_name::str() const noexcept {
    return yama::str(fmt());
}

yama::str yama::internal::qualified_name::str(const env& e) const noexcept {
    return yama::str(fmt(e));
}

std::string yama::internal::qualified_name::fmt() const noexcept {
    return std::format("{}:{}", import_path(), unqualified_name());
}

std::string yama::internal::qualified_name::fmt(const env& e) const noexcept {
    return std::format("{}:{}", import_path().fmt(e), unqualified_name());
}

std::optional<yama::internal::qualified_name> yama::internal::qualified_name::parse(const env& e, const yama::str& x, bool& head_was_bad) {
    head_was_bad = false; // guarantee this gets set
    // TODO: replace below w/ parse_qualified_name
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

yama::internal::qualified_name::qualified_name(yama::internal::import_path&& import_path, yama::str&& unqualified_name)
    : _import_path(std::forward<yama::internal::import_path>(import_path)),
    _unqualified_name(std::forward<yama::str>(unqualified_name)) {}

const yama::internal::qualified_name& yama::internal::fullname::qualified_name() const noexcept {
    return _qualified_name;
}

const yama::internal::import_path& yama::internal::fullname::import_path() const noexcept {
    return qualified_name().import_path();
}

const yama::str& yama::internal::fullname::unqualified_name() const noexcept {
    return qualified_name().unqualified_name();
}

yama::parcel_id yama::internal::fullname::head() const noexcept {
    return qualified_name().head();
}

const yama::str& yama::internal::fullname::relative_path() const noexcept {
    return qualified_name().relative_path();
}

bool yama::internal::fullname::is_root() const noexcept {
    return qualified_name().is_root();
}

size_t yama::internal::fullname::hash() const noexcept {
    return taul::hash(qualified_name());
}

yama::str yama::internal::fullname::str() const noexcept {
    return qualified_name().str();
}

yama::str yama::internal::fullname::str(const env& e) const noexcept {
    return qualified_name().str(e);
}

std::string yama::internal::fullname::fmt() const noexcept {
    return qualified_name().fmt();
}

std::string yama::internal::fullname::fmt(const env& e) const noexcept {
    return qualified_name().fmt(e);
}

std::optional<yama::internal::fullname> yama::internal::fullname::parse(const env& e, const yama::str& x, bool& head_was_bad) {
    head_was_bad = false; // guarantee this gets set
    // TODO: replace below w/ parse_fullname
    auto our_qualified_name = yama::internal::qualified_name::parse(e, x, head_was_bad);
    if (!our_qualified_name) return std::nullopt;
    return fullname(std::move(*our_qualified_name));
}

std::optional<yama::internal::fullname> yama::internal::fullname::parse(const env& e, const yama::str& x) {
    bool head_was_bad{};
    return parse(e, x, head_was_bad);
}

