

#pragma once


#include "../core/mas.h"


namespace yama {


    class null_mas final : public mas {
    public:

        null_mas(std::shared_ptr<debug> dbg = nullptr);


        std::string report() const override final;


    protected:
        
        mas_allocator_info get_info() noexcept override final;
    };
}

