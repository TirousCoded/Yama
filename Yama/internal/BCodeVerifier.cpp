

#include "BCodeVerifier.h"


#define _TRACE_SYMEXEC false

#if _TRACE_SYMEXEC
#include "../yama++/print.h"
#endif


_ym::BCodeVerifier::BCodeVerifier(BuiltinsCache fast) noexcept :
	fast(fast) {
}

bool _ym::BCodeVerifier::verify(
	YmType& t,
	const BCode& bcode,
	const BCodeDbgSyms* bsyms) {
	return _verify(t, bcode, bsyms);
}

YmType& _ym::BCodeVerifier::_getType() const noexcept {
	return ym::deref(_t);
}

const _ym::BCode& _ym::BCodeVerifier::_getBCode() const noexcept {
	return ym::deref(_bcode);
}

_ym::BCodeVerifier::_Block* _ym::BCodeVerifier::_getBasicBlock(size_t index) noexcept {
	if (auto it = _cfg.find(index); it != _cfg.end()) {
		return &it->second;
	}
	return nullptr;
}

std::string _ym::BCodeVerifier::_fmtCFG() const {
	std::string result{};
	result += "Bytecode CFG:";
	for (const auto& [start, b] : _cfg) {
		result += std::format("\n  {}", b.fmt());
		result += std::format("\n    ProcessedBy: {}", b.processed() ? b.fmtBranchToBlock(*b.processedBy) : "<Unprocessed>");
		result += std::format("\n    Input      : {}", b.input.fmt());
		result += std::format("\n    Output     : {}", b.output.fmt());
		for (size_t i = b.first; i < b.last; i++) {
			result += std::format("\n  {}", _getBCode().fmtInstr(i, _bsyms));
		}
	}
	return result;
}

std::string _ym::BCodeVerifier::_fmtBranch(size_t from, size_t to) {
	return std::format("{{Branch {} -> {}}}", from, to);
}

bool _ym::BCodeVerifier::_verify(YmType& t, const BCode& bcode, const BCodeDbgSyms* bsyms) {
	_setup(t, bcode, bsyms);
	if (!_bcodeIsNotEmpty()) {
		return false;
	}
	return _verifyCFG();
}

void _ym::BCodeVerifier::_setup(YmType& t, const BCode& bcode, const BCodeDbgSyms* bsyms) noexcept {
	_t = &t;
	_bcode = &bcode;
	_bsyms = bsyms;
}

bool _ym::BCodeVerifier::_bcodeIsNotEmpty() const {
	if (_getBCode().size() == 0) {
		Global::raiseErr(YmErrCode_InternalError, "{} bcode invalid; binary is empty!", _getType().fullname());
		return false;
	}
	return true;
}

bool _ym::BCodeVerifier::_verifyCFG() {
	_buildCFG();
	auto result = _visitEntrypointBlock();
	_checkForDeadCodeBlocks();
	return result;
}

void _ym::BCodeVerifier::_buildCFG() {
	_addDivisionPoints();
	_addBasicBlocks();
}

void _ym::BCodeVerifier::_addDivisionPoints() {
	// Collect up 'division points', defined by:
	//		1) start/end of bcode;
	//		2) jump destinations (which are not out-of-bounds);
	//		3) positions immediately after jump instrs (aka. fallthrough dests);
	//		4) positions immediately after 'exitpoint' instrs like 'ret'.
	_addStartAndEndDivisionPoints();
	for (size_t i = 0; i < _getBCode().size(); i++) {
		static_assert(Opcodes == 17);
		const auto& instr = _getBCode()[i];
		if (instr.opc == Opcode::ret) {
			_addPostInstrDivisionPoint(i);
		}
		else if (instr.opc == Opcode::jump) {
			_addPostInstrDivisionPoint(i);
			_addBranchDestDivisionPoint(i);
		}
		else if (instr.opc == Opcode::jumpTrue) {
			_addPostInstrDivisionPoint(i);
			_addBranchDestDivisionPoint(i);
		}
		else if (instr.opc == Opcode::jumpFalse) {
			_addPostInstrDivisionPoint(i);
			_addBranchDestDivisionPoint(i);
		}
	}
}

