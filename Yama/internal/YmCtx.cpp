

#include "YmCtx.h"

#include <ranges>

#include "general.h"
#include "SpecSolver.h"
#include "YmObj.h"
#include "YmParcel.h"
#include "YmType.h"

#include "../yama++/resources.h"
#include "../yama++/print.h"
#include "StkState.h"


YmCtx::YmCtx(ym::Safe<YmDm> domain) :
    domain(domain),
    loader(std::make_shared<_ym::CtxLoader>(domain->loader)),
    _stk(),
    _objs(this,
        [this](YmUInt32 index) -> YmObj* { return _stk.global(index); },
        std::make_unique<_ym::StopTheWorldGC>(_objs)),
    _vars(*this) {
}

YmCtx::~YmCtx() noexcept {
    reset();
}

std::shared_ptr<YmParcel> YmCtx::import(const std::string& path) {
    if (auto s = _ym::Spec::path(path)) {
        return loader->import(*s);
    }
    else {
        _ym::Global::raiseErr(
            YmErrCode_IllegalSpecifier,
            "Import failed; \"{}\" syntax error!",
            path);
        return nullptr;
    }
}

std::shared_ptr<YmType> YmCtx::load(const std::string& fullname) {
    if (auto s = _ym::Spec::type(fullname)) {
        return loader->load(*s);
    }
    else {
        _ym::Global::raiseErr(
            YmErrCode_IllegalSpecifier,
            "Load failed; \"{}\" syntax error!",
            fullname);
        return nullptr;
    }
}

YmType& YmCtx::ldNone() const noexcept {
    return loader->fast->none.value();
}

YmType& YmCtx::ldInt() const noexcept {
    return loader->fast->int0.value();
}

YmType& YmCtx::ldUInt() const noexcept {
    return loader->fast->uint.value();
}

YmType& YmCtx::ldFloat() const noexcept {
    return loader->fast->float0.value();
}

YmType& YmCtx::ldBool() const noexcept {
    return loader->fast->bool0.value();
}

YmType& YmCtx::ldRune() const noexcept {
    return loader->fast->rune.value();
}

YmType& YmCtx::ldType() const noexcept {
    return loader->fast->type.value();
}

void YmCtx::setObjDestroyCallback(YmObjDestroyCallbackFn fn, void* user) noexcept {
    _objs.setObjDestroyCallback(fn, user);
}

void YmCtx::reset() {
    ymAssert(isUser());
    _stk.reset(); // Do this first.
    _objs.reset();
    _vars.reset();
}

_ym::TempRef YmCtx::create(YmType& type, bool frontend) {
    return _objs.create(type, frontend);
}

YmRefCount YmCtx::secure(YmObj& obj, bool frontend) {
    return _objs.secure(obj, frontend);
}

YmRefCount YmCtx::release(YmObj& obj, bool frontend) {
    return _objs.release(obj, frontend);
}

_ym::TempRef YmCtx::newNone(bool frontendRef) {
    return _objs.newNone(frontendRef);
}

_ym::TempRef YmCtx::newInt(YmInt v, bool frontendRef) {
    return _objs.newInt(v, frontendRef);
}

_ym::TempRef YmCtx::newUInt(YmUInt v, bool frontendRef) {
    return _objs.newUInt(v, frontendRef);
}

_ym::TempRef YmCtx::newFloat(YmFloat v, bool frontendRef) {
    return _objs.newFloat(v, frontendRef);
}

_ym::TempRef YmCtx::newBool(YmBool v, bool frontendRef) {
    return _objs.newBool(v, frontendRef);
}

_ym::TempRef YmCtx::newRune(YmRune v, bool frontendRef) {
    return _objs.newRune(v, frontendRef);
}

_ym::TempRef YmCtx::newType(YmType& v, bool frontendRef) {
    return _objs.newType(v, frontendRef);
}

