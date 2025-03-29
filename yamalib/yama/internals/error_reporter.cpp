

#include "error_reporter.h"


bool yama::internal::error_reporter::is_fatal() const noexcept {
    //std::cerr << std::format("-- _err == {}\n", _err);
    return _err;
}

void yama::internal::error_reporter::fatal() {
    //std::cerr << "-- fatal!\n";
    _err = true;
}