void _ym::BCodeVerifier::_addStartAndEndDivisionPoints() {
	_addDivisionPoint(0);
	_addDivisionPoint(_getBCode().size());
}

void _ym::BCodeVerifier::_addPostInstrDivisionPoint(size_t index) {
	_addDivisionPoint(index + 1);
}

void _ym::BCodeVerifier::_addBranchDestDivisionPoint(size_t index) {
	if (auto dest = _getBCode().branchDest(index)) {
		_addDivisionPoint(*dest);
	}
}

void _ym::BCodeVerifier::_addDivisionPoint(size_t index) {
	_cfgDivisionPoints.emplace(index);
}

void _ym::BCodeVerifier::_addBasicBlocks() {
	// The trick here is that std::set, being *ordered*, will ensure that our
	// set of division points are organized sequentially, while also ensuring
	// that we avoid duplicate division points.
	ymAssert(_cfgDivisionPoints.size() >= 2);
	// Iterate such that we stop at the iterator immediately prior to the
	// past-the-end, rather than at the past-the-end itself.
	for (auto it = _cfgDivisionPoints.begin(); std::next(it) != _cfgDivisionPoints.end(); it++) {
		_addBasicBlock(*it, *std::next(it));
	}
	ymAssert(_cfg.size() == _cfgDivisionPoints.size() - 1);
	_cfgDivisionPoints.clear(); // They're unneeded now.
}

void _ym::BCodeVerifier::_addBasicBlock(size_t first, size_t last) {
	ymAssert(!_cfg.contains(first));
	ymAssert(last - first >= 1);
	_cfg[first] = _Block{
		.first = first,
		.last = last,
		.instrs = std::span(_getBCode()).subspan(first, last - first),
	};
}

void _ym::BCodeVerifier::_checkForDeadCodeBlocks() {
	for (const auto& [index, block] : _cfg) {
		if (block.processed()) {
			continue;
		}
		// TODO: These should be 'warnings', not 'errors'. Not sure how we'll
		//		 make the distinction though.
		Global::raiseErr(YmErrCode_InternalError, "{} bcode contains dead code {}!",
			_getType().fullname(), block.fmt());
	}
}

bool _ym::BCodeVerifier::_visitEntrypointBlock() {
	if (_visitBlock(0, _mkEntrypointStkState(), size_t(-1))) {
		return true;
	}
	Global::raiseErr(YmErrCode_InternalError, "{} bcode invalid; dumping diagnostics...\n{}\n{}",
		_getType().fullname(), _getType().fmtConsts(), _fmtCFG());
	return false;
}

_ym::BCodeVerifier::_StkState _ym::BCodeVerifier::_mkEntrypointStkState() const {
	_StkState result{};
	for (YmParamIndex i = 0; i < _getType().params(); i++) {
		result.args.push_back(_getType().param(i).value().type());
	}
	return result;
}

bool _ym::BCodeVerifier::_visitBlock(size_t blockInstr, const _StkState& incoming, size_t incomingBranchedFrom) {
	ymAssert(_cfg.contains(blockInstr));
	auto& b = _cfg.at(blockInstr);
	return
		b.processed()
		? _visitProcessedBlock(b, incoming, incomingBranchedFrom)
		: _visitUnprocessedBlock(b, incoming, incomingBranchedFrom);
}

bool _ym::BCodeVerifier::_visitProcessedBlock(_Block& block, const _StkState& incoming, size_t incomingBranchedFrom) {
	return _checkLogicalCoherence(block, incoming, incomingBranchedFrom);
}

bool _ym::BCodeVerifier::_visitUnprocessedBlock(_Block& block, const _StkState& incoming, size_t incomingBranchedFrom) {
	block.processedBy = incomingBranchedFrom;
	block.input = incoming;
	block.output = incoming; // For symbolic exec.
	if (!_symbolicExec(block, incoming, incomingBranchedFrom)) {
		return false;
	}
	// If either the primary or fallthrough branch destinations are out-of-bounds, we're
	// just gonna quietly *skip* them, as checking for branch validity is gonna be left
	// to other parts of the verifier to handle.
	if (auto dest = _getBCode().branchDest(block.finalInstr()); block.finalInstrIsBranch() && dest) {
		if (!_visitBlock(*dest, block.output, block.finalInstr())) {
			return false;
		}
	}
	if (block.finalInstrCanFallThrough() && block.last < _getBCode().size()) {
		if (!_visitBlock(block.last, block.output, block.finalInstr())) {
			return false;
		}
	}
	return true;
}

