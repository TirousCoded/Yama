

#pragma once


#include <map>
#include <set>
#include <vector>

#include "bcode.h"
#include "BuiltinsCache.h"
#include "YmType.h"


namespace _ym {


	// NOTE: Unlike our old impl, this verifier operates in the context of linkage info,
	//		 as things like generics (which our old impl didn't have), as well as things
	//		 like named param lists, made verif w/out linkage REALLY hard.
	//
	//		 This also means that this verifier is used after loading, rather than after
	//		 compilation, and that it runs for each type instantiation.

	// NOTE: See Yama/docs/design/bytecode.txt for a (somewhat out-of-date, but still
	//		 useful) description of the basics of bcode static verif.
	//
	//		 This impl may not be 1-to-1 w/ this description.

	// NOTE: In addition to the per-instruction verif rules, the following additional
	//		 semantics also apply:
	//			- The semantics in 'bytecode.txt' are ammended to also include arg
	//			  type state in its notion of symbolic execution, alongside local obj
	//			  stack state, as it's possible for bcode to change arg types.
	//			- Entrypoint block always begins at instr 0.
	//			- Dead code blocks are completely ignored.
	//			- Blocks which are not dead must not allow for fallthrough branches which
	//			  would put the program counter out-of-bounds of the bcode binary.
	//			- CFGs w/ no exitpoint blocks will be allowed so long as all control paths
	//			  result in infinite loops.
	//				- This might seem silly, but for static verif validity, it's reasonable
	//				  as the code never exiting means we don't have to worry about things
	//				  like return value type correctness, as the code will never return.


	// TODO: Should we refactor this to use thread-local stuff?

	// Used to verify bytecode binaries in the presence of linkage information.
	class BCodeVerifier final {
	public:
		BuiltinsCache fast;


		BCodeVerifier(BuiltinsCache fast) noexcept;


		bool verify(
			YmType& t,
			const BCode& bcode,
			const BCodeDbgSyms* bsyms = nullptr);


	private:
		// State of what args and locals looks like entering/exiting a basic block.
		// We need to account for args too due to it being possible to change the
		// types of args dynamically.
		struct _StkState final {
			std::vector<ym::Safe<YmType>> args, locals;


			bool operator==(const _StkState&) const noexcept = default;

			void setArg(YmUInt8 which, YmType& what) noexcept;
			void putLocal(YmUInt8 whereTo, YmType& what);
			void popLocals(YmUInt8 n) noexcept;

			std::string fmt() const;
		};

		// Basic block of the CFG.
		struct _Block final {
			// Instr index range the block describes.
			// This is an exclusive range [first, last).
			size_t first, last;
			// The instrs of the basic block.
			std::span<const BCodeInstr> instrs;
			// The input/output state of this block.
			_StkState input, output;
			// The instr branched-from to this block which caused this block to
			// get processed (or empty if the block hasn't yet been processed.)
			std::optional<size_t> processedBy;


			inline bool processed() const noexcept { return processedBy.has_value(); }
			inline size_t finalInstr() const noexcept { return last - 1; }

			// If final instr is a branch instr.
			bool finalInstrIsBranch() const noexcept;

			// If final instr can branch to next instr by 'falling through' to it.
			// This includes all instrs which aren't branches or exitpoints, as well
			// as all branch instrs w/ conditional branching behaviour that could fail.
			bool finalInstrCanFallThrough() const noexcept;

			std::string fmt() const;
			std::string fmtBranchToBlock(size_t from) const;
		};

		// CFG, defined as a map of instr indices to the basic blocks they define
		// the starting indices of.
		// We use an ordered map here, to make iteration nicer.
		using _CFG = std::map<size_t, _Block>;


		YmType* _t = nullptr;
		const BCode* _bcode = nullptr;
		const BCodeDbgSyms* _bsyms = nullptr;
		std::set<size_t> _cfgDivisionPoints;
		_CFG _cfg;


		YmType& _getType() const noexcept;
		const BCode& _getBCode() const noexcept;
		
