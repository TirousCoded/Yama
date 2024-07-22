

#include "context.h"


yama::context::context(res<domain> dm, std::shared_ptr<debug> dbg)
    : api_component(dbg),
    _dm(dm),
    _mas(dm->get_mas()) {}

yama::res<yama::domain> yama::context::get_domain() const noexcept {
    return _dm;
}

yama::domain& yama::context::dm() const noexcept {
    return *_dm;
}

yama::res<yama::mas> yama::context::get_mas() const noexcept {
    return _mas;
}

