

#pragma once


#ifdef _YM_FORBID_INCLUDE_IN_YAMA_DOT_H
#error Not allowed to expose resources.h to header file yama.h!
#endif


#include <memory>
#include <mutex>
#include <optional>
#include <unordered_map>

#include "../yama/yama.h"
#include "general.h"
#include "Res.h"
#include "Resource.h"

static_assert(_ym::enumSize<_ym::RType> == 5); // Reminder
// NOTE: Include ALL our internal resources here.
#include "ParcelData.h"


struct YmDm final : public _ym::Resource {
public:
    inline YmDm() :
        Resource(_ym::RType::Dm) {}


    // TODO: Maybe add string interning later.

    YmBool bindParcelDef(const std::string& path, ym::Safe<YmParcelDef> parceldef);
    std::optional<_ym::Res<_ym::ParcelData>> import(const std::string& path);


    template<typename... Args>
    static inline _ym::Res<YmDm> create(Args&&... args) {
        return _ym::Res(new YmDm(std::forward<Args>(args)...));
    }
    static inline void destroy(ym::Safe<const YmDm> x) noexcept {
        delete x;
    }


private:
    std::mutex _mtx;
    std::unordered_map<std::string, _ym::Res<_ym::ParcelData>> _bindings;
};

struct YmCtx final : public _ym::Resource {
public:
    const _ym::Res<YmDm> domain;


    YmCtx(_ym::Res<YmDm> domain) :
        Resource(_ym::RType::Ctx),
        domain(domain) {}


    // Returns already imported parcel under path, if any.
    std::optional<_ym::Res<YmParcel>> fetch(const std::string& path) const noexcept;

    // Returns imported parcel under path, if any, attempting import if needed.
    std::optional<_ym::Res<YmParcel>> import(const std::string& path);


    template<typename... Args>
    static inline _ym::Res<YmCtx> create(Args&&... args) {
        return _ym::Res(new YmCtx(std::forward<Args>(args)...));
    }
    static inline void destroy(ym::Safe<const YmCtx> x) noexcept {
        delete x;
    }


private:
    std::unordered_map<std::string, _ym::Res<YmParcel>> _imports;


    void _download(const std::string& path);
};

struct YmParcelDef final : public _ym::Resource {
public:
    YmParcelDef() :
        _ym::Resource(_ym::RType::ParcelDef) {}


    template<typename... Args>
    static inline _ym::Res<YmParcelDef> create(Args&&... args) {
        return _ym::Res(new YmParcelDef(std::forward<Args>(args)...));
    }
    static inline void destroy(ym::Safe<const YmParcelDef> x) noexcept {
        delete x;
    }
};

struct YmParcel final : public _ym::Resource {
public:
    _ym::Res<_ym::ParcelData> data;


    inline YmParcel(_ym::Res<_ym::ParcelData> data) :
        _ym::Resource(_ym::RType::Parcel),
        data(std::move(data)) {}


    inline const std::string& path() const noexcept { return data->path; }


    template<typename... Args>
    static inline _ym::Res<YmParcel> create(Args&&... args) {
        return _ym::Res(new YmParcel(std::forward<Args>(args)...));
    }
    static inline void destroy(ym::Safe<const YmParcel> x) noexcept {
        delete x;
    }
};

