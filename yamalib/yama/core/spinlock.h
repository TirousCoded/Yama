

#pragma once


#include <atomic>


namespace yama {


    // TODO: spinlock has not been unit tested (not sure how)

    // this is a simple spinlock implementation for later on when we want
    // to add multithreading stuff to Yama

    class spinlock final {
    public:
        spinlock() = default;


        void lock();
        void unlock() noexcept;


    private:
        std::atomic_flag _locked;
    };
}

