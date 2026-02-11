

#include "YmDm.h"

#include "general.h"
#include "YmParcelDef.h"


YmDm::YmDm() :
    loader(std::make_shared<_ym::DmLoader>()) {}

bool YmDm::bindParcelDef(const std::string& path, ym::Safe<YmParcelDef> parceldef) {
    return loader->bindParcelDef(path, parceldef);
}

bool YmDm::addRedirect(const std::string& subject, const std::string& before, const std::string& after) {
    return loader->addRedirect(subject, before, after);
}

