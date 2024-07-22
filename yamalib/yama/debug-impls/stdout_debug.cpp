

#include "stdout_debug.h"

#include <iostream>


yama::stdout_debug::stdout_debug(debug_cat cats) : debug(cats) {}

void yama::stdout_debug::do_log(const std::string& msg) {
    std::cout << msg << '\n';
}

