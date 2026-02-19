

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
    return new YmDm();
}

void ymDm_Destroy(YmDm* dm) {
    delete Safe(dm).get();
}

YmBool ymDm_BindParcelDef(YmDm* dm, const YmChar* path, YmParcelDef* parceldef) {
    return (YmBool)Safe(dm)->bindParcelDef(std::string(Safe(path)), Safe(parceldef));
}

YmBool ymDm_AddRedirect(YmDm* dm, const YmChar* subject, const YmChar* before, const YmChar* after) {
    return (YmBool)Safe(dm)->addRedirect(std::string(Safe(subject)), std::string(Safe(before)), std::string(Safe(after)));
}

YmCtx* ymCtx_Create(YmDm* dm) {
    return new YmCtx(Safe(dm));
}

void ymCtx_Destroy(YmCtx* ctx) {
    delete Safe(ctx).get();
}

YmDm* ymCtx_Dm(YmCtx* ctx) {
    return Safe(ctx)->domain;
}

YmParcel* ymCtx_Import(YmCtx* ctx, const YmChar* path) {
    return Safe(ctx)->import(std::string(Safe(path))).get();
}

YmItem* ymCtx_Load(YmCtx* ctx, const YmChar* fullname) {
    return Safe(ctx)->load(std::string(Safe(fullname))).get();
}

void ymCtx_NaturalizeParcel(YmCtx* ctx, YmParcel* parcel) {
    assertSafe(ctx);
    assertSafe(parcel);
    // TODO
}

void ymCtx_NaturalizeItem(YmCtx* ctx, YmItem* item) {
    assertSafe(ctx);
    assertSafe(item);
    // TODO
}

YmParcelDef* ymParcelDef_Create(void) {
    return new YmParcelDef();
}

void ymParcelDef_Destroy(YmParcelDef* parceldef) {
    delete Safe(parceldef).get();
}

YmItemIndex ymParcelDef_AddStruct(
    YmParcelDef* parceldef,
    const YmChar* name) {
    return Safe(parceldef)->addStruct(
        std::string(Safe(name)))
        .value_or(YM_NO_ITEM_INDEX);
}

YmItemIndex ymParcelDef_AddProtocol(YmParcelDef* parceldef, const YmChar* name) {
    return Safe(parceldef)->addProtocol(
        std::string(Safe(name)))
        .value_or(YM_NO_ITEM_INDEX);
}

YmItemIndex ymParcelDef_AddFn(
    YmParcelDef* parceldef,
    const YmChar* name,
    YmRefSym returnType,
    YmCallBhvrCallbackFn callBehaviour,
    void* callBehaviourData) {
    return Safe(parceldef)->addFn(
        std::string(Safe(name)),
        std::string(Safe(returnType)),
        _ym::CallBhvrCallbackInfo::mk(callBehaviour, callBehaviourData))
        .value_or(YM_NO_ITEM_INDEX);
}

YmItemIndex ymParcelDef_AddMethod(
    YmParcelDef* parceldef,
    YmItemIndex owner,
    const YmChar* name,
    YmRefSym returnType,
    YmCallBhvrCallbackFn callBehaviour,
    void* callBehaviourData) {
    return Safe(parceldef)->addMethod(
        owner,
        std::string(Safe(name)),
        std::string(Safe(returnType)),
        _ym::CallBhvrCallbackInfo::mk(callBehaviour, callBehaviourData))
        .value_or(YM_NO_ITEM_INDEX);
}

YmItemIndex ymParcelDef_AddMethodReq(YmParcelDef* parceldef, YmItemIndex owner, const YmChar* name, YmRefSym returnType) {
    return Safe(parceldef)->addMethodReq(
        owner,
        std::string(Safe(name)),
        std::string(Safe(returnType)))
        .value_or(YM_NO_ITEM_INDEX);
}

YmItemParamIndex ymParcelDef_AddItemParam(YmParcelDef* parceldef, YmItemIndex item, const YmChar* name, YmRefSym constraintType) {
    return Safe(parceldef)->addItemParam(
        item,
        std::string(Safe(name)),
        std::string(Safe(constraintType)))
        .value_or(YM_NO_ITEM_PARAM_INDEX);
}

YmParamIndex ymParcelDef_AddParam(
    YmParcelDef* parceldef,
    YmItemIndex item,
    const YmChar* name,
    YmRefSym paramType) {
    return Safe(parceldef)->addParam(
        item,
        std::string(Safe(name)),
        std::string(Safe(paramType)))
        .value_or(YM_NO_PARAM_INDEX);
}

YmRef ymParcelDef_AddRef(
    YmParcelDef* parceldef,
    YmItemIndex item,
    YmRefSym symbol) {
    return Safe(parceldef)->addRef(
        item,
        std::string(Safe(symbol)))
        .value_or(YM_NO_REF);
}

const YmChar* ymParcel_Path(YmParcel* parcel) {
    return Safe(parcel)->path.string().c_str();
}

YmParcel* ymItem_Parcel(YmItem* item) {
    return Safe(item)->parcel;
}

const YmChar* ymItem_Fullname(YmItem* item) {
    return Safe(item)->fullname().string().c_str();
}

YmKind ymItem_Kind(YmItem* item) {
    return Safe(item)->kind();
}

YmItem* ymItem_Owner(YmItem* item) {
    return Safe(item)->owner();
}

YmMembers ymItem_Members(YmItem* item) {
    return Safe(item)->members();
}

YmItem* ymItem_MemberByIndex(YmItem* item, YmMemberIndex member) {
    return Safe(item)->member(member);
}

YmItem* ymItem_MemberByName(YmItem* item, const YmChar* name) {
    return Safe(item)->member(std::string(Safe(name)));
}

YmItemParams ymItem_ItemParams(YmItem* item) {
    return Safe(item)->itemParams();
}

YmItem* ymItem_ItemParamByIndex(YmItem* item, YmItemParamIndex itemParam) {
    return Safe(item)->itemParam(itemParam);
}

YmItem* ymItem_ItemParamByName(YmItem* item, const YmChar* name) {
    return Safe(item)->itemParam(std::string(Safe(name)));
}

YmItem* ymItem_ReturnType(YmItem* item) {
    return Safe(item)->returnType();
}

YmParams ymItem_Params(YmItem* item) {
    return Safe(item)->params();
}

const YmChar* ymItem_ParamName(YmItem* item, YmParamIndex param) {
    return Safe(item)->paramName(param);
}

YmItem* ymItem_ParamType(YmItem* item, YmParamIndex param) {
    return Safe(item)->paramType(param);
}

YmItem* ymItem_Ref(YmItem* item, YmRef reference) {
    return Safe(item)->ref(reference);
}

YmRef ymItem_FindRef(YmItem* item, YmItem* referenced) {
    return Safe(item)->findRef(Safe(referenced)).value_or(YM_NO_REF);
}

YmBool ymItem_Converts(YmItem* from, YmItem* to, YmBool coercion) {
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

