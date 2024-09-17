

#include "context.h"

#include "general.h"
#include "kind-features.h"
#include "callsig.h"


using namespace yama::string_literals;


yama::context::context(res<domain> dm, ctx_config config, std::shared_ptr<debug> dbg)
    : api_component(dbg),
    _dm(dm),
    _config(config),
    _mas(dm->get_mas()),
    _al(std::allocator<void>{}),
    _callstk(_al) {
    YAMA_ASSERT(get_config().max_call_frames >= 1);
    _push_user_cf();
}

yama::res<yama::domain> yama::context::get_domain() const noexcept {
    return _dm;
}

yama::domain& yama::context::dm() const noexcept {
    return *_dm;
}

const yama::ctx_config& yama::context::get_config() const noexcept {
    return _config;
}

yama::res<yama::mas> yama::context::get_mas() const noexcept {
    return _mas;
}

std::optional<yama::type> yama::context::load(const str& fullname) {
    return _dm->load(fullname);
}

yama::type yama::context::load_none() {
    return _dm->load_none();
}

yama::type yama::context::load_int() {
    return _dm->load_int();
}

yama::type yama::context::load_uint() {
    return _dm->load_uint();
}

yama::type yama::context::load_float() {
    return _dm->load_float();
}

yama::type yama::context::load_bool() {
    return _dm->load_bool();
}

yama::type yama::context::load_char() {
    return _dm->load_char();
}

std::string yama::context::fmt_stacktrace(size_t skip, const char* tab) const {
    YAMA_ASSERT(tab);
    std::string result{};
    result += "stacktrace";
    size_t number = _callstk.size() - 1;
    const auto begin = std::next(_callstk.crbegin(), std::min(skip, _callstk.size()));
    const auto end = _callstk.crend();
    for (auto it = begin; it != end; std::advance(it, 1)) {
        const auto& callframe = *it;
        result +=
            it == begin
            ? std::format("\n{}[{}] ", tab, number)
            : std::format("\n{}[{}] (called by) ", tab, number);
        result +=
            callframe.t
            ? std::format("{}", deref_assert(callframe.t))
            : "<user>";
        // check for symbol info, and if so, add it to result
        if (callframe.t) {
            const auto info = internal::get_type_mem(deref_assert(callframe.t))->info;
            if (info->call_fn() == bcode_call_fn) {
                const auto& code = deref_assert(info->bcode());
                const auto& syms = deref_assert(info->bcodesyms());
                // remember that program counter will be incr when fetching instr, meaning
                // we need to decr it to get current instr index
                const size_t x_index = callframe.pc - 1;
                const bc::instr x_instr = code[x_index];
                if (const auto x_sym = syms[x_index]) {
                    result += std::format(" {}", *x_sym);
                }
                else {
                    // write info about instr index if actual sym isn't available
                    result += std::format(" [{}; instr {}]", x_instr.opc, x_index);
                }
            }
        }
        number--;
    }
    return result;
}

void yama::context::ll_add_ref(const object_ref& x) {
    // TODO: update this method later (when it's no longer a noop)
}

void yama::context::ll_remove_ref(const object_ref& x) {
    // TODO: update this method later (when it's no longer a noop)
}

void yama::context::ll_drop_ref(object_ref& x) {
    ll_remove_ref(x);
}

void yama::context::ll_copy_ref(const object_ref& src, object_ref& dest) {
    if (src == dest) return; // avoid potential issues w/ self-assignment
    ll_add_ref(src);
    ll_remove_ref(dest);
    dest = src;
}

void yama::context::ll_move_ref(object_ref& src, object_ref& dest) {
    if (src == dest) return; // avoid potential issues w/ self-assignment
    ll_drop_ref(dest);
    dest = std::move(src);
}

void yama::context::ll_swap_ref(object_ref& a, object_ref& b) {
    if (a == b) return; // no need to swap
    std::swap(a, b);
}

yama::object_ref yama::context::ll_clone_ref(borrowed_ref x) {
    ll_add_ref(x);
    return x;
}

