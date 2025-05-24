

#include "bcode.h"

#include "asserts.h"


std::string yama::bc::fmt_opcode(opcode x) {
    std::string result{};
    static_assert(opcodes == 13);
    switch (x) {
    case opcode::noop:              result = "noop";            break;
    case opcode::pop:               result = "pop";             break;
    case opcode::put_none:          result = "put_none";        break;
    case opcode::put_const:         result = "put_const";       break;
    case opcode::put_type_const:    result = "put_type_const";  break;
    case opcode::put_arg:           result = "put_arg";         break;
    case opcode::copy:              result = "copy";            break;
    case opcode::call:              result = "call";            break;
    case opcode::call_nr:           result = "call_nr";         break;
    case opcode::ret:               result = "ret";             break;
    case opcode::jump:              result = "jump";            break;
    case opcode::jump_true:         result = "jump_true";       break;
    case opcode::jump_false:        result = "jump_false";      break;
    default:                        YAMA_DEADEND;               break;
    }
    return result;
}

size_t yama::bc::code::count() const noexcept {
    return _instrs.size();
}

yama::bc::instr yama::bc::code::get(size_t x) const noexcept {
    YAMA_ASSERT(x < count());
    return _instrs[x];
}

yama::bc::instr yama::bc::code::operator[](size_t x) const noexcept {
    return get(x);
}

bool yama::bc::code::reinit_flag(size_t x) const noexcept {
    YAMA_ASSERT(x < count());
    return _reinit_flags[x];
}

std::string yama::bc::code::fmt_instr(size_t x, const char* tab) const {
    return _fmt_instr(x, nullptr, tab);
}

std::string yama::bc::code::fmt_instr(size_t x, const syms& syms, const char* tab) const {
    return _fmt_instr(x, &syms, tab);
}

std::string yama::bc::code::fmt_disassembly(const char* tab) const {
    YAMA_ASSERT(tab);
    std::string result{};
    result += std::format("disassembly ({} instrs)", count());
    for (size_t i = 0; i < count(); i++) {
        result += std::format("\n{}", fmt_instr(i, tab));
    }
    return result;
}

std::string yama::bc::code::fmt_disassembly(const syms& syms, const char* tab) const {
    YAMA_ASSERT(tab);
    std::string result{};
    result += std::format("disassembly ({} instrs)", count());
    for (size_t i = 0; i < count(); i++) {
        result += std::format("\n{}", fmt_instr(i, syms, tab));
    }
    return result;
}

yama::bc::code& yama::bc::code::add_noop() {
    _instrs.push_back(instr{ .opc = opcode::noop });
    _reinit_flags.push_back(false);
    return *this;
}

yama::bc::code& yama::bc::code::add_pop(uint8_t A) {
    _instrs.push_back(instr{ .opc = opcode::pop, .A = A });
    _reinit_flags.push_back(false);
    return *this;
}

yama::bc::code& yama::bc::code::add_put_none(uint8_t A, bool reinit) {
    _instrs.push_back(instr{ .opc = opcode::put_none, .A = A });
    _reinit_flags.push_back(reinit);
    return *this;
}

yama::bc::code& yama::bc::code::add_put_const(uint8_t A, uint8_t B, bool reinit) {
    _instrs.push_back(instr{ .opc = opcode::put_const, .A = A });
    _instrs.back().B = B;
    _reinit_flags.push_back(reinit);
    return *this;
}

yama::bc::code& yama::bc::code::add_put_type_const(uint8_t A, uint8_t B, bool reinit) {
    _instrs.push_back(instr{ .opc = opcode::put_type_const, .A = A });
    _instrs.back().B = B;
    _reinit_flags.push_back(reinit);
    return *this;
}

yama::bc::code& yama::bc::code::add_put_arg(uint8_t A, uint8_t B, bool reinit) {
    _instrs.push_back(instr{ .opc = opcode::put_arg, .A = A });
    _instrs.back().B = B;
    _reinit_flags.push_back(reinit);
    return *this;
}

yama::bc::code& yama::bc::code::add_copy(uint8_t A, uint8_t B, bool reinit) {
    _instrs.push_back(instr{ .opc = opcode::copy, .A = A });
    _instrs.back().B = B;
    _reinit_flags.push_back(reinit);
    return *this;
}

