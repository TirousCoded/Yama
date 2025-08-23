

#include "ids.h"

#include <taul/hashing.h>


size_t yama::global_id::hash() const noexcept {
    return taul::hash(lid, mid);
}

