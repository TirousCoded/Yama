

#include "dsignal_debug.h"


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

