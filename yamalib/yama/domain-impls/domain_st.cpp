

#include "domain_st.h"


yama::domain_st::domain_st(res<mas> mas, std::shared_ptr<debug> dbg) 
    : domain(dbg),
    _mas(mas),
    _type_data_db(),
    _type_db(),
    _type_batch_db(),
    _verif(dbg),
    _instant(_type_data_db, _type_db, _type_batch_db, std::allocator<void>{}, dbg) {}

yama::res<yama::mas> yama::domain_st::get_mas() {
    return _mas;
}

std::optional<yama::type> yama::domain_st::get_type(const str& fullname) {
    const auto first_attempt = _type_db.pull(fullname);
    if (first_attempt) {
        return std::make_optional(type(**first_attempt));
    }
    const size_t number = _instant.instantiate(fullname);
    if (number == 0) {
        return std::nullopt;
    }
    const auto second_attempt = _type_db.pull(fullname);
    if (second_attempt) {
        return std::make_optional(type(**second_attempt));
    }
    return std::nullopt;
}

bool yama::domain_st::push(type_data x) {
    if (!_verif.verify(x)) return false;
    _type_data_db.push(x);
    return true;
}

