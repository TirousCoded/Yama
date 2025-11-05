

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
#include "ParcelData.h"
#include "Res.h"
#include "Resource.h"


struct YmDm final : public _ym::FrontendResource<YmRType_Dm, _ym::RMode::ARC> {
public:
    inline YmDm() :
        FrontendResource() {}


    // TODO: Maybe add string interning later.

    YmBool bindParcelDef(const std::string& path, ym::Safe<YmParcelDef> parceldef);
    std::optional<_ym::Res<_ym::ParcelData>> import(const std::string& path);


    template<typename... Args>
    static inline _ym::Res<YmDm> create(Args&&... args) {
        return _ym::Res(new YmDm(std::forward<Args>(args)...));
    }
    inline void destroy() const noexcept override {
        delete this;
    }


private:
    std::mutex _mtx;
    std::unordered_map<std::string, _ym::Res<_ym::ParcelData>> _bindings;
};

struct YmCtx final : public _ym::FrontendResource<YmRType_Ctx, _ym::RMode::ARC> {
public:
    const _ym::Res<YmDm> domain;


    inline YmCtx(_ym::Res<YmDm> domain) :
        FrontendResource(),
        domain(domain) {}


    // Returns already imported parcel under path, if any.
    std::optional<_ym::Res<YmParcel>> fetch(const std::string& path) const noexcept;

    // Returns imported parcel under path, if any, attempting import if needed.
    std::optional<_ym::Res<YmParcel>> import(const std::string& path);


    template<typename... Args>
    static inline _ym::Res<YmCtx> create(Args&&... args) {
        return _ym::Res(new YmCtx(std::forward<Args>(args)...));
    }
    inline void destroy() const noexcept override {
        delete this;
    }


private:
    std::unordered_map<std::string, _ym::Res<YmParcel>> _imports;


    void _download(const std::string& path);
};

struct YmParcelDef final : public _ym::FrontendResource<YmRType_ParcelDef, _ym::RMode::ARC> {
public:
    inline YmParcelDef() :
        FrontendResource() {}


    template<typename... Args>
    static inline _ym::Res<YmParcelDef> create(Args&&... args) {
        return _ym::Res(new YmParcelDef(std::forward<Args>(args)...));
    }
    inline void destroy() const noexcept override {
        delete this;
    }
};

struct YmParcel final : public _ym::FrontendResource<YmRType_Parcel, _ym::RMode::RC> {
public:
    _ym::Res<_ym::ParcelData> data;


    inline YmParcel(_ym::Res<_ym::ParcelData> data) :
        FrontendResource(),
        data(std::move(data)) {}


    inline const std::string& path() const noexcept { return data->path; }


    template<typename... Args>
    static inline _ym::Res<YmParcel> create(Args&&... args) {
        return _ym::Res(new YmParcel(std::forward<Args>(args)...));
    }
    inline void destroy() const noexcept override {
        delete this;
    }
};