std::optional<yama::object_ref> yama::context::ll_clone_ref(std::optional<borrowed_ref> x) {
    return
        x
        ? std::make_optional(ll_clone_ref(*x))
        : std::nullopt;
}

yama::canonical_ref yama::context::ll_new_none() {
    return canonical_ref{
        .t = load_none(),
        .v{ .i = 0 },
    };
}

yama::canonical_ref yama::context::ll_new_int(int_t v) {
    return canonical_ref{
        .t = load_int(),
        .v{ .i = v },
    };
}

yama::canonical_ref yama::context::ll_new_uint(uint_t v) {
    return canonical_ref{
        .t = load_uint(),
        .v{ .ui = v },
    };
}

yama::canonical_ref yama::context::ll_new_float(float_t v) {
    return canonical_ref{
        .t = load_float(),
        .v{ .f = v },
    };
}

yama::canonical_ref yama::context::ll_new_bool(bool_t v) {
    return canonical_ref{
        .t = load_bool(),
        .v{ .b = v },
    };
}

yama::canonical_ref yama::context::ll_new_char(char_t v) {
    return canonical_ref{
        .t = load_char(),
        .v{ .c = v },
    };
}

std::optional<yama::canonical_ref> yama::context::ll_new_fn(type f) {
    canonical_ref result{
        .t = f,
        .v{ .i = 0 },
    };
    return
        is_function(f.kind())
        ? std::make_optional(result)
        : std::nullopt;
}

size_t yama::context::ll_panics() noexcept {
    return _panics;
}

bool yama::context::ll_panicking() noexcept {
    return _panicking;
}

bool yama::context::ll_is_user() noexcept {
    return ll_call_frames() == 1;
}

size_t yama::context::ll_max_call_frames() noexcept {
    return _config.max_call_frames;
}

size_t yama::context::ll_call_frames() noexcept {
    return _callstk.size();
}

yama::const_table yama::context::ll_consts() noexcept {
    return _curr_type().consts();
}

size_t yama::context::ll_args() noexcept {
    YAMA_ASSERT(!_callstk.empty());
    return _top_cf().args_count;
}

size_t yama::context::ll_locals() noexcept {
    YAMA_ASSERT(!_callstk.empty());
    return _top_cf().locals_count;
}

std::optional<yama::object_ref> yama::context::ll_arg(arg_t x) {
    const auto view = _view_top_cf();
    return
        view.args_bounds_check(x)
        ? std::make_optional(ll_clone_ref(view.args[x]))
        : std::nullopt;
}

std::optional<yama::object_ref> yama::context::ll_local(local_t x) {
    const auto view = _view_top_cf();
    return
        view.locals_bounds_check(x)
        ? std::make_optional(ll_clone_ref(view.locals[x]))
        : std::nullopt;
}

void yama::context::ll_panic() {
    YAMA_LOG(dbg(), ctx_llcmd_c, " > panic");
    if (ll_panicking()) return; // don't count another panic if we're already panicking
    _panics++;
    _panicking = true;
    _try_handle_panic_for_user_cf(); // in case panicking outside of a ll_call call
}

yama::cmd_status yama::context::ll_load(local_t x, borrowed_ref v) {
    YAMA_LOG(dbg(), ctx_llcmd_c, " >       {: <13} = {}", _fmt_R_no_preview(x), v);
    return _load(x, ll_clone_ref(v));
}

yama::cmd_status yama::context::ll_load_none(local_t x) {
    return ll_load(x, ll_new_none());
}

yama::cmd_status yama::context::ll_load_int(local_t x, int_t v) {
    return ll_load(x, ll_new_int(v));
}

yama::cmd_status yama::context::ll_load_uint(local_t x, uint_t v) {
    return ll_load(x, ll_new_uint(v));
}

yama::cmd_status yama::context::ll_load_float(local_t x, float_t v) {
    return ll_load(x, ll_new_float(v));
}

