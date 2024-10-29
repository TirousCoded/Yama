

#include "domain.h"

#include "../internals/builtin_type_info.h"


using namespace yama::string_literals;


yama::domain::domain(std::shared_ptr<debug> dbg)
    : api_component(dbg) {}

bool yama::domain::setup_domain() {
    const auto builtins = internal::get_builtin_type_info();

    if (!upload(
        {
        builtins.None_info,
        builtins.Int_info,
        builtins.UInt_info,
        builtins.Float_info,
        builtins.Bool_info,
        builtins.Char_info,
        })) {
        return false;
    }

    const auto _None = load("None"_str);
    const auto _Int = load("Int"_str);
    const auto _UInt = load("UInt"_str);
    const auto _Float = load("Float"_str);
    const auto _Bool = load("Bool"_str);
    const auto _Char = load("Char"_str);

    if (!_None) return false;
    if (!_Int) return false;
    if (!_UInt) return false;
    if (!_Float) return false;
    if (!_Bool) return false;
    if (!_Char) return false;

    _quick_access = std::make_optional(_quick_access_t{
        .none = _None.value(),
        .int0 = _Int.value(),
        .uint = _UInt.value(),
        .float0 = _Float.value(),
        .bool0 = _Bool.value(),
        .char0 = _Char.value(),
        });

    return true;
}

void yama::domain::fail_domain_setup() {
    throw domain_setup_error("domain setup failed!");
}

yama::type yama::domain::load_none() {
    return deref_assert(_quick_access).none;
}

yama::type yama::domain::load_int() {
    return deref_assert(_quick_access).int0;
}

yama::type yama::domain::load_uint() {
    return deref_assert(_quick_access).uint;
}

yama::type yama::domain::load_float() {
    return deref_assert(_quick_access).float0;
}

yama::type yama::domain::load_bool() {
    return deref_assert(_quick_access).bool0;
}

yama::type yama::domain::load_char() {
    return deref_assert(_quick_access).char0;
}

bool yama::domain::upload(type_info x) {
    if (!do_verify(x)) return false;
    do_upload(std::move(x));
    return true;
}

bool yama::domain::upload(std::initializer_list<type_info> x) {
    return upload(x.begin(), x.end());
}

bool yama::domain::upload(const taul::source_code& src) {
    if (const auto result = do_compile(src)) {
        return upload(*result);
    }
    else return false;
}

bool yama::domain::upload(const str& src) {
    taul::source_code s{};
    s.add_str("<src>"_str, src);
    return upload(s);
}

bool yama::domain::upload(const std::filesystem::path& src_path) {
    taul::source_code s{};
    if (!s.add_file(src_path)) {
        YAMA_RAISE(dbg(), dsignal::compile_file_not_found);
        YAMA_LOG(
            dbg(), compile_c,
            "error: file {} not found!",
            src_path.string());
        return false;
    }
    return upload(s);
}