bool _ym::BCodeVerifier::_checkLogicalCoherence(_Block& block, const _StkState& incoming, size_t incomingBranchedFrom) const {
	bool result = true;
	// Don't let short-circuiting skip any of below.
	result = result && _checkLocalCountCoherence(block, incoming, incomingBranchedFrom);
	result = result && _checkArgCountCoherence(block, incoming, incomingBranchedFrom);
	result = result && _checkLocalTypeCoherence(block, incoming, incomingBranchedFrom);
	result = result && _checkArgTypeCoherence(block, incoming, incomingBranchedFrom);
	return result;
}

bool _ym::BCodeVerifier::_checkLocalCountCoherence(_Block& block, const _StkState& incoming, size_t incomingBranchedFrom) const {
	if (block.input.locals.size() != incoming.locals.size()) {
		Global::raiseErr(YmErrCode_InternalError,
			"{} bcode logically incoherent; for {}, incoming local count is {}, but expected {}!",
			_getType().fullname(),
			_fmtBranch(incomingBranchedFrom, block.first),
			incoming.locals.size(),
			block.input.locals.size());
		return false;
	}
	return true;
}

bool _ym::BCodeVerifier::_checkArgCountCoherence(_Block& block, const _StkState& incoming, size_t incomingBranchedFrom) const {
	if (block.input.args.size() != incoming.args.size()) {
		Global::raiseErr(YmErrCode_InternalError,
			"{} bcode logically incoherent; for {}, incoming arg count is {}, but expected {}!",
			_getType().fullname(),
			_fmtBranch(incomingBranchedFrom, block.first),
			incoming.args.size(),
			block.input.args.size());
		return false;
	}
	return true;
}

bool _ym::BCodeVerifier::_checkLocalTypeCoherence(_Block& block, const _StkState& incoming, size_t incomingBranchedFrom) const {
	bool result = true;
	size_t s = std::min(block.input.locals.size(), incoming.locals.size());
	for (size_t i = 0; i < s; i++) {
		auto& incomingType = incoming.locals[i];
		auto& expectedType = block.input.locals[i];
		if (incomingType != expectedType) {
			Global::raiseErr(YmErrCode_InternalError,
				"{} bcode logically incoherent; for {}, incoming local {} is type {}, but dest expected {}!",
				_getType().fullname(),
				_fmtBranch(incomingBranchedFrom, block.first),
				i,
				incomingType->fullname(),
				expectedType->fullname());
			// Try to detect ALL errors.
			result = false;
		}
	}
	return result;
}

bool _ym::BCodeVerifier::_checkArgTypeCoherence(_Block& block, const _StkState& incoming, size_t incomingBranchedFrom) const {
	bool result = true;
	size_t s = std::min(block.input.args.size(), incoming.args.size());
	for (size_t i = 0; i < s; i++) {
		auto& incomingType = incoming.args[i];
		auto& expectedType = block.input.args[i];
		if (incomingType != expectedType) {
			Global::raiseErr(YmErrCode_InternalError,
				"{} bcode logically incoherent; for {}, incoming arg {} is type {}, but dest expected {}!",
				_getType().fullname(),
				_fmtBranch(incomingBranchedFrom, block.first),
				i,
				incomingType->fullname(),
				expectedType->fullname());
			// Try to detect ALL errors.
			result = false;
		}
	}
	return result;
}

bool _ym::BCodeVerifier::_symbolicExec(_Block& block, const _StkState& incoming, size_t incomingBranchedFrom) {
#if _TRACE_SYMEXEC
	ym::println("SymExecTrace: {}\n  Incoming: {}\n  From: {}",
		block.fmt(),
		incoming.fmt(),
		block.fmtBranchToBlock(incomingBranchedFrom));
#endif
	for (size_t i = block.first; i < block.last; i++) {
		if (!_symbolicExecStep(i, block, incoming, incomingBranchedFrom)) {
			return false;
		}
	}
	return _checkForBlockLevelErrors(block);
}

