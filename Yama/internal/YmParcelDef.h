

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

    bool addStruct(
        const std::string& name,
        _ym::KindEx k = _ym::KindEx::Struct);
    bool addProtocol(
        const std::string& name);
    bool addFn(
        const std::string& name,
        std::string returnTypeSymbol,
        _ym::CallBhvrCallbackInfo callBehaviour);
    bool addReadOnlyStoredVar(
        const std::string& name,
        std::string typeSymbol,
        _ym::CallBhvrCallbackInfo initBehaviour);
    bool addStoredVar(
        const std::string& name,
        std::string typeSymbol,
        _ym::CallBhvrCallbackInfo initBehaviour);
    bool addReadOnlyComputedVar(
        const std::string& name,
        std::string typeSymbol,
        _ym::CallBhvrCallbackInfo getBehaviour);
    bool addComputedVar(
        const std::string& name,
        std::string typeSymbol,
        _ym::CallBhvrCallbackInfo getBehaviour,
        _ym::CallBhvrCallbackInfo setBehaviour);
    bool addMethod(
        const std::string& ownerName,
        const std::string& name,
        std::string returnTypeSymbol,
        _ym::CallBhvrCallbackInfo callBehaviour);
    bool addMethodReq(
        const std::string& ownerName,
        const std::string& name,
        std::string returnTypeSymbol);
    bool addReadOnlyStoredProperty(
        const std::string& ownerName,
        const std::string& name,
        std::string typeSymbol);
    bool addStoredProperty(
        const std::string& ownerName,
        const std::string& name,
        std::string typeSymbol);
    bool addReadOnlyComputedProperty(
        const std::string& ownerName,
        const std::string& name,
        std::string typeSymbol,
        _ym::CallBhvrCallbackInfo getBehaviour);
    bool addComputedProperty(
        const std::string& ownerName,
        const std::string& name,
        std::string typeSymbol,
        _ym::CallBhvrCallbackInfo getBehaviour,
        _ym::CallBhvrCallbackInfo setBehaviour);

    std::optional<YmTypeParamIndex> addTypeParam(
        std::string typeName,
        std::string name,
        std::string constraintTypeSymbol);
    std::optional<YmParamIndex> addParam(
        std::string typeName,
        std::string name,
        std::string paramTypeSymbol);
    void beginNamedParams(
        const std::string& typeName);
    std::optional<YmRef> addRef(
        std::string typeName,
        std::string symbol);


private:
    bool _addReadOnlyVar(
        const std::string& name,
        std::string typeSymbol,
        _ym::CallBhvrCallbackInfo initBehaviour,
        _ym::CallBhvrCallbackInfo getBehaviour,
        _ym::KindEx getK);
    bool _addVar(
        const std::string& name,
        std::string typeSymbol,
        _ym::CallBhvrCallbackInfo initBehaviour,
        _ym::CallBhvrCallbackInfo getBehaviour,
        _ym::CallBhvrCallbackInfo setBehaviour,
        _ym::KindEx getK,
        _ym::KindEx setK);
    bool _addMethod(
        const std::string& ownerName,
        const std::string& name,
        std::string returnTypeSymbol,
        _ym::CallBhvrCallbackInfo callBehaviour,
        _ym::KindEx k);
    bool _addReadOnlyProperty(
        const std::string& ownerName,
        const std::string& name,
        std::string typeSymbol,
        _ym::CallBhvrCallbackInfo getBehaviour,
        _ym::KindEx getK);
    bool _addProperty(
        const std::string& ownerName,
        const std::string& name,
        std::string typeSymbol,
        _ym::CallBhvrCallbackInfo getBehaviour,
        _ym::CallBhvrCallbackInfo setBehaviour,
        _ym::KindEx getK,
        _ym::KindEx setK);
};

