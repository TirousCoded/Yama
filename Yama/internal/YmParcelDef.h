

#pragma once


#ifdef _YM_FORBID_INCLUDE_IN_YAMA_DOT_H
#error Not allowed to expose this header file to header file yama.h!
#endif


#include <memory>
#include <optional>
#include <string>

#include "../yama/yama.h"
#include "general.h"
#include "ParcelInfo.h"
#include "RefCounter.h"


struct YmParcelDef final {
public:
    // refs is not managed internally by this class.
    _ym::AtomicRefCounter<YmRefCount> refs;

    std::shared_ptr<_ym::ParcelInfo> info;


    inline YmParcelDef() :
        info(std::make_shared<_ym::ParcelInfo>()) {
    }


    bool verify() const;

    std::optional<YmTypeIndex> addStruct(
        const std::string& name);
    std::optional<YmTypeIndex> addProtocol(
        const std::string& name);
    std::optional<YmTypeIndex> addFn(
        const std::string& name,
        std::string returnTypeSymbol,
        _ym::CallBhvrCallbackInfo callBehaviour);
    std::optional<YmTypeIndex> addMethod(
        YmTypeIndex owner,
        const std::string& name,
        std::string returnTypeSymbol,
        _ym::CallBhvrCallbackInfo callBehaviour);
    std::optional<YmTypeIndex> addMethodReq(
        YmTypeIndex owner,
        const std::string& name,
        std::string returnTypeSymbol);

    std::optional<YmTypeParamIndex> addTypeParam(
        YmTypeIndex type,
        std::string name,
        std::string constraintTypeSymbol);
    std::optional<YmParamIndex> addParam(
        YmTypeIndex type,
        std::string name,
        std::string paramTypeSymbol);
    std::optional<YmRef> addRef(
        YmTypeIndex type,
        std::string symbol);
};

