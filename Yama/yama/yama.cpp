

#include "yama.h"

#include <array>

#include "../internal/general.h"
#include "../internal/frontend-resources.h"
#include "../yama++/Safe.h"


using namespace ym;
using namespace _ym;


const YmChar* ymKind_Fmt(YmKind x) {
    static constexpr std::array<const YmChar*, YmKind_Num> names{
        "Struct",
        "Protocol",
        "Fn",
        "Method",
    };
    return
        x < YmKind_Num
        ? names[size_t(x)]
        : "???";
}

YmBool ymKind_IsCallable(YmKind x) {
    ymAssert(x < YmKind_Num);
    static constexpr std::array<bool, YmKind_Num> values{
        false,
        false,
        true,
        true,
    };
    return values[size_t(x)];
}

YmBool ymKind_IsMember(YmKind x) {
    ymAssert(x < YmKind_Num);
    static constexpr std::array<bool, YmKind_Num> values{
        false,
        false,
        false,
        true,
    };
    return values[size_t(x)];
}

YmBool ymKind_HasMembers(YmKind x) {
    ymAssert(x < YmKind_Num);
    static constexpr std::array<bool, YmKind_Num> values{
        true,
        true,
        false,
        false,
    };
    return values[size_t(x)];
}

void ymInertCallBhvrFn(YmCtx*, void*) {
    // Do nothing.
}

YmDm* ymDm_Create(void) {
    auto result = new YmDm();
    result->refs.addRef();
    return result;
}

YmRefCount ymDm_Secure(YmDm* dm) {
    return Safe(dm)->refs.addRef();
}

YmRefCount ymDm_Release(YmDm* dm) {
    auto old = Safe(dm)->refs.drop();
    if (old == 1) {
        delete Safe(dm).get();
    }
    return old;
}

YmRefCount ymDm_RefCount(YmDm* dm) {
    return Safe(dm)->refs.count();
}

YmBool ymDm_BindParcelDef(YmDm* dm, YmPath path, YmParcelDef* parceldef) {
    return (YmBool)Safe(dm)->bindParcelDef(std::string(Safe(path)), Safe(parceldef));
}

YmBool ymDm_AddRedirect(YmDm* dm, YmPath subject, YmPath before, YmPath after) {
    return (YmBool)Safe(dm)->addRedirect(std::string(Safe(subject)), std::string(Safe(before)), std::string(Safe(after)));
}

size_t ymDm_ForEachParcel(YmDm* dm, YmForEachParcelCallbackFn callback, void* user) {
    ymAssert(callback != nullptr);
    return Safe(dm)->forEachParcel(callback, user);
}

YmCtx* ymCtx_Create(YmDm* dm) {
    auto result = new YmCtx(Safe(dm));
    result->refs.addRef();
    return result;
}

YmRefCount ymCtx_Secure(YmCtx* ctx) {
    return Safe(ctx)->refs.addRef();
}

YmRefCount ymCtx_Release(YmCtx* ctx) {
    auto old = Safe(ctx)->refs.drop();
    if (old == 1) {
        delete Safe(ctx).get();
    }
    return old;
}

YmRefCount ymCtx_RefCount(YmCtx* ctx) {
    return Safe(ctx)->refs.count();
}

YmDm* ymCtx_Dm(YmCtx* ctx, YmRefPolicy returnPolicy) {
    if (returnPolicy == YM_TAKE) {
        ymDm_Secure(Safe(ctx)->domain);
    }
    return Safe(ctx)->domain;
}

YmParcel* ymCtx_Import(YmCtx* ctx, YmPath path) {
    return Safe(ctx)->import(std::string(Safe(path))).get();
}

YmType* ymCtx_Load(YmCtx* ctx, YmFullname fullname) {
    return Safe(ctx)->load(std::string(Safe(fullname))).get();
}

YmType* ymCtx_LdNone(YmCtx* ctx) {
    return &Safe(ctx)->loader->ldNone();
}

YmType* ymCtx_LdInt(YmCtx* ctx) {
    return &Safe(ctx)->loader->ldInt();
}

YmType* ymCtx_LdUInt(YmCtx* ctx) {
    return &Safe(ctx)->loader->ldUInt();
}

