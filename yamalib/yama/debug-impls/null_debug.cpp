

#include "null_debug.h"


yama::null_debug::null_debug(dcat cats) 
    : debug(cats) {}

void yama::null_debug::do_log(dcat, const std::string&) {
    // do nothing
}

