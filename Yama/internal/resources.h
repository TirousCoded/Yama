

#pragma once


#ifdef _YM_FORBID_INCLUDE_IN_YAMA_DOT_H
#error Not allowed to expose resources.h to header file yama.h!
#endif


#include "../yama/yama.h"
#include "general.h"
#include "Res.h"
#include "Resource.h"

#include <unordered_map>


struct YmDm final : public _ym::Resource {
public:
    inline YmDm() : _ym::Resource(YmRType_Dm) {}


    // TODO: Maybe add string interning later.

    YmBool bindParcelDef(const std::string& path, ym::Safe<YmParcelDef> parceldef);


    static inline ym::Safe<YmDm> create() { return ym::Safe(new YmDm()); }
    static inline void destroy(ym::Safe<YmDm> x) noexcept { delete x.get(); }


private:
    friend struct YmCtx;


    // TODO: Currently, our impl here using _bindings is NOT THREAD-SAFE AT ALL!!!

    std::unordered_map<std::string, _ym::Res<YmParcel>> _bindings;
};

struct YmCtx final : public _ym::Resource {
public:
    const _ym::Res<YmDm> domain;


    YmCtx(_ym::Res<YmDm> domain) : _ym::Resource(YmRType_Ctx), domain(domain) {}


    YmParcel* import(const std::string& path);


    static ym::Safe<YmCtx> create(_ym::Res<YmDm> domain) { return ym::Safe(new YmCtx(std::move(domain))); }
    static void destroy(ym::Safe<YmCtx> x) noexcept { delete x.get(); }


private:
    std::unordered_map<std::string, _ym::Res<YmParcel>> _imports;
};

// TODO: When binding parcel defs. to domain, since most of the time the parcel def.
//       won't be modified afterwards, we can store its state in a std::shared_ptr,
//       and then ONLY COPY A REF TO THIS DATA.
//
//       Then, if the parcel def. DOES get modified, we can use 'Copy-On-Write (COW)'
//       to perform a copy only once we actually need to.
//
//       This method also means that binding a parcel def. to multiple domains, or
//       multiple times in the same domain under different paths, should likewise
//       also benefit from all sharing the same immutable data.

struct YmParcelDef final : public _ym::Resource {
public:
    YmParcelDef() : _ym::Resource(YmRType_ParcelDef) {}


    static ym::Safe<YmParcelDef> create() { return ym::Safe(new YmParcelDef()); }
    static void destroy(ym::Safe<YmParcelDef> x) noexcept { delete x.get(); }
};

struct YmParcel final : public _ym::Resource {
public:
    const std::string path;


    YmParcel(std::string path) : _ym::Resource(YmRType_Parcel), path(std::move(path)) {}


    static ym::Safe<YmParcel> create(std::string path) { return ym::Safe(new YmParcel(std::move(path))); }
    static void destroy(ym::Safe<YmParcel> x) noexcept { delete x.get(); }
};