YmType* ymCtx_LdFloat(YmCtx* ctx) {
    return &Safe(ctx)->loader->ldFloat();
}

YmType* ymCtx_LdBool(YmCtx* ctx) {
    return &Safe(ctx)->loader->ldBool();
}

YmType* ymCtx_LdRune(YmCtx* ctx) {
    return &Safe(ctx)->loader->ldRune();
}

YmType* ymCtx_LdType(YmCtx* ctx) {
    return &Safe(ctx)->loader->ldType();
}

void ymCtx_NaturalizeParcel(YmCtx* ctx, YmParcel* parcel) {
    assertSafe(ctx);
    assertSafe(parcel);
    // TODO
}

void ymCtx_NaturalizeType(YmCtx* ctx, YmType* type) {
    assertSafe(ctx);
    assertSafe(type);
    // TODO
}

void ymCtx_Panic(YmCtx* ctx, const YmChar* fmt, ...) {
    // TODO
}

void ymCtx_Recover(YmCtx* ctx) {
    // TODO
}

const YmChar* ymCtx_GetPanic(YmCtx* ctx) {
    return nullptr;
}

YmObj* ymCtx_NewNone(YmCtx* ctx) {
    return Safe(ctx)->newNone();
}

YmObj* ymCtx_NewInt(YmCtx* ctx, YmInt v) {
    return Safe(ctx)->newInt(v);
}

YmObj* ymCtx_NewUInt(YmCtx* ctx, YmUInt v) {
    return Safe(ctx)->newUInt(v);
}

YmObj* ymCtx_NewFloat(YmCtx* ctx, YmFloat v) {
    return Safe(ctx)->newFloat(v);
}

YmObj* ymCtx_NewBool(YmCtx* ctx, YmBool v) {
    return Safe(ctx)->newBool(v);
}

YmObj* ymCtx_NewRune(YmCtx* ctx, YmRune v) {
    return Safe(ctx)->newRune(v);
}

YmObj* ymCtx_NewType(YmCtx* ctx, YmType* v) {
    return Safe(ctx)->newType(deref(v));
}

YmCallStackHeight ymCtx_CallStackHeight(YmCtx* ctx) {
    return Safe(ctx)->callStkHeight();
}

const YmChar* ymCtx_FmtCallStack(YmCtx* ctx, YmCallStackHeight skip) {
    auto temp = Safe(ctx)->fmtCallStk(skip);
    return mkCStr(temp.c_str());
}

YmUInt16 ymCtx_Args(YmCtx* ctx) {
    return Safe(ctx)->args();
}

YmObj* ymCtx_Arg(YmCtx* ctx, YmUInt16 which, YmRefPolicy returnPolicy) {
    return Safe(ctx)->arg(which, returnPolicy);
}

YmType* ymCtx_Ref(YmCtx* ctx, YmRef reference) {
    return Safe(ctx)->ref(reference);
}

YmLocals ymCtx_Locals(YmCtx* ctx) {
    return Safe(ctx)->locals();
}

YmObj* ymCtx_Local(YmCtx* ctx, YmLocal where, YmRefPolicy returnPolicy) {
    return Safe(ctx)->local(where, returnPolicy);
}

void ymCtx_PopN(YmCtx* ctx, YmLocals n) {
    Safe(ctx)->popN(n);
}

YmObj* ymCtx_Pop(YmCtx* ctx) {
    return Safe(ctx)->pop();
}

YmBool ymCtx_Put(YmCtx* ctx, YmLocal where, YmObj* what, YmRefPolicy whatPolicy) {
    return Safe(ctx)->put(where, deref(what), whatPolicy);
}

YmBool ymCtx_PutNone(YmCtx* ctx, YmLocal where) {
    return ymCtx_Put(ctx, where, ymCtx_NewNone(ctx), YM_TAKE);
}

YmBool ymCtx_PutInt(YmCtx* ctx, YmLocal where, YmInt v) {
    return ymCtx_Put(ctx, where, ymCtx_NewInt(ctx, v), YM_TAKE);
}

