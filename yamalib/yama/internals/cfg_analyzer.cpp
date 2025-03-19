

#include "cfg_analyzer.h"

#include "../core/asserts.h"


bool yama::internal::cfg_analyzer::is_in_fn() const noexcept {
    return !_stk.empty();
}

bool yama::internal::cfg_analyzer::is_in_loop() const noexcept {
    return _loop_scopes >= 1;
}

bool yama::internal::cfg_analyzer::is_in_dead_code() const noexcept {
    return current().is_block() && current().block_exits >= 1;
}

yama::internal::cfg_analyzer::path& yama::internal::cfg_analyzer::current() noexcept {
    _assert_top();
    return _stk.back();
}

const yama::internal::cfg_analyzer::path& yama::internal::cfg_analyzer::current() const noexcept {
    _assert_top();
    return _stk.back();
}

bool yama::internal::cfg_analyzer::check_fn() const noexcept {
    _assert_top(path_type::fn);
    return current().fn_exits >= 1;
}

void yama::internal::cfg_analyzer::begin_fn() {
    _push(path_type::fn); // path for this fn itself
}

void yama::internal::cfg_analyzer::end_fn() {
    (void)_pop();
}

void yama::internal::cfg_analyzer::break_stmt() {
    _assert_top();
    if (is_in_dead_code()) return; // skip dead code
    if (!is_in_loop()) return; // ignore illegal break stmts
    current().block_exits++;
    current().breaks++;
}

void yama::internal::cfg_analyzer::continue_stmt() {
    _assert_top();
    if (is_in_dead_code()) return; // skip dead code
    if (!is_in_loop()) return; // ignore illegal continue stmts
    current().block_exits++;
    current().fn_exits++;
}

void yama::internal::cfg_analyzer::return_stmt() {
    _assert_top();
    if (is_in_dead_code()) return; // skip dead code
    current().block_exits++;
    current().fn_exits++;
}

void yama::internal::cfg_analyzer::begin_block() {
    _assert_top();
    _push(path_type::block);
}

void yama::internal::cfg_analyzer::end_block() {
    _assert_top(path_type::block);
    const auto top = _pop();
    if (is_in_dead_code()) return; // skip dead code
    if (top.block_exits >= 1)   current().block_exits++;
    if (top.fn_exits >= 1)      current().fn_exits++;
    if (top.breaks >= 1)        current().breaks++;
}

void yama::internal::cfg_analyzer::begin_if_stmt() {
    _assert_top();
    _push(path_type::if_stmt);
}

void yama::internal::cfg_analyzer::end_if_stmt() {
    _assert_top(path_type::if_stmt);
    const auto top = _pop();
    if (is_in_dead_code()) return; // skip dead code
    if (top.block_exits >= 2)   current().block_exits++;    // propagate only if both if/else parts have
    if (top.fn_exits >= 2)      current().fn_exits++;       // propagate only if both if/else parts have
    if (top.breaks >= 1)        current().breaks++;         // propagate if either part has
}

void yama::internal::cfg_analyzer::begin_loop_stmt() {
    _assert_top();
    _push(path_type::loop_stmt);
}

void yama::internal::cfg_analyzer::end_loop_stmt() {
    _assert_top(path_type::loop_stmt);
    const auto top = _pop();
    if (is_in_dead_code()) return; // skip dead code
    if (top.breaks == 0) {          // implicit continue-like behaviour if no subpaths exit loop stmt
        current().block_exits++;    // w/out breaks, loop stmt guarantees code after loop is dead code
        current().fn_exits++;       // w/out breaks, loop stmt guarantees its code will return from fn, or enter infinite loop
    }
}

void yama::internal::cfg_analyzer::_assert_top() const noexcept {
    YAMA_ASSERT(is_in_fn()); // assert stk not empty
}

void yama::internal::cfg_analyzer::_assert_top(path_type t) const noexcept {
    _assert_top();
    YAMA_ASSERT(_stk.back().is(t));
}

void yama::internal::cfg_analyzer::_push(path_type t) {
    _stk.push_back(path{ .type = t });
    if (current().is_loop_stmt()) _push_loop();
}

yama::internal::cfg_analyzer::path yama::internal::cfg_analyzer::_pop() {
    _assert_top();
    if (current().is_loop_stmt()) _pop_loop();
    const path result = _stk.back();
    _stk.pop_back();
    return result;
}

void yama::internal::cfg_analyzer::_push_loop() {
    _loop_scopes++;
}

void yama::internal::cfg_analyzer::_pop_loop() {
    YAMA_ASSERT(is_in_loop());
    _loop_scopes--;
}