_ym::TempRef YmCtx::newDefault(YmType* type, bool frontendRef) {
    return _objs.newDefault(type, frontendRef);
}

void YmCtx::gcCollect() {
    _objs.gcCollect();
}

YmCallStackHeight YmCtx::callStkHeight() const noexcept {
    return _stk.callStkHeight();
}

std::string YmCtx::fmtCallStk(YmCallStackHeight skip) const {
    return _stk.fmtCallStk(skip);
}

YmType* YmCtx::fn() const noexcept {
    return
        !isUser()
        ? _stk.cf().fn
        : nullptr;
}

std::optional<_ym::TempRef> YmCtx::fromConst(size_t index) {
    if (auto t = fn(); t && index < t->consts().size()) {
        auto v = t->consts()[index];
        if (v.is<YmInt>())			    return newInt(v.as<YmInt>(), false);
        if (v.is<YmUInt>())			    return newUInt(v.as<YmUInt>(), false);
        if (v.is<YmFloat>())			return newFloat(v.as<YmFloat>(), false);
        if (v.is<YmBool>())			    return newBool(v.as<YmBool>(), false);
        if (v.is<YmRune>())			    return newRune(v.as<YmRune>(), false);
        if (v.is<ym::Safe<YmType>>())	return newType(*v.as<ym::Safe<YmType>>(), false);
    }
    return std::nullopt;
}

bool YmCtx::isUser() const noexcept {
    return _stk.isUser();
}

YmUInt16 YmCtx::args() const noexcept {
    return _stk.args();
}

YmLocals YmCtx::locals() const noexcept {
    return _stk.locals();
}

_ym::TempRef YmCtx::arg(YmUInt16 which) {
    return _stk.arg(which);
}

bool YmCtx::setArg(YmUInt16 which, _ym::TempRef newArg) {
    return _stk.setArg(which, std::move(newArg));
}

YmType* YmCtx::ref(YmRef reference) {
    return _stk.ref(reference);
}

_ym::TempRef YmCtx::local(YmLocal where) {
    return _stk.local(where);
}

_ym::TempRef YmCtx::stealLocal(YmLocal where, bool frontendRef) {
    return _stk.stealLocal(where, frontendRef);
}

void YmCtx::pop(YmLocals n) {
    _stk.pop(n);
}

void YmCtx::popUntil(YmLocals n) {
    _stk.popUntil(n);
}

_ym::TempRef YmCtx::pull(bool frontendRef) noexcept {
    return _stk.pull(frontendRef);
}
bool YmCtx::copy(YmLocal from, YmLocal to) {
    return _stk.copy(from, to);
}

bool YmCtx::put(YmLocal where, _ym::TempRef what) {
    return _stk.put(where, std::move(what));
}

bool YmCtx::swap(YmLocal a, YmLocal b) {
    return _stk.swap(a, b);
}

bool YmCtx::defaultInit(YmType* type, YmLocal where) {
    return put(where, newDefault(type, false));
}

bool YmCtx::structInit(YmType* type, std::string_view argNames, YmLocal where) {
    if (!type) {
        return false;
    }
    auto& _type = ym::deref(type);
    if (!_stk.writeIsInBounds(where)) {
        _ym::Global::raiseErr(
            YmErrCode_LocalNotFound,
            "Struct init failed; local object index {} out-of-bounds!",
            where);
        return false;
    }
    if (_type.kind() != YmKind_Struct) {
        _ym::Global::raiseErr(
            YmErrCode_NonStructType,
            "Struct init failed; {} is not a struct type!",
            _type.fullname());
        return false;
    }
    if (_type.isPrimitive()) {
        if (!argNames.empty()) {
            _ym::Global::raiseErr(
                YmErrCode_IllegalNameList,
                "Struct init failed; {} has no stored properties!",
                _type.fullname());
            return false;
        }
        return defaultInit(type, where);
    }
    return put(where, _doStructInit(_type, _resolveStructInitArgPackAndCoerceArgs(_type, argNames)));
}

