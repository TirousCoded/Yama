

#include "debug.h"


yama::debug::debug(dcat cats) 
    : enable_shared_from_this(),
    cats(cats) {}

bool yama::debug::has_cat(dcat cat) const noexcept {
    return check(cats, cat);
}

void yama::debug::add_cat(dcat cat) noexcept {
    cats |= cat;
}

void yama::debug::remove_cat(dcat cat) noexcept {
    cats &= ~cat;
}

void yama::debug::log(dcat cat, const std::string& msg) {
    if (has_cat(cat)) {
        do_log(cat, msg);
    }
}

void yama::debug::raise(dsignal sig) {
    do_raise(sig);
}

