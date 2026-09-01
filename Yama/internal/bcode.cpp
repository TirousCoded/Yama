

#include "bcode.h"


std::string _ym::BCodeDbgSym::fmt() const {
	return std::format("{}:{}:{}", origin, ln, ch);
}

size_t _ym::BCodeDbgSyms::size() const noexcept {
	return _symbols.size();
}

const _ym::BCodeDbgSym* _ym::BCodeDbgSyms::symbol(size_t index) const noexcept {
	if (auto it = _symbols.find(index); it != _symbols.end()) {
		return &it->second;
	}
	return nullptr;
}

void _ym::BCodeDbgSyms::add(BCodeDbgSym symbol) {
	size_t index = symbol.index;
	_symbols.try_emplace(index, std::move(symbol));
}

size_t _ym::BCode::size() const noexcept {
	return _instrs.size();
}

_ym::BCodeInstr& _ym::BCode::operator[](size_t index) noexcept {
	ymAssert(index < size());
	return _instrs[index];
}

const _ym::BCodeInstr& _ym::BCode::operator[](size_t index) const noexcept {
	ymAssert(index < size());
	return _instrs[index];
}

std::vector<_ym::BCodeInstr>::const_iterator _ym::BCode::begin() const noexcept {
	return _instrs.begin();
}

std::vector<_ym::BCodeInstr>::const_iterator _ym::BCode::end() const noexcept {
	return _instrs.end();
}

std::optional<size_t> _ym::BCode::branchDest(size_t index) const noexcept {
	if (index >= size()) {
		return std::nullopt;
	}
	const auto& sBx = (*this)[index].sBx;
	// Gotta '+ 1' to sBx to *correct* it, as branches operate on program counter
	// value AFTER instr read incrs it.
	auto corrected_sBx = std::make_signed_t<size_t>(sBx) + 1;
	bool outOfBoundsBackwardJump = corrected_sBx < 0 && size_t(-corrected_sBx) > index;
	bool outOfBoundsForwardJump = corrected_sBx >= 0 && size_t(corrected_sBx) >= size();
	if (outOfBoundsBackwardJump || outOfBoundsForwardJump) {
		return std::nullopt;
	}
	return index + corrected_sBx;
}

std::string _ym::BCode::fmtSymbol(size_t index, const BCodeDbgSyms* symbols) const {
	if (symbols) {
		if (auto s = symbols->symbol(index)) {
			return s->fmt();
		}
	}
	return std::format("<Instr({})>", index);
}

std::string _ym::BCode::fmtInstr(size_t index, const BCodeDbgSyms* symbols) const {
	if (symbols) {
		if (auto s = symbols->symbol(index)) {
			return std::format("[{}] {} ({})", index, _fmtInstr(index), s->fmt());
		}
	}
	return std::format("[{}] {}", index, _fmtInstr(index));
}

std::string _ym::BCode::fmtDisassembly(const BCodeDbgSyms* symbols) const {
	std::string result{};
	result = std::format("Disassembly ({} Instrs)", size());
	for (size_t i = 0; i < size(); i++) {
		result += std::format("\n  {}", fmtInstr(i, symbols));
	}
	return result;
}

void _ym::BCode::push(BCodeInstr instr) {
	_instrs.push_back(instr);
}

void _ym::BCode::shrinkToFit() {
	_instrs.shrink_to_fit();
}