YmBool ymCtx_PutUInt(YmCtx* ctx, YmLocal where, YmUInt v) {
    return ymCtx_Put(ctx, where, ymCtx_NewUInt(ctx, v), YM_TAKE);
}

YmBool ymCtx_PutFloat(YmCtx* ctx, YmLocal where, YmFloat v) {
    return ymCtx_Put(ctx, where, ymCtx_NewFloat(ctx, v), YM_TAKE);
}

YmBool ymCtx_PutBool(YmCtx* ctx, YmLocal where, YmBool v) {
    return ymCtx_Put(ctx, where, ymCtx_NewBool(ctx, v), YM_TAKE);
}

YmBool ymCtx_PutRune(YmCtx* ctx, YmLocal where, YmRune v) {
    return ymCtx_Put(ctx, where, ymCtx_NewRune(ctx, v), YM_TAKE);
}

YmBool ymCtx_PutType(YmCtx* ctx, YmLocal where, YmType* v) {
    return ymCtx_Put(ctx, where, ymCtx_NewType(ctx, v), YM_TAKE);
}

YmBool ymCtx_PutDefault(YmCtx* ctx, YmLocal where, YmType* type) {
    if (auto obj = Safe(ctx)->newDefault(deref(type))) {
        return ymCtx_Put(ctx, where, obj, YM_TAKE);
    }
    return YM_FALSE;
}

YmBool ymCtx_Call(YmCtx* ctx, YmType* fn, YmUInt16 argsN, YmLocal returnTo) {
    return Safe(ctx)->call(deref(fn), argsN, returnTo);
}

void ymCtx_Ret(YmCtx* ctx, YmObj* what, YmRefPolicy whatPolicy) {
    Safe(ctx)->ret(what, whatPolicy);
}

YmBool ymCtx_Convert(YmCtx* ctx, YmType* type, YmLocal returnTo) {
    return Safe(ctx)->convert(deref(type), returnTo);
}

YmParcelDef* ymParcelDef_Create(void) {
    auto result = new YmParcelDef();
    result->refs.addRef();
    return result;
}

YmRefCount ymParcelDef_Secure(YmParcelDef* parceldef) {
    return Safe(parceldef)->refs.addRef();
}

YmRefCount ymParcelDef_Release(YmParcelDef* parceldef) {
    auto old = Safe(parceldef)->refs.drop();
    if (old == 1) {
        delete Safe(parceldef).get();
    }
    return old;
}

YmRefCount ymParcelDef_RefCount(YmParcelDef* parceldef) {
    return Safe(parceldef)->refs.count();
}

YmTypeIndex ymParcelDef_AddStruct(
    YmParcelDef* parceldef,
    const YmChar* name) {
    return Safe(parceldef)->addStruct(
        std::string(Safe(name)))
        .value_or(YM_NO_TYPE_INDEX);
}

YmTypeIndex ymParcelDef_AddProtocol(YmParcelDef* parceldef, const YmChar* name) {
    return Safe(parceldef)->addProtocol(
        std::string(Safe(name)))
        .value_or(YM_NO_TYPE_INDEX);
}

YmTypeIndex ymParcelDef_AddFn(
    YmParcelDef* parceldef,
    const YmChar* name,
    YmRefSym returnType,
    YmCallBhvrCallbackFn callBehaviour,
    void* callBehaviourData) {
    return Safe(parceldef)->addFn(
        std::string(Safe(name)),
        std::string(Safe(returnType)),
        _ym::CallBhvrCallbackInfo::mk(callBehaviour, callBehaviourData))
        .value_or(YM_NO_TYPE_INDEX);
}

YmTypeIndex ymParcelDef_AddMethod(
    YmParcelDef* parceldef,
    YmTypeIndex owner,
    const YmChar* name,
    YmRefSym returnType,
    YmCallBhvrCallbackFn callBehaviour,
    void* callBehaviourData) {
    return Safe(parceldef)->addMethod(
        owner,
        std::string(Safe(name)),
        std::string(Safe(returnType)),
        _ym::CallBhvrCallbackInfo::mk(callBehaviour, callBehaviourData))
        .value_or(YM_NO_TYPE_INDEX);
}

