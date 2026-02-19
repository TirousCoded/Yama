

#include "Spec.h"

#include "general.h"
#include "SpecSolver.h"
#include "../yama++/general.h"


std::optional<_ym::Spec> _ym::Spec::path(const std::string& path, SpecSolver& solver) {
	if (auto s = solver(path, SpecSolver::MustBe::Path)) {
		return ym::retopt(Spec(std::move(*s), Type::Path));
	}
	return std::nullopt;
}

std::optional<_ym::Spec> _ym::Spec::path(const std::string& path) {
	SpecSolver s{};
	return Spec::path(path, s);
}

std::optional<_ym::Spec> _ym::Spec::item(const std::string& fullname, SpecSolver& solver) {
	if (auto s = solver(fullname, SpecSolver::MustBe::Item)) {
		return ym::retopt(Spec(std::move(*s), Type::Item));
	}
	return std::nullopt;
}

std::optional<_ym::Spec> _ym::Spec::item(const std::string& fullname) {
	SpecSolver s{};
	return Spec::item(fullname, s);
}

std::optional<_ym::Spec> _ym::Spec::either(const std::string& specifier, SpecSolver& solver) {
	SpecSolver::Type t{};
	if (auto s = solver(specifier, t)) {
		return ym::retopt(Spec(std::move(*s), t == SpecSolver::Type::Path ? Type::Path : Type::Item));
	}
	return std::nullopt;
}

std::optional<_ym::Spec> _ym::Spec::either(const std::string& specifier) {
	SpecSolver s{};
	return Spec::either(specifier, s);
}

_ym::Spec _ym::Spec::pathFast(std::string normalizedPath) {
	assertNormal(normalizedPath);
	return std::move(Spec(std::move(normalizedPath), Type::Path).assertPath());
}

_ym::Spec _ym::Spec::itemFast(std::string normalizedFullname) {
	assertNormal(normalizedFullname);
	return std::move(Spec(std::move(normalizedFullname), Type::Item).assertItem());
}

std::strong_ordering _ym::Spec::operator<=>(const Spec& other) const noexcept {
	return _spec <=> other._spec;
}

const std::string& _ym::Spec::string() const noexcept {
	return _spec;
}

std::string_view _ym::Spec::base() const noexcept {
	return seperateCallSuff(string()).first;
}

std::optional<std::string_view> _ym::Spec::callsuff() const noexcept {
	return seperateCallSuff(string()).second;
}

_ym::Spec::Type _ym::Spec::type() const noexcept {
	return _type;
}

bool _ym::Spec::isPath() const noexcept {
	return type() == Type::Path;
}

bool _ym::Spec::isItem() const noexcept {
	return type() == Type::Item;
}

_ym::Spec& _ym::Spec::assertPath() noexcept {
	ymAssert(isPath());
	return *this;
}

const _ym::Spec& _ym::Spec::assertPath() const noexcept {
	ymAssert(isPath());
	return *this;
}

_ym::Spec& _ym::Spec::assertItem() noexcept {
	ymAssert(isItem());
	return *this;
}

const _ym::Spec& _ym::Spec::assertItem() const noexcept {
	ymAssert(isItem());
	return *this;
}

_ym::Spec& _ym::Spec::assertNoCallSuff() noexcept {
	ymAssert(!callsuff().has_value());
	return *this;
}

const _ym::Spec& _ym::Spec::assertNoCallSuff() const noexcept {
	ymAssert(!callsuff().has_value());
	return *this;
}

size_t _ym::Spec::hash() const noexcept {
	return std::hash<Spec>{}(*this);
}

std::string _ym::Spec::fmt() const {
	return _spec;
}

_ym::Spec _ym::Spec::transformed(RedirectSet* redirects, YmParcel* here, YmItem* itemParamsCtx, YmItem* self) const {
	return Spec(SpecSolver(here, itemParamsCtx, self, redirects)(_spec).value(), type());
}

_ym::Spec _ym::Spec::removeCallSuff() const {
	return
		callsuff()
		? Spec(std::string(base()), type())
		: *this;
}

_ym::Spec::Spec(std::string s, Type t) :
	_spec(std::move(s)),
	_type(t) {
}
