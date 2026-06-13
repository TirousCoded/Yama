

#include "YmCtx.h"

#include <ranges>

#include "general.h"
#include "SpecSolver.h"
#include "YmObj.h"
#include "YmParcel.h"
#include "YmType.h"

#include "../yama++/resources.h"
#include "../yama++/print.h"


YmCtx::YmCtx(ym::Safe<YmDm> domain) :
    domain(domain),
    loader(std::make_shared<_ym::CtxLoader>(domain->loader)) {
    _beginUserPseudoCall();
}

YmCtx::~YmCtx() noexcept {
    reset(); // Cleanup
    _endCall(); // End user pseudo-call.
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

YmObj* YmCtx::create(YmType& type) {
    auto al = mas.allocator<int>();
    ym::Safe result(_ym::ObjHAL::create(YmObj(*this, type), al));
    result->refs.addRef();
    ymAssert(result->refs.count() == 1);
    _objects.insert(result);
    return result;
}

#define _DUMP_REFCOUNT_CHANGES 0

YmRefCount YmCtx::secure(YmObj& obj) {
#if _DUMP_REFCOUNT_CHANGES
    ym::println("-- YmCtx::secure {}: {} -> {}", (void*)&obj, obj.refs.count(), obj.refs.count() + 1);
#endif
    return obj.refs.addRef();
}

YmRefCount YmCtx::release(YmObj& obj) {
#if _DUMP_REFCOUNT_CHANGES
    ym::println("-- YmCtx::release {}: {} -> {}", (void*)&obj, obj.refs.count(), obj.refs.count() - 1);
#endif
    auto old = obj.refs.drop();
    if (old == 1) {
#if _DUMP_REFCOUNT_CHANGES
        ym::println("-- YmCtx::release {}: Release!", (void*)&obj);
#endif
        obj.cleanup(); // Can't forget!
        _objects.erase(&obj);
        auto al = mas.allocator<int>();
        _ym::ObjHAL::destroy(obj, al);
    }
    return old;
}

void YmCtx::reset() {
    ymAssert(callStkHeight() == 1);
    // End current user pseudo-call.
    _endCall();
    // Copy _objects and iterate over the copy as otherwise we'd be modifying
    // objects as we iterate over it.
    auto objects = _objects;
    for (auto& object : objects) {
        while (release(*object) > 1) {}
    }
    ymAssert(_objects.empty());
    // Begin new user pseudo-call.
    _beginUserPseudoCall();
}

ym::Safe<YmObj> YmCtx::newNone() {
    return ym::Safe(create(loader->ldNone()));
}

ym::Safe<YmObj> YmCtx::newInt(YmInt v) {
    auto result = ym::Safe(create(loader->ldInt()));
    result->slot(0).i = v;
    return result;
}

ym::Safe<YmObj> YmCtx::newUInt(YmUInt v) {
    auto result = ym::Safe(create(loader->ldUInt()));
    result->slot(0).ui = v;
    return result;
}

ym::Safe<YmObj> YmCtx::newFloat(YmFloat v) {
    auto result = ym::Safe(create(loader->ldFloat()));
    result->slot(0).f = v;
    return result;
}

ym::Safe<YmObj> YmCtx::newBool(YmBool v) {
    auto result = ym::Safe(create(loader->ldBool()));
    result->slot(0).b = v;
    return result;
}

ym::Safe<YmObj> YmCtx::newRune(YmRune v) {
    auto result = ym::Safe(create(loader->ldRune()));
    result->slot(0).r = _uint2rune((YmUInt)v);
    return result;
}

ym::Safe<YmObj> YmCtx::newType(YmType& v) {
    auto result = ym::Safe(create(loader->ldType()));
    result->slot(0).type = &v;
    return result;
}

YmObj* YmCtx::newDefault(YmType& type) {
    static_assert(YmKind_Num == 6);
    if (type.sameAs(ldNone()))          return newNone();
    else if (type.sameAs(ldInt()))      return newInt(0);
    else if (type.sameAs(ldUInt()))     return newUInt(0);
    else if (type.sameAs(ldFloat()))    return newFloat(0.0);
    else if (type.sameAs(ldBool()))     return newBool(YM_FALSE);
    else if (type.sameAs(ldRune()))     return newRune(U'\0');
    else if (type.sameAs(ldType()))     return newType(loader->ldNone());
    else if (type.kind() == YmKind_Struct && type.hasDefaultValue()) {
        // TODO: Add ctor calls + handle panics.
        return create(type);
    }
    else {
        ymAssert(!type.hasDefaultValue());
        _ym::Global::raiseErr(
            YmErrCode_NoDefaultValue,
            "{} has no default value!",
            type.fullname());
        return nullptr;
    }
}

YmCallStackHeight YmCtx::callStkHeight() const noexcept {
    return (YmCallStackHeight)_callStk.size();
}

std::string YmCtx::fmtCallStk(YmCallStackHeight skip) const {
    std::string result{};
    result += std::format("Yama Stack Trace ({} frames)", callStkHeight() - std::min(skip, callStkHeight()));
    YmCallStackHeight number = callStkHeight() - 1;
    const auto begin = std::next(_callStk.crbegin(), std::min(skip, callStkHeight()));
    const auto end = _callStk.crend();
    for (auto it = begin; it != end; std::advance(it, 1)) {
        const auto& frame = *it;
        if (number >= 10) {
            result +=
                frame.fn
                ? std::format("\n    [{}] {}", number, frame.fn->fullname())
                : std::format("\n    [{}] <user>", number);
        }
        else {
            result +=
                frame.fn
                ? std::format("\n    [0{}] {}", number, frame.fn->fullname())
                : std::format("\n    [0{}] <user>", number);
        }
        // Check for symbol info, and if so, add it to result.
        if (frame.fn) {
            // TODO: Add when we add bcode.
        }
        number--;
    }
    return result;
}

bool YmCtx::isUser() const noexcept {
    return _callStk.size() == 1;
}

YmUInt16 YmCtx::args() const noexcept {
    ymAssert(!_callStk.empty());
    return _callStk.back().args();
}

YmLocals YmCtx::locals() const noexcept {
    ymAssert(!_callStk.empty());
    return YmLocals(_globalObjStk.size()) - _callStk.back().localsOffset;
}

YmObj* YmCtx::arg(YmUInt16 which, YmRefPolicy returnPolicy) {
    auto& cf = _callStk.back();
    auto result =
        which < args()
        ? _globalObjStk[cf.argOffset(which).value()].get()
        : nullptr;
    // If current call is one forwarded from protocol method call, then that means that
    // the first arg is the call object, which'll be a boxed value. In this circumstance,
    // we specially need to return the unboxed call object, as that's the object the
    // forwarded-to method call actually expects to be its call object.
    if (result && which == 0 && cf.fwdFromProto) {
        result = result->boxed();
    }
    if (result && returnPolicy != YM_BORROW) {
        secure(*result);
    }
    return result;
}

bool YmCtx::setArg(YmUInt16 which, YmObj& newArg, YmRefPolicy newArgPolicy) {
    if (isUser()) {
        if (newArgPolicy == YM_TAKE) {
            release(newArg);
        }
        return false;
    }
    if (which >= args()) {
        _ym::Global::raiseErr(
            YmErrCode_ArgNotFound,
            "Set arg failed; arg index {} out-of-bounds!",
            which);
        if (newArgPolicy == YM_TAKE) {
            release(newArg);
        }
        return false;
    }
    auto& cf = _callStk.back();
    auto& target = _globalObjStk[cf.argOffset(which).value()];
    // NOTE: It's theoretically possible that target == newArg. In that case, it's
    //       important to incr newArg's ref count BEFORE releasing target's incr.
    if (newArgPolicy == YM_BORROW) {
        secure(newArg);
    }
    release(*target);
    target = newArg;
    return true;
}

YmType* YmCtx::ref(YmRef reference) {
    auto& cf = _callStk.back();
    return
        cf.fn
        ? cf.fn->ref(reference)
        : nullptr;
}

YmObj* YmCtx::local(YmLocal where, YmRefPolicy returnPolicy) {
    auto& cf = _callStk.back();
    auto local = _absIndexForRead(where);
    auto result =
        local
        ? _globalObjStk[cf.localOffset(*local)].get()
        : nullptr;
    if (result && returnPolicy != YM_BORROW) {
        secure(*result);
    }
    return result;
}

YmObj* YmCtx::pull() noexcept {
    if (auto result = local(locals() - 1)) {
        _globalObjStk.pop_back();
        return result;
    }
    else {
        return nullptr;
    }
}

void YmCtx::pop(YmLocals n, bool releaseObjs) {
    if (n < 0) {
        return;
    }
    if (n > locals()) {
        n = locals();
    }
    if (releaseObjs) {
        for (YmLocal i = 0; i < n; i++) {
            release(ym::deref(pull()));
        }
    }
    else {
        // Can't use resize here as C++ doesn't know at compile-time if
        // it'll grow/shrink vector, and growing can't happen due to ym::Safe
        // not having a null value.
        _globalObjStk.erase(std::prev(_globalObjStk.end(), n), _globalObjStk.end());
    }
}

bool YmCtx::put(YmLocal where, YmObj& what, YmRefPolicy whatPolicy) {
    if (where == YM_DISCARD) {
        if (whatPolicy != YM_BORROW) {
            release(what);
        }
        return true;
    }
    if (where == YM_PUSH) {
        _globalObjStk.push_back(what);
        if (whatPolicy == YM_BORROW) {
            secure(what);
        }
        return true;
    }
    if (auto whereAbs = _absIndex(where)) {
        release(ym::deref(local(*whereAbs)));
        auto& cf = _callStk.back();
        _globalObjStk[cf.localOffset(*whereAbs)] = what;
        if (whatPolicy == YM_BORROW) {
            secure(what);
        }
        return true;
    }
    else {
        _ym::Global::raiseErr(
            YmErrCode_LocalNotFound,
            "Put failed; local object index {} out-of-bounds!",
            where);
        if (whatPolicy == YM_TAKE) {
            release(what);
        }
        return false;
    }
}

bool YmCtx::swap(YmLocal a, YmLocal b) {
    auto aLocal = _absIndex(a);
    auto bLocal = _absIndex(b);
    if (aLocal && bLocal) {
        auto& cf = _callStk.back();
        std::swap(
            _globalObjStk[cf.localOffset(*aLocal)],
            _globalObjStk[cf.localOffset(*bLocal)]);
        return true;
    }
    else {
        // TODO: Maybe error msg?
        return false;
    }
}

bool YmCtx::defaultInit(YmLocal where, YmType& type) {
    if (auto obj = newDefault(type)) {
        return put(where, *obj, YM_TAKE);
    }
    return false;
}

bool YmCtx::explicitInit(YmLocal where, YmType& type, std::string_view argNames) {
    if (!_absIndex(where)) {
        _ym::Global::raiseErr(
            YmErrCode_LocalNotFound,
            "Explicit init failed; local object index {} out-of-bounds!",
            where);
        return false;
    }
    if (type.kind() != YmKind_Struct) {
        _ym::Global::raiseErr(
            YmErrCode_NonStructType,
            "Explicit init failed; {} is not a struct type!",
            type.fullname());
        return false;
    }
    // TODO: Find a better way to do this!
    if (type.sameAs(ldNone()) ||
        type.sameAs(ldInt()) ||
        type.sameAs(ldUInt()) ||
        type.sameAs(ldFloat()) ||
        type.sameAs(ldBool()) ||
        type.sameAs(ldRune()) ||
        type.sameAs(ldType())) {
        if (!argNames.empty()) {
            _ym::Global::raiseErr(
                YmErrCode_IllegalNameList,
                "Explicit init failed; {} has no stored properties!",
                type.fullname());
            return false;
        }
        return defaultInit(where, type);
    }
    // TODO: What happens if argNameCount exceeds 255 due to user input?
    //       YmParams and YmLocals are only 8- and 32-bit, respectively.
    size_t argNameCount = std::ranges::distance(argNames | std::views::split(','));
    if (argNameCount > locals()) {
        _ym::Global::raiseErr(
            YmErrCode_LocalNotFound,
            "Explicit init failed; object stack has {} objects, but expected {}!",
            locals(),
            argNameCount);
        return false;
    }
    const auto& storedProperties = type.info->slots;
    // TODO: We need to figure out how we'll handle the notion of a max number of
    //       stored properties, as right now we don't properly account for that.
    _ym::ArgPackInfo<> argPack(0, uint8_t(storedProperties));
    for (const auto& it : argNames | std::views::split(',')) {
        std::string_view argName(it.begin(), it.end());
        // TODO: Optimize out this std::string heap alloc.
        if (auto getter = type.member((std::string)argName);
            getter && getter->info->type->isStoredPropertyGet()) {
            // TODO: When we figure out max stored properties, be sure to account for
            //       the 'YmUInt8(~)' here too.
            YmUInt8 storedPropertySlot = YmUInt8(getter->info->type->storedPropertySlot().value());
            if (!argPack.specifyNextNamedArg(storedPropertySlot)) {
                _ym::Global::raiseErr(
                    YmErrCode_IllegalNameList,
                    "Explicit init failed; stored property {} specified multiple times!",
                    (std::string)argName);
                return false;
            }
            uint8_t argOffset = argPack.argOffset(storedPropertySlot, true).value();
            auto& arg = ym::deref(local(locals() - YmLocals(argNameCount) + argOffset));
            if (arg.type != getter->type().returnType()) {
                _ym::Global::raiseErr(
                    YmErrCode_TypeMismatch,
                    "Explicit init failed; arg #{} (for stored property {}) is {}, but expected {}!",
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
                "Explicit init failed; unknown stored property {}!",
                (std::string)argName);
            return false;
        }
    }
    argPack.done();
    if (argPack.dummies() > 0) {
        _ym::Global::raiseErr(
            YmErrCode_IllegalNameList,
            "Explicit init failed; not all stored properties specified!");
        return false;
    }
    auto result = ym::Safe(create(type));
    for (uint16_t storedPropertyInd = 0; storedPropertyInd < storedProperties; storedPropertyInd++) {
        // TODO: But what if storedProperties exceeds 8-bit max?
        uint8_t argOffset = argPack.argOffset(YmUInt8(storedPropertyInd), true).value();
        auto arg = ym::Safe(local(locals() - YmLocals(argNameCount) + argOffset));
        // Copy raw ptr of arg object over to result's slot, w/out incr object's
        // ref count, as result's gonna *steal* arg objects from stack.
        result->slot(storedPropertyInd) = YmObj::Slot{ .ref = arg };
    }
    // Pop arg objects w/out decr object ref counts such that result thus *steals*
    // them from stack.
    pop(argPack.specifiedArgs(), false);
    put(where, *result);
    return true;
}

bool YmCtx::call(YmType& fn, YmUInt16 argsN, std::string_view argNames, YmLocal returnTo) {
    if (_beginCall(fn, argsN, argNames, returnTo)) {
        _dispatchCall(fn);
        return _endCall();
    }
    return false;
}

bool YmCtx::ret(YmObj* what, YmRefPolicy whatPolicy) {
    if (!what) {
        return false;
    }
    if (isUser()) {
        if (what && whatPolicy == YM_TAKE) {
            release(*what);
        }
        return false;
    }
    ymAssert(!_callStk.empty());
    if (_callStk.back().returnValue) {
        release(*_callStk.back().returnValue);
    }
    _callStk.back().returnValue = what;
    if (what && whatPolicy == YM_BORROW) {
        secure(*what);
    }
    return true;
}

bool YmCtx::getProperty(YmType& propertyType, YmLocal where) {
    if (!_absIndex(where)) {
        _ym::Global::raiseErr(
            YmErrCode_LocalNotFound,
            "Property get failed; local object index {} out-of-bounds!",
            where);
        return false;
    }
    if (propertyType.kind() != YmKind_Property) {
        _ym::Global::raiseErr(
            YmErrCode_NonPropertyType,
            "Property get failed; {} is not a property type!",
            propertyType.fullname());
        return false;
    }
    if (locals() == 0) {
        _ym::Global::raiseErr(
            YmErrCode_LocalNotFound,
            "Property get failed; subject not found!");
        return false;
    }
    auto& subject = ym::deref(local(locals() - 1));
    if (subject.type != propertyType.owner()) {
        _ym::Global::raiseErr(
            YmErrCode_TypeMismatch,
            "Property get failed; subject is {}, but expected {}!",
            subject.type->fullname(),
            propertyType.owner()->fullname());
        return false;
    }
    if (propertyType.info->isStoredPropertyGet()) { // Stored
        auto result = ym::Safe(subject.slot(propertyType.info->storedPropertySlot().value()).ref);
        secure(*result);
        pop(1);
        put(where, *result);
        return true;
    }
    else { // Computed
        return call(propertyType, 1, "", where);
    }
}

bool YmCtx::setProperty(YmType& propertyType) {
    if (propertyType.kind() != YmKind_Property) {
        _ym::Global::raiseErr(
            YmErrCode_NonPropertyType,
            "Property set failed; {} is not a property type!",
            propertyType.fullname());
        return false;
    }
    auto assigner_ptr = propertyType.assigner();
    if (!assigner_ptr) {
        _ym::Global::raiseErr(
            YmErrCode_ReadOnlyPropertyType,
            "Property set failed; {} is read-only!",
            propertyType.fullname());
        return false;
    }
    auto& assigner = ym::deref(assigner_ptr);
    if (locals() == 0) {
        _ym::Global::raiseErr(
            YmErrCode_LocalNotFound,
            "Property get failed; subject/value not found!");
        return false;
    }
    if (locals() == 1) {
        _ym::Global::raiseErr(
            YmErrCode_LocalNotFound,
            "Property get failed; value not found!");
        return false;
    }
    auto& subject = ym::deref(local(locals() - 2));
    auto& value = ym::deref(local(locals() - 1));
    if (subject.type != propertyType.owner()) {
        _ym::Global::raiseErr(
            YmErrCode_TypeMismatch,
            "Property set failed; subject is {}, but expected {}!",
            subject.type->fullname(),
            propertyType.owner()->fullname());
        return false;
    }
    if (value.type != propertyType.returnType()) {
        _ym::Global::raiseErr(
            YmErrCode_TypeMismatch,
            "Property set failed; value is {}, but expected {}!",
            value.type->fullname(),
            propertyType.returnType()->fullname());
        return false;
    }
    if (assigner.info->isStoredPropertySet()) { // Stored
        auto& target = subject.slot(assigner.info->storedPropertySlot().value()).ref;
        // Release slot's current ref.
        release(ym::deref(target));
        // Assign new ref, stealing it from stack.
        target = pull();
        // Pop subject.
        pop(1);
        return true;
    }
    else { // Computed
        return call(assigner, 2, "", YM_DISCARD);
    }
}

bool YmCtx::convert(YmType& type, YmLocal returnTo) {
    if (!_absIndex(returnTo)) {
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
    auto& input = ym::deref(local(locals() - 1));
    if (ymType_Converts(input.type, &type, YM_FALSE) == YM_FALSE) {
        _ym::Global::raiseErr(
            YmErrCode_IllegalConversion,
            "Conversion failed; {} -> {} is illegal!",
            input.type->fullname(),
            type.fullname());
        return false;
    }
    auto inIsP = input.type->kind() == YmKind_Protocol;
    auto outIsP = type.kind() == YmKind_Protocol;
    if (input.type == &type) {
        return put(returnTo, ym::deref(pull()), YM_TAKE);
    }
    else if (&type == ymCtx_LdNone(this)) {
        pop(1);
        return ymCtx_PutNone(this, returnTo) == YM_TRUE;
    }
    else if (!inIsP && outIsP) { // Box T -> P
        if (auto ptable = _ptables.load(type, *input.type)) {
            auto protoVal = ym::Safe(create(type));
            // Transfer object into box (ie. moving ownership of it.)
            protoVal->box(ym::Safe(pull()), *ptable);
            return put(returnTo, *protoVal, YM_TAKE);
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
        auto old = ym::bindScoped(ym::Safe(pull())); // RAII
        // The old protocol value might be referenced elsewhere, so it's ref can't
        // be stolen from it, so we pass YM_BORROW to copy the ref.
        return put(returnTo, *old->boxed(), YM_BORROW);
    }
    else if (inIsP && outIsP) { // P -> P
        if (auto ptable = _ptables.load(type, *input.boxed()->type)) {
            auto old = ym::bindScoped(ym::Safe(pull())); // RAII
            auto protoVal = ym::Safe(create(type));
            // The old protocol value might be referenced elsewhere, so it's ref can't
            // be stolen from it, so we add an incr for the new protocol value to own.
            secure(*old->boxed());
            protoVal->box(ym::Safe(old->boxed()), *ptable);
            return put(returnTo, *protoVal, YM_TAKE);
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
            return ymCtx_PutRune(this, returnTo, _uint2rune((YmUInt)*v)) == YM_TRUE;
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
            return ymCtx_PutRune(this, returnTo, _uint2rune((YmUInt)*v)) == YM_TRUE;
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
            return ymCtx_PutRune(this, returnTo, _uint2rune((YmUInt)*v)) == YM_TRUE;
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

void YmCtx::_beginUserPseudoCall() {
    ymAssert(_callStk.empty());
    _callStk.push_back(_CallFrame{
        .fn = nullptr,
        .returnTo = YmLocal{},
        .localsOffset = 0,
        });
}

bool YmCtx::_beginCall(YmType& fn, YmUInt16 args, std::string_view argNames, YmLocal returnTo) {
    ymAssert(!_callStk.empty());
    if (!fn.isCallable()) {
        _ym::Global::raiseErr(
            YmErrCode_NonCallableType,
            "Call to {} failed; {} is non-callable!",
            fn.fullname(),
            fn.fullname());
        return false;
    }
    if (!_absIndex(returnTo)) {
        _ym::Global::raiseErr(
            YmErrCode_LocalNotFound,
            "Call to {} failed; local object index {} out-of-bounds!",
            fn.fullname(),
            returnTo);
        return false;
    }
    if (YmLocals(args) > locals()) {
        _ym::Global::raiseErr(
            YmErrCode_LocalNotFound,
            "Call to {} failed; {} args provided, but local object stack height is {}!",
            fn.fullname(),
            args,
            locals());
        return false;
    }
    if (callStkHeight() == YM_MAX_CALL_STACK_HEIGHT) {
        _ym::Global::raiseErr(
            YmErrCode_CallStackOverflow,
            "Call to {} failed; call stack overflow!",
            fn.fullname());
        return false;
    }
    _ym::ArgPackInfo argPack(fn);
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
                return false;
            }
            if (!argPack.specifyNextNamedArg(tparam->index())) {
                _ym::Global::raiseErr(
                    YmErrCode_IllegalNameList,
                    "Call to {} failed; named param {} specified multiple times!",
                    fn.fullname(),
                    tparam->name());
                return false;
            }
        }
        else {
            _ym::Global::raiseErr(
                YmErrCode_IllegalNameList,
                "Call to {} failed; unknown named param \"{}\"!",
                fn.fullname(),
                (std::string)argName);
            return false;
        }
    }
    argPack.done(); // Don't forget!
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
            auto t = ym::deref(local(locals() - args + *argOffset)).type;
            if (auto p = fn.param(param); !p->type().sameAs(t)) {
                _ym::Global::raiseErr(
                    YmErrCode_TypeMismatch,
                    "Call to {} failed; arg #{} (for {} param {}) is {}, but expected {}!",
                    fn.fullname(),
                    *argOffset + 1,
                    p->isPositional() ? "positional" : "named",
                    p->name(),
                    t->fullname(),
                    p->type().fullname());
                return false;
            }
        }
    }
    // Append arg pack w/ dummy objects, then push call frame, and return.
    for (YmParams i = 0; i < argPack.dummies(); i++) {
        ymCtx_PutNone(this, YM_PUSH);
    }
    _callStk.push_back(_CallFrame{
        .fn = &fn,
        .argPack = std::move(argPack),
        .returnTo = returnTo,
        .localsOffset = YmUInt32(_globalObjStk.size()),
        });
    return true;
}

bool YmCtx::_endCall() noexcept {
    ymAssert(!_callStk.empty());
    _CallFrame cf = _callStk.back();
    pop(locals());
    _callStk.pop_back();
    if (!cf.fn) {
        // This is user pseudo-call.
        return true;
    }
    else if (!cf.returnValue) {
        _ym::Global::raiseErr(
            YmErrCode_CallProcedureError,
            "Call to {} failed; didn't bind a return value!",
            cf.fn->fullname());
        pop(cf.dummies());
        return false;
    }
    else if (cf.returnValue->type != cf.fn->returnType()) {
        _ym::Global::raiseErr(
            YmErrCode_CallProcedureError,
            "Call to {} failed; returned {}, but expected {}!",
            cf.fn->fullname(),
            cf.returnValue->type->fullname(),
            cf.fn->returnType()->fullname());
        pop(cf.dummies());
        release(*cf.returnValue);
        return false;
    }
    else {
        pop(cf.args());
        return put(cf.returnTo, *cf.returnValue);
    }
}

void YmCtx::_dispatchCall(YmType& fn) {
    auto& cf = _callStk.back();
    if (fn.isMethodReq()) { // Protocol Method Dispatch
        // NOTE: Prior to changing fwdFromProto, arg(0) shouldn't see through boxing.
        auto callobj = arg(0);
        auto ptableInd = (uintptr_t)ym::deref(fn.info->callBehaviour()).user;
        auto forwardedTo = callobj->ptable()[ptableInd];
        cf.fn = forwardedTo;
        cf.fwdFromProto = true;
        // If indirectly called method has named params, we gotta add proper number
        // of dummies to cf.argPack, and we gotta push dummy objects for each.
        if (auto named = forwardedTo->namedParams(); named >= 1) {
            for (YmParams i = 0; i < named; i++) {
                ymCtx_PutNone(this, YM_PUSH);
            }
            cf.argPack.addDummies(named);
            cf.localsOffset += named;
        }
    }
    auto& callBhvrInfo = ym::deref(cf.fn->info->callBehaviour());
    callBhvrInfo.fn(this, callBhvrInfo.user);
}

std::optional<YmLocal> YmCtx::_absIndex(YmLocal x) const noexcept {
    if (x == YM_PUSH || x == YM_DISCARD) {
        return x;
    }
    if (x >= locals()) {
        return std::nullopt;
    }
    // The 'x < 0' part is to avoid the potential overflow related edge cases.
    if (x < 0 && locals() + x < 0) {
        return std::nullopt;
    }
    return
        x >= 0
        ? x
        : locals() + x;
}

std::optional<YmLocal> YmCtx::_absIndexForRead(YmLocal x) const noexcept {
    if (x == YM_PUSH || x == YM_DISCARD) {
        return std::nullopt;
    }
    return _absIndex(x);
}

YmRune YmCtx::_uint2rune(YmUInt x) noexcept {
    // TODO: Is there a bitwise trick we can use to avoid modulus?
    return x % 0x110000;
}

