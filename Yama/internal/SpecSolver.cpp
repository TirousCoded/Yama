

#include "SpecSolver.h"

#include "../yama++/general.h"
#include "YmItem.h"


#define _DUMP_LOG 0


_ym::SpecSolver::SpecSolver(YmParcel* here, YmItem* itemParamsCtx, YmItem* self) noexcept :
	_here(here),
	_itemParamCtx(itemParamsCtx),
	_self(self) {
}

std::optional<std::string> _ym::SpecSolver::operator()(const SpecParser::Result& specifier, MustBe mustBe) {
	if (!specifier) {
		return std::nullopt;
	}
	_beginSolve();
	eval(specifier);
	return _endSolve(mustBe);
}

std::optional<std::string> _ym::SpecSolver::operator()(const taul::str& specifier, MustBe mustBe) {
	return operator()(SpecParser{}(specifier), mustBe);
}

std::optional<std::string> _ym::SpecSolver::operator()(const std::string& specifier, MustBe mustBe) {
	// NOTE: It's safe to pass a non-owning taul::str here, as we know 100% that the parse
	//		 tree memory that will ref it won't outlive this fn call.
	return operator()(taul::str::lit(specifier.c_str()), mustBe);
}

bool _ym::SpecSolver::_isPath() const noexcept {
	ymAssert(!_expectItemNotPath.empty());
	return !_expectItemNotPath.back();
}

bool _ym::SpecSolver::_isItem() const noexcept {
	return !_isPath();
}

bool _ym::SpecSolver::_expectPath() {
#if _DUMP_LOG
	ym::println("SpecSolver: Expects path! -> {}", _isPath());
#endif
	if (!_isPath()) _fail();
	return _isPath();
}

bool _ym::SpecSolver::_expectItem() {
#if _DUMP_LOG
	ym::println("SpecSolver: Expects item! -> {}", _isItem());
#endif
	if (!_isItem()) _fail();
	return _isItem();
}

void _ym::SpecSolver::_shouldExpectPath() noexcept {
#if _DUMP_LOG
	if (_isItem()) {
		ym::println("SpecSolver: Should expect path!");
	}
#endif
	ymAssert(!_expectItemNotPath.empty());
	_expectItemNotPath.back() = false;
}
void _ym::SpecSolver::_shouldExpectItem() noexcept {
#if _DUMP_LOG
	if (_isPath()) {
		ym::println("SpecSolver: Should expect item!");
	}
#endif
	ymAssert(!_expectItemNotPath.empty());
	_expectItemNotPath.back() = true;
}

bool _ym::SpecSolver::_good() const {
	return _output.has_value();
}

void _ym::SpecSolver::_fail() {
#if _DUMP_LOG
	if (_good()) {
		ym::println("SpecSolver: Fail!");
	}
#endif
	_output.reset();
}

void _ym::SpecSolver::_beginSolve() {
#if _DUMP_LOG
	ym::println("SpecSolver: Begin solve!");
#endif
	ymAssert(_expectItemNotPath.empty());
	_output = "";
	_atRootOfEntireTree = true;
	_beginScope();
}

std::optional<std::string> _ym::SpecSolver::_endSolve(MustBe mustBe) {
	if (mustBe == MustBe::Path)			_expectPath();
	else if (mustBe == MustBe::Item)	_expectItem();
	_endScope();
	_expectItemNotPath.clear();
#if _DUMP_LOG
	ym::println("SpecSolver: End solve!");
#endif
	return std::move(_output);
}

void _ym::SpecSolver::_beginScope() {
#if _DUMP_LOG
	ym::println("SpecSolver: Begin scope!");
#endif
	_firstId = true;
	_expectItemNotPath.push_back(false);
}

void _ym::SpecSolver::_endScope() {
	ymAssert(!_expectItemNotPath.empty());
	_expectItemNotPath.pop_back();
#if _DUMP_LOG
	ym::println("SpecSolver: End scope!");
#endif
}

