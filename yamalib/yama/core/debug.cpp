

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

yama::proxy_debug::proxy_debug(std::shared_ptr<debug> base, dcat cats_mask)
    : debug(base->cats & cats_mask),
    base(base) {}

void yama::proxy_debug::do_log(dcat cat, const std::string& msg) {
    if (base) {
        base->log(cat, msg);
    }
}

void yama::proxy_debug::do_raise(dsignal sig) {
    if (base) {
        base->raise(sig);
    }
}

std::shared_ptr<yama::proxy_debug> yama::proxy_dbg(std::shared_ptr<debug> base, dcat cats_mask) {
    return
        base
        ? std::make_shared<proxy_debug>(base, cats_mask)
        : nullptr;
}

yama::dsignal_debug::dsignal_debug(std::shared_ptr<debug> base)
    : debug(base ? base->cats : none_c),
    base(base) {
    reset();
}

size_t yama::dsignal_debug::count(dsignal sig) const noexcept {
    YAMA_ASSERT(size_t(sig) < dsignals);
    return counts[size_t(sig)];
}

void yama::dsignal_debug::reset() noexcept {
    counts.fill(0);
}

void yama::dsignal_debug::do_log(dcat cat, const std::string& msg) {
    if (base) {
        base->log(cat, msg);
    }
}

void yama::dsignal_debug::do_raise(dsignal sig) {
    YAMA_ASSERT(size_t(sig) < dsignals);
    if (base) {
        base->raise(sig);
    }
    counts[size_t(sig)]++;
}

