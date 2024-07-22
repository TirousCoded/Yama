

#pragma once


#include "../core/debug.h"


namespace yama {


    class null_debug final : public debug {
    public:

        null_debug(debug_cat cats = defaults_c);


    protected:

        void do_log(const std::string&) override final;
    };
}

