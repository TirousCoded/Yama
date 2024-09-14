

#include "bcode.h"

#include "asserts.h"


std::string yama::bc::fmt_opcode(opcode x) {
    std::string result{};
    static_assert(opcodes == 11);
    switch (x) {
    case opcode::noop:          result = "noop";        break;
    case opcode::load_none:     result = "load_none";   break;
    case opcode::load_const:    result = "load_const";  break;
    case opcode::load_arg:      result = "load_arg";    break;
    case opcode::copy:          result = "copy";        break;
    case opcode::call:          result = "call";        break;
    case opcode::call_nr:       result = "call_nr";     break;
    case opcode::ret:           result = "ret";         break;
    case opcode::jump:          result = "jump";        break;
    case opcode::jump_true:     result = "jump_true";   break;
    case opcode::jump_false:    result = "jump_false";  break;
    default:                    YAMA_DEADEND;           break;
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
    YAMA_ASSERT(tab);
    YAMA_ASSERT(x < count());
    const auto& instr = get(x);
    std::string a{};
    static_assert(opcodes == 11);
    switch (instr.opc) {
    case opcode::noop:          a = std::format("{: <12}", ' ');                                    break;
    case opcode::load_none:     a = std::format("{: <12}", instr.A);                                break;
    case opcode::load_const:    a = std::format("{: <4}{: <8}", instr.A, instr.B);                  break;
    case opcode::load_arg:      a = std::format("{: <4}{: <8}", instr.A, instr.B);                  break;
    case opcode::copy:          a = std::format("{: <4}{: <8}", instr.A, instr.B);                  break;
    case opcode::call:          a = std::format("{: <4}{: <4}{: <4}", instr.A, instr.B, instr.C);   break;
    case opcode::call_nr:       a = std::format("{: <4}{: <8}", instr.A, instr.B);                  break;
    case opcode::ret:           a = std::format("{: <12}", instr.A);                                break;
    case opcode::jump:          a = std::format("{: <12}", instr.sBx);                              break;
    case opcode::jump_true:     a = std::format("{: <4}{: <8}", instr.A, instr.sBx);                break;
    case opcode::jump_false:    a = std::format("{: <4}{: <8}", instr.A, instr.sBx);                break;
    default:                    YAMA_DEADEND;                                                       break;
    }
    std::string result = std::format("{}{: <5}{: <16}{}", tab, x, get(x).opc, a);
    if (reinit_flag(x)) {
        result += "(reinit)";
    }
    static_assert(opcodes == 11);
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

std::string yama::bc::code::fmt_disassembly(const char* tab) const {
    YAMA_ASSERT(tab);
    std::string result{};
    result += std::format("disassembly ({} instrs)", count());
    for (size_t i = 0; i < count(); i++) {
        result += std::format("\n{}", fmt_instr(i, tab));
    }
    return result;
}

yama::bc::code& yama::bc::code::add_noop() {
    _instrs.push_back(instr{ .opc = opcode::noop });
    _reinit_flags.push_back(false);
    return *this;
}

yama::bc::code& yama::bc::code::add_load_none(uint8_t A, bool reinit) {
    _instrs.push_back(instr{ .opc = opcode::load_none, .A = A });
    _reinit_flags.push_back(reinit);
    return *this;
}

yama::bc::code& yama::bc::code::add_load_const(uint8_t A, uint8_t B, bool reinit) {
    _instrs.push_back(instr{ .opc = opcode::load_const, .A = A });
    _instrs.back().B = B;
    _reinit_flags.push_back(reinit);
    return *this;
}

yama::bc::code& yama::bc::code::add_load_arg(uint8_t A, uint8_t B, bool reinit) {
    _instrs.push_back(instr{ .opc = opcode::load_arg, .A = A });
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

yama::bc::code& yama::bc::code::add_call(uint8_t A, uint8_t B, uint8_t C, bool reinit) {
    _instrs.push_back(instr{ .opc = opcode::call, .A = A });
    _instrs.back().B = B;
    _instrs.back().C = C;
    _reinit_flags.push_back(reinit);
    return *this;
}

yama::bc::code& yama::bc::code::add_call_nr(uint8_t A, uint8_t B) {
    _instrs.push_back(instr{ .opc = opcode::call_nr, .A = A });
    _instrs.back().B = B;
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

