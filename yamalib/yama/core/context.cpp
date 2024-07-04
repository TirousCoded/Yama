

#include "context.h"


yama::context::context(res<domain> dm, std::shared_ptr<debug> dbg)
    : system(dbg),
    _mas(dm->get_mas_for_context()),
    _dm(dm) {}

yama::res<yama::domain> yama::context::get_domain() const noexcept {
    return _dm;
}

yama::res<yama::domain> yama::context::dm() const noexcept {
    return get_domain();
}

yama::res<yama::mas> yama::context::get_mas() const noexcept {
    return _mas;
}

yama::qs::untyped_provider* yama::context::get_provider(qs::qtype_t qtype) const noexcept {
    // TODO
    return nullptr;
}

void yama::context::do_discard_all() {
    // TODO
}

