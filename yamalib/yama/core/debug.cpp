

#include "debug.h"


yama::debug::debug(dcat cats) 
    : enable_shared_from_this(),
    cats(cats) {}

bool yama::debug::has_cat(dcat cat) const noexcept {
    return check(cats, cat);
}

