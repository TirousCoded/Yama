

#pragma once


#ifdef _YM_FORBID_INCLUDE_IN_YAMA_DOT_H
#error Not allowed to expose this header file to header file yama.h!
#endif


#include <memory>
#include <optional>
#include <string>

#include "../yama/yama.h"
#include "ParcelInfo.h"


struct YmParcelDef final {
public:
    std::shared_ptr<_ym::ParcelInfo> info;


    inline YmParcelDef() :
        info(std::make_shared<_ym::ParcelInfo>()) {
    }


    bool verify() const;

    std::optional<YmItemIndex> structItem(const std::string& name);
    std::optional<YmItemIndex> fnItem(const std::string& name, std::string returnTypeSymbol);
    std::optional<YmItemIndex> methodItem(YmItemIndex owner, const std::string& name, std::string returnTypeSymbol);

    std::optional<YmParamIndex> addParam(YmItemIndex item, std::string name, std::string paramTypeSymbol);
    std::optional<YmRef> addRef(YmItemIndex item, std::string symbol);
};

