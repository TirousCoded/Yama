

#pragma once


#include "../core/debug.h"


namespace yama {


    class null_debug final : public debug {
    public:

        null_debug(dcat cats = defaults_c);


    protected:

        void do_log(dcat, const std::string&) override final;
    };
}

