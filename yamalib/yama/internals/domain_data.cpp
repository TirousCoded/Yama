

#include "domain_data.h"


yama::internal::domain_data::domain_data(const std::shared_ptr<debug>& dbg)
    : installs(),
    state(),
    installer(dbg, *this),
    importer(dbg, *this),
    loader(dbg, *this, state),
    verif(dbg),
    compiler(dbg, *this) {}

