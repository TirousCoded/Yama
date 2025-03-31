

#include "domain_data.h"


yama::internal::domain_data::domain_data(const std::shared_ptr<debug>& dbg)
    : verif(dbg),
    compiler(dbg, *this),
    state(),
    install_manager(dbg),
    importer(dbg, *this),
    loader(dbg, *this, state) {}