bool _ym::BCodeVerifier::_symbolicExecStep(size_t i, _Block& block, const _StkState& incoming, size_t incomingBranchedFrom) {
#if _TRACE_SYMEXEC
	ym::println("SymExecTrace: {}", _getBCode().fmtInstr(i, _bsyms));
#endif
	auto& instr = _getBCode()[i];
	auto& s = block.output;
	static_assert(Opcodes == 17);
	switch (instr.opc) {
	case Opcode::noop:
	{
		// Do nothing.
	}
	break;
	case Opcode::pop:
	{
		s.popLocals(instr.A);
	}
	break;
	case Opcode::putNone:
	{
		if (!(
			_check_Lw(block, i, instr.A, 'A', 0)
			)) {
			return false;
		}
		s.putLocal(instr.A, *fast.none);
	}
	break;
	case Opcode::putConst:
	{
		if (!(
			_check_Ko(block, i, instr.A, 'A') &&
			_check_Lw(block, i, instr.B, 'B', 0)
			)) {
			return false;
		}
		s.putLocal(instr.B, ym::deref(_constToType(_getType().consts()[instr.A])));
	}
	break;
	case Opcode::putArg:
	{
		if (!(
			_check_Arg(block, i, instr.A, 'A') &&
			_check_Lw(block, i, instr.B, 'B', 0)
			)) {
			return false;
		}
		s.putLocal(instr.B, _getType().param(instr.A).value().type());
	}
	break;
	case Opcode::copy:
	{
		if (!(
			_check_L(block, i, instr.A, 'A') &&
			_check_Lw(block, i, instr.B, 'B', 0)
			)) {
			return false;
		}
		s.putLocal(instr.B, *s.locals.at(instr.A));
	}
	break;
	case Opcode::defaultInit:
	{
		if (!(
			_check_Kt(block, i, instr.A, 'A') &&
			_check_Kt_hasDefaultValue(block, i, instr.A, 'A') &&
			_check_Lw(block, i, instr.B, 'B', 0)
			)) {
			return false;
		}
		s.putLocal(instr.B, *_getType().constAsRef(instr.A));
	}
	break;
	//case Opcode::structInit:
	//{
	//
	//}
	//break;
	case Opcode::pcall:
	{
		if (!(
			_check_Kt(block, i, instr.A, 'A') &&
			_check_Kt_IsCallable(block, i, instr.A, 'A')
			)) {
			return false;
		}
		auto& called = *_getType().constAsRef(instr.A);
		if (!(
			_check_Lw(block, i, instr.B, 'B', called.positionalParams()) &&
			_checkLocalsPresent(block, i, called.positionalParams()) &&
			_checkLocalsArePositionalParamTypes(block, i, called)
			)) {
			return false;
		}
		auto& returnType = ym::deref(called.returnType());
		s.popLocals(called.positionalParams());
		s.putLocal(instr.B, returnType);
	}
	break;
	//case Opcode::pncall:
	//{
	//
	//}
	//break;
	case Opcode::ret:
	{
		auto& returnType = ym::deref(_getType().returnType());
		if (!(
			_checkLocalsPresent(block, i, 1) &&
			_checkLocalIsCorrectType(block, i, s.locals.size() - 1, returnType)
			)) {
			return false;
		}
	}
	break;
	case Opcode::getVar:
	{
		if (!(
			_check_Kt(block, i, instr.A, 'A') &&
			_check_Kt_isCorrectKind(block, i, instr.A, 'A', YmKind_Var) &&
			_check_Lw(block, i, instr.B, 'B', 0)
			)) {
			return false;
		}
		auto& valueType = ym::deref(_getType().constAsRef(instr.A)->returnType());
		s.putLocal(instr.B, valueType);
	}
	break;
	case Opcode::setVar:
	{
		if (!(
			_check_Kt(block, i, instr.A, 'A') &&
			_check_Kt_isCorrectKind(block, i, instr.A, 'A', YmKind_Var) &&
			_check_Kt_hasAssigner(block, i, instr.A, 'A') &&
			_checkLocalsPresent(block, i, 1)
			)) {
			return false;
		}
		auto& valueType = ym::deref(_getType().constAsRef(instr.A)->returnType());
		if (!(
			_checkLocalIsCorrectType(block, i, s.locals.size() - 1, valueType))) {
			return false;
		}
		s.popLocals(1);
	}
	break;
	case Opcode::getProp:
	{
		if (!(
			_check_Kt(block, i, instr.A, 'A') &&
			_check_Kt_isCorrectKind(block, i, instr.A, 'A', YmKind_Property) &&
			_check_Lw(block, i, instr.B, 'B', 1) &&
			_checkLocalsPresent(block, i, 1)
			)) {
			return false;
		}
		auto& subjectType = _getType().constAsRef(instr.A)->param(0)->type();
		auto& valueType = ym::deref(_getType().constAsRef(instr.A)->returnType());
		if (!(
			_checkLocalIsCorrectType(block, i, s.locals.size() - 1, subjectType)
			)) {
			return false;
		}
		s.popLocals(1);
		s.putLocal(instr.B, valueType);
	}
	break;
	case Opcode::setProp:
	{
		if (!(
			_check_Kt(block, i, instr.A, 'A') &&
			_check_Kt_isCorrectKind(block, i, instr.A, 'A', YmKind_Property) &&
			_check_Kt_hasAssigner(block, i, instr.A, 'A') &&
			_checkLocalsPresent(block, i, 2)
			)) {
			return false;
		}
		auto& subjectType = _getType().constAsRef(instr.A)->param(0)->type();
		auto& valueType = ym::deref(_getType().constAsRef(instr.A)->returnType());
		if (!(
			_checkLocalIsCorrectType(block, i, s.locals.size() - 2, subjectType) &&
			_checkLocalIsCorrectType(block, i, s.locals.size() - 1, valueType)
			)) {
			return false;
		}
		s.popLocals(2);
	}
	break;
	case Opcode::conv:
	{
		if (!(
			_check_Kt(block, i, instr.A, 'A') &&
			_check_Lw(block, i, instr.B, 'B', 1) &&
			_checkLocalsPresent(block, i, 1)
			)) {
			return false;
		}
		auto& targetType = *_getType().constAsRef(instr.A);
		if (!(
			_checkLocalCanConvertToType(block, i, s.locals.size() - 1, targetType)
			)) {
			return false;
		}
		s.popLocals(1);
		s.putLocal(instr.B, targetType);
	}
	break;
	case Opcode::jump:
	{
		// These checks are done elsewhere.
	}
	break;
	case Opcode::jumpTrue:
	{
		if (!(
			_checkLocalsPresent(block, i, 1) &&
			_checkLocalCanConvertToType(block, i, s.locals.size() - 1, *fast.bool0)
			)) {
			return false;
		}
		s.popLocals(1);
	}
	break;
	case Opcode::jumpFalse:
	{
		if (!(
			_checkLocalsPresent(block, i, 1) &&
			_checkLocalCanConvertToType(block, i, s.locals.size() - 1, *fast.bool0)
			)) {
			return false;
		}
		s.popLocals(1);
	}
	break;
	default: YM_DEADEND; break;
	}
	return true;
}

