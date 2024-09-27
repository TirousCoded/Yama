

#include "stdout_debug.h"

#include <iostream>


yama::stdout_debug::stdout_debug(dcat cats) : debug(cats) {}

void yama::stdout_debug::do_log(dcat, const std::string& msg) {
    std::cout << msg << '\n';
}