bool YmCtx::call(YmType* fn, YmUInt16 argsN, std::string_view argNames, YmLocal returnTo) {
    if (_beginCall(fn, argsN, argNames, returnTo)) {
        _dispatchCall(fn);
        return _endCall();
    }
    return false;
}

bool YmCtx::retObj(_ym::TempRef what) {
    return _stk.retObj(std::move(what));
}

bool YmCtx::getVar(YmType* varType, YmLocal where) {
    if (!varType) {
        return false;
    }
    auto& _varType = ym::deref(varType);
    if (_varType.kind() != YmKind_Var) {
        _ym::Global::raiseErr(
            YmErrCode_NonVarType,
            "Var get failed; {} is not a var type!",
            _varType.fullname());
        return false;
    }
    if (!_stk.writeIsInBounds(where)) {
        _ym::Global::raiseErr(
            YmErrCode_LocalNotFound,
            "Var get failed; local object index {} out-of-bounds!",
            where);
        return false;
    }
    if (_varType.isStoredVarGet()) { // Stored
        return put(where, _vars.pull(_varType));
    }
    else { // Computed
        return call(varType, 0, "", where);
    }
}

bool YmCtx::setVar(YmType* varType) {
    if (!varType) {
        return false;
    }
    auto& _varType = ym::deref(varType);
    if (_varType.kind() != YmKind_Var) {
        _ym::Global::raiseErr(
            YmErrCode_NonVarType,
            "Var get failed; {} is not a var type!",
            _varType.fullname());
        return false;
    }
    if (!_varType.assigner()) {
        _ym::Global::raiseErr(
            YmErrCode_ReadOnlyVarType,
            "Var get failed; {} is read-only!",
            _varType.fullname());
        return false;
    }
    if (locals() == 0) {
        _ym::Global::raiseErr(
            YmErrCode_LocalNotFound,
            "Var set failed; value not found!");
        return false;
    }
    if (!_coerce(-1, ym::deref(_varType.returnType()))) {
        _ym::Global::raiseErr(
            YmErrCode_TypeMismatch,
            "Var set failed; value is {}, but expected {}!",
            local(-1)->type->fullname(),
            _varType.returnType()->fullname());
        return false;
    }
    if (_varType.isStoredVarGet()) { // Stored
        return _vars.push(_varType, pull(false), true);
    }
    else { // Computed
        return call(_varType.assigner(), 1, "", YM_DISCARD);
    }
}

bool YmCtx::getProperty(YmType* propertyType, YmLocal where) {
    if (!propertyType) {
        return false;
    }
    auto& _propertyType = ym::deref(propertyType);
    if (_propertyType.kind() != YmKind_Property) {
        _ym::Global::raiseErr(
            YmErrCode_NonPropertyType,
            "Property get failed; {} is not a property type!",
            _propertyType.fullname());
        return false;
    }
    if (!_stk.writeIsInBounds(where)) {
        _ym::Global::raiseErr(
            YmErrCode_LocalNotFound,
            "Property get failed; local object index {} out-of-bounds!",
            where);
        return false;
    }
    if (locals() == 0) {
        _ym::Global::raiseErr(
            YmErrCode_LocalNotFound,
            "Property get failed; subject not found!");
        return false;
    }
    auto& subject = ym::deref(local(-1));
    if (subject.type != _propertyType.owner()) {
        _ym::Global::raiseErr(
            YmErrCode_TypeMismatch,
            "Property get failed; subject is {}, but expected {}!",
            subject.type->fullname(),
            _propertyType.owner()->fullname());
        return false;
    }
    if (_propertyType.isStoredPropertyGet()) { // Stored
        auto result = subject.refSlot(_propertyType.storedPropertySlot().value()).take(false);
        pop(1);
        put(where, std::move(result));
        return true;
    }
    else { // Computed
        return call(propertyType, 1, "", where);
    }
}