yama::cmd_status yama::context::ll_load_bool(local_t x, bool v) {
    return ll_load(x, ll_new_bool(v));
}

yama::cmd_status yama::context::ll_load_char(local_t x, char_t v) {
    return ll_load(x, ll_new_char(v));
}

yama::cmd_status yama::context::ll_load_fn(local_t x, type f) {
    return _load_fn(x, f);
}

yama::cmd_status yama::context::ll_load_const(local_t x, const_t c) {
    YAMA_LOG(dbg(), ctx_llcmd_c, " >       {: <13} = {}", _fmt_R_no_preview(x), _fmt_Ko(c));
    return _load_const(x, c);
}

yama::cmd_status yama::context::ll_load_arg(local_t x, arg_t arg) {
    YAMA_LOG(dbg(), ctx_llcmd_c, " >       {: <13} = {}", _fmt_R_no_preview(x), _fmt_Arg(arg));
    return _load_arg(x, arg);
}

yama::cmd_status yama::context::ll_copy(local_t src, local_t dest) {
    YAMA_LOG(dbg(), ctx_llcmd_c, " >       {: <13} = {}", _fmt_R_no_preview(dest), _fmt_R(src));
    return _copy(src, dest);
}

yama::cmd_status yama::context::ll_call(local_t args_start, size_t args_n, local_t ret) {
    if (args_n == 0) {
        YAMA_LOG(dbg(), ctx_llcmd_c, 
            " >       {: <13} = *call* (n/a)",
            _fmt_R_no_preview(ret));
    }
    else if (args_n == 1) {
        YAMA_LOG(dbg(), ctx_llcmd_c,
            " >       {: <13} = *call* ({})",
            _fmt_R_no_preview(ret),
            _fmt_R(args_start));
    }
    else if (args_n == 2) {
        YAMA_LOG(dbg(), ctx_llcmd_c,
            " >       {: <13} = *call* ({}, {})",
            _fmt_R_no_preview(ret),
            _fmt_R(args_start),
            _fmt_R(args_start + 1));
    }
    else {
        YAMA_LOG(dbg(), ctx_llcmd_c,
            " >       {: <13} = *call* ({}, {}, ..., {})",
            _fmt_R_no_preview(ret),
            _fmt_R(args_start),
            _fmt_R(args_start + 1),
            _fmt_R(args_start + args_n - 1));
    }
    if (!_call_ret_to_local_precheck(ret)) return yama::cmd_status::init(false);
    return _call_ret_to_local(_call(args_start, args_n), ret);
}

yama::cmd_status yama::context::ll_call_nr(local_t args_start, size_t args_n) {
    if (args_n == 0) {
        YAMA_LOG(dbg(), ctx_llcmd_c,
            " >       n/a           = *call* (n/a)");
    }
    else if (args_n == 1) {
        YAMA_LOG(dbg(), ctx_llcmd_c,
            " >       n/a           = *call* ({})",
            _fmt_R(args_start));
    }
    else if (args_n == 2) {
        YAMA_LOG(dbg(), ctx_llcmd_c,
            " >       n/a           = *call* ({}, {})",
            _fmt_R(args_start),
            _fmt_R(args_start + 1));
    }
    else {
        YAMA_LOG(dbg(), ctx_llcmd_c,
            " >       n/a           = *call* ({}, {}, ..., {})",
            _fmt_R(args_start),
            _fmt_R(args_start + 1),
            _fmt_R(args_start + args_n - 1));
    }
    return _call_ret_to_no_result(_call(args_start, args_n));
}

yama::cmd_status yama::context::ll_ret(local_t x) {
    YAMA_LOG(dbg(), ctx_llcmd_c, " >       return        = {}", _fmt_R(x));
    return _ret(x);
}

bool yama::context::_cf_view_t::args_bounds_check(arg_t x) const noexcept {
    return x < args.size();
}

bool yama::context::_cf_view_t::locals_bounds_check(local_t x) const noexcept {
    return x < locals.size();
}