bool _ym::BCodeVerifier::_checkForBlockLevelErrors(_Block& block) const {
	return
		_checkBlockFinalInstrBranchIsInBounds(block) &&
		_checkBlockFinalInstrFallThroughIsInBounds(block);
}

bool _ym::BCodeVerifier::_checkBlockFinalInstrBranchIsInBounds(_Block& block) const {
	if (block.finalInstrIsBranch() && !_getBCode().branchDest(block.finalInstr())) {
		Global::raiseErr(YmErrCode_InternalError,
			"{} bcode invalid; instr {} ({}; {}); branch by sBx={} would put program counter out-of-bounds!",
			_getType().fullname(),
			block.finalInstr(),
			_getBCode()[block.finalInstr()].opc,
			_getBCode().fmtSymbol(block.finalInstr(), _bsyms),
			_getBCode()[block.finalInstr()].sBx);
		return false;
	}
	return true;
}

bool _ym::BCodeVerifier::_checkBlockFinalInstrFallThroughIsInBounds(_Block& block) const {
	if (block.finalInstrCanFallThrough() && block.last >= _getBCode().size()) {
		Global::raiseErr(YmErrCode_InternalError,
			"{} bcode invalid; instr {} ({}; {}); {} fallthrough would put program counter out-of-bounds!",
			_getType().fullname(),
			block.finalInstr(),
			_getBCode()[block.finalInstr()].opc,
			_getBCode().fmtSymbol(block.finalInstr(), _bsyms),
			block.fmt());
		return false;
	}
	return true;
}

