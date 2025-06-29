

#include "csymtab.h"

#include "util.h"
#include "compiler.h"
#include "domain_data.h"


using namespace yama::string_literals;


std::string yama::internal::fmt_lookup_proc(lookup_proc x) {
    std::string result{};
    if (x == lookup_proc::normal)           result = "normal";
    else if (x == lookup_proc::qualifier)   result = "qualifier";
    else                                    YAMA_DEADEND;
    return result;
}

std::string yama::internal::import_csym::fmt(size_t tabs, const char* tab) {
    const auto& e = tu->cs->dd->installs.domain_env();
    const auto _tabs = fmt_tabs(tabs, tab);
    std::string result{};
    result += std::format("{0}import-decl {{\n", _tabs);
    result += std::format("{0}{1}name        : {2}\n", _tabs, tab, name);
    result += std::format("{0}{1}id          : {2}\n", _tabs, tab, node ? std::format("{0}", node->id) : "n/a");
    result += std::format("{0}{1}starts      : {2}\n", _tabs, tab, starts);
    result += std::format("{0}{1}path        : {2}\n", _tabs, tab, path);
    result += std::format("{0}}}\n", _tabs);
    return result;
}

std::string yama::internal::prim_csym::fmt(size_t tabs, const char* tab) {
    const auto& e = tu->cs->dd->installs.domain_env();
    const auto _tabs = fmt_tabs(tabs, tab);
    std::string result{};
    result += std::format("{0}prim-decl {{\n", _tabs);
    result += std::format("{0}{1}name        : {2}\n", _tabs, tab, name);
    result += std::format("{0}{1}id          : {2}\n", _tabs, tab, node ? std::format("{0}", node->id) : "n/a");
    result += std::format("{0}{1}starts      : {2}\n", _tabs, tab, starts);
    result += std::format("{0}}}\n", _tabs);
    return result;
}

std::optional<yama::internal::ctype> yama::internal::var_csym::get_type() const {
    if (annot_type)         return annot_type->get_type(*tu->cs);
    else if (initializer)   return initializer->get_type(*tu->cs);
    else                    return std::nullopt;
}

std::string yama::internal::var_csym::fmt(size_t tabs, const char* tab) {
    const auto& e = tu->cs->dd->installs.domain_env();
    const auto _tabs = fmt_tabs(tabs, tab);
    std::string result{};
    result += std::format("{0}var-decl {{\n", _tabs);
    result += std::format("{0}{1}name        : {2}\n", _tabs, tab, name);
    result += std::format("{0}{1}id          : {2}\n", _tabs, tab, node ? std::format("{0}", node->id) : "n/a");
    result += std::format("{0}{1}starts      : {2}\n", _tabs, tab, starts);
    result += std::format("{0}{1}annot-type  : {2}\n", _tabs, tab, annot_type ? annot_type->get_type(*tu->cs).value().fmt(e) : "n/a");
    result += std::format("{0}{1}initializer : {2}\n", _tabs, tab, initializer ? initializer->get_type(*tu->cs).value().fmt(e) : "n/a");
    result += std::format("{0}}}\n", _tabs);
    return result;
}

yama::internal::fullname yama::internal::fn_like_csym::get_owner_fln() const {
    return qualified_name(tu->src_path, unqualified_name(name).owner_name());
}

std::optional<yama::internal::ctype> yama::internal::fn_like_csym::get_owner_type() const {
    return tu->cs->types.load(get_owner_fln());
}

std::optional<yama::internal::ctype> yama::internal::fn_like_csym::get_return_type() const {
    return return_type ? return_type->get_type(*tu->cs) : std::nullopt;
}

yama::internal::ctype yama::internal::fn_like_csym::get_return_type_or_none() const {
    return tu->types.default_none(get_return_type());
}

std::string yama::internal::fn_like_csym::fmt_params() const {
    const auto& e = tu->cs->dd->installs.domain_env();
    std::string result{};
    result += "(";
    bool first = true;
    for (const auto& param : params) {
        if (!first) {
            result += ", ";
        }
        first = false;
        result += std::format(
            "{0}: {1}",
            param->name,
            param->expect<param_csym>()->get_type().value().fmt(e));
    }
    result += ")";
    return result;
}

std::string yama::internal::fn_like_csym::fmt(size_t tabs, const char* tab) {
    const auto& e = tu->cs->dd->installs.domain_env();
    const auto _tabs = fmt_tabs(tabs, tab);
    std::string result{};
    result += std::format("{0}fn-like-decl {{\n", _tabs);
    result += std::format("{0}{1}name        : {2}\n", _tabs, tab, name);
    result += std::format("{0}{1}id          : {2}\n", _tabs, tab, node ? std::format("{0}", node->id) : "n/a");
    result += std::format("{0}{1}starts      : {2}\n", _tabs, tab, starts);
    result += std::format("{0}{1}params      : {2}\n", _tabs, tab, fmt_params());
    result += std::format("{0}{1}return-type : {2}\n", _tabs, tab, return_type ? return_type->get_type(*tu->cs).value().fmt(e) : "n/a");
    result += std::format("{0}}}\n", _tabs);
    return result;
}

std::optional<yama::internal::ctype> yama::internal::param_csym::get_type() const {
    return
        type
        ? type->get_type(*tu->cs)
        : (self_param ? deref_assert(fn).get_owner_type() : std::nullopt);
}

std::string yama::internal::param_csym::fmt(size_t tabs, const char* tab) {
    const auto& e = tu->cs->dd->installs.domain_env();
    const auto _tabs = fmt_tabs(tabs, tab);
    const auto t = get_type();
    std::string result{};
    result += std::format("{0}param-decl {{\n", _tabs);
    result += std::format("{0}{1}name        : {2}\n", _tabs, tab, name);
    result += std::format("{0}{1}id          : {2}\n", _tabs, tab, node ? std::format("{0}", node->id) : "n/a");
    result += std::format("{0}{1}starts      : {2}\n", _tabs, tab, starts);
    result += std::format("{0}{1}index       : {2}\n", _tabs, tab, index);
    result += std::format("{0}{1}type        : {2}\n", _tabs, tab, t ? t->fmt(e) : "n/a");
    result += std::format("{0}{1}self-param  : {2}\n", _tabs, tab, self_param);
    result += std::format("{0}}}\n", _tabs);
    return result;
}

std::string yama::internal::struct_csym::fmt(size_t tabs, const char* tab) {
    const auto& e = tu->cs->dd->installs.domain_env();
    const auto _tabs = fmt_tabs(tabs, tab);
    std::string result{};
    result += std::format("{0}struct-decl {{\n", _tabs);
    result += std::format("{0}{1}name        : {2}\n", _tabs, tab, name);
    result += std::format("{0}{1}id          : {2}\n", _tabs, tab, node ? std::format("{0}", node->id) : "n/a");
    result += std::format("{0}{1}starts      : {2}\n", _tabs, tab, starts);
    result += std::format("{0}}}\n", _tabs);
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