yama::context::_cf_t& yama::context::_top_cf() noexcept {
    YAMA_ASSERT(!_callstk.empty());
    return _callstk.back();
}

yama::context::_cf_t& yama::context::_below_top_cf() noexcept {
    YAMA_ASSERT(_callstk.size() >= 2);
    return _callstk[_callstk.size() - 2];
}

yama::context::_cf_view_t yama::context::_view_top_cf() {
    const auto& cf = _top_cf();
    return _cf_view_t{
        .args = std::span<object_ref>{
            _registers.begin() + cf.args_offset,
            cf.args_count,
        },
        .locals = std::span<object_ref>{
            _registers.begin() + cf.locals_offset,
            cf.locals_count,
        },
    };
}

yama::context::_cf_view_t yama::context::_view_below_top_cf() {
    const auto& cf = _below_top_cf();
    return _cf_view_t{
        .args = std::span<object_ref>{
            _registers.begin() + cf.args_offset,
            cf.args_count,
        },
        .locals = std::span<object_ref>{
            _registers.begin() + cf.locals_offset,
            cf.locals_count,
        },
    };
}

yama::type yama::context::_curr_type() noexcept {
    return deref_assert(_top_cf().t);
}

bool yama::context::_has_curr_type() noexcept {
    return _top_cf().t.has_value();
}

bool yama::context::_push_cf(std::optional<type> t, local_t args_start, size_t args_n, size_t locals) {
    if (ll_call_frames() == ll_max_call_frames()) return false; // overflow
    const bool has_top_cf = !_callstk.empty();
    YAMA_ASSERT(has_top_cf || args_start == 0); // if pushing user call frame then args_start == 0
    // prep new call frame
    _cf_t new_cf{
        .t              = t,
        // 0 below is fail-safe for user call frame
        .args_offset    = has_top_cf ? _top_cf().locals_offset + args_start : 0,
        .args_count     = args_n,
        .locals_offset  = _registers.size(), // <- breaks if _add_registers is moved to above this
        .locals_count   = locals,
        .returned       = std::nullopt,

        .pc             = 0,
        .should_halt    = false,
    };
    // push new call frame
    _callstk.push_back(std::move(new_cf));
    // add registers
    _add_registers(locals);
    return true;
}

void yama::context::_pop_cf() {
    if (_callstk.empty()) return;
    _remove_registers(_top_cf().locals_count);
    _callstk.pop_back();
}

void yama::context::_push_user_cf() {
    YAMA_ASSERT(_callstk.empty());
    const bool success = _push_cf(std::nullopt, 0, 0, _config.user_locals);
    YAMA_ASSERT(success);
}

void yama::context::_add_registers(size_t n) {
    // since ll_new_none returns a canonical_ref, we need-not incr ref counts
    _registers.resize(_registers.size() + n, ll_new_none());
}

void yama::context::_remove_registers(size_t n) {
    if (n > _registers.size()) n = _registers.size();
    for (; n > 0; n--) {
        // ll_drop_ref each one, in reverse order (ie. going top -> bottom)
        ll_drop_ref(_registers.back());
        _registers.pop_back();
    }
}

void yama::context::_try_handle_panic_for_user_cf() {
    if (!ll_panicking()) return;
    if (!ll_is_user()) return;
    _pop_cf();
    YAMA_LOG(dbg(), general_c, "reinitializing...");
    _panicking = false;
    _push_user_cf();
}

std::string yama::context::_fmt_R_no_preview(local_t x) {
    return
        x < ll_locals()
        ? std::format("R({})", x)
        : std::format("R({}) (error)", x);
}

std::string yama::context::_fmt_R(local_t x) {
    const auto a = ll_local(x);
    return
        a
        ? std::format("R({}) [{}]", x, *a)
        : std::format("R({}) (error)", x);
}

std::string yama::context::_fmt_Ko(const_t x) {
    return
        _has_curr_type()
        ? std::format("Ko({}) [{}]", x, ll_consts().fmt_const(x))
        : std::format("Ko({}) (error)", x);
}

