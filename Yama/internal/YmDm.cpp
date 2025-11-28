

#include "YmDm.h"

#include "general.h"
#include "YmParcelDef.h"


YmDm::YmDm() :
    loader(std::make_shared<_ym::DmLoader>()) {}

bool YmDm::bindParcelDef(const std::string& path, ym::Safe<YmParcelDef> parceldef) {
    return loader->bindParcelDef(path, parceldef);
}