yama::bc::code& yama::bc::code::add_call(uint8_t A, uint8_t B, bool reinit) {
    _instrs.push_back(instr{ .opc = opcode::call, .A = A });
    _instrs.back().B = B;
    _reinit_flags.push_back(reinit);
    return *this;
}

yama::bc::code& yama::bc::code::add_call_nr(uint8_t A) {
    _instrs.push_back(instr{ .opc = opcode::call_nr, .A = A });
    _reinit_flags.push_back(false);
    return *this;
}

yama::bc::code& yama::bc::code::add_ret(uint8_t A) {
    _instrs.push_back(instr{ .opc = opcode::ret, .A = A });
    _reinit_flags.push_back(false);
    return *this;
}

yama::bc::code& yama::bc::code::add_jump(int16_t sBx) {
    _instrs.push_back(instr{ .opc = opcode::jump, .sBx = sBx });
    _reinit_flags.push_back(false);
    return *this;
}

yama::bc::code& yama::bc::code::add_jump_true(uint8_t A, int16_t sBx) {
    _instrs.push_back(instr{ .opc = opcode::jump_true, .A = A, .sBx = sBx });
    _reinit_flags.push_back(false);
    return *this;
}

yama::bc::code& yama::bc::code::add_jump_false(uint8_t A, int16_t sBx) {
    _instrs.push_back(instr{ .opc = opcode::jump_false, .A = A, .sBx = sBx });
    _reinit_flags.push_back(false);
    return *this;
}

std::string yama::bc::code::_fmt_instr(size_t x, const syms* syms, const char* tab) const {
    YAMA_ASSERT(tab);
    YAMA_ASSERT(x < count());
    const auto& instr = get(x);
    auto maybe_nt =
        [](uint8_t x) -> std::string {
        return
            x == uint8_t(newtop)
            ? "NT"
            : std::format("{}", x);
        };
    std::string a{};
    static_assert(opcodes == 13);
    switch (instr.opc) {
    case opcode::noop:              a = std::format("{: <12}", ' ');                                break;
    case opcode::pop:               a = std::format("{: <12}", instr.A);                            break;
    case opcode::put_none:          a = std::format("{: <12}", maybe_nt(instr.A));                  break;
    case opcode::put_const:         a = std::format("{: <4}{: <8}", maybe_nt(instr.A), instr.B);    break;
    case opcode::put_type_const:    a = std::format("{: <4}{: <8}", maybe_nt(instr.A), instr.B);    break;
    case opcode::put_arg:           a = std::format("{: <4}{: <8}", maybe_nt(instr.A), instr.B);    break;
    case opcode::copy:              a = std::format("{: <4}{: <8}", instr.A, maybe_nt(instr.B));    break;
    case opcode::call:              a = std::format("{: <4}{: <8}", instr.A, maybe_nt(instr.B));    break;
    case opcode::call_nr:           a = std::format("{: <12}", instr.A);                            break;
    case opcode::ret:               a = std::format("{: <12}", instr.A);                            break;
    case opcode::jump:              a = std::format("{: <12}", instr.sBx);                          break;
    case opcode::jump_true:         a = std::format("{: <4}{: <8}", instr.A, instr.sBx);            break;
    case opcode::jump_false:        a = std::format("{: <4}{: <8}", instr.A, instr.sBx);            break;
    default:                        YAMA_DEADEND;                                                   break;
    }
    std::string result = std::format("{}{: <5}{: <16}{}", tab, x, get(x).opc, a);
    if (syms) {
        if (const auto sym = syms->fetch(x)) {
            result += std::format("{} ", *sym);
        }
    }
    if (reinit_flag(x)) {
        result += "(reinit)";
    }
    static_assert(opcodes == 13);
    if (
        get(x).opc == opcode::jump ||
        get(x).opc == opcode::jump_true ||
        get(x).opc == opcode::jump_false) {
        // TODO: maybe add syntax for when sBx offset puts PC out-of-bounds
        // NOTE: take note that we MUST have the '+ 1' bit below for target, since the PC is incr prior to jumping
        size_t target = x + 1 + std::make_signed_t<size_t>(get(x).sBx);
        result += std::format("(branches to instr {})", target);
    }
    return result;
}