bool _ym::BCodeVerifier::_check_L(_Block& block, size_t i, YmUInt8 field, YmChar letter) const {
	auto& s = block.output;
	if (field >= s.locals.size()) {
		Global::raiseErr(YmErrCode_InternalError,
			"{} bcode invalid; instr {} ({}; {}); L({}={}) out-of-bounds!",
			_getType().fullname(),
			i,
			_getBCode()[i].opc,
			_getBCode().fmtSymbol(i, _bsyms),
			letter,
			field);
		return false;
	}
	return true;
}

bool _ym::BCodeVerifier::_check_Lw(_Block& block, size_t i, YmUInt8 field, YmChar letter, size_t localsPoppedByInstr) const {
	auto& s = block.output;
	size_t correctedLocalCount = s.locals.size() - std::min(localsPoppedByInstr, s.locals.size());
	if (field >= correctedLocalCount && field != bPush && field != bDiscard) {
		Global::raiseErr(YmErrCode_InternalError,
			"{} bcode invalid; instr {} ({}; {}); Lw({}={}) out-of-bounds!",
			_getType().fullname(),
			i,
			_getBCode()[i].opc,
			_getBCode().fmtSymbol(i, _bsyms),
			letter,
			field);
		return false;
	}
	return true;
}

bool _ym::BCodeVerifier::_check_Arg(_Block& block, size_t i, YmUInt8 field, YmChar letter) const {
	auto& s = block.output;
	if (field >= s.args.size()) {
		Global::raiseErr(YmErrCode_InternalError,
			"{} bcode invalid; instr {} ({}; {}); Arg({}={}) out-of-bounds!",
			_getType().fullname(),
			i,
			_getBCode()[i].opc,
			_getBCode().fmtSymbol(i, _bsyms),
			letter,
			field);
		return false;
	}
	return true;
}

bool _ym::BCodeVerifier::_check_Ko(_Block& block, size_t i, YmUInt8 field, YmChar letter) const {
	if (!_getType().isObjConst(field)) {
		Global::raiseErr(YmErrCode_InternalError,
			"{} bcode invalid; instr {} ({}; {}); Ko({}={}) not a valid obj const!",
			_getType().fullname(),
			i,
			_getBCode()[i].opc,
			_getBCode().fmtSymbol(i, _bsyms),
			letter,
			field);
		return false;
	}
	return true;
}

bool _ym::BCodeVerifier::_check_Kt(_Block& block, size_t i, YmUInt8 field, YmChar letter) const {
	if (!_getType().isTypeConst(field)) {
		Global::raiseErr(YmErrCode_InternalError,
			"{} bcode invalid; instr {} ({}; {}); Kt({}={}) not a valid type const!",
			_getType().fullname(),
			i,
			_getBCode()[i].opc,
			_getBCode().fmtSymbol(i, _bsyms),
			letter,
			field);
		return false;
	}
	return true;
}

bool _ym::BCodeVerifier::_check_Kt_hasDefaultValue(_Block& block, size_t i, YmUInt8 field, YmChar letter) const {
	ymAssert(_getType().isTypeConst(field));
	YmType& type = *_getType().constAsRef(field);
	if (!type.hasDefaultValue()) {
		Global::raiseErr(YmErrCode_InternalError,
			"{} bcode invalid; instr {} ({}; {}); Kt({}={}) type {} has no default value!",
			_getType().fullname(),
			i,
			_getBCode()[i].opc,
			_getBCode().fmtSymbol(i, _bsyms),
			letter,
			field,
			type.fullname());
		return false;
	}
	return true;
}

