

#include "domain_st.h"


yama::domain_st::domain_st(res<mas> mas, std::shared_ptr<debug> dbg)
    : domain(dbg),
    _mas(mas),
    _type_info_db(),
    _type_db(),
    _type_batch_db(),
    _compiler(*this, dbg),
    _verif(dbg),
    _instant(_type_info_db, _type_db, _type_batch_db, std::allocator<void>{}, dbg) {
    if (!setup_domain()) {
        fail_domain_setup();
    }
}

yama::res<yama::mas> yama::domain_st::get_mas() {
    return _mas;
}

std::optional<yama::type> yama::domain_st::load(const str& fullname) {
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

std::optional<std::vector<yama::type_info>> yama::domain_st::do_compile(const taul::source_code& src) {
    return _compiler.compile(src);
}

bool yama::domain_st::do_verify(const type_info& x) {
    return _verif.verify(x);
}

void yama::domain_st::do_upload(type_info&& x) {
    _type_info_db.push(make_res<type_info>(std::forward<type_info>(x)));
}

