

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
    _ym::AtomicRefCounter refs;

    std::shared_ptr<_ym::ParcelInfo> info;


    inline YmParcelDef() :
        info(std::make_shared<_ym::ParcelInfo>()) {
    }


    bool addStruct(
        const std::string& name,
        _ym::KindEx k = _ym::KindEx::Struct,
        _ym::ConstTableInfo initial = _ym::ConstTableInfo{});
    bool addProtocol(
        const std::string& name,
        _ym::ConstTableInfo initial = _ym::ConstTableInfo{});
    bool addFn(
        const std::string& name,
        const std::string& returnTypeSymbol,
        _ym::CallBhvrCallbackInfo callBehaviour,
        _ym::ConstTableInfo initial = _ym::ConstTableInfo{});
    bool addReadOnlyStoredVar(
        const std::string& name,
        const std::string& typeSymbol,
        _ym::CallBhvrCallbackInfo initBehaviour,
        _ym::ConstTableInfo initial = _ym::ConstTableInfo{});
    bool addStoredVar(
        const std::string& name,
        const std::string& typeSymbol,
        _ym::CallBhvrCallbackInfo initBehaviour,
        _ym::ConstTableInfo initial = _ym::ConstTableInfo{});
    bool addReadOnlyComputedVar(
        const std::string& name,
        const std::string& typeSymbol,
        _ym::CallBhvrCallbackInfo getBehaviour,
        _ym::ConstTableInfo initial = _ym::ConstTableInfo{});
    bool addComputedVar(
        const std::string& name,
        const std::string& typeSymbol,
        _ym::CallBhvrCallbackInfo getBehaviour,
        _ym::CallBhvrCallbackInfo setBehaviour,
        _ym::ConstTableInfo initial = _ym::ConstTableInfo{});
    bool addMethod(
        const std::string& ownerName,
        const std::string& name,
        const std::string& returnTypeSymbol,
        _ym::CallBhvrCallbackInfo callBehaviour,
        _ym::ConstTableInfo initial = _ym::ConstTableInfo{});
    bool addMethodReq(
        const std::string& ownerName,
        const std::string& name,
        const std::string& returnTypeSymbol,
        _ym::ConstTableInfo initial = _ym::ConstTableInfo{});
    bool addReadOnlyStoredProperty(
        const std::string& ownerName,
        const std::string& name,
        const std::string& typeSymbol,
        _ym::ConstTableInfo initial = _ym::ConstTableInfo{});
    bool addStoredProperty(
        const std::string& ownerName,
        const std::string& name,
        const std::string& typeSymbol,
        _ym::ConstTableInfo initial = _ym::ConstTableInfo{});
    bool addReadOnlyComputedProperty(
        const std::string& ownerName,
        const std::string& name,
        const std::string& typeSymbol,
        _ym::CallBhvrCallbackInfo getBehaviour,
        _ym::ConstTableInfo initial = _ym::ConstTableInfo{});
    bool addComputedProperty(
        const std::string& ownerName,
        const std::string& name,
        const std::string& typeSymbol,
        _ym::CallBhvrCallbackInfo getBehaviour,
        _ym::CallBhvrCallbackInfo setBehaviour,
        _ym::ConstTableInfo initial = _ym::ConstTableInfo{});

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

    bool bindBCode(
        const std::string& localName,
        _ym::BCode code,
        _ym::BCodeDbgSyms syms = _ym::BCodeDbgSyms{});

    // NOTE: Remember, constant table info is impl details!
    // NOTE: These are mainly for helping our unit tests.

    inline std::optional<size_t> pullRefConst(
        const std::string& localname, const std::string& symbol, size_t sizeLimit = size_t(-1)) {
        if (auto t = info->type(localname)) {
            if (auto result = t->consts.pullRef(_ym::Spec::type(symbol), sizeLimit)) {
                return (size_t)*result;
            }
        }
        return std::nullopt;
    }
    template<typename T>
    inline std::optional<size_t> pullValConst(
        const std::string& localname, const T& v, size_t sizeLimit = size_t(-1)) {
        if (auto t = info->type(localname)) {
            if (auto result = t->consts.pullVal(v, sizeLimit)) {
                return (size_t)*result;
            }
        }
        return std::nullopt;
    }
};

