

#include "yama.h"

#include <array>

#include "../internal/general.h"
#include "../internal/frontend-resources.h"
#include "../yama++/Safe.h"


using namespace ym;
using namespace _ym;


const YmChar* ymFmtKind(YmKind x) {
    static constexpr std::array<const YmChar*, YmKind_Num> names{
        "Fn.",
    };
    return
        x < YmKind_Num
        ? names[size_t(x)]
        : "???";
}

const YmChar* ymFmtConstType(YmConstType x) {
    static constexpr std::array<const YmChar*, YmConstType_Num> names{
        "Int Const.",
        "UInt Const.",
        "Float Const.",
        "Bool Const.",
        "Rune Const.",

        "Ref. Const.",
    };
    return
        x < YmConstType_Num
        ? names[size_t(x)]
        : "???";
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

YmItemIndex ymParcelDef_FnItem(struct YmParcelDef* parceldef, const YmChar* name) {
    return Safe(parceldef)->fnItem(std::string(Safe(name))).value_or(YM_NO_ITEM_INDEX);
}

YmConst ymParcelDef_IntConst(YmParcelDef* parceldef, YmItemIndex item, YmInt value) {
    return deref(Safe(parceldef)->info).pullConst(item, ConstInfo(value));
}

YmConst ymParcelDef_UIntConst(YmParcelDef* parceldef, YmItemIndex item, YmUInt value) {
    return deref(Safe(parceldef)->info).pullConst(item, ConstInfo(value));
}

YmConst ymParcelDef_FloatConst(YmParcelDef* parceldef, YmItemIndex item, YmFloat value) {
    return deref(Safe(parceldef)->info).pullConst(item, ConstInfo(value));
}

YmConst ymParcelDef_BoolConst(YmParcelDef* parceldef, YmItemIndex item, YmBool value) {
    return deref(Safe(parceldef)->info).pullConst(item, ConstInfo(value));
}

YmConst ymParcelDef_RuneConst(YmParcelDef* parceldef, YmItemIndex item, YmRune value) {
    return deref(Safe(parceldef)->info).pullConst(item, ConstInfo(value));
}

YmConst ymParcelDef_RefConst(YmParcelDef* parceldef, YmItemIndex item, const YmChar* symbol) {
    return deref(Safe(parceldef)->info).pullConst(item, ConstInfo(RefConstInfo{ .sym = symbol }));
}

const YmChar* ymParcel_Path(YmParcel* parcel) {
    return Safe(parcel)->path.c_str();
}

YmParcel* ymItem_Parcel(YmItem* item) {
    return Safe(item)->parcel;
}

const YmChar* ymItem_Fullname(YmItem* item) {
    return Safe(item)->fullname.c_str();
}

YmKind ymItem_Kind(YmItem* item) {
    return Safe(item)->kind();
}

YmWord ymItem_Consts(YmItem* item) {
    return Safe(item)->consts().size();
}

YmConstType ymItem_ConstType(YmItem* item, YmConst index) {
    ymAssert(index < Safe(item)->consts().size());
    return constTypeOf(Safe(item)->consts()[index]);
}

YmInt ymItem_IntConst(YmItem* item, YmConst index) {
    return Safe(item)->constAs<YmConstType_Int>(index);
}

YmUInt ymItem_UIntConst(YmItem* item, YmConst index) {
    return Safe(item)->constAs<YmConstType_UInt>(index);
}

YmFloat ymItem_FloatConst(YmItem* item, YmConst index) {
    return Safe(item)->constAs<YmConstType_Float>(index);
}

YmBool ymItem_BoolConst(YmItem* item, YmConst index) {
    return Safe(item)->constAs<YmConstType_Bool>(index);
}

YmRune ymItem_RuneConst(YmItem* item, YmConst index) {
    return Safe(item)->constAs<YmConstType_Rune>(index);
}

YmItem* ymItem_RefConst(YmItem* item, YmConst index) {
    return Safe(item)->constAs<YmConstType_Ref>(index);
}

void ymParcelIter_Start(YmCtx* ctx) {
    Global::pIterStart(Safe(ctx));
}

void ymParcelIter_StartFrom(struct YmCtx* ctx, YmParcel* startFrom) {
    Global::pIterStartFrom(Safe(ctx), Safe(startFrom));
}

void ymParcelIter_Advance(YmWord n) {
    Global::pIterAdvance(n);
}

YmParcel* ymParcelIter_Get() {
    return Global::pIterGet();
}

YmBool ymParcelIter_Done() {
    return Global::pIterDone();
}

