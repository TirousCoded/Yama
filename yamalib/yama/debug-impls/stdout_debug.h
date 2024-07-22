

#pragma once


#include "../core/debug.h"


namespace yama {


    class stdout_debug final : public debug {
    public:

        stdout_debug(debug_cat cats = defaults_c);


    protected:

        void do_log(const std::string& msg) override final;
    };
}