bool _ym::BCodeVerifier::_check_Kt_IsCallable(_Block& block, size_t i, YmUInt8 field, YmChar letter) const {
	ymAssert(_getType().isTypeConst(field));
	YmType& type = *_getType().constAsRef(field);
	if (!type.isCallable()) {
		Global::raiseErr(YmErrCode_InternalError,
			"{} bcode invalid; instr {} ({}; {}); Kt({}={}) type {} is not callable!",
			_getType().fullname(),
			i,
			_getBCode()[i].opc,
			_getBCode().fmtSymbol(i, _bsyms),
			letter,
			field,
			type.fullname());
		return false;
	}
	return true;
}

bool _ym::BCodeVerifier::_check_Kt_isCorrectKind(_Block& block, size_t i, YmUInt8 field, YmChar letter, YmKind k) const {
	ymAssert(_getType().isTypeConst(field));
	YmType& type = *_getType().constAsRef(field);
	if (type.kind() != k) {
		Global::raiseErr(YmErrCode_InternalError,
			"{} bcode invalid; instr {} ({}; {}); Kt({}={}) type {} kind is {}, but expected {}!",
			_getType().fullname(),
			i,
			_getBCode()[i].opc,
			_getBCode().fmtSymbol(i, _bsyms),
			letter,
			field,
			type.fullname(),
			ymKind_Fmt(type.kind()),
			ymKind_Fmt(k));
		return false;
	}
	return true;
}

bool _ym::BCodeVerifier::_check_Kt_hasAssigner(_Block& block, size_t i, YmUInt8 field, YmChar letter) const {
	ymAssert(_getType().isTypeConst(field));
	YmType& type = *_getType().constAsRef(field);
	if (!type.assigner()) {
		Global::raiseErr(YmErrCode_InternalError,
			"{} bcode invalid; instr {} ({}; {}); Kt({}={}) type {} has no assigner!",
			_getType().fullname(),
			i,
			_getBCode()[i].opc,
			_getBCode().fmtSymbol(i, _bsyms),
			letter,
			field,
			type.fullname());
		return false;
	}
	return true;
}

bool _ym::BCodeVerifier::_checkLocalsPresent(_Block& block, size_t i, size_t locals) const {
	auto& s = block.output;
	if (s.locals.size() < locals) {
		Global::raiseErr(YmErrCode_InternalError,
			"{} bcode invalid; instr {} ({}; {}); {} locals, but expected {}!",
			_getType().fullname(),
			i,
			_getBCode()[i].opc,
			_getBCode().fmtSymbol(i, _bsyms),
			s.locals.size(),
			locals);
		return false;
	}
	return true;
}

bool _ym::BCodeVerifier::_checkLocalsArePositionalParamTypes(_Block& block, size_t i, YmType& called) const {
	auto& s = block.output;
	ymAssert(s.locals.size() >= called.positionalParams());
	for (YmParams param = 0; param < called.positionalParams(); param++) {
		auto paramLocal = s.locals.size() - called.positionalParams() + param;
		auto& paramType = called.param(param).value().type();
		if (!_checkLocalIsCorrectType(block, i, paramLocal, paramType)) {
			return false;
		}
	}
	return true;
}

bool _ym::BCodeVerifier::_checkLocalIsCorrectType(_Block& block, size_t i, size_t where, YmType& type) const {
	auto& s = block.output;
	ymAssert(where < s.locals.size());
	if (s.locals[where] != type) {
		Global::raiseErr(YmErrCode_InternalError,
			"{} bcode invalid; instr {} ({}; {}); local {} is type {}, but expected {}!",
			_getType().fullname(),
			i,
			_getBCode()[i].opc,
			_getBCode().fmtSymbol(i, _bsyms),
			where,
			s.locals[where]->fullname(),
			type.fullname());
		return false;
	}
	return true;
}

