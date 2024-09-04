

#include "bcode.h"

#include "asserts.h"


std::string yama::bc::fmt_opcode(opcode x) {
    std::string result{};
    static_assert(opcodes == 10);
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
    case opcode::jump_if:       result = "jump_if";     break;
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

std::string yama::bc::code::fmt_disassembly(const char* tab) const {
    YAMA_ASSERT(tab);
    auto _fmt_oprands = 
        [&](const instr& x) -> std::string {
        std::string result{};
        static_assert(opcodes == 10);
        switch (x.opc) {
        case opcode::noop:          result = std::format("{: <12}", ' ');                       break;
        case opcode::load_none:     result = std::format("{: <12}", x.A);                       break;
        case opcode::load_const:    result = std::format("{: <4}{: <8}", x.A, x.B);             break;
        case opcode::load_arg:      result = std::format("{: <4}{: <8}", x.A, x.B);             break;
        case opcode::copy:          result = std::format("{: <4}{: <8}", x.A, x.B);             break;
        case opcode::call:          result = std::format("{: <4}{: <4}{: <4}", x.A, x.B, x.C);  break;
        case opcode::call_nr:       result = std::format("{: <4}{: <8}", x.A, x.B);             break;
        case opcode::ret:           result = std::format("{: <12}", x.A);                       break;
        case opcode::jump:          result = std::format("{: <12}", x.sBx);                     break;
        case opcode::jump_if:       result = std::format("{: <4}{: <8}", x.A, x.sBx);           break;
        default:                    YAMA_DEADEND;                                               break;
        }
        return result;
        };
    std::string result{};
    result += std::format("disassembly ({} instrs)", count());
    for (size_t i = 0; i < count(); i++) {
        result += std::format("\n{}{: <5}{: <16}{}", tab, i, get(i).opc, _fmt_oprands(get(i)));
        if (reinit_flag(i)) {
            result += "(reinit)";
        }
        static_assert(opcodes == 10);
        if (get(i).opc == opcode::jump || get(i).opc == opcode::jump_if) {
            // TODO: maybe add syntax for when sBx offset puts PC out-of-bounds
            // NOTE: take note that we MUST have the '+ 1' bit below for target, since the PC is incr prior to jumping
            size_t target = i + 1 + std::make_signed_t<size_t>(get(i).sBx);
            result += std::format("(branches to instr {})", target);
        }
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

yama::bc::code& yama::bc::code::add_jump_if(uint8_t A, int16_t sBx) {
    _instrs.push_back(instr{ .opc = opcode::jump_if, .A = A, .sBx = sBx });
    _reinit_flags.push_back(false);
    return *this;
}

