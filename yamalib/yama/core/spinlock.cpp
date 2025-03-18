

#include "spinlock.h"


void yama::spinlock::lock() {
    // spin until we succeed
    while (try_lock()) {}
}

bool yama::spinlock::try_lock() {
    // succeed if we're the one who set _locked to true
    return _locked.test_and_set(std::memory_order_acquire);
}

void yama::spinlock::unlock() noexcept {
    _locked.clear(std::memory_order_release);
}

