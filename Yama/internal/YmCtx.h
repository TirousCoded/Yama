

#pragma once


#ifdef _YM_FORBID_INCLUDE_IN_YAMA_DOT_H
#error Not allowed to expose this header file to header file yama.h!
#endif


#include <unordered_map>

#include "../yama/yama.h"
#include "../yama++/Safe.h"
#include "YmDm.h"


struct YmCtx final {
public:
    const ym::Safe<YmDm> domain;


    inline YmCtx(ym::Safe<YmDm> domain) :
        domain(domain) {
    }


    // Fetching acquires parcel/item w/out attempting to import/load it.

    YmParcel* fetchParcel(const std::string& path) const noexcept;
    YmParcel* fetchParcel(YmPID pid) const noexcept;
    YmItem* fetchItem(const std::string& fullname) const noexcept;
    YmItem* fetchItem(YmGID gid) const noexcept;

    YmParcel* import(const std::string& path);
    YmParcel* import(YmPID pid);
    YmItem* load(const std::string& fullname);
    YmItem* load(YmGID gid);


    static void pIterStart(ym::Safe<YmCtx> ctx) noexcept;
    static void pIterStartFrom(ym::Safe<YmCtx> ctx, ym::Safe<YmParcel> parcel) noexcept;
    static void pIterAdvance(size_t n) noexcept;
    static YmParcel* pIterGet() noexcept;
    static bool pIterDone() noexcept;


private:
    std::unordered_map<std::string, std::shared_ptr<YmParcel>> _importsByPath;
    std::unordered_map<YmPID, std::shared_ptr<YmParcel>> _importsByPID;

    std::unordered_map<std::string, std::shared_ptr<YmItem>> _loadsByFullname;
    std::unordered_map<YmGID, std::shared_ptr<YmItem>> _loadsByGID;


    static thread_local decltype(_importsByPID)::const_iterator _iter, _pastTheEndIter;
    static thread_local YmParcel* _iterCurr;


    void _download(const std::string& path);
    void _download(YmPID pid);
    void _resolve(const std::string& fullname);


    static void _updateIterCurr() noexcept;
};