void _ym::SpecSolver::syntaxErr() {
	YM_DEADEND;
}

void _ym::SpecSolver::rootId(const taul::str& id) {
	if (_good()) {
#if _DUMP_LOG
		ym::println("SpecSolver: rootId {} ({} input)", id, _isPath() ? "path" : "item");
#endif
		if (!_firstId) {
			_expectItem(); // Expect what's prior to ',' to be an item.
#if _DUMP_LOG
			ym::println("SpecSolver: Onto next item!");
#endif
			_emit(", ");
			_shouldExpectPath(); // After ',' expect path, or something like $Self.
		}
		if (id == "%here") {
			if (hereCallback) hereCallback();
		}
		else if (id == "$Self") {
			if (selfCallback) selfCallback();
		}
		else if (id.substr(0, 1) == "$") {
			if (itemParamCallback) itemParamCallback(id, _atRootOfEntireTree);
		}
		ymAssert(!_expectItemNotPath.back());
		if (_here && id == "%here") {
#if _DUMP_LOG
			ym::println("SpecSolver: %here! -> {}", _here->path);
#endif
			_emit("{}", _here->path);
		}
		else if (_self && id == "$Self") {
#if _DUMP_LOG
			ym::println("SpecSolver: $Self! -> {}", _self->fullname());
#endif
			_emit("{}", _self->fullname());
			_shouldExpectItem();
		}
		else if (auto itemParam = (id.substr(0, 1) == "$" && _itemParamCtx) ? _itemParamCtx->itemParam(std::string(id)) : nullptr) {
#if _DUMP_LOG
			ym::println("SpecSolver: Item param! -> {}", itemParam->fullname());
#endif
			_emit("{}", itemParam->fullname());
			_shouldExpectItem();
		}
		else {
			_emit("{}", id);
			// Detect items in situations where _self/_itemParamCtx aren't provided.
			if (id.substr(0, 1) == "$") _shouldExpectItem();
		}
		_firstId = false;
		_atRootOfEntireTree = false;
	}
}

void _ym::SpecSolver::openArgs() {
	if (_good()) {
#if _DUMP_LOG
		ym::println("SpecSolver: openArgs ({} input)", _isPath() ? "path" : "item");
#endif
		if (_expectItem()) {
			_emit("[");
			_beginScope();
		}
	}
}

void _ym::SpecSolver::closeArgs() {
	if (_good()) {
#if _DUMP_LOG
		ym::println("SpecSolver: openArgs");
#endif
		_emit("]");
		_endScope();
		_shouldExpectItem();
	}
}

void _ym::SpecSolver::slashId(const taul::str& id) {
	if (_good()) {
#if _DUMP_LOG
		ym::println("SpecSolver: slashId {} ({} input)", id, _isPath() ? "path" : "item");
#endif
		if (_expectPath()) {
			_emit("/{}", id);
		}
	}
}

void _ym::SpecSolver::colonId(const taul::str& id) {
	if (_good()) {
#if _DUMP_LOG
		ym::println("SpecSolver: colonId {} ({} input)", id, _isPath() ? "path" : "item");
#endif
		if (_expectPath()) {
			_emit(":{}", id);
			_shouldExpectItem();
		}
	}
}

void _ym::SpecSolver::dblColonId(const taul::str& id) {
	if (_good()) {
#if _DUMP_LOG
		ym::println("SpecSolver: dblColonId {} ({} input)", id, _isPath() ? "path" : "item");
#endif
		if (_expectItem()) {
			_emit("::{}", id);
			_shouldExpectItem();
		}
	}
}

void _ym::assertNormal(const std::string& specifier) noexcept {
	ymAssert(SpecSolver()(specifier).has_value());
}

bool _ym::specifierHasSelf(const std::string& specifier) {
	// TODO: Optimize this to avoid reiniting regex w/ every call.
	return std::regex_search(specifier, std::regex("\\$Self[ \t\r\n]*([\\[\\],:/]|$)"));
}

