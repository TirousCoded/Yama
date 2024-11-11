

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

    _quick_access = _quick_access_t{
        .none = _None.value(),
        .int0 = _Int.value(),
        .uint = _UInt.value(),
        .float0 = _Float.value(),
        .bool0 = _Bool.value(),
        .char0 = _Char.value(),
    };

    return true;
}

bool yama::domain::setup_subdomain(std::weak_ptr<domain> upstream) {
    if (const auto up = upstream.lock()) {
        _quick_access = _quick_access_t{
            .none = up->load_none(),
            .int0 = up->load_int(),
            .uint = up->load_uint(),
            .float0 = up->load_float(),
            .bool0 = up->load_bool(),
            .char0 = up->load_char(),
        };
        return true;
    }
    YAMA_DEADEND;
    return false;
}

void yama::domain::fail_domain_setup() {
    throw domain_setup_error("domain setup failed!");
}

std::shared_ptr<yama::subdomain> yama::domain::fork() {
    return fork(dbg());
}

std::shared_ptr<yama::subdomain> yama::domain::fork(std::shared_ptr<debug> dbg) {
    YAMA_LOG(
        this->dbg(), domain_c,
        "forking domain...");
    return do_fork(dbg);
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

yama::subdomain::subdomain(std::weak_ptr<domain> upstream)
    : subdomain(upstream, deref_assert(upstream.lock()).dbg()) {}

yama::subdomain::subdomain(std::weak_ptr<domain> upstream, std::shared_ptr<debug> dbg)
    : domain(dbg),
    _upstream(upstream) {}

yama::subdomain::~subdomain() noexcept {
    YAMA_LOG(
        dbg(), domain_c,
        "dropping subdomain...");
}

std::weak_ptr<yama::domain> yama::subdomain::upstream() const noexcept {
    return _upstream;
}

void yama::subdomain::commit() {
    YAMA_LOG(
        dbg(), domain_c,
        "committing...");
    do_commit();
}

yama::default_domain::default_domain(std::shared_ptr<debug> dbg)
    : domain(dbg),
    // we're using a proxy for dbg to cut out annoying internal instantiation error
    // messages which will arise during successful compilation
    _state(proxy_dbg(dbg, ~instant_error_c)) {
    if (!setup_domain()) {
        fail_domain_setup();
    }
}

bool yama::default_domain::locked() const noexcept {
    return _state.locked;
}

std::optional<yama::type> yama::default_domain::load(const str& fullname) {
    if (locked()) {
        return std::nullopt;
    }
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

std::optional<std::vector<yama::type_info>> yama::default_domain::do_compile(const taul::source_code& src) {
    auto our_fork = fork();
    const auto result = _state.compiler.compile(res(our_fork), src);
    if (result) { // if successful, commit contents of subdomain to *this
        our_fork->commit();
    }
    our_fork.reset(); // drop subdomain
    return result;
}

std::shared_ptr<yama::subdomain> yama::default_domain::do_fork(std::shared_ptr<debug> dbg) {
    if (locked()) {
        return nullptr;
    }
    _state.locked = true;
    return std::make_shared<subdomain_t>(std::static_pointer_cast<default_domain>(shared_from_this()), dbg);
}

bool yama::default_domain::do_upload(type_info x) {
    if (locked()) {
        return false;
    }
    return
        _verify(x)
        ? (_upload(std::move(x)), true)
        : false;
}

bool yama::default_domain::do_upload(std::span<const type_info> x) {
    if (locked()) {
        return false;
    }
    return
        _verify(x)
        ? (_upload(std::move(x)), true)
        : false;
}

yama::default_domain::_state_t::_state_t(std::shared_ptr<debug> dbg)
    : verif(dbg),
    compiler(dbg),
    type_info_db(),
    type_db(),
    type_batch_db(),
    instant(type_info_db, type_db, type_batch_db, std::allocator<void>{}, dbg) {}

yama::default_domain::_state_t::_state_t(std::shared_ptr<debug> dbg, _state_t& upstream)
    : verif(dbg),
    compiler(dbg),
    type_info_db(upstream.type_info_db),
    type_db(upstream.type_db),
    type_batch_db(),
    instant(type_info_db, type_db, type_batch_db, std::allocator<void>{}, dbg) {}

bool yama::default_domain::_verify(const type_info& x) {
    return _state.verif.verify(x);
}

bool yama::default_domain::_verify(std::span<const type_info> x) {
    for (const auto& I : x) {
        if (!_state.verif.verify(I)) {
            return false;
        }
    }
    return true;
}

void yama::default_domain::_upload(type_info&& x) {
    _state.type_info_db.push(make_res<type_info>(std::forward<type_info>(x)));
}

void yama::default_domain::_upload(std::span<const type_info> x) {
    for (const auto& I : x) {
        _upload(type_info(I));
    }
}

yama::default_domain::subdomain_t::subdomain_t(std::weak_ptr<default_domain> upstream, std::shared_ptr<debug> dbg)
    : subdomain(upstream, dbg),
    // we're using a proxy for dbg to cut out annoying internal instantiation error
    // messages which will arise during successful compilation
    _state(proxy_dbg(dbg, ~instant_error_c), deref_assert(upstream.lock())._state) {
    _state.upstream_is_default_domain = true;
    if (!setup_subdomain(upstream)) {
        fail_domain_setup();
    }
}

yama::default_domain::subdomain_t::subdomain_t(std::weak_ptr<subdomain_t> upstream, std::shared_ptr<debug> dbg)
    : subdomain(upstream, dbg),
    // we're using a proxy for dbg to cut out annoying internal instantiation error
    // messages which will arise during successful compilation
    _state(proxy_dbg(dbg, ~instant_error_c), deref_assert(upstream.lock())._state) {
    _state.upstream_is_default_domain = false;
    if (!setup_subdomain(upstream)) {
        fail_domain_setup();
    }
}

yama::default_domain::subdomain_t::~subdomain_t() noexcept {
    if (const auto up = upstream().lock()) { // <- ensure upstream still exists before trying anything
        _upstream_state().locked = false;
    }
}

bool yama::default_domain::subdomain_t::locked() const noexcept {
    return _state.locked;
}

std::optional<yama::type> yama::default_domain::subdomain_t::load(const str& fullname) {
    if (locked()) {
        return std::nullopt;
    }
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

std::optional<std::vector<yama::type_info>> yama::default_domain::subdomain_t::do_compile(const taul::source_code& src) {
    auto our_fork = fork();
    const auto result = _state.compiler.compile(res(our_fork), src);
    if (result) { // if successful, commit contents of subdomain to *this
        our_fork->commit();
    }
    our_fork.reset(); // drop subdomain
    return result;
}

std::shared_ptr<yama::subdomain> yama::default_domain::subdomain_t::do_fork(std::shared_ptr<debug> dbg) {
    if (locked()) {
        return nullptr;
    }
    _state.locked = true;
    return std::make_shared<subdomain_t>(std::static_pointer_cast<subdomain_t>(shared_from_this()), dbg);
}

bool yama::default_domain::subdomain_t::do_upload(type_info x) {
    if (locked()) {
        return false;
    }
    return
        _verify(x)
        ? (_upload(std::move(x)), true)
        : false;
}

bool yama::default_domain::subdomain_t::do_upload(std::span<const type_info> x) {
    if (locked()) {
        return false;
    }
    return
        _verify(x)
        ? (_upload(std::move(x)), true)
        : false;
}

void yama::default_domain::subdomain_t::do_commit() {
    const bool success =
        _state.type_info_db.commit() &&
        _state.type_db.commit();
    YAMA_ASSERT(success);
}

bool yama::default_domain::subdomain_t::_verify(const type_info& x) {
    return _state.verif.verify(x);
}

bool yama::default_domain::subdomain_t::_verify(std::span<const type_info> x) {
    for (const auto& I : x) {
        if (!_state.verif.verify(I)) {
            return false;
        }
    }
    return true;
}

void yama::default_domain::subdomain_t::_upload(type_info&& x) {
    _state.type_info_db.push(make_res<type_info>(std::forward<type_info>(x)));
}

void yama::default_domain::subdomain_t::_upload(std::span<const type_info> x) {
    for (const auto& I : x) {
        _upload(type_info(I));
    }
}

yama::default_domain::_state_t& yama::default_domain::subdomain_t::_upstream_state() const {
    return
        _state.upstream_is_default_domain
        ? res<default_domain>(upstream().lock())->_state
        : res<subdomain_t>(upstream().lock())->_state;
}

