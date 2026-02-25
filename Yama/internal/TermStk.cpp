

#include "TermStk.h"


#define _DUMP_LOG 0

#if _DUMP_LOG
#include "../yama++/print.h"
#endif


_ym::TermStk::TermStk(
    Area& staging,
    PathBindings& binds,
    Redirects& redirects,
    GenNonMemberTypeDataCallback genNonMemberTypeDataCallback) :
    staging(staging),
    binds(binds),
    redirects(redirects),
    genNonMemberTypeDataCallback(std::move(genNonMemberTypeDataCallback)) {
}

size_t _ym::TermStk::height() const noexcept {
    return _terms.size();
}

_ym::Term* _ym::TermStk::tryTerm(TermIndex where) noexcept {
    if (auto ind = _absInd(where); ind < _terms.size()) {
        return &_terms[ind];
    }
    return nullptr;
}

const _ym::Term* _ym::TermStk::tryTerm(TermIndex where) const noexcept {
    if (auto ind = _absInd(where); ind < _terms.size()) {
        return &_terms[ind];
    }
    return nullptr;
}

_ym::Term& _ym::TermStk::term(TermIndex where) {
    return ym::deref(tryTerm(where));
}

const _ym::Term& _ym::TermStk::term(TermIndex where) const {
    return ym::deref(tryTerm(where));
}

std::span<const _ym::Term> _ym::TermStk::topN(size_t n) const noexcept {
    return
        n >= 1
        ? std::span(&term(-TermIndex(n)), std::next(&term(-1)))
        : std::span<const Term>{};
}

void _ym::TermStk::beginSession(std::string errPrefix, YmType& type, std::string here, YmType& self) {
#if _DUMP_LOG
    ym::println("TermStk: {} {}, {}, {}, {}", __func__, errPrefix, type.fullname(), here, self.fullname());
#endif
    ymAssert(!_session.has_value());
    ymAssert(height() == 0);
    _session = _Sess{
        .errPrefix = std::move(errPrefix),
        .env = _Env{
            .type = type,
            .here = std::move(here),
            .self = self,
        },
    };
}

void _ym::TermStk::beginSession(std::string errPrefix) {
#if _DUMP_LOG
    ym::println("TermStk: {} {}, n/a, n/a, n/a", __func__, errPrefix);
#endif
    ymAssert(!_session.has_value());
    ymAssert(height() == 0);
    _session = _Sess{
        .errPrefix = std::move(errPrefix),
    };
}

void _ym::TermStk::endSession() {
#if _DUMP_LOG
    ym::println("TermStk: {}", __func__);
#endif
    _assertSess();
    popAll();
    _session.reset();
}

void _ym::TermStk::push(_ym::Term x) {
    _assertSess();
#if _DUMP_LOG
    size_t old = height();
#endif
    _terms.push_back(std::move(x));
#if _DUMP_LOG
    _printTermStk(old);
#endif
}

void _ym::TermStk::pop(size_t n) {
    _assertSess();
#if _DUMP_LOG
    size_t old = height();
#endif
    _terms.resize(_terms.size() - std::min(n, height()));
#if _DUMP_LOG
    if (n >= 1) {
        _printTermStk(old);
    }
#endif
}

void _ym::TermStk::popAll() {
    pop(height());
}

void _ym::TermStk::transact(size_t n, _ym::Term x) {
    pop(n);
    push(std::move(x));
}

void _ym::TermStk::transactErr(size_t n) {
    transact(n, Term{});
}

bool _ym::TermStk::verifyInputs(size_t n) {
    for (const auto& t : topN(n)) {
        if (t.isErr()) {
            transactErr(n);
            return false;
        }
    }
    return true;
}

bool _ym::TermStk::expectType() const {
#if _DUMP_LOG
    ym::println("TermStk: {}", __func__);
#endif
    _assertSess();
    auto& t = term(-1);
    if (t.isErr()) {
        return false;
    }
    if (!t.isType()) {
        _err(
            // TODO: Not 100% sure what error this should be.
            YmErrCode_InternalError,
            "{}; \"{}\" is not an type!",
            _errPrefix(),
            t.fmt());
        return false;
    }
    return true;
}

YmType* _ym::TermStk::expectConcrete() const {
#if _DUMP_LOG
    ym::println("TermStk: {}", __func__);
#endif
    _assertSess();
    auto& t = term(-1);
    if (t.isErr()) {
        return nullptr;
    }
    if (!t.isConcrete()) {
        _err(
            YmErrCode_GenericType,
            "{}; \"{}\" is a generic type!",
            _errPrefix(),
            t.fmt());
        return nullptr;
    }
    return t.concrete();
}

