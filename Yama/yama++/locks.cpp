

#include "locks.h"


void ym::SpinLock::lock() {
    // Spin until we succeed.
    while (try_lock()) {}
}

bool ym::SpinLock::try_lock() {
    // Succeed if we're the one who set _locked to true.
    return _locked.test_and_set(std::memory_order_acquire);
}

void ym::SpinLock::unlock() noexcept {
    _locked.clear(std::memory_order_release);
}