std::string yama::context::_fmt_Arg(arg_t x) {
    const auto a = ll_arg(x);
    return
        a
        ? std::format("Arg({}) [{}]", x, *a)
        : std::format("Arg({}) (error)", x);
}

yama::cmd_status yama::context::_load(local_t x, stolen_ref v) {
    if (_load_err_x_out_of_bounds(x, v)) {
        return cmd_status::init(false);
    }
    auto& x_local = _view_top_cf().locals[x];
    ll_move_ref(v, x_local);
    return cmd_status::init(true);
}

bool yama::context::_load_err_x_out_of_bounds(local_t x, borrowed_ref v) {
    if (_view_top_cf().locals_bounds_check(x)) return false;
    _panic(
        "error: panic from attempt to load object {} into local register at out-of-bounds index {}!",
        v, x);
    return true;
}

yama::cmd_status yama::context::_load_fn(local_t x, type f) {
    if (_load_fn_err_f_not_callable_type(f)) {
        return cmd_status::init(false);
    }
    return ll_load(x, ll_new_fn(f).value());
}

bool yama::context::_load_fn_err_f_not_callable_type(type f) {
    if (is_callable(f.kind())) return false;
    _panic(
        "error: panic from attempt to load function object of non-callable type {}!",
        f);
    return true;
}

yama::cmd_status yama::context::_load_const(local_t x, const_t c) {
    static_assert(
        []() constexpr -> bool {
            // NOTE: extend this each time we add a new loadable object constant
            static_assert(yama::const_types == 7);
            return
                yama::is_object_const(yama::int_const) &&
                yama::is_object_const(yama::uint_const) &&
                yama::is_object_const(yama::float_const) &&
                yama::is_object_const(yama::bool_const) &&
                yama::is_object_const(yama::char_const) &&
                !yama::is_object_const(yama::primitive_type_const) &&
                yama::is_object_const(yama::function_type_const);
        }());
    if (_load_const_err_in_user_call_frame()) {
        return cmd_status::init(false);
    }
    if (_load_const_err_c_out_of_bounds(c)) {
        return cmd_status::init(false);
    }
    if (_load_const_err_c_is_not_object_constant(c)) {
        return cmd_status::init(false);
    }
    return _load(x,
        [&]() -> canonical_ref {
            const auto& cc = ll_consts();
            static_assert(const_types == 7);
            if (const auto r = cc.get<int_const>(c))                return ll_new_int(*r);
            else if (const auto r = cc.get<uint_const>(c))          return ll_new_uint(*r);
            else if (const auto r = cc.get<float_const>(c))         return ll_new_float(*r);
            else if (const auto r = cc.get<bool_const>(c))          return ll_new_bool(*r);
            else if (const auto r = cc.get<char_const>(c))          return ll_new_char(*r);
            else if (const auto r = cc.get<function_type_const>(c)) return ll_new_fn(*r).value();
            else                                                    YAMA_DEADEND;
            return ll_new_none(); // dummy
        }());
}

bool yama::context::_load_const_err_in_user_call_frame() {
    if (!ll_is_user()) return false;
    _panic(
        "error: panic from attempt to load object constant from the user call frame!");
    return true;
}

bool yama::context::_load_const_err_c_out_of_bounds(const_t c) {
    if (c < ll_consts().size()) {
        return false;
    }
    _panic(
        "error: panic from attempt to load object constant from out-of-bounds constant index {}!",
        c);
    return true;
}

bool yama::context::_load_const_err_c_is_not_object_constant(const_t c) {
    if (is_object_const(ll_consts().const_type(c).value())) {
        return false;
    }
    _panic(
        "error: panic from attempt to load object constant from constant index {}, but the constant is not an object constant!",
        c);
    return true;
}

yama::cmd_status yama::context::_load_arg(local_t x, arg_t arg) {
    if (_load_arg_err_in_user_call_frame()) {
        return cmd_status::init(false);
    }
    if (_load_arg_err_arg_out_of_bounds(arg)) {
        return cmd_status::init(false);
    }
    return _load(x, ll_arg(arg).value());
}

