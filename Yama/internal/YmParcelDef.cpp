

#include "YmParcelDef.h"

#include "../internal/general.h"


bool YmParcelDef::verify() const {
    return info->verify();
}

std::optional<YmItemIndex> YmParcelDef::structItem(const std::string& name) {
    return info->addItem(name, YmKind_Struct);
}

std::optional<YmItemIndex> YmParcelDef::fnItem(const std::string& name, std::string returnTypeSymbol) {
    return info->addItem(name, YmKind_Fn, std::make_optional(std::move(returnTypeSymbol)));
}

std::optional<YmItemIndex> YmParcelDef::methodItem(YmItemIndex owner, const std::string& name, std::string returnTypeSymbol) {
    return info->addItem(owner, name, YmKind_Method, std::make_optional(std::move(returnTypeSymbol)));
}

std::optional<YmParamIndex> YmParcelDef::addParam(YmItemIndex item, std::string name, std::string paramTypeSymbol) {
    return info->addParam(item, std::move(name), std::move(paramTypeSymbol));
}

std::optional<YmRef> YmParcelDef::addRef(YmItemIndex item, std::string symbol) {
    return info->addRef(item, std::move(symbol));
}

