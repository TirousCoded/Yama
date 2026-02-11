

#include "SpecSolver.h"

#include "../yama++/general.h"
#include "YmItem.h"


#define _DUMP_LOG 0

#if _DUMP_LOG
#include "../yama++/print.h"
#endif


_ym::SpecSolver::SpecSolver(YmParcel* here, YmItem* itemParamsCtx, YmItem* self, RedirectSet* redirects) noexcept :
    _here(here),
    _itemParamCtx(itemParamsCtx),
    _self(self),
    _redirects(redirects) {
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

bool _ym::SpecSolver::_hasScope() const noexcept {
    return !_scopes.empty();
}

_ym::SpecSolver::_Scope& _ym::SpecSolver::_scope() noexcept {
    ymAssert(_hasScope());
    return _scopes.back();
}

const _ym::SpecSolver::_Scope& _ym::SpecSolver::_scope() const noexcept {
    ymAssert(_hasScope());
    return _scopes.back();
}

bool _ym::SpecSolver::_hasExpr() const noexcept {
    return (bool)_scope().latest;
}

bool _ym::SpecSolver::_isPath() const noexcept {
    return _scope().latest == _Expr::Path;
}

bool _ym::SpecSolver::_isItem() const noexcept {
    return _scope().latest == _Expr::Item;
}

bool _ym::SpecSolver::_expectPath() {
#if _DUMP_LOG
    ym::println("SpecSolver: Expects path! -> {}", _isPath());
#endif
    if (!_isPath()) _fail();
    return _isPath();
}

bool _ym::SpecSolver::_expectPathOrNone() {
    return !_hasExpr() || _expectPath();
}

bool _ym::SpecSolver::_expectItem() {
#if _DUMP_LOG
    ym::println("SpecSolver: Expects item! -> {}", _isItem());
#endif
    if (!_isItem()) _fail();
    return _isItem();
}

bool _ym::SpecSolver::_expectItemOrNone() {
    return !_hasExpr() || _expectItem();
}

void _ym::SpecSolver::_markAsPath() noexcept {
#if _DUMP_LOG
    if (!_hasExpr() || _isItem()) {
        ym::println("SpecSolver: Marked as path!");
    }
#endif
    _scope().latest = _Expr::Path;
}
void _ym::SpecSolver::_markAsItem() noexcept {
#if _DUMP_LOG
    if (!_hasExpr() || _isPath()) {
        ym::println("SpecSolver: Marked as item!");
    }
#endif
    _scope().latest = _Expr::Item;
}

void _ym::SpecSolver::_handleRedirectsIfPath() {
    if (_redirects && _isPath()) {
#if _DUMP_LOG
        auto old = _scope().output;
#endif
        _scope().output = _redirects->resolve(_scope().output);
#if _DUMP_LOG
        if (_scope().output != old) {
            ym::println("SpecSolver: Redirect! {} -> {}", old, _scope().output);
        }
#endif
    }
}

bool _ym::SpecSolver::_good() const {
    return !_failFlag;
}

void _ym::SpecSolver::_fail() {
#if _DUMP_LOG
    if (_good()) {
        ym::println("SpecSolver: Fail!");
    }
#endif
    _failFlag = true;
}

void _ym::SpecSolver::_beginSolve() {
#if _DUMP_LOG
    ym::println("SpecSolver: Begin solve!");
#endif
    ymAssert(!_hasScope());
    _failFlag = false;
    _atRootOfEntireTree = true;
    _beginScope();
}

std::optional<std::string> _ym::SpecSolver::_endSolve(MustBe mustBe) {
    _handleRedirectsIfPath();
    if (mustBe == MustBe::Path)			_expectPath();
    else if (mustBe == MustBe::Item)    _expectItem();
    auto result = _endScope();
#if _DUMP_LOG
    ym::println("SpecSolver: End solve! -> {}, {}", _good(), result);
#endif
    return
        _good()
        ? ym::retopt(result)
        : std::nullopt;
}

void _ym::SpecSolver::_beginScope() {
#if _DUMP_LOG
    ym::println("SpecSolver: Begin scope!");
#endif
    _firstId = true;
    _scopes.push_back(_Scope{});
}

std::string _ym::SpecSolver::_endScope() {
    std::string lastScopeOutput(std::move(_scope().output));
    _scopes.pop_back();
#if _DUMP_LOG
    ym::println("SpecSolver: End scope!");
#endif
    return lastScopeOutput;
}

void _ym::SpecSolver::_endScopeAndEmit() {
    _emit("{}", _endScope());
}

void _ym::SpecSolver::syntaxErr() {
    YM_DEADEND;
}

void _ym::SpecSolver::rootId(const taul::str& id) {
    if (_good()) {
#if _DUMP_LOG
        ym::println("SpecSolver: rootId {} ({} input)", id, _isPath() ? "path" : "item");
#endif
        if (id == "%here") {
            if (hereCallback) hereCallback();
        }
        else if (id == "$Self") {
            if (selfCallback) selfCallback();
        }
        else if (id.substr(0, 1) == "$") {
            if (itemParamCallback) itemParamCallback(id, _atRootOfEntireTree);
        }
        if (_here && id == "%here") {
#if _DUMP_LOG
            ym::println("SpecSolver: %here! -> {}", _here->path);
#endif
            _emit("{}", _here->path);
            _markAsPath();
        }
        else if (_self && id == "$Self") {
#if _DUMP_LOG
            ym::println("SpecSolver: $Self! -> {}", _self->fullname());
#endif
            _emit("{}", _self->fullname());
            _markAsItem();
        }
        else if (auto itemParam = (id.substr(0, 1) == "$" && _itemParamCtx) ? _itemParamCtx->itemParam(std::string(id)) : nullptr) {
#if _DUMP_LOG
            ym::println("SpecSolver: Item param! -> {}", itemParam->fullname());
#endif
            _emit("{}", itemParam->fullname());
            _markAsItem();
        }
        else {
            _emit("{}", id);
            // Detect items in situations where _self/_itemParamCtx aren't provided.
            if (id.substr(0, 1) == "$") _markAsItem();
            else						_markAsPath(); // Default
        }
        _firstId = false;
        _atRootOfEntireTree = false;
    }
}

void _ym::SpecSolver::slashId(const taul::str& id) {
    if (_good()) {
#if _DUMP_LOG
        ym::println("SpecSolver: slashId {} ({} input)", id, _isPath() ? "path" : "item");
#endif
        if (_expectPath()) {
            _emit("/{}", id);
            //_markAsPath();
        }
    }
}

void _ym::SpecSolver::colonId(const taul::str& id) {
    if (_good()) {
#if _DUMP_LOG
        ym::println("SpecSolver: colonId {} ({} input)", id, _isPath() ? "path" : "item");
#endif
        if (_expectPath()) {
            _handleRedirectsIfPath();
            _emit(":{}", id);
            _markAsItem();
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
            //_markAsItem();
        }
    }
}

void _ym::SpecSolver::openItemArgs() {
    if (_good()) {
#if _DUMP_LOG
        ym::println("SpecSolver: openItemArgs ({} input)", _isPath() ? "path" : "item");
#endif
        if (_expectItem()) {
            _emit("[");
            _beginScope();
        }
    }
}

void _ym::SpecSolver::itemArgsArgDelimiter() {
    if (_good()) {
#if _DUMP_LOG
        ym::println("SpecSolver: itemArgsArgDelimiter");
#endif
        _expectItem(); // Previous arg should be an item.
        _endScopeAndEmit();
        _emit(", ");
        _beginScope();
    }
}

void _ym::SpecSolver::closeItemArgs() {
    if (_good()) {
#if _DUMP_LOG
        ym::println("SpecSolver: closeItemArgs");
#endif
        _expectItemOrNone(); // Final arg should be an item.
        _endScopeAndEmit();
        _emit("]");
        _markAsItem();
    }
}

void _ym::SpecSolver::openCallSuff() {
    if (_good()) {
#if _DUMP_LOG
        ym::println("SpecSolver: openCallSuff");
#endif
        _emit("(");
        _beginScope();
    }
}

void _ym::SpecSolver::callSuffParamDelimiter() {
    if (_good()) {
#if _DUMP_LOG
        ym::println("SpecSolver: callSuffParamDelimiter");
#endif
        _expectItem(); // Previous param should be an item.
        _endScopeAndEmit();
        _emit(", ");
        _beginScope();
    }
}

void _ym::SpecSolver::callSuffReturnType() {
    if (_good()) {
#if _DUMP_LOG
        ym::println("SpecSolver: callSuffReturnType");
#endif
        _expectItemOrNone(); // Final param should be an item.
        _endScopeAndEmit();
        _emit(") -> ");
        _beginScope();
    }
}

void _ym::SpecSolver::closeCallSuff() {
    if (_good()) {
#if _DUMP_LOG
        ym::println("SpecSolver: closeCallSuff");
#endif
        _expectItem(); // Return type should have been an item.
        _endScopeAndEmit();
    }
}

void _ym::assertNormal(const std::string& specifier) noexcept {
    ymAssert(SpecSolver()(specifier).has_value());
}

bool _ym::specifierHasSelf(const std::string& specifier) {
    // TODO: Optimize this to avoid reiniting regex w/ every call.
    return std::regex_search(specifier, std::regex("\\$Self[ \t\r\n]*([\\[\\],:/]|$)"));
}

std::pair<std::string_view, std::optional<std::string_view>> _ym::seperateCallSuff(const std::string& normalizedSpecifier) noexcept {
    assertNormal(normalizedSpecifier);
    const auto [fullname, callsuff] = split_s<char>(normalizedSpecifier, "(", true);
    return std::make_pair(fullname, !callsuff.empty() ? std::make_optional(callsuff) : std::nullopt);
}

void _ym::assertNormalNonCallSig(const std::string& specifier) noexcept {
    ymAssert(!seperateCallSuff(specifier).second.has_value());
}

void _ym::assertNormalCallSig(const std::string& specifier) noexcept {
    ymAssert(seperateCallSuff(specifier).second.has_value());
}