bool yama::context::_load_arg_err_in_user_call_frame() {
    if (!ll_is_user()) return false;
    _panic(
        "error: panic from attempt to load argument object from the user call frame!");
    return true;
}

bool yama::context::_load_arg_err_arg_out_of_bounds(arg_t arg) {
    if (_view_top_cf().args_bounds_check(arg)) return false;
    _panic(
        "error: panic from attempt to load argument object from out-of-bounds argument index {}!",
        arg);
    return true;
}

yama::cmd_status yama::context::_copy(local_t src, local_t dest) {
    if (_copy_err_src_out_of_bounds(src)) {
        return cmd_status::init(false);
    }
    auto& src_local = _view_top_cf().locals[src];
    if (_copy_err_dest_out_of_bounds(dest, src_local)) {
        // TODO: I'm *PRETTY SURE* we don't need to decr x ref count, right? as panic will clean up everything, right?
        return cmd_status::init(false);
    }
    auto& dest_local = _view_top_cf().locals[dest];
    ll_move_ref(src_local, dest_local); // perform move-assign
    return cmd_status::init(true);
}

bool yama::context::_copy_err_src_out_of_bounds(local_t src) {
    if (_view_top_cf().locals_bounds_check(src)) return false;
    _panic(
        "error: panic from attempt to copy object from out-of-bounds local register index {}!",
        src);
    return true;
}

bool yama::context::_copy_err_dest_out_of_bounds(local_t dest, borrowed_ref src_object) {
    if (_view_top_cf().locals_bounds_check(dest)) return false;
    _panic(
        "error: panic from attempt to copy object {} to out-of-bounds local register index {}!",
        src_object, dest);
    return true;
}

std::optional<yama::object_ref> yama::context::_call(local_t args_start, size_t args_n) {
    if (_call_err_no_callobj(args_n)) {
        return std::nullopt;
    }
    const auto& top_cf = _top_cf();
    if (_call_err_args_out_of_bounds(args_start, args_n)) {
        return std::nullopt;
    }
    const borrowed_ref callobj = _view_top_cf().locals[args_start];
    const type callobj_type = callobj.t;
    if (_call_err_callobj_not_callable_type(callobj)) {
        return std::nullopt;
    }
    const size_t param_args = args_n - 1;
    if (_call_err_param_arg_count_mismatch(callobj, param_args)) {
        return std::nullopt;
    }
    // push our new call frame
    const bool push_cf_result = _push_cf(std::make_optional(callobj_type), args_start, args_n, callobj_type.locals());
    if (_call_err_push_cf_would_overflow(callobj, push_cf_result)) {
        return std::nullopt;
    }
    // fire call behaviour
    YAMA_LOG(dbg(), ctx_llcmd_c, " > call frame {}: *enter*", ll_call_frames() - 1);
    const call_fn callobj_call_fn = deref_assert(callobj_type.call_fn());
    if (callobj_call_fn != bcode_call_fn) { // fire non-bcode-based call behaviour
        callobj_call_fn(*this);
    }
    else { // fire bcode-based call behaviour
        _bcode_exec();
    }
    YAMA_LOG(dbg(), ctx_llcmd_c, " > call frame {}: *exit*", ll_call_frames() - 1);
    // propagate panic if call behaviour panicked
    if (ll_panicking()) {
        _pop_cf(); // cleanup before abort
        _try_handle_panic_for_user_cf(); // handle reset of user cf if needed
        return std::nullopt;
    }
    if (_call_err_no_return_value_object(callobj)) {
        _pop_cf(); // cleanup before abort
        _try_handle_panic_for_user_cf(); // handle reset of user cf if needed
        return std::nullopt;
    }
    // move-assign call frame's returned object (ie. don't need to incr/decr any ref counts)
    auto returned = std::move(_top_cf().returned);
    // cleanup call frame of call and return
    _pop_cf();
    return returned;
}

