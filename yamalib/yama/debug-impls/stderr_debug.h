

#pragma once


#include "../core/debug.h"


namespace yama {


    class stderr_debug final : public debug {
    public:

        stderr_debug(dcat cats = defaults_c);


    protected:

        void do_log(dcat, const std::string& msg) override final;
    };
}