bool YmCtx::setProperty(YmType* propertyType) {
    if (!propertyType) {
        return false;
    }
    auto& _propertyType = ym::deref(propertyType);
    if (_propertyType.kind() != YmKind_Property) {
        _ym::Global::raiseErr(
            YmErrCode_NonPropertyType,
            "Property set failed; {} is not a property type!",
            _propertyType.fullname());
        return false;
    }
    auto assigner_ptr = _propertyType.assigner();
    if (!assigner_ptr) {
        _ym::Global::raiseErr(
            YmErrCode_ReadOnlyPropertyType,
            "Property set failed; {} is read-only!",
            _propertyType.fullname());
        return false;
    }
    auto& assigner = ym::deref(assigner_ptr);
    if (locals() == 0) {
        _ym::Global::raiseErr(
            YmErrCode_LocalNotFound,
            "Property set failed; subject/value not found!");
        return false;
    }
    if (locals() == 1) {
        _ym::Global::raiseErr(
            YmErrCode_LocalNotFound,
            "Property set failed; value not found!");
        return false;
    }
    auto& subject = ym::deref(local(-2));
    if (subject.type != _propertyType.owner()) {
        _ym::Global::raiseErr(
            YmErrCode_TypeMismatch,
            "Property set failed; subject is {}, but expected {}!",
            subject.type->fullname(),
            _propertyType.owner()->fullname());
        return false;
    }
    if (!_coerce(-1, ym::deref(_propertyType.returnType()))) {
        _ym::Global::raiseErr(
            YmErrCode_TypeMismatch,
            "Property set failed; value is {}, but expected {}!",
            local(-1)->type->fullname(),
            _propertyType.returnType()->fullname());
        return false;
    }
    if (assigner.isStoredPropertySet()) { // Stored
        subject.assignRefSlot(_propertyType.storedPropertySlot().value(), pull(false));
        pop(1); // Pop subject.
        return true;
    }
    else { // Computed
        return call(&assigner, 2, "", YM_DISCARD);
    }
}

