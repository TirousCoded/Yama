

#include "debug.h"


yama::debug::debug(debug_cat cats) 
    : cats(cats) {}

bool yama::debug::has_cat(debug_cat cat) const noexcept {
    return check(cats, cat);
}

