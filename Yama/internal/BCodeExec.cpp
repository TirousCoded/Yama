

#include "BCodeExec.h"

#include "general.h"
#include "YmCtx.h"
#include "YmObj.h"


#define _TRACE_BCEXEC false

#if _TRACE_BCEXEC
#include "../yama++/print.h"
#endif


_ym::BCodeExec::BCodeExec(YmCtx& ctx, const BCode& code, size_t start) :
	_ctx(ctx),
	_code(code),
	_pc(start) {
}

void _ym::BCodeExec::operator()() {
	// TODO: Later on, have it so our interpreter doesn't crash the whole
	//		 process upon out-of-bounds instr fetch.
	while (_exec(_fetch().value())) {};
}

void _ym::BCodeExec::_jump(std::make_signed_t<size_t> offset) noexcept {
	_pc += offset;
}

std::optional<_ym::BCodeInstr> _ym::BCodeExec::_fetch() {
	if (_pc >= _code->size()) {
		// TODO: Improve this error.
		Global::raiseErr(
			YmErrCode_InternalError,
			"Out-of-bounds fetch of instr {}!\n{}",
			_pc,
			_ctx->fmtCallStk());
		return std::nullopt;
	}
#if _TRACE_BCEXEC
	ym::println("{}", _code->fmtInstr(_pc));
#endif
	return _code.value()[_pc++];
}

bool _ym::BCodeExec::_exec(BCodeInstr instr) {
	bool shouldNotHalt = true;
	auto halt = [&]() { shouldNotHalt = false; };
	auto Lw = [&](YmUInt8 x) -> YmLocal {
		if (x == bPush)			return YM_PUSH;
		else if (x == bDiscard) return YM_DISCARD;
		else					return (YmLocal)x;
		};
	static_assert(Opcodes == 17);
	switch (instr.opc) {
	case Opcode::noop:
	{
		// Do nothing.
	}
	break;
	case Opcode::pop:
	{
		_ctx->pop(instr.A);
	}
	break;
	case Opcode::putNone:
	{
		_ctx->put(Lw(instr.A), _ctx->newNone(false));
	}
	break;
	case Opcode::putConst:
	{
		auto c = _ctx->fromConst(instr.A);
		ymAssert(c.has_value());
		_ctx->put(Lw(instr.B), std::move(c.value()));
	}
	break;
	case Opcode::putArg:
	{
		_ctx->put(Lw(instr.B), _ctx->arg(instr.A));
	}
	break;
	case Opcode::copy:
	{
		_ctx->copy(instr.A, Lw(instr.B));
	}
	break;
	case Opcode::defaultInit:
	{
		_ctx->defaultInit(_ctx->fn()->constAsRef(instr.A), Lw(instr.B));
	}
	break;
	case Opcode::pcall:
	{
		auto called = _ctx->fn()->constAsRef(instr.A);
		_ctx->call(called, called->positionalParams(), "", Lw(instr.B));
	}
	break;
	case Opcode::ret:
	{
		_ctx->retObj(_ctx->pull(false));
		halt();
	}
	break;
	case Opcode::getVar:
	{
		_ctx->getVar(_ctx->fn()->constAsRef(instr.A), Lw(instr.B));
	}
	break;
	case Opcode::setVar:
	{
		_ctx->setVar(_ctx->fn()->constAsRef(instr.A));
	}
	break;
	case Opcode::getProp:
	{
		_ctx->getProperty(_ctx->fn()->constAsRef(instr.A), Lw(instr.B));
	}
	break;
	case Opcode::setProp:
	{
		_ctx->setProperty(_ctx->fn()->constAsRef(instr.A));
	}
	break;
	case Opcode::conv:
	{
		_ctx->convert(*_ctx->fn()->constAsRef(instr.A), Lw(instr.B), false);
	}
	break;
	case Opcode::jump:
	{
		_jump(instr.sBx);
	}
	break;
	case Opcode::jumpTrue:
	{
		if (auto cond = _ctx->pull(false); cond) {
			ymAssert(cond->type->sameAs(_ctx->ldBool()));
			if (cond->toBool().value()) {
				_jump(instr.sBx);
			}
		}
		else YM_DEADEND;
	}
	break;
	case Opcode::jumpFalse:
	{
		if (auto cond = _ctx->pull(false); cond) {
			ymAssert(cond->type->sameAs(_ctx->ldBool()));
			if (!cond->toBool().value()) {
				_jump(instr.sBx);
			}
		}
		else YM_DEADEND;
	}
	break;
	default: YM_DEADEND; break;
	}
	return shouldNotHalt;
}