yama::cmd_status yama::context::_call_ret_to_local(std::optional<stolen_ref> ret, local_t target) {
    if (!ret) return cmd_status::init(false); // fail quietly if upstream failed
    // NOTE: this method occurs in the caller's call frame, w/ the call's having already been unwound
    YAMA_ASSERT(_view_top_cf().locals_bounds_check(target));
    auto& target_objref = _view_top_cf().locals[target];
    ll_move_ref(*ret, target_objref); // perform move-assign
    return cmd_status::init(true);
}

yama::cmd_status yama::context::_call_ret_to_no_result(std::optional<stolen_ref> ret) {
    if (!ret) return cmd_status::init(false); // fail quietly if upstream failed
    ll_drop_ref(*ret); // just drop *ret and we're done
    // TODO: when we add dtors, what happens if ll_drop_ref causes dtor which panics?
    //       should we return bad cmd_status?
    return cmd_status::init(true);
}

bool yama::context::_call_ret_to_local_precheck(local_t target) {
    return !_call_err_ret_ouf_of_bounds(target);
}

bool yama::context::_call_err_no_callobj(size_t args_n) {
    if (args_n >= 1) return false;
    _panic(
        "error: panic from call with no call object!");
    return true;
}

bool yama::context::_call_err_args_out_of_bounds(local_t args_start, size_t args_n) {
    const auto top_cf_view = _view_top_cf();
    // remember that here [args_start, args_start+args_n) is referring to indices
    // into the caller's local register table
    const bool first_in_bounds = top_cf_view.locals_bounds_check(args_start);
    const bool last_in_bounds = top_cf_view.locals_bounds_check(args_start + args_n - 1);
    if (first_in_bounds && last_in_bounds) return false;
    _panic(
        "error: panic from call with out-of-bounds args range [{}, {})!",
        args_start, args_start + args_n);
    return true;
}

bool yama::context::_call_err_callobj_not_callable_type(borrowed_ref callobj) {
    if (is_callable(callobj.t.kind())) return false;
    _panic(
        "error: panic from call to {} which is of non-callable type {}!",
        callobj, callobj.t);
    return true;
}

bool yama::context::_call_err_param_arg_count_mismatch(borrowed_ref callobj, size_t param_args) {
    if (callobj.t.callsig()->params() == param_args) return false;
    _panic(
        "error: panic from call to {} with {} param args, but expects {}!",
        callobj, param_args, callobj.t.callsig()->params());
    return true;
}

bool yama::context::_call_err_push_cf_would_overflow(borrowed_ref callobj, bool push_cf_result) {
    if (push_cf_result) return false;
    _panic(
        "error: panic from call to {} due to call stack overflow!",
        callobj);
    return true;
}

bool yama::context::_call_err_no_return_value_object(borrowed_ref callobj) {
    if (_top_cf().returned) return false;
    _panic(
        "error: panic from call to {} due to call behaviour never returning anything!",
        callobj);
    return true;
}

bool yama::context::_call_err_ret_ouf_of_bounds(local_t ret) {
    if (_view_top_cf().locals_bounds_check(ret)) return false;
    _panic(
        "error: panic from attempt to call with return value object at out-of-bounds local register index {}!",
        ret);
    return true;
}

yama::cmd_status yama::context::_ret(local_t x) {
    if (_ret_err_in_user_call_frame()) {
        return cmd_status::init(false);
    }
    const auto top_cf_view = _view_top_cf();
    if (_ret_err_x_out_of_bounds(x)) {
        return cmd_status::init(false);
    }
    auto& top_cf = _top_cf();
    if (_ret_err_already_returned_something()) {
        return cmd_status::init(false);
    }
    // perform actual return value object cache
    top_cf.returned = std::move(ll_local(x));
    return cmd_status::init(true);
}

bool yama::context::_ret_err_in_user_call_frame() {
    if (!ll_is_user()) return false;
    _panic(
        "error: panic due to attempting to return something from the user call frame!");
    return true;
}

