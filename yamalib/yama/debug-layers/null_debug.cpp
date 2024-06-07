

#include "null_debug.h"


yama::null_debug::null_debug(debug_cat cats) 
    : debug(cats) {}

void yama::null_debug::do_log(const std::string&) {
    // do nothing
}

