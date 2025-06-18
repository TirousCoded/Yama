

#include "module_info.h"


size_t yama::module_info::size() const noexcept {
    return types.size();
}

bool yama::module_info::contains(const str& unqualified_name) const noexcept {
    return types.contains(unqualified_name);
}

const yama::type_info& yama::module_info::type(const str& unqualified_name) const noexcept {
    YAMA_ASSERT(contains(unqualified_name));
    return types.at(unqualified_name);
}

std::string yama::module_info::fmt(const char* tab) const {
    YAMA_ASSERT(tab);
    std::string result{};
    result += std::format("module_info ({} types)", size());
    for (const auto& I : *this) {
        result += std::format("\n{}", I.second);
    }
    return result;
}

yama::module_info yama::module_factory::done() noexcept {
    module_info result{};
    std::swap(_result, result);
    return result;
}

yama::module_factory& yama::module_factory::add(type_info&& x) {
    type_info temp(std::forward<type_info>(x));
    // extracting this into a local var to avoid potential issues w/ C++ arg eval order
    const str& unqualified_name = temp.unqualified_name();
    YAMA_ASSERT(!_result.contains(unqualified_name));
    _result.types.insert({ unqualified_name, std::move(temp) });
    return *this;
}

yama::module_factory& yama::module_factory::add_primitive(const str& unqualified_name, const_table_info consts, ptype ptype) {
    return add(make_primitive(unqualified_name, std::move(consts), ptype));
}

yama::module_factory& yama::module_factory::add_function(const str& unqualified_name, const_table_info consts, callsig_info callsig, size_t max_locals, call_fn call_fn) {
    return add(make_function(unqualified_name, std::move(consts), std::move(callsig), max_locals, call_fn));
}

yama::module_factory& yama::module_factory::add_function(const str& unqualified_name, const_table_info consts, callsig_info callsig, size_t max_locals, bc::code code, bc::syms syms) {
    return add(make_function(unqualified_name, std::move(consts), std::move(callsig), max_locals, std::move(code), std::move(syms)));
}

yama::module_factory& yama::module_factory::add_method(const str& unqualified_name, const_table_info consts, callsig_info callsig, size_t max_locals, call_fn call_fn) {
    return add(make_method(unqualified_name, std::move(consts), std::move(callsig), max_locals, call_fn));
}

yama::module_factory& yama::module_factory::add_method(const str& unqualified_name, const_table_info consts, callsig_info callsig, size_t max_locals, bc::code code, bc::syms syms) {
    return add(make_method(unqualified_name, std::move(consts), std::move(callsig), max_locals, std::move(code), std::move(syms)));
}

yama::module_factory& yama::module_factory::add_struct(const str& unqualified_name, const_table_info consts) {
    return add(make_struct(unqualified_name, std::move(consts)));
}

