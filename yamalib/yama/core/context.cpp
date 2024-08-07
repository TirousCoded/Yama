

#include "context.h"

#include "general.h"


using namespace yama::string_literals;


yama::context::context(res<domain> dm, ctx_config config, std::shared_ptr<debug> dbg)
    : api_component(dbg),
    _dm(dm),
    _config(config),
    _mas(dm->get_mas()),
    _al(std::allocator<void>{}),
    _callstk(_al) {
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
    ll_remove_ref(dest);
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

yama::object_ref yama::context::ll_new_none() {
    // NOTE: skip the ll_add_ref, as we know our object is canonicalized
    return object_ref{
        .t = load_none(),
        .v{ .i = 0 },
    };
}

yama::object_ref yama::context::ll_new_int(int_t v) {
    // NOTE: skip the ll_add_ref, as we know our object is canonicalized
    return object_ref{
        .t = load_int(),
        .v{ .i = v },
    };
}

yama::object_ref yama::context::ll_new_uint(uint_t v) {
    // NOTE: skip the ll_add_ref, as we know our object is canonicalized
    return object_ref{
        .t = load_uint(),
        .v{ .ui = v },
    };
}

yama::object_ref yama::context::ll_new_float(float_t v) {
    // NOTE: skip the ll_add_ref, as we know our object is canonicalized
    return object_ref{
        .t = load_float(),
        .v{ .f = v },
    };
}

yama::object_ref yama::context::ll_new_bool(bool_t v) {
    // NOTE: skip the ll_add_ref, as we know our object is canonicalized
    return object_ref{
        .t = load_bool(),
        .v{ .b = v },
    };
}

yama::object_ref yama::context::ll_new_char(char_t v) {
    // NOTE: skip the ll_add_ref, as we know our object is canonicalized
    return object_ref{
        .t = load_char(),
        .v{ .c = v },
    };
}

std::optional<yama::object_ref> yama::context::ll_new_fn(type f) {
    // NOTE: skip the ll_add_ref, as we know our object is canonicalized
    object_ref result{
        .t = f,
        .v{ .i = 0 },
    };
    return
        f.kind() == kind::function
        ? std::make_optional(result)
        : std::nullopt;
}

size_t yama::context::ll_crashes() noexcept {
    return _crashes;
}

bool yama::context::ll_is_crashing() noexcept {
    return _crashing;
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

size_t yama::context::ll_max_locals() noexcept {
    YAMA_ASSERT(!_callstk.empty());
    return _callstk.back().max_locals();
}

size_t yama::context::ll_locals() noexcept {
    YAMA_ASSERT(!_callstk.empty());
    return _callstk.back().locals();
}

std::optional<yama::object_ref> yama::context::ll_local(local_t x) {
    return ll_clone_ref(_callstk.back().borrow_local(x));
}

void yama::context::ll_crash() {
    if (ll_is_crashing()) return; // don't count another crash if we're already crashing
    _crashes++;
    _crashing = true;
    _try_handle_crash_for_user_cf(); // in case crashing outside of a ll_call call
}

yama::cmd_status yama::context::ll_call(local_t x, std::initializer_list<local_t> args, local_t ret) {
    if (!_call_ret_to_local_precheck(ret)) return yama::cmd_status::init(false);
    return _call_ret_to_local(_call(_call_callobj_by_local(x), args), ret);
}

yama::cmd_status yama::context::ll_call(local_t x, std::initializer_list<local_t> args, newtop_t) {
    if (!_call_ret_to_newtop_precheck()) return yama::cmd_status::init(false);
    return _call_ret_to_newtop(_call(_call_callobj_by_local(x), args));
}

yama::cmd_status yama::context::ll_call(local_t x, std::initializer_list<local_t> args, forget_t) {
    return _call_ret_to_forget(_call(_call_callobj_by_local(x), args));
}

yama::cmd_status yama::context::ll_call(borrowed_ref x, std::initializer_list<local_t> args, local_t ret) {
    if (!_call_ret_to_local_precheck(ret)) return yama::cmd_status::init(false);
    return _call_ret_to_local(_call(_call_callobj_by_lvalue(x), args), ret);
}

yama::cmd_status yama::context::ll_call(borrowed_ref x, std::initializer_list<local_t> args, newtop_t) {
    if (!_call_ret_to_newtop_precheck()) return yama::cmd_status::init(false);
    return _call_ret_to_newtop(_call(_call_callobj_by_lvalue(x), args));
}

yama::cmd_status yama::context::ll_call(borrowed_ref x, std::initializer_list<local_t> args, forget_t) {
    return _call_ret_to_forget(_call(_call_callobj_by_lvalue(x), args));
}

yama::cmd_status yama::context::ll_call(type f, std::initializer_list<local_t> args, local_t ret) {
    if (!_call_ret_to_local_precheck(ret)) return yama::cmd_status::init(false);
    return _call_ret_to_local(_call(_call_callobj_by_fntype(f), args), ret);
}

yama::cmd_status yama::context::ll_call(type f, std::initializer_list<local_t> args, newtop_t) {
    if (!_call_ret_to_newtop_precheck()) return yama::cmd_status::init(false);
    return _call_ret_to_newtop(_call(_call_callobj_by_fntype(f), args));
}

yama::cmd_status yama::context::ll_call(type f, std::initializer_list<local_t> args, forget_t) {
    return _call_ret_to_forget(_call(_call_callobj_by_fntype(f), args));
}

yama::cmd_status yama::context::ll_put(local_t src, local_t dest) {
    return _put_output_to_local(_put_input_by_local(src), dest);
}

yama::cmd_status yama::context::ll_put(local_t src, newtop_t) {
    return _put_output_to_newtop(_put_input_by_local(src));
}

yama::cmd_status yama::context::ll_put(local_t src, forget_t) {
    return _put_output_to_forget(_put_input_by_local(src));
}

yama::cmd_status yama::context::ll_put(borrowed_ref src, local_t dest) {
    return _put_output_to_local(_put_input_by_lvalue(src), dest);
}

yama::cmd_status yama::context::ll_put(borrowed_ref src, newtop_t) {
    return _put_output_to_newtop(_put_input_by_lvalue(src));
}

yama::cmd_status yama::context::ll_put(borrowed_ref src, forget_t) {
    return _put_output_to_forget(_put_input_by_lvalue(src));
}

yama::cmd_status yama::context::ll_pop(size_t n) {
    for (; n > 0; n--) {
        if (!_top_cf().pop()) break; // stop popping if no more locals
    }
    return cmd_status::init(true);
}

yama::internal::call_frame<decltype(yama::context::_al)>& yama::context::_top_cf() {
    YAMA_ASSERT(!_callstk.empty());
    return _callstk.back();
}

yama::internal::call_frame<decltype(yama::context::_al)>& yama::context::_below_top_cf() {
    YAMA_ASSERT(_callstk.size() >= 2);
    return _callstk[_callstk.size() - 2];
}

bool yama::context::_push_cf(size_t max_locals) {
    if (ll_call_frames() == ll_max_call_frames()) return false;
    _callstk.push_back(internal::call_frame<decltype(_al)>(_al, *this, max_locals));
    return true;
}

void yama::context::_pop_cf() noexcept {
    if (_callstk.empty()) return;
    _callstk.pop_back();
}

void yama::context::_push_user_cf() {
    YAMA_ASSERT(_callstk.empty());
    const bool success = _push_cf(_config.user_max_locals);
    YAMA_ASSERT(success);
}

void yama::context::_try_handle_crash_for_user_cf() {
    if (!ll_is_crashing()) return;
    if (!ll_is_user()) return;
    _pop_cf();
    YAMA_LOG(dbg(), general_c, "reinitializing...");
    _crashing = false;
    _push_user_cf();
}

std::optional<yama::object_ref> yama::context::_call(std::optional<stolen_ref> callobj, std::initializer_list<local_t> args) {
    if (!callobj) return std::nullopt; // fail if _call_callobj_by_# method failed
    YAMA_ASSERT(callobj->t.kind() == kind::function); // the _call_callobj_by_# method must ensure this
    if (callobj->t.callsig()->params() != args.size()) { // arg count mismatch
        YAMA_LOG(
            dbg(), ctx_crash_c, 
            "error: crash from call to {} with {} args, but expects {}!", 
            *callobj, args.size(), callobj->t.callsig()->params());
        ll_crash();
        return std::nullopt;
    }
    // push our new call frame
    if (!_push_cf(callobj->t.max_locals())) { // fail means call stack overflow
        YAMA_LOG(
            dbg(), ctx_crash_c,
            "error: crash from call to {} due to call stack overflow!",
            *callobj);
        ll_crash();
        return std::nullopt;
    }
    // push callobj to new call frame's stack
    const bool okay0 = _top_cf().push_move(*callobj); // move ownership of callobj to the new call frame's stack
    YAMA_ASSERT(okay0); // static verif guarantees this
    // push arguments from our (now below the top) call frame's stack
    // to the new call frame's stack
    size_t arg_index = 0;
    for (const auto& I : args) {
        const auto arg = _below_top_cf().borrow_local(I);
        if (!arg) { // I is out-of-bounds
            YAMA_LOG(
                dbg(), ctx_crash_c,
                "error: crash from call to {} with arg {} using out-of-bounds local object stack index {}!",
                *callobj, arg_index, I);
            ll_crash();
            _pop_cf(); // cleanup before abort
            _try_handle_crash_for_user_cf(); // handle reset of user cf if needed
            return std::nullopt;
        }
        const bool okay1 = _top_cf().push(*arg); // clone x to the new call frame's stack
        YAMA_ASSERT(okay1); // static verif guarantees this
        arg_index++;
    }
    // fire call behaviour
    YAMA_ASSERT(callobj->t.call_fn());
    (*callobj->t.call_fn())(*this, callobj->t.links());
    // propagate crash if call behaviour crashed
    if (ll_is_crashing()) {
        _pop_cf(); // cleanup before abort
        _try_handle_crash_for_user_cf(); // handle reset of user cf if needed
        return std::nullopt;
    }
    // acquire top object of call's local object stack to be our return value
    const auto ret = _top_cf().pop_move();
    if (!ret) { // no return value object provided
        YAMA_LOG(
            dbg(), ctx_crash_c,
            "error: crash from call to {} due to no return value object!",
            *callobj);
        ll_crash();
        _pop_cf(); // cleanup before abort
        _try_handle_crash_for_user_cf(); // handle reset of user cf if needed
        return std::nullopt;
    }
    // cleanup call frame of call and return
    _pop_cf(); // pop call frame
    return ret; // return ret to _call_ret_to_# method
}

std::optional<yama::object_ref> yama::context::_call_callobj_by_local(local_t x) {
    const auto callobj = ll_local(x);
    if (!callobj) { // x is out-of-bounds
        YAMA_LOG(
            dbg(), ctx_crash_c,
            "error: crash from call to call object at out-of-bounds local object stack index {}!",
            x);
        ll_crash();
        return std::nullopt;
    }
    if (callobj->t.kind() != kind::function) { // callobj is not callable
        YAMA_LOG(
            dbg(), ctx_crash_c,
            "error: crash from call to non-callable call object {}!",
            *callobj);
        ll_crash();
        return std::nullopt;
    }
    return callobj;
}

std::optional<yama::object_ref> yama::context::_call_callobj_by_lvalue(borrowed_ref x) {
    if (x.t.kind() != kind::function) { // x is not callable
        YAMA_LOG(
            dbg(), ctx_crash_c,
            "error: crash from call to non-callable call object {}!",
            x);
        ll_crash();
        return std::nullopt;
    }
    return ll_clone_ref(x);
}

std::optional<yama::object_ref> yama::context::_call_callobj_by_fntype(type f) {
    const auto callobj = ll_new_fn(f);
    if (!callobj) { // f is not callable
        YAMA_LOG(
            dbg(), ctx_crash_c,
            "error: crash from call to non-callable type {}!",
            f);
        ll_crash();
        return std::nullopt;
    }
    return *callobj;
}

yama::cmd_status yama::context::_call_ret_to_local(std::optional<stolen_ref> ret, local_t target) {
    if (!ret) return cmd_status::init(false); // fail quietly if upstream failed
    const bool success = _top_cf().put_move(target, *ret); // move ownership of ret to the stack
    YAMA_ASSERT(success); // we prechecked, so this shouldn't be able to fail
    return cmd_status::init(true);
}

yama::cmd_status yama::context::_call_ret_to_newtop(std::optional<stolen_ref> ret) {
    if (!ret) return cmd_status::init(false); // fail quietly if upstream failed
    const bool success = _top_cf().push_move(*ret); // move ownership of ret to the stack
    YAMA_ASSERT(success); // we prechecked, so this shouldn't be able to fail
    return cmd_status::init(true);
}

yama::cmd_status yama::context::_call_ret_to_forget(std::optional<stolen_ref> ret) {
    if (!ret) return cmd_status::init(false); // fail quietly if upstream failed
    ll_drop_ref(*ret); // just drop *ret and we're done
    // TODO: when we add dtors, what happens if ll_drop_ref causes dtor which crashes?
    //       should we return bad cmd_status?
    return cmd_status::init(true);
}

bool yama::context::_call_ret_to_local_precheck(local_t target) {
    if (!_top_cf().in_bounds(target)) { // out-of-bounds
        YAMA_LOG(
            dbg(), ctx_crash_c,
            "error: crash from attempt to call with out-of-bounds return value object local object stack index {}!",
            target);
        ll_crash();
        return false;
    }
    return true;
}

bool yama::context::_call_ret_to_newtop_precheck() {
    if (_top_cf().locals() == _top_cf().max_locals()) { // will overflow
        YAMA_LOG(
            dbg(), ctx_crash_c,
            "error: crash from attempt to call but local object stack would overflow from trying to push its return value object!");
        ll_crash();
        return false;
    }
    return true;
}

std::optional<yama::object_ref> yama::context::_put_input_by_local(local_t src) {
    const auto x = ll_local(src);
    if (!x) { // src is out-of-bounds
        YAMA_LOG(
            dbg(), ctx_crash_c,
            "error: crash from attempt to put object from out-of-bounds local object stack index {}!",
            src);
        ll_crash();
        return std::nullopt;
    }
    return x;
}

std::optional<yama::object_ref> yama::context::_put_input_by_lvalue(borrowed_ref src) {
    return ll_clone_ref(src);
}

yama::cmd_status yama::context::_put_output_to_local(std::optional<stolen_ref> x, local_t dest) {
    if (!x) return cmd_status::init(false); // fail quietly if upstream failed
    const bool success = _top_cf().put_move(dest, *x); // move ownership of x to the stack
    if (!success) { // out-of-bounds!
        YAMA_LOG(
            dbg(), ctx_crash_c,
            "error: crash from attempt to put object {} to out-of-bounds local object stack index {}!",
            *x, dest);
        ll_crash();
        ll_drop_ref(*x); // can't forget
        return cmd_status::init(false);
    }
    return cmd_status::init(true);
}

yama::cmd_status yama::context::_put_output_to_newtop(std::optional<stolen_ref> x) {
    if (!x) return cmd_status::init(false); // fail quietly if upstream failed
    const bool success = _top_cf().push_move(*x); // move ownership of x to the stack
    if (!success) { // overflow!
        YAMA_LOG(
            dbg(), ctx_crash_c,
            "error: crash due to local object stack overflow from attempting to push {}!",
            *x);
        ll_crash();
        ll_drop_ref(*x); // can't forget
        return cmd_status::init(false);
    }
    return cmd_status::init(true);
}

yama::cmd_status yama::context::_put_output_to_forget(std::optional<stolen_ref> x) {
    if (!x) return cmd_status::init(false); // fail quietly if upstream failed
    ll_drop_ref(*x); // just drop *x and we're done
    // TODO: when we add dtors, what happens if ll_drop_ref causes dtor which crashes?
    //       should we return bad cmd_status?
    return cmd_status::init(true);
}