bool _ym::TermStk::expectGeneric() const {
#if _DUMP_LOG
    ym::println("TermStk: {}", __func__);
#endif
    _assertSess();
    auto& t = term(-1);
    if (t.isErr()) {
        return false;
    }
    if (!t.isGeneric()) {
        _err(
            YmErrCode_ConcreteType,
            "{}; \"{}\" is a concrete type!",
            _errPrefix(),
            t.fmt());
        return false;
    }
    return true;
}

YmParcel* _ym::TermStk::importParcel() {
#if _DUMP_LOG
    ym::println("TermStk: {}", __func__);
#endif
    _assertSess();
    auto& t = term(-1);
    if (t.isErr()) {
        return nullptr;
    }
    if (!t.isPath()) {
        _err(
            YmErrCode_IllegalSpecifier,
            "{}; \"{}\" is not an import path!",
            _errPrefix(),
            t.fmt());
        return nullptr;
    }
    else {
        return _import(t.path().value());
    }
}

void _ym::TermStk::specifier(const Spec& specifier) {
#if _DUMP_LOG
    ym::println("TermStk: {} {}", __func__, specifier);
#endif
    _assertSess();
    _Interp(*this)(specifier);
}

void _ym::TermStk::path(const Spec& path) {
#if _DUMP_LOG
    ym::println("TermStk: {} {}", __func__, path);
#endif
    _assertSess();
    push(Term(path.assertPath()));
}

void _ym::TermStk::fullname(const Spec& fullname) {
#if _DUMP_LOG
    ym::println("TermStk: {} {}", __func__, fullname);
#endif
    _assertSess();
    fullname.assertNoCallSuff();
    if (auto result = staging->types.fetch(fullname)) {
        push(Term(ym::Safe(result.get())));
    }
    else {
        specifier(fullname);
        expectType();
    }
}

void _ym::TermStk::here() {
#if _DUMP_LOG
    ym::println("TermStk: {}", __func__);
#endif
    _assertSess();
    if (!_env()) {
        operErr(
            0,
            YmErrCode_ParcelNotFound,
            "{}; %here illegal!",
            _errPrefix());
    }
    else {
        push(Term(_env()->here));
    }
}

void _ym::TermStk::root(const std::string& id) {
#if _DUMP_LOG
    ym::println("TermStk: {} {}", __func__, id);
#endif
    _assertSess();
    push(Term(id));
}

void _ym::TermStk::subdir(const std::string& id) {
#if _DUMP_LOG
    ym::println("TermStk: {} {}", __func__, id);
#endif
    _assertSess();
    auto& t = term(-1);
    if (t.isErr()) {
        return;
    }
    if (!t.isPath()) {
        operErr(
            1,
            YmErrCode_IllegalSpecifier,
            "{}; \"{}\" is not an import path!",
            _errPrefix(),
            id,
            t.fmt());
    }
    else {
        transact(1, Term(std::format("{}/{}", t.fmt(), id)));
    }
}

void _ym::TermStk::self() {
#if _DUMP_LOG
    ym::println("TermStk: {}", __func__);
#endif
    _assertSess();
    if (!_env()) {
        operErr(
            0,
            YmErrCode_TypeNotFound,
            "{}; $Self illegal!",
            _errPrefix());
    }
    else {
        push(Term(_env()->self));
    }
}

void _ym::TermStk::typeParam(const std::string& id) {
#if _DUMP_LOG
    ym::println("TermStk: {} {}", __func__, id);
#endif
    _assertSess();
    if (!_env()) {
        operErr(
            0,
            YmErrCode_TypeNotFound,
            "{}; ${} illegal!",
            _errPrefix(),
            id);
    }
    else if (auto arg = _env()->self->typeParam(id)) {
        push(Term(ym::Safe(arg)));
    }
    else {
        operErr(
            0,
            YmErrCode_TypeNotFound,
            "{}; ${} illegal!",
            _errPrefix(),
            id);
    }
}

void _ym::TermStk::typeInParcel(const std::string& id) {
#if _DUMP_LOG
    ym::println("TermStk: {} {}", __func__, id);
#endif
    _assertSess();
    if (auto p = importParcel()) {
        if (auto info = p->type(id)) {
            transact(
                1,
                !info->isParameterized()
                ? Term(genNonMemberTypeDataCallback(*p, *info, {})) // Concrete (ie. and w/out type params.)
                : Term(p->path, *info));                            // Generic
        }
        else {
            operErr(
                1,
                YmErrCode_TypeNotFound,
                "{}; {} contains no type \"{}\"!",
                _errPrefix(),
                p->path,
                id);
        }
    }
    else transactErr(1); // Don't forget!
}

