

#include "stderr_debug.h"

#include <iostream>


yama::stderr_debug::stderr_debug(debug_cat cats) : debug(cats) {}

void yama::stderr_debug::do_log(const std::string& msg) {
    std::cerr << msg << '\n';
}

