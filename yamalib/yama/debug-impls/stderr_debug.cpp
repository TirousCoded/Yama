

#include "stderr_debug.h"

#include <iostream>


yama::stderr_debug::stderr_debug(dcat cats) : debug(cats) {}

void yama::stderr_debug::do_log(dcat, const std::string& msg) {
    std::cerr << msg << '\n';
}

