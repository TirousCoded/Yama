

#pragma once


#include "Resource.h"


namespace _ym {


    // Encapsulates the thread-safe immutable shared data of YmParcel resources.
    class ParcelData final : public Resource {
    public:
        const std::string path;


        inline ParcelData(std::string path) :
            Resource(RMode::ARC),
            path(std::move(path)) {}


        inline const YmChar* debugName() const noexcept override {
            return "Parcel Data";
        }

        template<typename... Args>
        static inline Res<ParcelData> create(Args&&... args) {
            return Res(new ParcelData(std::forward<Args>(args)...));
        }
        inline void destroy() const noexcept override {
            delete this;
        }
    };
}