void _ym::TermStk::member(const std::string& id) {
#if _DUMP_LOG
    ym::println("TermStk: {} {}", __func__, id);
#endif
    _assertSess();
    auto& t = term(-1);
    if (t.isErr()) {
        return;
    }
    if (t.isPath()) {
        operErr(
            1,
            // TODO: Not 100% sure what error this should be.
            YmErrCode_InternalError,
            "{}; \"{}\" is not an type!",
            _errPrefix(),
            t.fmt());
    }
    else if (t.isGeneric()) {
        // TODO: I'm not 100% about this error case, nor do I think we have
        //       this covered in our unit tests.
        //
        //       Also, beginArgs/endArgs will need to be considered when we
        //       look into this w/ regards to revising our code.
        operErr(
            1,
            YmErrCode_GenericType,
            "{}; \"{}\" is not a concrete type!",
            _errPrefix(),
            t.fmt());
    }
    else if (!t.hasMembers()) {
        operErr(
            1,
            YmErrCode_TypeCannotHaveMembers,
            "{}; \"{}\" has no members!",
            _errPrefix(),
            t.fmt());
    }
    else if (!t.hasMember(id)) {
        operErr(
            1,
            YmErrCode_TypeNotFound,
            "{}; \"{}\" has no member \"{}\"!",
            _errPrefix(),
            t.fmt(),
            id);
    }
    else {
        transact(1, Term(ym::Safe(ym::deref(t.concrete()).member(id))));
    }
}

void _ym::TermStk::beginArgs() {
#if _DUMP_LOG
    ym::println("TermStk: {}", __func__);
#endif
    _assertSess();
    // We don't check if term we're marking is correctly a generic type term, w/
    // that being deferred until endArgs.
    term(-1).awaitingArgs = true;
}

void _ym::TermStk::endArgs() {
#if _DUMP_LOG
    ym::println("TermStk: {}", __func__);
#endif
    _assertSess();
    // Crashes if no corresponding beginArgs call.
    auto inputs = topN(_countInputsToEndArgs().value());
    if (!verifyInputs(inputs.size())) {
        return;
    }
    auto& generic = inputs[0];
    auto args = inputs.subspan(1);
    auto fmtConstructed = [&]() -> std::string {
        std::string argPack{};
        bool first = true;
        for (const auto& arg : args) {
            if (!first) {
                argPack += ", ";
            }
            argPack += arg.fmt();
            first = false;
        }
        return std::format("{}[{}]", generic.fmt(), argPack);
        };
    if (generic.isPath()) {
        operErr(
            inputs.size(),
            // TODO: Not 100% sure what error this should be.
            YmErrCode_InternalError,
            "{}; \"{}\" is not an type!",
            _errPrefix(),
            generic.fmt());
        return;
    }
    if (generic.isConcrete()) {
        operErr(
            inputs.size(),
            YmErrCode_ConcreteType,
            "{}; \"{}\" is not a generic type!",
            _errPrefix(),
            generic.fmt());
        return;
    }
    if (args.size() != generic.typeParamCount()) {
        operErr(
            inputs.size(),
            YmErrCode_TypeArgsError,
            "{}; \"{}\" has {} type argument(s), but expects {}!",
            _errPrefix(),
            fmtConstructed(),
            args.size(),
            generic.typeParamCount());
        return;
    }
    bool badArgs = false;
    for (size_t i = 0; i < args.size(); i++) {
        auto& typeParamName = ym::deref(generic.info()).typeParams[i]->name;
        auto& arg = args[i];
        if (arg.isPath()) {
            badArgs = true;
            // NOTE: Since multiple errors can arise here, don't use operErr.
            _err(
                // TODO: Not 100% sure what error this should be.
                YmErrCode_InternalError,
                "{}; \"{}\" type argument #{} ({}=\"{}\") is not an type!",
                _errPrefix(),
                fmtConstructed(),
                i + 1,
                typeParamName,
                arg.fmt());
            continue;
        }
        if (arg.isGeneric()) {
            badArgs = true;
            // NOTE: Since multiple errors can arise here, don't use operErr.
            _err(
                YmErrCode_GenericType,
                "{}; \"{}\" type argument #{} ({}=\"{}\") is not a concrete type!",
                _errPrefix(),
                fmtConstructed(),
                i + 1,
                typeParamName,
                arg.fmt());
            continue;
        }
    }
    if (badArgs) {
        // Since above doesn't use operErr, gotta transactErr.
        transactErr(inputs.size());
        return;
    }
    transact(
        inputs.size(),
        Term(genNonMemberTypeDataCallback(
            ym::deref(_import(generic.path().value())),
            ym::deref(generic.info()),
            _assembleTypeArgs(args))));
}