bool YmCtx::convert(YmType& type, YmLocal returnTo, bool coercion) {
    if (!_stk.writeIsInBounds(returnTo)) {
        _ym::Global::raiseErr(
            YmErrCode_LocalNotFound,
            "Conversion failed; local object index {} out-of-bounds!",
            returnTo);
        return false;
    }
    if (locals() == 0) {
        _ym::Global::raiseErr(
            YmErrCode_LocalNotFound,
            "Conversion failed; local object stack is empty!");
        return false;
    }
    auto& input = ym::deref(local(-1));
    if (ymType_Converts(input.type, &type, YM_FALSE) == YM_FALSE) {
        _ym::Global::raiseErr(
            YmErrCode_IllegalConversion,
            "Conversion failed; {} -> {} is illegal!",
            input.type->fullname(),
            type.fullname());
        return false;
    }
    if (coercion && ymType_Converts(input.type, &type, YM_TRUE) == YM_FALSE) {
        _ym::Global::raiseErr(
            YmErrCode_IllegalConversion,
            "Conversion failed; {} -> {} is not implicit!",
            input.type->fullname(),
            type.fullname());
        return false;
    }
    auto inIsP = input.type->kind() == YmKind_Protocol;
    auto outIsP = type.kind() == YmKind_Protocol;
    if (input.type == &type) {
        return put(returnTo, pull(false));
    }
    else if (&type == ymCtx_LdNone(this)) {
        pop(1);
        return ymCtx_PutNone(this, returnTo) == YM_TRUE;
    }
    else if (!inIsP && outIsP) { // Box T -> P
        if (auto ptable = _ptables.load(type, *input.type)) {
            auto protoVal = create(type, false);
            // Transfer object into box (ie. moving ownership of it.)
            protoVal->box(pull(false), *ptable);
            return put(returnTo, std::move(protoVal));
        }
        else {
            _ym::Global::raiseErr(
                YmErrCode_IllegalConversion,
                "Conversion failed; {} -> {} is illegal!",
                input.type->fullname(),
                type.fullname());
            return false;
        }
    }
    else if (inIsP && !outIsP) { // Unbox P -> T
        if (input.boxed()->type != type) {
            _ym::Global::raiseErr(
                YmErrCode_IllegalConversion,
                "Conversion failed; {} (boxed as {}) cannot be unboxed as {}!",
                input.boxed()->type->fullname(),
                input.type->fullname(),
                type.fullname());
            return false;
        }
        auto old = pull(false); // RAII
        // The old protocol value might be referenced elsewhere, so it's ref can't
        // be stolen from it, so we pass YM_BORROW to copy the ref.
        return put(returnTo, old->boxed());
    }
    else if (inIsP && outIsP) { // P -> P
        if (auto ptable = _ptables.load(type, *input.boxed()->type)) {
            auto old = pull(false); // RAII
            auto protoVal = create(type, false);
            protoVal->box(old->boxed(), *ptable);
            return put(returnTo, std::move(protoVal));
        }
        else {
            _ym::Global::raiseErr(
                YmErrCode_IllegalConversion,
                "Conversion failed; {} (boxed as {}) -> {} is illegal!",
                input.boxed()->type->fullname(),
                input.type->fullname(),
                type.fullname());
            return false;
        }
    }
    else if (auto v = input.toInt()) {
        if (&type == ymCtx_LdUInt(this)) {
            pop(1);
            return ymCtx_PutUInt(this, returnTo, (YmUInt)*v) == YM_TRUE;
        }
        else if (&type == ymCtx_LdFloat(this)) {
            pop(1);
            return ymCtx_PutFloat(this, returnTo, (YmFloat)*v) == YM_TRUE;
        }
        else if (&type == ymCtx_LdRune(this)) {
            pop(1);
            return ymCtx_PutRune(this, returnTo, _ym::uint2rune((YmUInt)*v)) == YM_TRUE;
        }
        else return false;
    }
    else if (auto v = input.toUInt()) {
        if (&type == ymCtx_LdInt(this)) {
            pop(1);
            return ymCtx_PutInt(this, returnTo, (YmInt)*v) == YM_TRUE;
        }
        else if (&type == ymCtx_LdFloat(this)) {
            pop(1);
            return ymCtx_PutFloat(this, returnTo, (YmFloat)*v) == YM_TRUE;
        }
        else if (&type == ymCtx_LdRune(this)) {
            pop(1);
            return ymCtx_PutRune(this, returnTo, _ym::uint2rune((YmUInt)*v)) == YM_TRUE;
        }
        else return false;
    }
    else if (auto v = input.toFloat()) {
        if (&type == ymCtx_LdInt(this)) {
            pop(1);
            return ymCtx_PutInt(this, returnTo, (YmInt)*v) == YM_TRUE;
        }
        else if (&type == ymCtx_LdUInt(this)) {
            pop(1);
            return ymCtx_PutUInt(this, returnTo, (YmUInt)*v) == YM_TRUE;
        }
        else if (&type == ymCtx_LdRune(this)) {
            pop(1);
            return ymCtx_PutRune(this, returnTo, _ym::uint2rune((YmUInt)*v)) == YM_TRUE;
        }
        else return false;
    }
    else if (auto v = input.toBool()) {
        if (&type == ymCtx_LdInt(this)) {
            pop(1);
            return ymCtx_PutInt(this, returnTo, v == YM_TRUE ? 1 : 0) == YM_TRUE;
        }
        else if (&type == ymCtx_LdUInt(this)) {
            pop(1);
            return ymCtx_PutUInt(this, returnTo, v == YM_TRUE ? 1 : 0) == YM_TRUE;
        }
        else if (&type == ymCtx_LdFloat(this)) {
            pop(1);
            return ymCtx_PutFloat(this, returnTo, v == YM_TRUE ? 1.0 : 0.0) == YM_TRUE;
        }
        else return false;
    }
    else if (auto v = input.toRune()) {
        if (&type == ymCtx_LdInt(this)) {
            pop(1);
            return ymCtx_PutInt(this, returnTo, (YmInt)*v) == YM_TRUE;
        }
        else if (&type == ymCtx_LdUInt(this)) {
            pop(1);
            return ymCtx_PutUInt(this, returnTo, (YmUInt)*v) == YM_TRUE;
        }
        else return false;
    }
    else {
        _ym::Global::raiseErr(
            YmErrCode_InternalError,
            "{} -> {} is ymType_Converts defined, but its behaviour isn't!",
            input.type->fullname(),
            type.fullname());
        return false;
    }
}