YmTypeIndex ymParcelDef_AddMethodReq(YmParcelDef* parceldef, YmTypeIndex owner, const YmChar* name, YmRefSym returnType) {
    return Safe(parceldef)->addMethodReq(
        owner,
        std::string(Safe(name)),
        std::string(Safe(returnType)))
        .value_or(YM_NO_TYPE_INDEX);
}

YmTypeParamIndex ymParcelDef_AddTypeParam(YmParcelDef* parceldef, YmTypeIndex type, const YmChar* name, YmRefSym constraintType) {
    return Safe(parceldef)->addTypeParam(
        type,
        std::string(Safe(name)),
        std::string(Safe(constraintType)))
        .value_or(YM_NO_TYPE_PARAM_INDEX);
}

YmParamIndex ymParcelDef_AddParam(
    YmParcelDef* parceldef,
    YmTypeIndex type,
    const YmChar* name,
    YmRefSym paramType) {
    return Safe(parceldef)->addParam(
        type,
        std::string(Safe(name)),
        std::string(Safe(paramType)))
        .value_or(YM_NO_PARAM_INDEX);
}

YmRef ymParcelDef_AddRef(
    YmParcelDef* parceldef,
    YmTypeIndex type,
    YmRefSym symbol) {
    return Safe(parceldef)->addRef(
        type,
        std::string(Safe(symbol)))
        .value_or(YM_NO_REF);
}

YmPath ymParcel_Path(YmParcel* parcel) {
    return Safe(parcel)->path.string().c_str();
}

YmParcel* ymType_Parcel(YmType* type) {
    return Safe(type)->parcel;
}

YmFullname ymType_Fullname(YmType* type) {
    return Safe(type)->fullname().string().c_str();
}

YmKind ymType_Kind(YmType* type) {
    return Safe(type)->kind();
}

YmType* ymType_Owner(YmType* type) {
    return Safe(type)->owner();
}

YmMembers ymType_Members(YmType* type) {
    return Safe(type)->members();
}

YmType* ymType_MemberByIndex(YmType* type, YmMemberIndex member) {
    return Safe(type)->member(member);
}

YmType* ymType_MemberByName(YmType* type, const YmChar* name) {
    return Safe(type)->member(std::string(Safe(name)));
}

YmTypeParams ymType_TypeParams(YmType* type) {
    return Safe(type)->typeParams();
}

YmType* ymType_TypeParamByIndex(YmType* type, YmTypeParamIndex typeParam) {
    return Safe(type)->typeParam(typeParam);
}

YmType* ymType_TypeParamByName(YmType* type, const YmChar* name) {
    return Safe(type)->typeParam(std::string(Safe(name)));
}

YmType* ymType_ReturnType(YmType* type) {
    return Safe(type)->returnType();
}

YmParams ymType_Params(YmType* type) {
    return Safe(type)->params();
}

const YmChar* ymType_ParamName(YmType* type, YmParamIndex param) {
    return Safe(type)->paramName(param);
}

YmType* ymType_ParamType(YmType* type, YmParamIndex param) {
    return Safe(type)->paramType(param);
}

YmType* ymType_Ref(YmType* type, YmRef reference) {
    return Safe(type)->ref(reference);
}

YmBool ymType_Depends(YmType* type, YmType* other) {
    return (YmBool)Safe(type)->depends(Safe(other));
}