bool _ym::BCodeVerifier::_checkLocalCanConvertToType(_Block& block, size_t i, size_t where, YmType& target) const {
	auto& s = block.output;
	ymAssert(where < s.locals.size());
	if (!ymType_Converts(s.locals[where].get(), &target, false)) {
		Global::raiseErr(YmErrCode_InternalError,
			"{} bcode invalid; instr {} ({}; {}); local {} type {} doesn't convert to {}!",
			_getType().fullname(),
			i,
			_getBCode()[i].opc,
			_getBCode().fmtSymbol(i, _bsyms),
			where,
			s.locals[where]->fullname(),
			target.fullname());
		return false;
	}
	return true;
}

YmType* _ym::BCodeVerifier::_constToType(const Const& c) const noexcept {
	if (c.is<YmInt>())					return fast.int0;
	else if (c.is<YmUInt>())			return fast.uint;
	else if (c.is<YmFloat>())			return fast.float0;
	else if (c.is<YmBool>())			return fast.bool0;
	else if (c.is<YmRune>())			return fast.rune;
	else if (c.is<ym::Safe<YmType>>())	return fast.type;
	else								return nullptr;
}

bool _ym::BCodeVerifier::_Block::finalInstrIsBranch() const noexcept {
	static_assert(Opcodes == 17);
	switch (instrs.back().opc) {
	case Opcode::ret:		return false;
	case Opcode::jump:		return true;
	case Opcode::jumpTrue:	return true;
	case Opcode::jumpFalse: return true;
	default:				return false;
	}
}

bool _ym::BCodeVerifier::_Block::finalInstrCanFallThrough() const noexcept {
	static_assert(Opcodes == 17);
	switch (instrs.back().opc) {
	case Opcode::ret:		return false;
	case Opcode::jump:		return false;
	case Opcode::jumpTrue:	return true;
	case Opcode::jumpFalse: return true;
	default:				return true;
	}
}

std::string _ym::BCodeVerifier::_Block::fmt() const {
	return std::format("{{Block [{}, {})}}", first, last);
}

std::string _ym::BCodeVerifier::_Block::fmtBranchToBlock(size_t from) const {
	if (from == size_t(-1))	return "<Entrypoint>";
	else					return _fmtBranch(from, first);
}

void _ym::BCodeVerifier::_StkState::setArg(YmUInt8 which, YmType& what) noexcept {
	ymAssert(which < args.size());
#if _TRACE_SYMEXEC
	ym::println("SymExecTrace:   Arg({}): {} -> {}", which, args[which]->fullname(), what.fullname());
#endif
	args[which] = what;
}

void _ym::BCodeVerifier::_StkState::putLocal(YmUInt8 whereTo, YmType& what) {
	if (whereTo == bPush) {
#if _TRACE_SYMEXEC
		ym::println("SymExecTrace:   Push {} -> L({})", what.fullname(), locals.size());
#endif
		locals.push_back(what);
		return;
	}
	if (whereTo == bDiscard) {
		return;
	}
	ymAssert(whereTo < locals.size());
#if _TRACE_SYMEXEC
	ym::println("SymExecTrace:   L({}): {} -> {}", whereTo, locals[whereTo]->fullname(), what.fullname());
#endif
	locals[whereTo] = what;
}

void _ym::BCodeVerifier::_StkState::popLocals(YmUInt8 n) noexcept {
#if _TRACE_SYMEXEC
	ym::println("SymExecTrace:   Pop {}", std::min(size_t(n), locals.size()));
#endif
	locals.erase(std::next(locals.begin(), locals.size() - std::min(size_t(n), locals.size())), locals.end());
}

std::string _ym::BCodeVerifier::_StkState::fmt() const {
	std::string argsTxt{}, localsTxt{};
	for (const auto& arg : args) {
		if (!argsTxt.empty()) {
			argsTxt += ", ";
		}
		argsTxt += arg->fullname().string();
	}
	for (const auto& local : locals) {
		if (!localsTxt.empty()) {
			localsTxt += ", ";
		}
		localsTxt += local->fullname().string();
	}
	return std::format("[{}][{}]", argsTxt, localsTxt);
}