std::optional<_ym::StructInitArgPackInfo> YmCtx::_resolveStructInitArgPackAndCoerceArgs(YmType& type, std::string_view argNames) {
    const auto storedProperties = YmUInt8(type.slots());
    size_t argNameCount = std::ranges::distance(argNames | std::views::split(','));
    if (argNameCount > locals()) {
        _ym::Global::raiseErr(
            YmErrCode_LocalNotFound,
            "Struct init failed; object stack has {} objects, but expected {}!",
            locals(),
            argNameCount);
        return std::nullopt;
    }
    _ym::StructInitArgPackInfo argPack(0, storedProperties);
    for (const auto& it : argNames | std::views::split(',')) {
        std::string_view argName(it.begin(), it.end());
        // TODO: Optimize out this std::string heap alloc.
        if (auto getter = type.member((std::string)argName);
            getter && getter->info->type->isStoredPropertyGet()) {
            YmUInt8 storedPropertySlot = YmUInt8(getter->info->type->storedPropertySlot().value());
            if (!argPack.specifyNextNamedArg(storedPropertySlot)) {
                _ym::Global::raiseErr(
                    YmErrCode_IllegalNameList,
                    "Struct init failed; stored property {} specified multiple times!",
                    (std::string)argName);
                return std::nullopt;
            }
            YmUInt8 argOffset = argPack.argOffset(storedPropertySlot, true).value();
            YmLocal argLocal = locals() - YmLocals(argNameCount) + argOffset;
            auto& expectedArgType = ym::deref(getter->type().returnType());
            if (!_coerce(argLocal, expectedArgType)) {
                _ym::Global::raiseErr(
                    YmErrCode_TypeMismatch,
                    "Struct init failed; arg #{} (for stored property {}) is {}, but expected {}!",
                    argOffset + 1,
                    (std::string)argName,
                    local(argLocal)->type->fullname(),
                    expectedArgType.fullname());
                return std::nullopt;
            }
        }
        else {
            _ym::Global::raiseErr(
                YmErrCode_IllegalNameList,
                "Struct init failed; unknown stored property {}!",
                (std::string)argName);
            return std::nullopt;
        }
    }
    argPack.done();
    if (argPack.dummies() > 0) {
        _ym::Global::raiseErr(
            YmErrCode_IllegalNameList,
            "Struct init failed; not all stored properties specified!");
        return std::nullopt;
    }
    return ym::retopt(argPack);
}

_ym::TempRef YmCtx::_doStructInit(YmType& type, const std::optional<_ym::StructInitArgPackInfo>& argPack) {
    if (!argPack) {
        return nullptr;
    }
    const auto storedProperties = YmUInt8(type.slots());
    const YmLocals argCount = (YmLocals)argPack->specifiedArgs();
    auto result = create(type, false);
    for (YmUInt8 storedPropertyInd = 0; storedPropertyInd < storedProperties; storedPropertyInd++) {
        YmUInt8 argOffset = argPack->argOffset(storedPropertyInd, true).value();
        YmLocal argInd = locals() - argCount + argOffset;
        // *Steal* the arg object's refs, putting them in the slots, nullifying InternalRef(s).
        result->assignRefSlot(storedPropertyInd, stealLocal(argInd, false));
    }
    // Pop all the nullified InternalRef(s).
    pop(argCount);
    return result;
}