YmBool ymType_Converts(YmType* from, YmType* to, YmBool coercion) {
    Safe _from(from), _to(to);
    auto fromK = _from->kind();
    auto toK = _to->kind();
    if (ymKind_IsCallable(fromK) == YM_TRUE || ymKind_IsCallable(toK) == YM_TRUE) {
        return YM_FALSE;
    }
    if (_from == _to) {
        return YM_TRUE;
    }
    // TODO: The usage of fullname specs below is suboptimal compared to using the
    //       actual YmType* themselves. Try to fix this later, which may require
    //       the moving of this fn over to ymCtx_*** fns to let it use ymCtx_Ld***.
    auto& fromFln = _from->fullname();
    auto& toFln = _to->fullname();
    if (toFln == "yama:None" && !coercion) {
        return YM_TRUE;
    }
    else if (fromFln == "yama:Int" && !coercion) {
        return YmBool(
            toFln == "yama:UInt" ||
            toFln == "yama:Float" ||
            toFln == "yama:Rune");
    }
    else if (fromFln == "yama:UInt" && !coercion) {
        return YmBool(
            toFln == "yama:Int" ||
            toFln == "yama:Float" ||
            toFln == "yama:Rune");
    }
    else if (fromFln == "yama:Float" && !coercion) {
        return YmBool(
            toFln == "yama:Int" ||
            toFln == "yama:UInt" ||
            toFln == "yama:Rune");
    }
    else if (fromFln == "yama:Bool" && !coercion) {
        return YmBool(
            toFln == "yama:Int" ||
            toFln == "yama:UInt" ||
            toFln == "yama:Float");
    }
    else if (fromFln == "yama:Rune" && !coercion) {
        return YmBool(
            toFln == "yama:Int" ||
            toFln == "yama:UInt");
    }
    else {
        bool fromIsP = fromK == YmKind_Protocol;
        bool toIsP = toK == YmKind_Protocol;
        if (fromIsP && toIsP)                               return YM_TRUE;
        else if (!fromIsP && toIsP && _from->conforms(_to)) return YM_TRUE;
        else if (fromIsP && !toIsP && _to->conforms(_from)) return (YmBool)!coercion;
    }
    return YM_FALSE;
}

YmRefCount ymObj_Secure(YmObj* obj) {
    return Safe(obj)->ctx->secure(deref(obj));
}

YmRefCount ymObj_Release(YmObj* obj) {
    return Safe(obj)->ctx->release(deref(obj));
}

YmRefCount ymObj_RefCount(YmObj* obj) {
    return Safe(obj)->refs.count();
}

YmType* ymObj_Type(YmObj* obj) {
    return Safe(obj)->type;
}

const YmChar* ymObj_Fmt(YmObj* obj) {
    auto _obj = Safe(obj);
    if (_obj->isNone())         return mkCStr("n/a");
    else if (_obj->isInt())     return ymInt_Fmt(_obj->toInt().value(), YM_TRUE, YmIntFmt_Dec, nullptr);
    else if (_obj->isUInt())    return ymUInt_Fmt(_obj->toUInt().value(), YM_TRUE, YmIntFmt_Dec, nullptr);
    else if (_obj->isFloat())   return ymFloat_Fmt(_obj->toFloat().value(), nullptr);
    else if (_obj->isBool())    return mkCStr(ymBool_Fmt(_obj->toBool().value()));
    else if (_obj->isRune())    return ymRune_Fmt(_obj->toRune().value(), YM_TRUE, YM_TRUE, YM_TRUE, YM_TRUE, nullptr);
    else                        YM_DEADEND;
    return nullptr;
}

YmInt ymObj_ToInt(YmObj* obj, YmBool* succeeded) {
    auto r = Safe(obj)->toInt();
    if (succeeded) {
        *succeeded = r.has_value();
    }
    return r.value_or(0);
}

YmUInt ymObj_ToUInt(YmObj* obj, YmBool* succeeded) {
    auto r = Safe(obj)->toUInt();
    if (succeeded) {
        *succeeded = r.has_value();
    }
    return r.value_or(0);
}

YmFloat ymObj_ToFloat(YmObj* obj, YmBool* succeeded) {
    auto r = Safe(obj)->toFloat();
    if (succeeded) {
        *succeeded = r.has_value();
    }
    return r.value_or(0.0);
}

YmBool ymObj_ToBool(YmObj* obj, YmBool* succeeded) {
    auto r = Safe(obj)->toBool();
    if (succeeded) {
        *succeeded = r.has_value();
    }
    return r.value_or(YM_FALSE);
}

YmRune ymObj_ToRune(YmObj* obj, YmBool* succeeded) {
    auto r = Safe(obj)->toRune();
    if (succeeded) {
        *succeeded = r.has_value();
    }
    return r.value_or(U'\0');
}

YmType* ymObj_ToType(YmObj* obj, YmBool* succeeded) {
    auto r = Safe(obj)->toType();
    if (succeeded) {
        *succeeded = (bool)r;
    }
    return r;
}

