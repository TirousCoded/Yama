

#include "csymtab.h"

#include "util.h"


using namespace yama::string_literals;


std::string yama::internal::fn_csym::fmt_params() const {
    std::string result{};
    result += "(";
    for (size_t i = 0; i < params.size(); i++) {
        if (i > 0) {
            result += ", ";
        }
        result += std::format("{0}: {1}", params[i].name, params[i].type.value_or("n/a"_str));
    }
    result += ")";
    return result;
}

std::string yama::internal::csymtab::entry::fmt(size_t tabs, const char* tab) {
    const auto _tabs = fmt_tabs(tabs, tab);
    std::string result{};
    if (is<prim_csym>()) {
        result += std::format("{0}prim-decl {{\n", _tabs);
        result += std::format("{0}{1}name       : {2}\n", _tabs, tab, name);
        result += std::format("{0}{1}id         : {2}\n", _tabs, tab, node ? std::format("{0}", node->id) : "n/a");
        result += std::format("{0}{1}starts     : {2}\n", _tabs, tab, starts);
        result += std::format("{0}}}\n", _tabs);
    }
    else if (is<var_csym>()) {
        result += std::format("{0}var-decl {{\n", _tabs);
        result += std::format("{0}{1}name       : {2}\n", _tabs, tab, name);
        result += std::format("{0}{1}id         : {2}\n", _tabs, tab, node ? std::format("{0}", node->id) : "n/a");
        result += std::format("{0}{1}starts     : {2}\n", _tabs, tab, starts);
        result += std::format("{0}{1}type       : {2}\n", _tabs, tab, as<var_csym>().type.value_or("n/a"_str));
        result += std::format("{0}}}\n", _tabs);
    }
    else if (is<fn_csym>()) {
        result += std::format("{0}fn-decl {{\n", _tabs);
        result += std::format("{0}{1}name       : {2}\n", _tabs, tab, name);
        result += std::format("{0}{1}id         : {2}\n", _tabs, tab, node ? std::format("{0}", node->id) : "n/a");
        result += std::format("{0}{1}starts     : {2}\n", _tabs, tab, starts);
        result += std::format("{0}{1}params     : {2}\n", _tabs, tab, as<fn_csym>().fmt_params());
        result += std::format("{0}{1}return_type: {2}\n", _tabs, tab, as<fn_csym>().return_type.value_or("n/a"_str));
        result += std::format("{0}}}\n", _tabs);
    }
    else if (is<param_csym>()) {
        result += std::format("{0}param-decl {{\n", _tabs);
        result += std::format("{0}{1}name       : {2}\n", _tabs, tab, name);
        result += std::format("{0}{1}id         : {2}\n", _tabs, tab, node ? std::format("{0}", node->id) : "n/a");
        result += std::format("{0}{1}starts     : {2}\n", _tabs, tab, starts);
        result += std::format("{0}{1}type       : {2}\n", _tabs, tab, as<param_csym>().type.value_or("n/a"_str));
        result += std::format("{0}}}\n", _tabs);
    }
    else YAMA_DEADEND;
    return result;
}

std::string yama::internal::csymtab::fmt(size_t tabs, const char* tab) {
    const auto _tabs = fmt_tabs(tabs, tab);
    std::string result{};
    result += std::format("{0}csymtab ({1} symbols) {{\n", _tabs, count());
    for (const auto& I : _entries) {
        result += I.second->fmt(tabs + 1, tab);
    }
    result += std::format("{0}}}\n", _tabs);
    return result;
}

std::string yama::internal::csymtab_group::fmt(size_t tabs, const char* tab) {
    const auto _tabs = fmt_tabs(tabs, tab);
    std::string result{};
    result += std::format("{0}csymtab_group ({1} csymtabs) {{\n", _tabs, count());
    for (const auto& I : _csymtabs) {
        result += std::format("{0}{1}(for ID={2})\n", _tabs, tab, I.first);
        result += I.second->fmt(tabs + 1, tab);
    }
    result += std::format("{0}}}\n", _tabs);
    return result;
}