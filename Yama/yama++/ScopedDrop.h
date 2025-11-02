

#pragma once


#include "../yama/yama.h"
#include "Safe.h"


namespace ym {


    // TODO: ScopedDrop has not been unit tested.

    // A helper class used to RAII call ymDrop on a resource.
    class ScopedDrop final {
    public:
        inline ScopedDrop() : _res(nullptr) {}
        template<typename T>
        inline ScopedDrop(T* x) noexcept : _res((void*)x) {}
        template<typename T>
        inline ScopedDrop(Safe<T> x) noexcept : ScopedDrop(x.get()) {}

        inline ~ScopedDrop() noexcept {
            drop(); // RAII
        }

        inline ScopedDrop(ScopedDrop&& other) noexcept :
            ScopedDrop() {
            std::swap(_res, other._res);
        }
        inline ScopedDrop& operator=(ScopedDrop&& other) noexcept {
            drop();
            std::swap(_res, other._res);
            return *this;
        }


        // Returns the resource pointer wrapped by the ScopedDrop.
        inline void* get() const noexcept { return _res; }

        // Disarms the ScopedDrop, making it inert without calling ymDrop, and returning the resource pointer it held.
        inline void* disarm() noexcept {
            auto result = _res;
            _res = nullptr;
            return result;
        }

        // Performs the ymDrop then makes the ScopedDrop inert.
        inline void drop() noexcept {
            if (_res) {
                ymDrop(_res);
                _res = nullptr;
            }
        }


    private:
        // We use a raw pointer here so that nullptr can be for when
        // a ScopedDrop object is invalidated (ie. via drop or move.)
        void* _res;
    };
}

