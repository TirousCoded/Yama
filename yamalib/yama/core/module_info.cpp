

#include "module_info.h"


size_t yama::module_info::size() const noexcept {
    return types.size();
}

bool yama::module_info::contains(const str& name) const noexcept {
    return types.contains(name);
}

const yama::type_info& yama::module_info::type(const str& name) const noexcept {
    YAMA_ASSERT(contains(name));
    return types.at(name);
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

yama::module_factory& yama::module_factory::add_type(type_info&& x) {
    type_info temp(std::forward<type_info>(x));
    YAMA_ASSERT(!_result.contains(temp.fullname));
    _result.types[temp.fullname] = std::move(temp);
    return *this;
}

yama::module_factory& yama::module_factory::add_primitive_type(str fullname, const_table_info&& consts, ptype ptype) {
    type_info new_info{
        .fullname = fullname,
        .consts = std::forward<decltype(consts)>(consts),
        .info = yama::primitive_info{
            .ptype = ptype,
        },
    };
    return add_type(std::move(new_info));
}

yama::module_factory& yama::module_factory::add_function_type(str fullname, const_table_info&& consts, callsig_info&& callsig, size_t max_locals, call_fn call_fn) {
    type_info new_info{
        .fullname = fullname,
        .consts = std::forward<decltype(consts)>(consts),
        .info = yama::function_info{
            .callsig = std::forward<decltype(callsig)>(callsig),
            .call_fn = call_fn,
            .max_locals = max_locals,
            .bcode = bc::code{},
            .bcodesyms = bc::syms{},
        },
    };
    return add_type(std::move(new_info));
}

yama::module_factory& yama::module_factory::add_function_type(str fullname, const_table_info&& consts, callsig_info&& callsig, size_t max_locals, bc::code&& code, bc::syms&& syms) {
    type_info new_info{
        .fullname = fullname,
        .consts = std::forward<decltype(consts)>(consts),
        .info = yama::function_info{
            .callsig = std::forward<decltype(callsig)>(callsig),
            .call_fn = bcode_call_fn,
            .max_locals = max_locals,
            .bcode = std::forward<decltype(code)>(code),
            .bcodesyms = std::forward<decltype(syms)>(syms),
        },
    };
    return add_type(std::move(new_info));
}