bool YmCtx::_beginCall(YmType* fn, YmUInt16 args, std::string_view argNames, YmLocal returnTo) {
    if (fn) {
        if (!fn->isCallable()) {
            _ym::Global::raiseErr(
                YmErrCode_NonCallableType,
                "Call to {} failed; {} is non-callable!",
                fn->fullname(),
                fn->fullname());
            return false;
        }
        if (!_stk.writeIsInBounds(returnTo)) {
            _ym::Global::raiseErr(
                YmErrCode_LocalNotFound,
                "Call to {} failed; local object index {} out-of-bounds!",
                fn->fullname(),
                returnTo);
            return false;
        }
        if (YmLocals(args) > locals()) {
            _ym::Global::raiseErr(
                YmErrCode_LocalNotFound,
                "Call to {} failed; {} args provided, but local object stack height is {}!",
                fn->fullname(),
                args,
                locals());
            return false;
        }
        if (callStkHeight() == YM_MAX_CALL_STACK_HEIGHT) {
            _ym::Global::raiseErr(
                YmErrCode_CallStackOverflow,
                "Call to {} failed; call stack overflow!",
                fn->fullname());
            return false;
        }
        if (auto argPack = _resolveArgPackCoerceArgsAndPushDummies(*fn, args, argNames)) {
            _stk.pushCF(fn, returnTo, std::move(*argPack));
            return true;
        }
    }
    return false;
}

bool YmCtx::_endCall() noexcept {
    auto cf = _stk.pullCF().value();
    if (!cf.fn) {
        // This is user pseudo-call.
        return true;
    }
    if (!cf.returnVal) {
        _ym::Global::raiseErr(
            YmErrCode_CallProcedureError,
            "Call to {} failed; didn't bind a return value!",
            cf.fn->fullname());
        pop(cf.dummies());
        return false;
    }
    // TODO: The below usage of a temporary stk value used as a middle-man
    //       for our return value coercion feels super hacky.
    put(YM_PUSH, cf.returnVal.borrow());
    if (!_coerce(-1, *cf.fn->returnType())) {
        _ym::Global::raiseErr(
            YmErrCode_CallProcedureError,
            "Call to {} failed; returned {}, but expected {}!",
            cf.fn->fullname(),
            cf.returnVal->type->fullname(),
            cf.fn->returnType()->fullname());
        pop(1); // Pop our quick-n'-dirty temp.
        pop(cf.dummies());
        return false;
    }
    cf.returnVal = pull(false);
    pop(cf.args());
    return put(cf.returnTo, cf.returnVal.steal(false));
}

void YmCtx::_dispatchCall(YmType* fn) {
    if (fn) {
        _fwdIfProtocolMethodDispatch(*fn);
        auto& callBhvrInfo = ym::deref(_stk.cf().fn->info->callBehaviour());
        callBhvrInfo.fn(this, fn, callBhvrInfo.user);
    }
}

void YmCtx::_fwdIfProtocolMethodDispatch(YmType& fn) {
    if (!fn.isMethodReq()) {
        return;
    }
    // Prior to changing fwdFromProto, arg(0) shouldn't see through boxing.
    auto callobj = arg(0);
    auto ptableInd = (uintptr_t)ym::deref(fn.info->callBehaviour()).user;
    auto forwardedTo = callobj->ptable()[ptableInd];
    auto& cf = _stk.cf();
    cf.fn = forwardedTo;
    cf.fwdFromProto = true;
    // If indirectly called method has named params, we gotta add proper number
    // of dummies to cf().argPack, and we gotta push dummy objects for each.
    if (auto named = forwardedTo->namedParams(); named >= 1) {
        for (YmParams i = 0; i < named; i++) {
            put(YM_PUSH, newNone(false));
        }
        cf.argPack.addDummies(named);
        cf.localsOffset += named;
    }
}

