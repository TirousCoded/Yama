

#include "yama.h"

#include "../internal/general.h"
#include "../internal/resources.h"
#include "../yama++/Safe.h"


using namespace ym;
using namespace _ym;


const YmChar* ymFmtYmRType(YmRType rtype) {
    static_assert(YmRType_Num == 4);
    switch (rtype) {
    case YmRType_Dm:        return "Domain";
    case YmRType_Ctx:       return "Context";
    case YmRType_ParcelDef: return "Parcel Def.";
    case YmRType_Parcel:    return "Parcel";
    default:                return "???";
    }
}

YmDm* ymDm_Create(void) {
    return _ym::cloneRef(YmDm::create());
}

YmBool ymDm_BindParcelDef(YmDm* dm, const YmChar* path, YmParcelDef* parceldef) {
    return Safe(dm)->bindParcelDef(std::string(Safe(path)), Safe(parceldef));
}

YmCtx* ymCtx_Create(YmDm* dm) {
    return _ym::cloneRef(YmCtx::create(_ym::Res(dm)));
}

YmDm* ymCtx_Dm(YmCtx* ctx) {
    return _ym::cloneRef(Safe(ctx)->domain);
}

YmParcel* ymCtx_Import(YmCtx* ctx, const YmChar* path) {
    return Safe(ctx)->import(std::string(Safe(path)));
}

YmParcelDef* ymParcelDef_Create(void) {
    return _ym::cloneRef(YmParcelDef::create());
}

const YmChar* ymParcel_Path(YmParcel* parcel) {
    return Safe(parcel)->path.c_str();
}

YmRType _ymRType(void* x) {
    return Safe<Resource>(x)->rtype();
}

YmRefCount _ymRefCount(void* x) {
    return Safe<Resource>(x)->refcount();
}

void _ymAddRef(void* x) {
    Safe<Resource>(x)->addRef();
}

void _ymDrop(void* x) {
    Safe<Resource>(x)->drop();
}

