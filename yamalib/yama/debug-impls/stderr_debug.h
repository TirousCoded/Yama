

#pragma once


#include "../core/debug.h"


namespace yama {


    class stderr_debug final : public debug {
    public:

        stderr_debug(debug_cat cats = defaults_c);


    protected:

        void do_log(const std::string& msg) override final;
    };
}