std::string _ym::BCode::_fmtInstr(size_t index) const {
	auto _L = [&](YmUInt8 x) -> std::string {
		return std::format("L({})", x);
		};
	auto _Lw = [&](YmUInt8 x) -> std::string {
		if (x == bPush) return "Lw(push)";
		if (x == bDiscard) return "Lw(discard)";
		return std::format("Lw({})", x);
		};
	auto _Arg = [&](YmUInt8 x) -> std::string {
		return std::format("Arg({})", x);
		};
	auto _Ko = [&](YmUInt8 x) -> std::string {
		return std::format("Ko({})", x);
		};
	auto _Kt = [&](YmUInt8 x) -> std::string {
		return std::format("Kt({})", x);
		};
	auto _Knl = [&](YmUInt8 x) -> std::string {
		return std::format("Knl({})", x);
		};
	auto _branch = [&]() -> std::string {
		auto ind = branchDest(index);
		return std::format("->{}", ind ? std::format("{}", *ind) : "<OutOfBounds>");
		};
	auto instr = (*this)[index];
	static_assert(Opcodes == 17);
	switch (instr.opc) {
	case Opcode::noop:			return std::format("{}", instr.opc);
	case Opcode::pop:			return std::format("{} {}", instr.opc, instr.A);
	case Opcode::putNone:		return std::format("{} {}", instr.opc, _Lw(instr.A));
	case Opcode::putConst:		return std::format("{} {} {}", instr.opc, _Ko(instr.A), _Lw(instr.B));
	case Opcode::putArg:		return std::format("{} {} {}", instr.opc, _Arg(instr.A), _Lw(instr.B));
	case Opcode::copy:			return std::format("{} {} {}", instr.opc, _L(instr.A), _Lw(instr.B));
	case Opcode::defaultInit:	return std::format("{} {} {}", instr.opc, _Kt(instr.A), _Lw(instr.B));
	//case Opcode::structInit:	return std::format("{} {} {} {}", instr.opc, _Kt(instr.A), _Knl(instr.B), _Lw(instr.C));
	case Opcode::pcall:			return std::format("{} {} {}", instr.opc, _Kt(instr.A), _Lw(instr.B));
	//case Opcode::pncall:		return std::format("{} {} {} {}", instr.opc, _Kt(instr.A), _Knl(instr.B), _Lw(instr.C));
	case Opcode::ret:			return std::format("{}", instr.opc);
	case Opcode::getVar:		return std::format("{} {} {}", instr.opc, _Kt(instr.A), _Lw(instr.B));
	case Opcode::setVar:		return std::format("{} {}", instr.opc, _Kt(instr.A));
	case Opcode::getProp:		return std::format("{} {} {}", instr.opc, _Kt(instr.A), _Lw(instr.B));
	case Opcode::setProp:		return std::format("{} {}", instr.opc, _Kt(instr.A));
	case Opcode::conv:			return std::format("{} {} {}", instr.opc, _Kt(instr.A), _Lw(instr.B));
	case Opcode::jump:			return std::format("{} {}", instr.opc, _branch());
	case Opcode::jumpTrue:		return std::format("{} {}", instr.opc, _branch());
	case Opcode::jumpFalse:		return std::format("{} {}", instr.opc, _branch());
	default:					return std::format("{}", instr.opc);
	}
}

_ym::BCodeWriter::BCodeWriter(BCodeDbgSyms* symbols) :
	_symbols(symbols) {
}

size_t _ym::BCodeWriter::size() const noexcept {
	return _bcode.size();
}

_ym::BCodeWriter& _ym::BCodeWriter::addNoop() {
	_outputSymbol();
	_bcode.push(BCodeInstr::mk1(Opcode::noop));
	return *this;
}

_ym::BCodeWriter& _ym::BCodeWriter::addPop(YmUInt8 A) {
	_outputSymbol();
	_bcode.push(BCodeInstr::mk1(Opcode::pop, A));
	return *this;
}

_ym::BCodeWriter& _ym::BCodeWriter::addPutNone(YmUInt8 LwA) {
	_outputSymbol();
	_bcode.push(BCodeInstr::mk1(Opcode::putNone, LwA));
	return *this;
}

_ym::BCodeWriter& _ym::BCodeWriter::addPutConst(YmUInt8 KoA, YmUInt8 LwB) {
	_outputSymbol();
	_bcode.push(BCodeInstr::mk1(Opcode::putConst, KoA, LwB));
	return *this;
}

_ym::BCodeWriter& _ym::BCodeWriter::addPutArg(YmUInt8 ArgA, YmUInt8 LwB) {
	_outputSymbol();
	_bcode.push(BCodeInstr::mk1(Opcode::putArg, ArgA, LwB));
	return *this;
}

_ym::BCodeWriter& _ym::BCodeWriter::addCopy(YmUInt8 LA, YmUInt8 LwB) {
	_outputSymbol();
	_bcode.push(BCodeInstr::mk1(Opcode::copy, LA, LwB));
	return *this;
}

