

#include "SpecSolver.h"

#include "../yama++/general.h"
#include "YmType.h"


#define _DUMP_LOG 0

#if _DUMP_LOG
#include "../yama++/print.h"
#endif


_ym::SpecSolver::SpecSolver(YmParcel* here, YmType* typeParamsCtx, YmType* self, RedirectSet* redirects) noexcept :
    _here(here),
    _typeParamCtx(typeParamsCtx),
    _self(self),
    _redirects(redirects) {
}

std::optional<std::string> _ym::SpecSolver::operator()(const SpecParser::Result& specifier, Type& type, MustBe mustBe) {
    if (!specifier) {
        return std::nullopt;
    }
    _beginSolve();
    eval(specifier);
    return _endSolve(type, mustBe);
}

std::optional<std::string> _ym::SpecSolver::operator()(const SpecParser::Result& specifier, MustBe mustBe) {
    Type t{};
    return operator()(specifier, t, mustBe);
}

std::optional<std::string> _ym::SpecSolver::operator()(const taul::str& specifier, Type& type, MustBe mustBe) {
    return operator()(SpecParser{}(specifier), mustBe);
}

std::optional<std::string> _ym::SpecSolver::operator()(const taul::str& specifier, MustBe mustBe) {
    Type t{};
    return operator()(specifier, t, mustBe);
}

std::optional<std::string> _ym::SpecSolver::operator()(const std::string& specifier, Type& type, MustBe mustBe) {
    // NOTE: It's safe to pass a non-owning taul::str here, as we know 100% that the parse
    //		 tree memory that will ref it won't outlive this fn call.
    return operator()(taul::str::lit(specifier.c_str()), mustBe);
}

std::optional<std::string> _ym::SpecSolver::operator()(const std::string& specifier, MustBe mustBe) {
    Type t{};
    return operator()(specifier, t, mustBe);
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
    return _scope().latest == Type::Path;
}

bool _ym::SpecSolver::_isType() const noexcept {
    return _scope().latest == Type::Type;
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

bool _ym::SpecSolver::_expectType() {
#if _DUMP_LOG
    ym::println("SpecSolver: Expects type! -> {}", _isType());
#endif
    if (!_isType()) _fail();
    return _isType();
}

bool _ym::SpecSolver::_expectTypeOrNone() {
    return !_hasExpr() || _expectType();
}

void _ym::SpecSolver::_markAsPath() noexcept {
#if _DUMP_LOG
    if (!_hasExpr() || _isType()) {
        ym::println("SpecSolver: Marked as path!");
    }
#endif
    _scope().latest = Type::Path;
}
void _ym::SpecSolver::_markAsType() noexcept {
#if _DUMP_LOG
    if (!_hasExpr() || _isPath()) {
        ym::println("SpecSolver: Marked as type!");
    }
#endif
    _scope().latest = Type::Type;
}

void _ym::SpecSolver::_handleRedirectsIfPath() {
    if (_redirects && _isPath()) {
#if _DUMP_LOG
        auto old = _scope().output;
#endif
        // TODO: This conversion into and then out of Spec is suboptimal.
        _scope().output = _redirects->resolve(Spec::pathFast(_scope().output)).string();
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

std::optional<std::string> _ym::SpecSolver::_endSolve(Type& type, MustBe mustBe) {
    _handleRedirectsIfPath();
    if (mustBe == MustBe::Path)			_expectPath();
    else if (mustBe == MustBe::Type)    _expectType();
    type = _scope().latest.value_or(Type::Path);
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
        ym::println("SpecSolver: rootId {} ({} input)", id, _isPath() ? "path" : "type");
#endif
        if (id == "%here") {
            if (hereCallback) hereCallback();
        }
        else if (id == "$Self") {
            if (selfCallback) selfCallback();
        }
        else if (id.substr(0, 1) == "$") {
            if (typeParamCallback) typeParamCallback(id, _atRootOfEntireTree);
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
            _markAsType();
        }
        else if (auto typeParam = (id.substr(0, 1) == "$" && _typeParamCtx) ? _typeParamCtx->typeParam(std::string(id)) : nullptr) {
#if _DUMP_LOG
            ym::println("SpecSolver: Type param! -> {}", typeParam->fullname());
#endif
            _emit("{}", typeParam->fullname());
            _markAsType();
        }
        else {
            _emit("{}", id);
            // Detect types in situations where _self/_typeParamCtx aren't provided.
            if (id.substr(0, 1) == "$") _markAsType();
            else						_markAsPath(); // Default
        }
        _firstId = false;
        _atRootOfEntireTree = false;
    }
}

void _ym::SpecSolver::slashId(const taul::str& id) {
    if (_good()) {
#if _DUMP_LOG
        ym::println("SpecSolver: slashId {} ({} input)", id, _isPath() ? "path" : "type");
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
        ym::println("SpecSolver: colonId {} ({} input)", id, _isPath() ? "path" : "type");
#endif
        if (_expectPath()) {
            _handleRedirectsIfPath();
            _emit(":{}", id);
            _markAsType();
        }
    }
}

void _ym::SpecSolver::dblColonId(const taul::str& id) {
    if (_good()) {
#if _DUMP_LOG
        ym::println("SpecSolver: dblColonId {} ({} input)", id, _isPath() ? "path" : "type");
#endif
        if (_expectType()) {
            _emit("::{}", id);
            //_markAsType();
        }
    }
}

void _ym::SpecSolver::openTypeArgs() {
    if (_good()) {
#if _DUMP_LOG
        ym::println("SpecSolver: openTypeArgs ({} input)", _isPath() ? "path" : "type");
#endif
        if (_expectType()) {
            _emit("[");
            _beginScope();
        }
    }
}

void _ym::SpecSolver::typeArgsArgDelimiter() {
    if (_good()) {
#if _DUMP_LOG
        ym::println("SpecSolver: typeArgsArgDelimiter");
#endif
        _expectType(); // Previous arg should be an type.
        _endScopeAndEmit();
        _emit(", ");
        _beginScope();
    }
}

void _ym::SpecSolver::closeTypeArgs() {
    if (_good()) {
#if _DUMP_LOG
        ym::println("SpecSolver: closeTypeArgs");
#endif
        _expectTypeOrNone(); // Final arg should be an type.
        _endScopeAndEmit();
        _emit("]");
        _markAsType();
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
        _expectType(); // Previous param should be an type.
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
        _expectTypeOrNone(); // Final param should be an type.
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
        _expectType(); // Return type should have been an type.
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

