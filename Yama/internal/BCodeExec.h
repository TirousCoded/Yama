

#pragma once


#include "bcode.h"

#include "../yama/yama.h"
#include "../yama++/Safe.h"


namespace _ym {


	// Each BCodeExec only handles one call stack frame.
	class BCodeExec final {
	public:
		// start specifies the instr to start from.
		BCodeExec(YmCtx& ctx, const BCode& code, size_t start = 0);


		void operator()();


	private:
		ym::Safe<YmCtx> _ctx;
		ym::Safe<const BCode> _code;
		size_t _pc = 0; // Program Counter


		void _jump(std::make_signed_t<size_t> offset) noexcept;
		std::optional<BCodeInstr> _fetch();
		// Returns if execution should continue.
		bool _exec(std::optional<BCodeInstr> instr);
		// Returns if execution should continue.
		bool _exec(BCodeInstr instr);
	};
}