bool yama::context::_ret_err_x_out_of_bounds(local_t x) {
    if (_view_top_cf().locals_bounds_check(x)) return false;
    _panic(
        "error: panic from attempt to return object from out-of-bounds local register index {}!",
        x);
    return true;
}

bool yama::context::_ret_err_already_returned_something() {
    if (!_top_cf().returned) return false;
    _panic(
        "error: panic due to attempting to return object after already returning one!");
    return true;
}

void yama::context::_bcode_exec() {
    _bcode_setup();
    while (!_top_cf().should_halt) {
        const auto x = _bcode_fetch_instr();
        _bcode_exec_instr(x);
        _bcode_halt_if_panicking();
    }
}

void yama::context::_bcode_setup() {
    YAMA_ASSERT(_top_cf().pc == 0);
    YAMA_ASSERT(!_top_cf().should_halt);
    _top_cf().bcode_ptr = _bcode_acquire_bcode();
    YAMA_LOG(dbg(), bcode_exec_c, " > bcode exec");
}

const yama::bc::code* yama::context::_bcode_acquire_bcode() {
    return internal::get_type_mem(_curr_type())->info->bcode();
}

yama::bc::instr yama::context::_bcode_fetch_instr() {
    // fetch next instr
    const auto result = deref_assert(_top_cf().bcode_ptr)[_top_cf().pc];
    // incr pc after instr fetch
    // this is inline w/ expectations of instrs like jump
    _top_cf().pc++;
    return result;
}

void yama::context::_bcode_exec_instr(bc::instr x) {
    YAMA_LOG(dbg(), bcode_exec_c, "{}", deref_assert(_top_cf().bcode_ptr).fmt_instr(_top_cf().pc - 1));
    static_assert(bc::opcodes == 11);
    switch (x.opc) {
    case bc::opcode::noop:
    {
        // do nothing
    }
    break;
    case bc::opcode::load_none:
    {
        if (ll_load_none(x.A).bad()) return;
    }
    break;
    case bc::opcode::load_const:
    {
        if (ll_load_const(x.A, x.B).bad()) return;
    }
    break;
    case bc::opcode::load_arg:
    {
        if (ll_load_arg(x.A, x.B).bad()) return;
    }
    break;
    case bc::opcode::copy:
    {
        if (ll_copy(x.A, x.B).bad()) return;
    }
    break;
    case bc::opcode::call:
    {
        if (ll_call(x.A, x.B, x.C).bad()) return;
    }
    break;
    case bc::opcode::call_nr:
    {
        if (ll_call_nr(x.A, x.B).bad()) return;
    }
    break;
    case bc::opcode::ret:
    {
        if (ll_ret(x.A).bad()) return;
        _bcode_halt();
    }
    break;
    case bc::opcode::jump:
    {
        _bcode_jump(x.sBx);
    }
    break;
    case bc::opcode::jump_true:
    {
        const auto& input = deref_assert(ll_local(x.A));
        const bool should_jump = input.as_bool();
        if (should_jump) {
            _bcode_jump(x.sBx);
        }
    }
    break;
    case bc::opcode::jump_false:
    {
        const auto& input = deref_assert(ll_local(x.A));
        const bool should_jump = !input.as_bool();
        if (should_jump) {
            _bcode_jump(x.sBx);
        }
    }
    break;
    default: YAMA_DEADEND; break;
    }
}

void yama::context::_bcode_halt_if_panicking() {
    if (ll_panicking()) {
        _bcode_halt();
    }
}

void yama::context::_bcode_halt() {
    _top_cf().should_halt = true;
    YAMA_LOG(dbg(), bcode_exec_c, " > halt");
}

void yama::context::_bcode_jump(int16_t offset) {
    _top_cf().pc += std::make_signed_t<size_t>(offset);
    YAMA_LOG(dbg(), bcode_exec_c, " > jump (to {})", _top_cf().pc);
}

