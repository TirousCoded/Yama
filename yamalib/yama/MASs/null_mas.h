

#pragma once


#include "../core/mas.h"


namespace yama {


    class null_mas final : public mas {
    public:

        std::string report() const override final;


    protected:
        
        mas_allocator_info get_info() noexcept override final;
    };
}

