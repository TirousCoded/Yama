

#include "res.h"

#include "asserts.h"


yama::res_error::res_error(const std::string& msg) 
    : runtime_error(msg) {}

yama::res_error::res_error(const char* msg)
    : runtime_error(msg) {
    YAMA_ASSERT(msg);
}
