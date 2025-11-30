

#include "YmParcelDef.h"


std::optional<YmItemIndex> YmParcelDef::fnItem(const std::string& name) {
    return info->addItem(name, YmKind_Fn);
}

