

#include "error_reporter.h"

#include "compilation_state.h"


yama::internal::error_reporter::error_reporter(translation_unit& tu)
    : tu(tu) {}

bool yama::internal::error_reporter::good() const noexcept {
    return !is_fatal();
}

bool yama::internal::error_reporter::is_fatal() const noexcept {
    return _err;
}

void yama::internal::error_reporter::fatal() {
    _err = true;
}

std::shared_ptr<yama::debug> yama::internal::error_reporter::_dbg() {
    return tu->cs->dbg();
}

taul::source_location yama::internal::error_reporter::_loc_at(taul::source_pos where) {
    return tu->src.location_at(where);
}

