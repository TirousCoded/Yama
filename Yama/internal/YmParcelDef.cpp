

#include "YmParcelDef.h"


std::optional<YmLID> YmParcelDef::fnItem(const std::string& name) {
    return info->addItem(name, YmKind_Fn);
}