size_t _ym::TermStk::_absInd(TermIndex index) const noexcept {
    return
        index >= 0
        ? size_t(index)
        : height() - size_t(-index);
}

void _ym::TermStk::_assertSess() const noexcept {
    ymAssert(_session.has_value());
}

const _ym::TermStk::_Sess& _ym::TermStk::_sess() const noexcept {
    _assertSess();
    return *_session;
}

const std::string& _ym::TermStk::_errPrefix() const noexcept {
    return _sess().errPrefix;
}

const _ym::TermStk::_Env* _ym::TermStk::_env() const noexcept {
    auto& e = _sess().env;
    return e ? &*e : nullptr;
}

YmParcel* _ym::TermStk::_import(const Spec& path) {
    path.assertPath();
    if (auto existing = staging->parcels.fetch(path)) {
        return existing.get();
    }
    if (auto binding = binds->get(path); staging->parcels.push(binding)) {
        binding->resolveRedirects(*redirects);
    }
    else {
        _err(
            YmErrCode_ParcelNotFound,
            "{}; no parcel found at path \"{}\"!",
            _errPrefix(),
            path);
    }
    return staging->parcels.fetch(path).get();
}

std::optional<size_t> _ym::TermStk::_countInputsToEndArgs() const noexcept {
    for (size_t i = 0; i < height(); i++) {
        if (_terms[_terms.size() - 1 - i].awaitingArgs) {
            return i + 1;
        }
    }
    return std::nullopt;
}

std::vector<ym::Safe<YmType>> _ym::TermStk::_assembleTypeArgs(std::span<const Term> args) {
    std::vector<ym::Safe<YmType>> result{};
    for (const auto& arg : args) {
        result.push_back(ym::Safe(arg.type()));
    }
    return result;
}

void _ym::TermStk::_printTermStk(size_t oldHeight) {
#if _DUMP_LOG
    std::string output{};
    auto t = topN(height());
    bool first = true;
    for (size_t i = 0; i < t.size(); i++) {
        if (!first) {
            output += " ";
        }
        output += t[i].fmt(true);
        if (_terms[_terms.size() - height() + i].awaitingArgs) {
            output += "/marked";
        }
        first = false;
    }
    ym::println("    ({} -> {}) {}", oldHeight, height(), output);
#endif
}

_ym::TermStk::_Interp::_Interp(TermStk& client) :
    _client(client) {
}

void _ym::TermStk::_Interp::operator()(const std::string& specifier) {
    _specifierPtr = &specifier;
    eval(SpecParser{}(taul::str(specifier)));
}

const std::string& _ym::TermStk::_Interp::_specifier() const noexcept {
    return ym::deref(_specifierPtr);
}

void _ym::TermStk::_Interp::syntaxErr() {
    YM_DEADEND;
}

void _ym::TermStk::_Interp::rootId(const taul::str& id) {
    using namespace taul::string_literals;
    if (id == "%here"_str)                  _client->here();
    else if (id == "$Self"_str)             _client->self();
    // TODO: Remove the avoidable std::string heap alloc.
    else if (id.substr(0, 1) == "$"_str)    _client->typeParam(std::string(id.substr(1)));
    else                                    _client->root(std::string(id));
}

void _ym::TermStk::_Interp::slashId(const taul::str& id) {
    // TODO: Remove the avoidable std::string heap alloc.
    _client->subdir(std::string(id));
}

void _ym::TermStk::_Interp::colonId(const taul::str& id) {
    // TODO: Remove the avoidable std::string heap alloc.
    _client->typeInParcel(std::string(id));
}

void _ym::TermStk::_Interp::dblColonId(const taul::str& id) {
    // TODO: Remove the avoidable std::string heap alloc.
    _client->member(std::string(id));
}

void _ym::TermStk::_Interp::openTypeArgs() {
    _client->beginArgs();
}

void _ym::TermStk::_Interp::typeArgsArgDelimiter() {
    //
}

void _ym::TermStk::_Interp::closeTypeArgs() {
    _client->endArgs();
}

void _ym::TermStk::_Interp::openCallSuff() {
    YM_DEADEND;
}

void _ym::TermStk::_Interp::callSuffParamDelimiter() {
    YM_DEADEND;
}

void _ym::TermStk::_Interp::callSuffReturnType() {
    YM_DEADEND;
}

void _ym::TermStk::_Interp::closeCallSuff() {
    YM_DEADEND;
}