_ym::BCodeWriter& _ym::BCodeWriter::addDefaultInit(YmUInt8 KtA, YmUInt8 LwB) {
	_outputSymbol();
	_bcode.push(BCodeInstr::mk1(Opcode::defaultInit, KtA, LwB));
	return *this;
}

_ym::BCodeWriter& _ym::BCodeWriter::addPCall(YmUInt8 KtA, YmUInt8 LwB) {
	_outputSymbol();
	_bcode.push(BCodeInstr::mk1(Opcode::pcall, KtA, LwB));
	return *this;
}

_ym::BCodeWriter& _ym::BCodeWriter::addRet() {
	_outputSymbol();
	_bcode.push(BCodeInstr::mk1(Opcode::ret));
	return *this;
}

_ym::BCodeWriter& _ym::BCodeWriter::addGetVar(YmUInt8 KtA, YmUInt8 LwB) {
	_outputSymbol();
	_bcode.push(BCodeInstr::mk1(Opcode::getVar, KtA, LwB));
	return *this;
}

_ym::BCodeWriter& _ym::BCodeWriter::addSetVar(YmUInt8 KtA) {
	_outputSymbol();
	_bcode.push(BCodeInstr::mk1(Opcode::setVar, KtA));
	return *this;
}

_ym::BCodeWriter& _ym::BCodeWriter::addGetProp(YmUInt8 KtA, YmUInt8 LwB) {
	_outputSymbol();
	_bcode.push(BCodeInstr::mk1(Opcode::getProp, KtA, LwB));
	return *this;
}

_ym::BCodeWriter& _ym::BCodeWriter::addSetProp(YmUInt8 KtA) {
	_outputSymbol();
	_bcode.push(BCodeInstr::mk1(Opcode::setProp, KtA));
	return *this;
}

_ym::BCodeWriter& _ym::BCodeWriter::addConv(YmUInt8 KtA, YmUInt8 LwB) {
	_outputSymbol();
	_bcode.push(BCodeInstr::mk1(Opcode::conv, KtA, LwB));
	return *this;
}

_ym::BCodeWriter& _ym::BCodeWriter::addJump(LabelID label) {
	_outputSymbol();
	_bindLabelUser(label);
	_bcode.push(BCodeInstr::mk2(Opcode::jump, 0, 0));
	return *this;
}

_ym::BCodeWriter& _ym::BCodeWriter::addJumpTrue(LabelID label) {
	_outputSymbol();
	_bindLabelUser(label);
	_bcode.push(BCodeInstr::mk2(Opcode::jumpTrue, 0, 0));
	return *this;
}

_ym::BCodeWriter& _ym::BCodeWriter::addJumpFalse(LabelID label) {
	_outputSymbol();
	_bindLabelUser(label);
	_bcode.push(BCodeInstr::mk2(Opcode::jumpFalse, 0, 0));
	return *this;
}

_ym::BCodeWriter& _ym::BCodeWriter::addLabel(LabelID id) {
	_bindLabel(id);
	return *this;
}

_ym::BCodeWriter& _ym::BCodeWriter::setSymbol(BCodeDbgSym&& symbol) {
	_current = std::forward<BCodeDbgSym>(symbol);
	return *this;
}

_ym::BCodeWriter& _ym::BCodeWriter::setSymbol(std::string origin, size_t ln, size_t ch) {
	return setSymbol(BCodeDbgSym{
		.index = 0,
		.origin = std::move(origin),
		.ln = ln,
		.ch = ch,
		});
}

_ym::BCodeWriter& _ym::BCodeWriter::unsetSymbol() noexcept {
	_current.reset();
	return *this;
}

std::optional<_ym::BCode> _ym::BCodeWriter::done(bool* labelNotFound) {
	std::optional<BCode> result{};
	if (_resolveStubs(labelNotFound)) {
		result = std::move(_bcode);
		_reset();
	}
	return result;
}

size_t _ym::BCodeWriter::_writePos() const noexcept {
	return size();
}

std::optional<size_t> _ym::BCodeWriter::_instrIndex(LabelID id) const noexcept {
	if (auto it = _labelMap.find(id); it != _labelMap.end()) {
		return it->second;
	}
	return std::nullopt;
}

