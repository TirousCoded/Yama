

#include "domain.h"

#include "../internals/builtin_type_info.h"


using namespace yama::string_literals;


yama::domain::domain(std::shared_ptr<debug> dbg)
    : api_component(dbg) {}

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
    return do_upload(std::move(x));
}

bool yama::domain::upload(std::span<const type_info> x) {
    return do_upload(x);
}

bool yama::domain::upload(const std::vector<type_info>& x) {
    return upload(std::span(x.begin(), x.end()));
}

bool yama::domain::upload(std::initializer_list<type_info> x) {
    return upload(std::span(x.begin(), x.end()));
}

bool yama::domain::upload(const taul::source_code& src) {
    if (const auto result = do_compile(src)) {
        return upload(*result);
    }
    return false;
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
            dbg(), compile_error_c,
            "error: file {} not found!",
            src_path.string());
        return false;
    }
    return upload(s);
}

void yama::domain::finish_setup() {
    _quick_access = do_preload_builtins();
}

yama::default_domain::default_domain(std::shared_ptr<debug> dbg)
    : domain(dbg),
    // we're using a proxy for dbg to cut out annoying internal instantiation error
    // messages which will arise during successful compilation
    _state(proxy_dbg(dbg, ~instant_error_c)) {
    finish_setup();
    _setup_compiler_services(); // only call after finish_setup
}

std::optional<yama::type> yama::default_domain::load(const str& fullname) {
    if (const auto first_attempt = _state.type_db.pull(fullname)) {
        return type(**first_attempt);
    }
    if (const size_t number = _state.instant.instantiate(fullname); number == 0) {
        return std::nullopt;
    }
    if (const auto second_attempt = _state.type_db.pull(fullname)) {
        return type(**second_attempt);
    }
    return std::nullopt;
}

yama::domain::quick_access yama::default_domain::do_preload_builtins() {
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
        YAMA_DEADEND;
        abort(); // for release builds
    }
    return quick_access{
        .none = load("None"_str).value(),
        .int0 = load("Int"_str).value(),
        .uint = load("UInt"_str).value(),
        .float0 = load("Float"_str).value(),
        .bool0 = load("Bool"_str).value(),
        .char0 = load("Char"_str).value(),
    };
}

std::optional<std::vector<yama::type_info>> yama::default_domain::do_compile(const taul::source_code& src) {
    const auto result = _state.compiler.compile(_get_compiler_services(), src);
    if (result) _state.commit_proxy();
    else        _state.discard_proxy();
    return result;
}

bool yama::default_domain::do_upload(type_info x) {
    return
        _verify(x)
        ? (_upload(std::move(x)), true)
        : false;
}

bool yama::default_domain::do_upload(std::span<const type_info> x) {
    return
        _verify(x)
        ? (_upload(std::move(x)), true)
        : false;
}

yama::default_domain::_state_t::_state_t(std::shared_ptr<debug> dbg)
    : verif(dbg),
    compiler(dbg),
    type_info_db(),
    type_info_db_proxy(type_info_db),
    type_db(),
    type_db_proxy(type_db),
    type_batch_db(),
    instant(type_info_db, type_db, type_batch_db, std::allocator<void>{}, dbg),
    instant_proxy(type_info_db_proxy, type_db_proxy, type_batch_db, std::allocator<void>{}, dbg) {}

void yama::default_domain::_state_t::commit_proxy() {
    // these commit calls will reset our proxy for us
    const bool result =
        type_info_db_proxy.commit() &&
        type_db_proxy.commit();
    YAMA_ASSERT(result);
}

void yama::default_domain::_state_t::discard_proxy() {
    type_info_db_proxy.reset();
    type_db_proxy.reset();
}

void yama::default_domain::_setup_compiler_services() {
    _compiler_services = std::make_shared<_compiler_services_t>(*this);
}

yama::res<yama::domain> yama::default_domain::_get_compiler_services() {
    return res(_compiler_services);
}

bool yama::default_domain::_verify(const type_info& x) {
    return _state.verif.verify(x);
}

bool yama::default_domain::_verify(std::span<const type_info> x) {
    for (const auto& I : x) {
        if (!_state.verif.verify(I)) return false;
    }
    return true;
}

void yama::default_domain::_upload(type_info&& x) {
    _state.type_info_db.push(make_res<type_info>(std::forward<type_info>(x)));
}

void yama::default_domain::_upload(std::span<const type_info> x) {
    for (const auto& I : x) _upload(type_info(I));
}

yama::default_domain::_compiler_services_t::_compiler_services_t(default_domain& upstream)
    : _upstream_ptr(&upstream) {}

std::optional<yama::type> yama::default_domain::_compiler_services_t::load(const str& fullname) {
    if (const auto first_attempt = _get_state().type_db_proxy.pull(fullname)) {
        return type(**first_attempt);
    }
    if (const size_t number = _get_state().instant_proxy.instantiate(fullname); number == 0) {
        return std::nullopt;
    }
    if (const auto second_attempt = _get_state().type_db_proxy.pull(fullname)) {
        return type(**second_attempt);
    }
    return std::nullopt;
}

yama::domain::quick_access yama::default_domain::_compiler_services_t::do_preload_builtins() {
    return quick_access{
        .none = _get_upstream().load_none(),
        .int0 = _get_upstream().load_int(),
        .uint = _get_upstream().load_uint(),
        .float0 = _get_upstream().load_float(),
        .bool0 = _get_upstream().load_bool(),
        .char0 = _get_upstream().load_char(),
    };
}

std::optional<std::vector<yama::type_info>> yama::default_domain::_compiler_services_t::do_compile(const taul::source_code& src) {
    // TODO: we'll need to revise how compiling works if in the future compilation is
    //       able to recursively depend upon *upstream* compilation
    return std::nullopt;
}

bool yama::default_domain::_compiler_services_t::do_upload(type_info x) {
    return
        _verify(x)
        ? (_upload(std::move(x)), true)
        : false;
}

bool yama::default_domain::_compiler_services_t::do_upload(std::span<const type_info> x) {
    return
        _verify(x)
        ? (_upload(std::move(x)), true)
        : false;
}

bool yama::default_domain::_compiler_services_t::_verify(const type_info& x) {
    return _get_state().verif.verify(x);
}

bool yama::default_domain::_compiler_services_t::_verify(std::span<const type_info> x) {
    for (const auto& I : x) {
        if (!_get_state().verif.verify(I)) return false;
    }
    return true;
}

void yama::default_domain::_compiler_services_t::_upload(type_info&& x) {
    _get_state().type_info_db_proxy.push(make_res<type_info>(std::forward<type_info>(x)));
}

void yama::default_domain::_compiler_services_t::_upload(std::span<const type_info> x) {
    for (const auto& I : x) _upload(type_info(I));
}