		_Block* _getBasicBlock(size_t index) noexcept;
		std::string _fmtCFG() const;
		static std::string _fmtBranch(size_t from, size_t to);

		bool _verify(YmType& t, const BCode& bcode, const BCodeDbgSyms* bsyms);
		void _setup(YmType& t, const BCode& bcode, const BCodeDbgSyms* bsyms) noexcept;
		bool _bcodeIsNotEmpty() const;
		bool _verifyCFG();
		void _buildCFG();

		void _addDivisionPoints();
		void _addStartAndEndDivisionPoints();
		void _addPostInstrDivisionPoint(size_t index);
		void _addBranchDestDivisionPoint(size_t index);
		void _addDivisionPoint(size_t index);
		
		void _addBasicBlocks();
		void _addBasicBlock(size_t first, size_t last);
		
		void _checkForDeadCodeBlocks();
		bool _visitEntrypointBlock();
		_StkState _mkEntrypointStkState() const;
		bool _visitBlock(size_t blockInstr, const _StkState& incoming, size_t incomingBranchedFrom);
		bool _visitProcessedBlock(_Block& block, const _StkState& incoming, size_t incomingBranchedFrom);
		bool _visitUnprocessedBlock(_Block& block, const _StkState& incoming, size_t incomingBranchedFrom);

		bool _checkLogicalCoherence(_Block& block, const _StkState& incoming, size_t incomingBranchedFrom) const;
		bool _checkLocalCountCoherence(_Block& block, const _StkState& incoming, size_t incomingBranchedFrom) const;
		bool _checkArgCountCoherence(_Block& block, const _StkState& incoming, size_t incomingBranchedFrom) const;
		bool _checkLocalTypeCoherence(_Block& block, const _StkState& incoming, size_t incomingBranchedFrom) const;
		bool _checkArgTypeCoherence(_Block& block, const _StkState& incoming, size_t incomingBranchedFrom) const;
		
		bool _symbolicExec(_Block& block, const _StkState& incoming, size_t incomingBranchedFrom);
		bool _symbolicExecStep(size_t i, _Block& block, const _StkState& incoming, size_t incomingBranchedFrom);

		// Block-Level

		bool _checkForBlockLevelErrors(_Block& block) const;
		bool _checkBlockFinalInstrBranchIsInBounds(_Block& block) const;
		bool _checkBlockFinalInstrFallThroughIsInBounds(_Block& block) const;

		// Instruction-Level

		bool _check_L(_Block& block, size_t i, YmUInt8 field, YmChar letter) const;
		bool _check_Lw(_Block& block, size_t i, YmUInt8 field, YmChar letter, size_t localsPoppedByInstr) const;
		bool _check_Arg(_Block& block, size_t i, YmUInt8 field, YmChar letter) const;
		bool _check_Ko(_Block& block, size_t i, YmUInt8 field, YmChar letter) const;
		bool _check_Kt(_Block& block, size_t i, YmUInt8 field, YmChar letter) const;
		bool _check_Kt_hasDefaultValue(_Block& block, size_t i, YmUInt8 field, YmChar letter) const;
		bool _check_Kt_IsCallable(_Block& block, size_t i, YmUInt8 field, YmChar letter) const;
		bool _check_Kt_isCorrectKind(_Block& block, size_t i, YmUInt8 field, YmChar letter, YmKind k) const;
		bool _check_Kt_hasAssigner(_Block& block, size_t i, YmUInt8 field, YmChar letter) const;
		bool _checkLocalsPresent(_Block& block, size_t i, size_t locals) const;
		bool _checkLocalsArePositionalParamTypes(_Block& block, size_t i, YmType& called) const;
		bool _checkLocalIsCorrectType(_Block& block, size_t i, size_t where, YmType& type) const;
		bool _checkLocalCanConvertToType(_Block& block, size_t i, size_t where, YmType& target) const;

		YmType* _constToType(const Const& c) const noexcept;
	};
}

