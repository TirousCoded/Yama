

#pragma once


#include "../core/mas.h"


namespace yama {


    class heap_mas final : public mas {
    public:

        heap_mas(std::shared_ptr<debug> dbg = nullptr);


        std::string report() const override final;


    protected:

        size_t _mem_in_use = 0;


        mas_allocator_info get_info() noexcept override final;
    };
}