std::string yama::bc::sym::fmt() const {
    return std::format("[ln {}, ch {}; \"{}\"]", ln, ch, origin);
}

std::optional<yama::bc::sym> yama::bc::syms::fetch(size_t index) const noexcept {
    const auto found = _syms.find(index);
    return
        found != _syms.end()
        ? std::make_optional(found->second)
        : std::nullopt;
}

std::optional<yama::bc::sym> yama::bc::syms::operator[](size_t index) const noexcept {
    return fetch(index);
}

yama::bc::syms& yama::bc::syms::add(size_t index, str origin, size_t ch, size_t ln) {
    _syms[index] = sym{ .index = index, .origin = origin, .ch = ch, .ln = ln };
    return *this;
}

yama::bc::syms& yama::bc::syms::add(sym x) {
    _syms[x.index] = std::move(x);
    return *this;
}

std::string yama::bc::syms::fmt_sym(size_t index) const {
    if (const auto sym = fetch(index)) {
        return sym->fmt();
    }
    else {
        return internal::fmt_no_sym(index);
    }
}

yama::bc::code_writer::code_writer()
    : _syms(nullptr) {}

yama::bc::code_writer::code_writer(syms& autosym_target)
    : _syms(&autosym_target) {}

size_t yama::bc::code_writer::count() const noexcept {
    return _result.count();
}

yama::bc::code_writer& yama::bc::code_writer::add_noop() {
    _result.add_noop();
    _autosym_output();
    return *this;
}

yama::bc::code_writer& yama::bc::code_writer::add_pop(uint8_t A) {
    _result.add_pop(A);
    _autosym_output();
    return *this;
}

yama::bc::code_writer& yama::bc::code_writer::add_put_none(uint8_t A, bool reinit) {
    _result.add_put_none(A, reinit);
    _autosym_output();
    return *this;
}

yama::bc::code_writer& yama::bc::code_writer::add_put_const(uint8_t A, uint8_t B, bool reinit) {
    _result.add_put_const(A, B, reinit);
    _autosym_output();
    return *this;
}

yama::bc::code_writer& yama::bc::code_writer::add_put_type_const(uint8_t A, uint8_t B, bool reinit) {
    _result.add_put_type_const(A, B, reinit);
    _autosym_output();
    return *this;
}

yama::bc::code_writer& yama::bc::code_writer::add_put_arg(uint8_t A, uint8_t B, bool reinit) {
    _result.add_put_arg(A, B, reinit);
    _autosym_output();
    return *this;
}

yama::bc::code_writer& yama::bc::code_writer::add_copy(uint8_t A, uint8_t B, bool reinit) {
    _result.add_copy(A, B, reinit);
    _autosym_output();
    return *this;
}

yama::bc::code_writer& yama::bc::code_writer::add_call(uint8_t A, uint8_t B, bool reinit) {
    _result.add_call(A, B, reinit);
    _autosym_output();
    return *this;
}

yama::bc::code_writer& yama::bc::code_writer::add_call_nr(uint8_t A) {
    _result.add_call_nr(A);
    _autosym_output();
    return *this;
}

yama::bc::code_writer& yama::bc::code_writer::add_ret(uint8_t A) {
    _result.add_ret(A);
    _autosym_output();
    return *this;
}

yama::bc::code_writer& yama::bc::code_writer::add_jump(label_id_t label_id) {
    _bind_label_id_user(label_id);
    _result.add_jump(0); // stub
    _autosym_output();
    return *this;
}

yama::bc::code_writer& yama::bc::code_writer::add_jump_true(uint8_t A, label_id_t label_id) {
    _bind_label_id_user(label_id);
    _result.add_jump_true(A, 0); // stub
    _autosym_output();
    return *this;
}

yama::bc::code_writer& yama::bc::code_writer::add_jump_false(uint8_t A, label_id_t label_id) {
    _bind_label_id_user(label_id);
    _result.add_jump_false(A, 0); // stub
    _autosym_output();
    return *this;
}

