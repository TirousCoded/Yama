

#include "yama.h"

#include <array>

#include "../internal/general.h"
#include "../internal/resources.h"
#include "../yama++/Safe.h"


using namespace ym;
using namespace _ym;


const YmChar* ymFmtYmRType(YmRType rtype) {
    constexpr std::array<const YmChar*, YmRType_Num> names{
        "Domain",
        "Context",
        "Parcel Def.",
        "Parcel",
    };
    return
        rtype < YmRType_Num
        ? names[size_t(rtype)]
        : "???";
}

YmDm* ymDm_Create(void) {
    return YmDm::create().disarm();
}

YmBool ymDm_BindParcelDef(YmDm* dm, const YmChar* path, YmParcelDef* parceldef) {
    return Safe(dm)->bindParcelDef(std::string(Safe(path)), Safe(parceldef));
}

YmCtx* ymCtx_Create(YmDm* dm) {
    return YmCtx::create(Res(dm)).disarm();
}

YmDm* ymCtx_Dm(YmCtx* ctx) {
    return cloneRef(Safe(ctx)->domain);
}

YmParcel* ymCtx_Import(YmCtx* ctx, const YmChar* path) {
    return disarmOrNull(Safe(ctx)->import(std::string(Safe(path))));
}

YmParcelDef* ymParcelDef_Create(void) {
    return YmParcelDef::create().disarm();
}

const YmChar* ymParcel_Path(YmParcel* parcel) {
    return Safe(parcel)->path().c_str();
}

YmRType _ymRType(void* x) {
    return Safe<Resource>(x)->rtype().value();
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

