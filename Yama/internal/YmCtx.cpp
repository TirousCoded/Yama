

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
    _stk(this),
    _objs(this, [this](YmUInt32 index) -> YmObj* { return _stk.global(index); }),
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
    return loader->ldNone();
}

YmType& YmCtx::ldInt() const noexcept {
    return loader->ldInt();
}

YmType& YmCtx::ldUInt() const noexcept {
    return loader->ldUInt();
}

YmType& YmCtx::ldFloat() const noexcept {
    return loader->ldFloat();
}

YmType& YmCtx::ldBool() const noexcept {
    return loader->ldBool();
}

YmType& YmCtx::ldRune() const noexcept {
    return loader->ldRune();
}

YmType& YmCtx::ldType() const noexcept {
    return loader->ldType();
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

_ym::TempRef YmCtx::pull(bool frontendRef) noexcept {
    return _stk.pull(frontendRef);
}

void YmCtx::pop(YmLocals n) {
    _stk.pop(n);
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
    if (!_stk.checkLocalIsInBounds(where, "Struct init failed")) {
        return false;
    }
    if (_type.kind() != YmKind_Struct) {
        _ym::Global::raiseErr(
            YmErrCode_NonStructType,
            "Struct init failed; {} is not a struct type!",
            _type.fullname());
        return false;
    }
    // TODO: Find a better way to do this!
    if (_type.sameAs(ldNone()) ||
        _type.sameAs(ldInt()) ||
        _type.sameAs(ldUInt()) ||
        _type.sameAs(ldFloat()) ||
        _type.sameAs(ldBool()) ||
        _type.sameAs(ldRune()) ||
        _type.sameAs(ldType())) {
        if (!argNames.empty()) {
            _ym::Global::raiseErr(
                YmErrCode_IllegalNameList,
                "Struct init failed; {} has no stored properties!",
                _type.fullname());
            return false;
        }
        return defaultInit(type, where);
    }
    // TODO: What happens if argNameCount exceeds 255 due to user input?
    //       YmParams and YmLocals are only 8- and 32-bit, respectively.
    size_t argNameCount = std::ranges::distance(argNames | std::views::split(','));
    if (argNameCount > locals()) {
        _ym::Global::raiseErr(
            YmErrCode_LocalNotFound,
            "Struct init failed; object stack has {} objects, but expected {}!",
            locals(),
            argNameCount);
        return false;
    }
    const auto& storedProperties = _type.slots();
    // TODO: We need to figure out how we'll handle the notion of a max number of
    //       stored properties, as right now we don't properly account for that.
    _ym::ArgPackInfo<> argPack(0, uint8_t(storedProperties));
    for (const auto& it : argNames | std::views::split(',')) {
        std::string_view argName(it.begin(), it.end());
        // TODO: Optimize out this std::string heap alloc.
        if (auto getter = _type.member((std::string)argName);
            getter && getter->info->type->isStoredPropertyGet()) {
            // TODO: When we figure out max stored properties, be sure to account for
            //       the 'YmUInt8(~)' here too.
            YmUInt8 storedPropertySlot = YmUInt8(getter->info->type->storedPropertySlot().value());
            if (!argPack.specifyNextNamedArg(storedPropertySlot)) {
                _ym::Global::raiseErr(
                    YmErrCode_IllegalNameList,
                    "Struct init failed; stored property {} specified multiple times!",
                    (std::string)argName);
                return false;
            }
            uint8_t argOffset = argPack.argOffset(storedPropertySlot, true).value();
            auto& arg = ym::deref(local(locals() - YmLocals(argNameCount) + argOffset));
            if (arg.type != getter->type().returnType()) {
                _ym::Global::raiseErr(
                    YmErrCode_TypeMismatch,
                    "Struct init failed; arg #{} (for stored property {}) is {}, but expected {}!",
                    argOffset + 1,
                    (std::string)argName,
                    arg.type->fullname(),
                    getter->type().fullname());
                return false;
            }
        }
        else {
            _ym::Global::raiseErr(
                YmErrCode_IllegalNameList,
                "Struct init failed; unknown stored property {}!",
                (std::string)argName);
            return false;
        }
    }
    argPack.done();
    if (argPack.dummies() > 0) {
        _ym::Global::raiseErr(
            YmErrCode_IllegalNameList,
            "Struct init failed; not all stored properties specified!");
        return false;
    }
    auto result = create(_type, false);
    for (uint16_t storedPropertyInd = 0; storedPropertyInd < storedProperties; storedPropertyInd++) {
        // TODO: But what if storedProperties exceeds 8-bit max?
        uint8_t argOffset = argPack.argOffset(YmUInt8(storedPropertyInd), true).value();
        YmLocal argInd = locals() - YmLocals(argNameCount) + argOffset;
        // *Steal* the arg object's refs, putting them in the slots, nullifying InternalRef(s).
        result->assignRefSlot(storedPropertyInd, stealLocal(argInd, false));
    }
    // Pop all the nullified InternalRef(s).
    pop(argPack.specifiedArgs());
    put(where, std::move(result));
    return true;
}

bool YmCtx::call(YmType* fn, YmUInt16 argsN, std::string_view argNames, YmLocal returnTo) {
    return _stk.call(fn, argsN, argNames, returnTo);
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
    if (!_stk.checkLocalIsInBounds(where, "Var get failed")) {
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
    if (auto value = local(-1); value && value->type != _varType.returnType()) {
        _ym::Global::raiseErr(
            YmErrCode_TypeMismatch,
            "Var set failed; value is {}, but expected {}!",
            value->type->fullname(),
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
    if (!_stk.checkLocalIsInBounds(where, "Property get failed")) {
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
    auto& value = ym::deref(local(-1));
    if (subject.type != _propertyType.owner()) {
        _ym::Global::raiseErr(
            YmErrCode_TypeMismatch,
            "Property set failed; subject is {}, but expected {}!",
            subject.type->fullname(),
            _propertyType.owner()->fullname());
        return false;
    }
    if (value.type != _propertyType.returnType()) {
        _ym::Global::raiseErr(
            YmErrCode_TypeMismatch,
            "Property set failed; value is {}, but expected {}!",
            value.type->fullname(),
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
    if (!_stk.checkLocalIsInBounds(returnTo, "Conversion failed")) {
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