std::optional<_ym::CallArgPackInfo> YmCtx::_resolveArgPackCoerceArgsAndPushDummies(YmType& fn, YmUInt16 args, std::string_view argNames) {
    if (auto result = _parseArgPack(fn, argNames); result && _checkArgPackAndCoerceArgs(fn, args, *result)) {
        _appendArgPackDummyObjs(*result);
        return result;
    }
    return std::nullopt;
}

std::optional<_ym::CallArgPackInfo> YmCtx::_parseArgPack(YmType& fn, std::string_view argNames) const {
    _ym::CallArgPackInfo argPack(fn);
    for (const auto& it : argNames | std::views::split(',')) {
        std::string_view argName(it.begin(), it.end());
        // TODO: Optimize out this std::string heap alloc.
        if (auto tparam = fn.param((std::string)argName)) {
            if (tparam->isPositional()) {
                _ym::Global::raiseErr(
                    YmErrCode_IllegalNameList,
                    "Call to {} failed; {} is a positional param, not a named one!",
                    fn.fullname(),
                    tparam->name());
                return std::nullopt;
            }
            if (!argPack.specifyNextNamedArg(tparam->index())) {
                _ym::Global::raiseErr(
                    YmErrCode_IllegalNameList,
                    "Call to {} failed; named param {} specified multiple times!",
                    fn.fullname(),
                    tparam->name());
                return std::nullopt;
            }
        }
        else {
            _ym::Global::raiseErr(
                YmErrCode_IllegalNameList,
                "Call to {} failed; unknown named param \"{}\"!",
                fn.fullname(),
                (std::string)argName);
            return std::nullopt;
        }
    }
    argPack.done(); // Don't forget!
    return ym::retopt(argPack);
}

bool YmCtx::_checkArgPackAndCoerceArgs(YmType& fn, YmUInt16 args, const _ym::CallArgPackInfo& argPack) {
    if (args != argPack.specifiedArgs()) {
        _ym::Global::raiseErr(
            YmErrCode_CallProcedureError,
            "Call to {} failed; {} args provided, but expected {}! ({} positional + {} named)",
            fn.fullname(),
            args,
            argPack.specifiedArgs(),
            argPack.positionalArgs(),
            argPack.namedArgs());
        return false;
    }
    for (YmParamIndex param = 0; param < argPack.paramCount(); param++) {
        // Quietly skip unspecified named args.
        if (auto argOffset = argPack.argOffset(param, true)) {
            auto argLocal = locals() - args + *argOffset;
            if (auto p = fn.param(param); !_coerce(argLocal, fn.param(param)->type())) {
                _ym::Global::raiseErr(
                    YmErrCode_TypeMismatch,
                    "Call to {} failed; arg #{} (for {} param {}) is {}, but expected {}!",
                    fn.fullname(),
                    *argOffset + 1,
                    p->isPositional() ? "positional" : "named",
                    p->name(),
                    local(argLocal)->type->fullname(),
                    p->type().fullname());
                return false;
            }
        }
    }
    return true;
}

void YmCtx::_appendArgPackDummyObjs(const _ym::CallArgPackInfo& argPack) {
    for (YmParams i = 0; i < argPack.dummies(); i++) {
        put(YM_PUSH, newNone(false));
    }
}

bool YmCtx::_coerce(YmLocal where, YmType& newType) {
    ymAssert(_stk.readIsInBounds(where));
    if (local(where)->type->sameAs(newType)) {
        return true;
    }
    // TODO: Maybe make it so that many obj stk affecting API fns ALWAYS consume
    //       stk inputs, even in case of API fn error, in which case they just
    //       consume inputs, but don't push anything.
    copy(where, YM_PUSH);
    if (!convert(newType, where, true)) {
        pop(1); // TODO: Remove this if we make convert always consume inputs.
        return false;
    }
    return true;
}

