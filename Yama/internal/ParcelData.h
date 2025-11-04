

#pragma once


#include "Resource.h"


namespace _ym {


    // Encapsulates the thread-safe immutable shared data of YmParcel resources.
    class ParcelData final : public Resource {
    public:
        const std::string path;


        inline ParcelData(std::string path) :
            Resource(_ym::RType::ParcelData),
            path(std::move(path)) {}


        template<typename... Args>
        static inline Res<ParcelData> create(Args&&... args) {
            return Res(new ParcelData(std::forward<Args>(args)...));
        }
        static inline void destroy(ym::Safe<const ParcelData> x) noexcept {
            delete x;
        }
    };
}

