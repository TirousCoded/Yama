

#include "spinlock.h"


void yama::spinlock::lock() {
    // spin until we are the one to change the value from false
    while (_locked.test_and_set(std::memory_order_acquire)) {}
}

void yama::spinlock::unlock() noexcept {
    _locked.clear(std::memory_order_release);
}

