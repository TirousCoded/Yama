

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

std::optional<yama::internal::ctype> yama::internal::var_csym::get_type(compiler& cs) const {
    if (annot_type)         return annot_type->get_type(cs);
    else if (initializer)   return initializer->get_type(cs);
    else                    return std::nullopt;
}

std::optional<yama::internal::ctype> yama::internal::fn_like_csym::param::get_type(compiler& cs) const {
    return
        type
        ? type->get_type(cs)
        : (is_self_param ? cs.types.load(fn_qn) : std::nullopt);
}

std::optional<yama::internal::ctype> yama::internal::fn_like_csym::get_return_type(compiler& cs) const {
    return return_type ? return_type->get_type(cs) : std::nullopt;
}

yama::internal::ctype yama::internal::fn_like_csym::get_return_type_or_none(translation_unit& tu) const {
    return tu.types.default_none(get_return_type(*tu.cs));
}

std::string yama::internal::fn_like_csym::fmt_params(compiler& cs) const {
    std::string result{};
    result += "(";
    for (const auto& param : params) {
        const bool not_first = &param != &params.front();
        if (not_first) {
            result += ", ";
        }
        std::string t = param.get_type(cs).value().fmt(cs.dd->installs.domain_env());
        //std::string t =
        //    param.is_self_param
        //    ? "*self*"
        //    : (param.type ? param.type->get_type(cs).value().fmt(cs.dd->installs.domain_env()) : "n/a");
        result += std::format("{0}: {1}", param.name, t);
    }
    result += ")";
    return result;
}

bool yama::internal::param_csym::good() const noexcept {
    return ptr;
}

yama::internal::fn_like_csym::param& yama::internal::param_csym::get() const noexcept {
    return deref_assert(ptr);
}

std::optional<yama::internal::ctype> yama::internal::param_csym::get_type(compiler& cs) const {
    return good() ? get().get_type(cs) : std::nullopt;
}

std::string yama::internal::csymtab_entry::fmt(compiler& cs, size_t tabs, const char* tab) {
    const auto _tabs = fmt_tabs(tabs, tab);
    std::string result{};
    static_assert(std::variant_size_v<info_t> == 6); // reminder
    if (is<import_csym>()) {
        result += std::format("{0}import-decl {{\n", _tabs);
        result += std::format("{0}{1}name        : {2}\n", _tabs, tab, name);
        result += std::format("{0}{1}lp          : {2}\n", _tabs, tab, fmt_lookup_proc(lp));
        result += std::format("{0}{1}id          : {2}\n", _tabs, tab, node ? std::format("{0}", node->id) : "n/a");
        result += std::format("{0}{1}starts      : {2}\n", _tabs, tab, starts);
        result += std::format("{0}{1}path        : {2}\n", _tabs, tab, as<import_csym>().path);
        result += std::format("{0}}}\n", _tabs);
    }
    else if (is<prim_csym>()) {
        result += std::format("{0}prim-decl {{\n", _tabs);
        result += std::format("{0}{1}name        : {2}\n", _tabs, tab, name);
        result += std::format("{0}{1}lp          : {2}\n", _tabs, tab, fmt_lookup_proc(lp));
        result += std::format("{0}{1}id          : {2}\n", _tabs, tab, node ? std::format("{0}", node->id) : "n/a");
        result += std::format("{0}{1}starts      : {2}\n", _tabs, tab, starts);
        result += std::format("{0}}}\n", _tabs);
    }
    else if (is<var_csym>()) {
        result += std::format("{0}var-decl {{\n", _tabs);
        result += std::format("{0}{1}name        : {2}\n", _tabs, tab, name);
        result += std::format("{0}{1}lp          : {2}\n", _tabs, tab, fmt_lookup_proc(lp));
        result += std::format("{0}{1}id          : {2}\n", _tabs, tab, node ? std::format("{0}", node->id) : "n/a");
        result += std::format("{0}{1}starts      : {2}\n", _tabs, tab, starts);
        result += std::format("{0}{1}annot_type  : {2}\n", _tabs, tab, as<var_csym>().annot_type ? as<var_csym>().annot_type->get_type(cs).value().fmt(cs.dd->installs.domain_env()) : "n/a");
        result += std::format("{0}{1}initializer : {2}\n", _tabs, tab, as<var_csym>().initializer ? as<var_csym>().initializer->get_type(cs).value().fmt(cs.dd->installs.domain_env()) : "n/a");
        result += std::format("{0}}}\n", _tabs);
    }
    else if (is<fn_like_csym>()) {
        result += std::format("{0}fn-like-decl {{\n", _tabs);
        result += std::format("{0}{1}name        : {2}\n", _tabs, tab, name);
        result += std::format("{0}{1}lp          : {2}\n", _tabs, tab, fmt_lookup_proc(lp));
        result += std::format("{0}{1}id          : {2}\n", _tabs, tab, node ? std::format("{0}", node->id) : "n/a");
        result += std::format("{0}{1}starts      : {2}\n", _tabs, tab, starts);
        result += std::format("{0}{1}params      : {2}\n", _tabs, tab, as<fn_like_csym>().fmt_params(cs));
        result += std::format("{0}{1}return_type : {2}\n", _tabs, tab, as<fn_like_csym>().return_type ? as<fn_like_csym>().return_type->get_type(cs).value().fmt(cs.dd->installs.domain_env()) : "n/a");
        result += std::format("{0}}}\n", _tabs);
    }
    else if (is<param_csym>()) {
        result += std::format("{0}param-decl {{\n", _tabs);
        result += std::format("{0}{1}name        : {2}\n", _tabs, tab, name);
        result += std::format("{0}{1}lp          : {2}\n", _tabs, tab, fmt_lookup_proc(lp));
        result += std::format("{0}{1}id          : {2}\n", _tabs, tab, node ? std::format("{0}", node->id) : "n/a");
        result += std::format("{0}{1}starts      : {2}\n", _tabs, tab, starts);
        result += std::format("{0}}}\n", _tabs);
    }
    else if (is<struct_csym>()) {
        result += std::format("{0}struct-decl {{\n", _tabs);
        result += std::format("{0}{1}name        : {2}\n", _tabs, tab, name);
        result += std::format("{0}{1}lp          : {2}\n", _tabs, tab, fmt_lookup_proc(lp));
        result += std::format("{0}{1}id          : {2}\n", _tabs, tab, node ? std::format("{0}", node->id) : "n/a");
        result += std::format("{0}{1}starts      : {2}\n", _tabs, tab, starts);
        result += std::format("{0}}}\n", _tabs);
    }
    else YAMA_DEADEND;
    return result;
}

std::string yama::internal::csymtab::fmt(compiler& cs, size_t tabs, const char* tab) {
    const auto _tabs = fmt_tabs(tabs, tab);
    std::string result{};
    result += std::format("{0}csymtab ({1} symbols) {{\n", _tabs, count());
    for (const auto& I : _entries) {
        result += I.second->fmt(cs, tabs + 1, tab);
    }
    result += std::format("{0}}}\n", _tabs);
    return result;
}

std::string yama::internal::csymtab_group::fmt(compiler& cs, size_t tabs, const char* tab) {
    const auto _tabs = fmt_tabs(tabs, tab);
    std::string result{};
    result += std::format("{0}csymtab_group ({1} csymtabs) {{\n", _tabs, count());
    for (const auto& I : _csymtabs) {
        result += std::format("{0}{1}(for ID={2})\n", _tabs, tab, I.first);
        result += I.second->fmt(cs, tabs + 1, tab);
    }
    result += std::format("{0}}}\n", _tabs);
    return result;
}

