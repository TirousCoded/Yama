

#include "domain.h"


yama::domain::domain(res<mas> mas, std::shared_ptr<debug> dbg)
    : system(dbg), 
    _mas(mas) {}

yama::res<yama::mas> yama::domain::get_mas() const noexcept {
    return _mas;
}