void _ym::BCodeWriter::_bindLabel(LabelID id) {
	_labelMap[id] = _writePos();
}

void _ym::BCodeWriter::_bindLabelUser(LabelID id) {
	_labelUserMap[_writePos()] = id;
}

void _ym::BCodeWriter::_outputSymbol() {
	if (_symbols && _current) {
		_current->index = _writePos();
		_symbols->add(*_current);
	}
}

void _ym::BCodeWriter::_reset() {
	_bcode = BCode{};
	_labelMap.clear();
	_labelUserMap.clear();
}

bool _ym::BCodeWriter::_resolveStubs(bool* labelNotFound) {
	for (const auto& [instrToPatch, instrLabel] : _labelUserMap) {
		if (!_resolveStub(instrToPatch, instrLabel, labelNotFound)) {
			return false;
		}
	}
	return true;
}

bool _ym::BCodeWriter::_resolveStub(size_t instrToPatch, LabelID instrLabel, bool* labelNotFound) {
	if (auto sBxVal = _sBxOf(_jumpInfo(instrToPatch, _labelInstrIndex(instrLabel, labelNotFound)), labelNotFound)) {
		_bcode[instrToPatch].sBx = *sBxVal;
		return true;
	}
	return false;
}

std::optional<size_t> _ym::BCodeWriter::_labelInstrIndex(LabelID instrLabel, bool* labelNotFound) const noexcept {
	if (auto result = _instrIndex(instrLabel)) {
		return *result;
	}
	_labelNotFoundError(labelNotFound);
	return std::nullopt;
}

std::optional<YmInt16> _ym::BCodeWriter::_sBxOf(std::optional<_JumpInfo> info, bool* labelNotFound) const noexcept {
	if (!info) {
		return std::nullopt;
	}
	return
		info->isBackwardsJump
		? _sBxOfBackwardJump(*info, labelNotFound)
		: _sBxOfForwardJump(*info, labelNotFound);
}

std::optional<YmInt16> _ym::BCodeWriter::_sBxOfForwardJump(const _JumpInfo& info, bool* labelNotFound) const noexcept {
	bool overflow = info.diff > size_t(std::numeric_limits<YmInt16>::max());
	if (overflow) {
		_sBxOverflowOrUnderflowError(labelNotFound);
		return std::nullopt;
	}
	return YmInt16(info.diff);
}

std::optional<YmInt16> _ym::BCodeWriter::_sBxOfBackwardJump(const _JumpInfo& info, bool* labelNotFound) const noexcept {
	// NOTE: below, size_t(std::numeric_limits<YmInt16>::max()) + 1 gets us the minimum
	//       16-bit value, but expressed as a positive size_t value.
	bool underflow = info.diff > size_t(std::numeric_limits<YmInt16>::max()) + 1;
	if (underflow) {
		_sBxOverflowOrUnderflowError(labelNotFound);
		return std::nullopt;
	}
	// Careful to avoid overflow/underflow.
	return YmInt16(-std::make_signed_t<size_t>(info.diff));
}

std::optional<_ym::BCodeWriter::_JumpInfo> _ym::BCodeWriter::_jumpInfo(
	size_t instrToPatch,
	std::optional<size_t> labelInstrIndex) noexcept {
	if (!labelInstrIndex) {
		return std::nullopt;
	}
	// Remember that branch instrs sBx offsets are relative to
	// the instr index immediately after the instr, so we do the
	// '+ 1' bit below.
	size_t jumpedFrom = instrToPatch + 1;
	size_t jumpedTo = *labelInstrIndex;
	_JumpInfo result{
		.low = jumpedFrom,
		.high = jumpedTo,
		.isBackwardsJump = jumpedFrom > jumpedTo,
	};
	if (result.isBackwardsJump) {
		std::swap(result.low, result.high);
	}
	// Absolute Value
	result.diff = result.high - result.low;
	return result;
}

void _ym::BCodeWriter::_labelNotFoundError(bool* labelNotFound) noexcept {
	if (labelNotFound) {
		*labelNotFound = true;
	}
}

void _ym::BCodeWriter::_sBxOverflowOrUnderflowError(bool* labelNotFound) noexcept {
	if (labelNotFound) {
		*labelNotFound = false;
	}
}

