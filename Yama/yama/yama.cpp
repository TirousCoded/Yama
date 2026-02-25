

#include "yama.h"

#include <array>

#include "../internal/general.h"
#include "../internal/frontend-resources.h"
#include "../yama++/Safe.h"


using namespace ym;
using namespace _ym;


const YmChar* ymFmtKind(YmKind x) {
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

YmDm* ymCtx_Dm(YmCtx* ctx) {
    return Safe(ctx)->domain;
}

YmParcel* ymCtx_Import(YmCtx* ctx, YmPath path) {
    return Safe(ctx)->import(std::string(Safe(path))).get();
}

YmType* ymCtx_Load(YmCtx* ctx, YmFullname fullname) {
    return Safe(ctx)->load(std::string(Safe(fullname))).get();
}

YmType* ymCtx_LdNone(YmCtx* ctx) {
    // TODO: Add actual fast path later.
    return ymCtx_Load(ctx, "yama:None");
}

YmType* ymCtx_LdInt(YmCtx* ctx) {
    // TODO: Add actual fast path later.
    return ymCtx_Load(ctx, "yama:Int");
}

YmType* ymCtx_LdUInt(YmCtx* ctx) {
    // TODO: Add actual fast path later.
    return ymCtx_Load(ctx, "yama:UInt");
}

YmType* ymCtx_LdFloat(YmCtx* ctx) {
    // TODO: Add actual fast path later.
    return ymCtx_Load(ctx, "yama:Float");
}

YmType* ymCtx_LdBool(YmCtx* ctx) {
    // TODO: Add actual fast path later.
    return ymCtx_Load(ctx, "yama:Bool");
}

YmType* ymCtx_LdRune(YmCtx* ctx) {
    // TODO: Add actual fast path later.
    return ymCtx_Load(ctx, "yama:Rune");
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

YmRef ymType_FindRef(YmType* type, YmType* referenced) {
    return Safe(type)->findRef(Safe(referenced)).value_or(YM_NO_REF);
}

YmBool ymType_Converts(YmType* from, YmType* to, YmBool coercion) {
    Safe _from(from), _to(to);
    if (_from == _to) {
        return YM_TRUE;
    }
    auto fromK = _from->kind();
    auto toK = _to->kind();
    bool fromIsP = fromK == YmKind_Protocol;
    bool toIsP = toK == YmKind_Protocol;
    if (fromIsP && toIsP)                               return YM_TRUE;
    else if (!fromIsP && toIsP && _from->conforms(_to)) return YM_TRUE;
    else if (fromIsP && !toIsP && _to->conforms(_from)) return (YmBool)!coercion;
    else                                                return YM_FALSE;
}

void ymParcelIter_Start(YmCtx* ctx) {
    Global::pIterStart(Safe(ctx));
}

void ymParcelIter_StartFrom(YmCtx* ctx, YmParcel* startFrom) {
    Global::pIterStartFrom(Safe(ctx), Safe(startFrom));
}

void ymParcelIter_Advance(size_t n) {
    Global::pIterAdvance(n);
}

YmParcel* ymParcelIter_Get(void) {
    return Global::pIterGet();
}

YmBool ymParcelIter_Done(void) {
    return Global::pIterDone();
}

