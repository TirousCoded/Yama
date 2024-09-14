

#include "bcode_interp.h"


yama::context& yama::internal::bcode_interp::ctx() {
    return deref_assert(ctx_ptr);
}

const yama::bc::code& yama::internal::bcode_interp::bcode() {
    return deref_assert(bcode_ptr);
}

void yama::internal::bcode_interp::halt() {
    should_halt = true;
    YAMA_LOG(ctx().dbg(), bcode_exec_c, ">> halt");
}

void yama::internal::bcode_interp::jump(int16_t offset) {
    pc += std::make_signed_t<size_t>(offset);
    YAMA_LOG(ctx().dbg(), bcode_exec_c, ">> jump (to {})", pc);
}

void yama::internal::bcode_interp::fire() {
    setup();
    while (!should_halt) {
        fetch_instr();
        exec_instr();
        halt_if_crashing();
    }
}

void yama::internal::bcode_interp::setup() {
    acquire_bcode();
    pc = 0;
    should_halt = false;
    YAMA_LOG(ctx().dbg(), bcode_exec_c, ">> bcode exec");
}

void yama::internal::bcode_interp::acquire_bcode() {
    const auto& callobj = deref_assert(ctx().ll_arg(0));
    bcode_ptr = internal::get_type_mem(callobj.t)->info->bcode();
    YAMA_ASSERT(bcode_ptr);
}

void yama::internal::bcode_interp::fetch_instr() {
    // fetch next instr
    curr = bcode()[pc];
    // incr pc after instr fetch
    // this is inline w/ expectations of instrs like jump
    pc++;
}

void yama::internal::bcode_interp::exec_instr() {
    YAMA_LOG(ctx().dbg(), bcode_exec_c, "{}", bcode().fmt_instr(pc - 1));
    static_assert(bc::opcodes == 11);
    switch (curr.opc) {
    case bc::opcode::noop:
    {
        // do nothing
    }
    break;
    case bc::opcode::load_none:
    {
        if (ctx().ll_load_none(curr.A).bad()) return;
    }
    break;
    case bc::opcode::load_const:
    {
        if (ctx().ll_load_const(curr.A, curr.B).bad()) return;
    }
    break;
    case bc::opcode::load_arg:
    {
        if (ctx().ll_load_arg(curr.A, curr.B).bad()) return;
    }
    break;
    case bc::opcode::copy:
    {
        if (ctx().ll_copy(curr.A, curr.B).bad()) return;
    }
    break;
    case bc::opcode::call:
    {
        if (ctx().ll_call(curr.A, curr.B, curr.C).bad()) return;
    }
    break;
    case bc::opcode::call_nr:
    {
        if (ctx().ll_call(curr.A, curr.B, no_result).bad()) return;
    }
    break;
    case bc::opcode::ret:
    {
        if (ctx().ll_ret(curr.A).bad()) return;
        halt();
    }
    break;
    case bc::opcode::jump:
    {
        jump(curr.sBx);
    }
    break;
    case bc::opcode::jump_true:
    {
        const auto& input = deref_assert(ctx().ll_local(curr.A));
        const bool should_jump = input.as_bool();
        if (should_jump) {
            jump(curr.sBx);
        }
    }
    break;
    case bc::opcode::jump_false:
    {
        const auto& input = deref_assert(ctx().ll_local(curr.A));
        const bool should_jump = !input.as_bool();
        if (should_jump) {
            jump(curr.sBx);
        }
    }
    break;
    default: YAMA_DEADEND; break;
    }
}

void yama::internal::bcode_interp::halt_if_crashing() {
    if (ctx().ll_crashing()) {
        halt();
    }
}