yama::bc::code_writer& yama::bc::code_writer::add_label(label_id_t label_id) {
    _bind_label_id(label_id);
    return *this;
}

std::optional<yama::bc::code> yama::bc::code_writer::done(bool* label_not_found) {
    std::optional<code> result{};
    if (_resolve(label_not_found)) {
        result = std::move(_result);
        _reset();
    }
    return result;
}

void yama::bc::code_writer::autosym(str origin, size_t ch, size_t ln) {
    autosym(sym{ .index = 0, .origin = origin, .ch = ch, .ln = ln });
}

void yama::bc::code_writer::autosym(const sym& x) {
    _autosym = x;
}

void yama::bc::code_writer::drop_autosym() noexcept {
    _autosym.reset();
}

size_t yama::bc::code_writer::_write_pos() const noexcept {
    return _result.count();
}

std::optional<size_t> yama::bc::code_writer::_get_instr_index(label_id_t label_id) {
    const auto it = _label_id_map.find(label_id);
    return
        it != _label_id_map.end()
        ? std::make_optional(it->second)
        : std::nullopt;
}

void yama::bc::code_writer::_bind_label_id(label_id_t label_id) {
    _label_id_map[label_id] = _write_pos(); // overwrites existing
}

void yama::bc::code_writer::_bind_label_id_user(label_id_t label_id) {
    _label_id_user_map[_write_pos()] = label_id;
}

void yama::bc::code_writer::_reset() {
    _result = code{}; // reinit
    _label_id_map.clear();
    _label_id_user_map.clear();
}

bool yama::bc::code_writer::_resolve(bool* label_not_found) {
    // iterate over label ID user map, resolving each one, and failing and
    // propagating correct error if issue is found
    for (const auto& I : _label_id_user_map) {
        const size_t instr_index = I.first; // the instr who's stub needs patching
        const label_id_t label_id = I.second; // label ID to use to patch stub
        if (const auto label_instr_index = _get_instr_index(label_id)) {
            // IMPORTANT: remember that branch instrs sBx offsets are relative to
            //            the instr index immediately after the instr, so we do the
            //            '+ 1' bit below
            size_t low = instr_index + 1; // initial value is jumped-from instr index
            size_t high = *label_instr_index; // initial value is jumped-to instr index
            const bool jump_backwards = low > high; // are we jumping forwards, or backwards?
            if (jump_backwards) {
                std::swap(low, high); // ensure diff below is non-negative
            }
            size_t diff = high - low; // jump offset (w/out accounting for forward/backwards)
            if (!jump_backwards) { // forward jump
                if (diff > size_t(std::numeric_limits<int16_t>::max())) { // sBx overflow error
                    if (label_not_found) {
                        *label_not_found = false;
                    }
                    return false;
                }
                // if branch is reasonable, then apply patch and move on
                _result._instrs[instr_index].sBx = int16_t(diff);
            }
            else { // backward jump
                // NOTE: below, size_t(std::numeric_limits<int16_t>::max()) + 1 gets us the minimum
                //       16-bit value, but expressed as a positive size_t value
                if (diff > size_t(std::numeric_limits<int16_t>::max()) + 1) { // sBx underflow error
                    if (label_not_found) {
                        *label_not_found = false;
                    }
                    return false;
                }
                // if branch is reasonable, then apply patch and move on
                _result._instrs[instr_index].sBx = int16_t(-ssize_t(diff)); // careful to avoid overflow/underflow
            }
        }
        else { // label not found error
            if (label_not_found) {
                *label_not_found = true;
            }
            return false;
        }
    }
    return true;
}

void yama::bc::code_writer::_autosym_output() {
    if (_syms && _autosym) {
        _autosym->index = count() - 1; // update index
        _syms->add(*_autosym); // output copy
    }
}

bool yama::bc::instr::operator==(const instr& other) const noexcept {
    // TODO: it's UB to access union components that are not formally *initialized*
    //       at the point of access... so I'm not sure exactly what to do about that...
    return
        opc == other.opc &&
        A == other.A &&
        B == other.B &&
        C == other.C;
}

std::string yama::internal::fmt_no_sym(size_t index) {
    return std::format("[instr {}]", index);
}

