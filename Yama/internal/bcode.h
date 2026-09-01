

#pragma once


#include <format>
#include <iostream>
#include <optional>
#include <string>
#include <unordered_map>
#include <vector>

#include "../yama/yama.h"


namespace _ym {


    // TODO: Doesn't bPush/bDiscard taking up two YmUInt8 values mean that
    //       we need to impose a 254 limit on local count? As otherwise
    //       writes to them will be misinterpreted.
    // TODO: Also, we should account for issue of what happens if pushing
    //       causes obj stack overflow during bcode exec.

    // TODO: When we add Knl(X), we're gonna need to update our unit tests
    //       for bcode verif to account for Ko(X) being able to be invalid
    //       due to non-object consts, rather than just from out-of-bounds.
    //          * I have TODO stub tests to be filled out later.

    // Each instruction is 32-bit, w/ the first 8 bits encoding
    // the opcode, and the remaining 24 encoding oprand fields.
    //
    // Instructions may use the following oprand fields:
    //      A           : Unsigned, 8-bit
    //      B           : Unsigned, 8-bit
    //      C           : Unsigned, 8-bit
    //      sBx         : Signed,   16-bit (Uses bits of B and C.)
    //
    // In opcode descs below, the following notation is used
    // to describe intent, w/ X specifying an oprand field:
    //      L(X)        : Local X.
    //      Lw(X)       : Local X (for write; may use bPush/bDiscard.)
    //      Arg(X)      : Arg X.
    //      Ko(X)       : Object constant at X.
    //      Kt(X)       : Type constant at X.
    //		Knl(X)		: Name list constant at X.
    //
    // Below, in 'Verif. Rules', rules saying the above are
    // 'valid' means that their index is in-bounds, and in case
    // of constants, the constant is the expected type.
    enum class Opcode : YmUInt8 {
        // Synopsis: noop
        // StkFx: --
        // No-op.
        // Verif. Rules:
        //      n/a
        noop,

        // Synopsis: pop A
        // StkFx: ...topN --
        // Pops topN objects (w/ number specified by A.)
        // Verif. Rules:
        //      n/a
        pop,

        // Synopsis: putNone Lw(A)
        // StkFx: -- result->Lw(A)
        // Puts None object onto the stack.
        // Verif. Rules:
        //      - Lw(A) is valid.
        putNone,

        // Synopsis: putConst Ko(A) Lw(B)
        // StkFx: -- result->Lw(B)
        // Puts Ko(A) onto the stack.
        // Verif. Rules:
        //      - Ko(A) is valid.
        //      - Lw(B) is valid.
        putConst,

        // TODO: I don't think our verifier is capable of allowing us to impl
        //       a 'setArg' instr which modifies arg type, for sake of the impl
        //       of named args.
        //
        //       Think about it: if the idea is to have named args be either
        //       their value type, or None, and then have special bcode which
        //       replaces None args w/ default arg values, then this presents
        //       a MAJOR problem in that our args start off w/ AMBIGUOUS TYPES!
        //       which breaks our verifier...

        // Synopsis: putArg Arg(A) Lw(B)
        // StkFx: -- result->Lw(B)
        // Puts Arg(A) onto the stack.
        // Verif. Rules:
        //      - Arg(A) is valid.
        //      - Lw(B) is valid.
        putArg,

        // Synopsis: copy L(A) Lw(B)
        // StkFx: -- resultb->Lw(B)
        // Copies L(A) into Lw(B).
        // Verif. Rules:
        //      - L(A) is valid.
        //      - Lw(B) is valid.
        copy,

        // Synopsis: defaultInit Kt(A) Lw(B)
        // StkFx: -- result->Lw(B)
        // Defaults inits an object of Kt(A).
        // Verif. Rules:
        //      - Kt(A) is valid.
        //      - Kt(A) has no default value.
        //      - Lw(B) is valid.
        defaultInit,

        // TODO: Add later.

        // Synopsis: structInit Kt(A) Knl(B) Lw(C)
        // StkFx: ...args -- result->Lw(C)
        // Struct inits an object of Kt(A), using args.
        // Knl(B) specifies the layout of args.
        // Verif. Rules:
        //      - Kt(A) is valid.
        //      - Kt(A) is a struct type.
        //      - Kt(A) can be struct init w/ Knl(B).
        //      - Knl(B) is valid.
        //      - Lw(C) is valid.
        //      - args are present.
        //      - args are correct types.
        //structInit,

        // Synopsis: pcall Kt(A) Lw(B)
        // StkFx: ...args -- result->Lw(B)
        // Performs a call to Kt(A) using args (number specified by Kt(A).)
        // Only positional args are provided.
        // Verif. Rules:
        //      - Kt(A) is valid.
        //      - Kt(A) is callable.
        //      - Lw(B) is valid.
        //      - args are present.
        //      - args are correct types.
        pcall,

        // TODO: Add later.

        // Synopsis: pncall Kt(A) Knl(B) Lw(C)
        // StkFx: ...args -- result->Lw(C)
        // Performs a call to Kt(A) using args (number specified by Kt(A) and Knl(B).)
        // Knl(B) specifies the named args of the call.
        // Verif. Rules:
        //      - Kt(A) is valid.
        //      - Kt(A) is callable.
        //      - Kt(A) can be used w/ Knl(B).
        //      - Knl(B) is valid.
        //      - Lw(C) is valid.
        //      - args are present.
        //      - args are correct types.
        //pncall,

        // Synopsis: ret
        // StkFx: value -- value
        // Binds return value, then instructs interpreter to halt.
        // Verif. Rules:
        //      - value is present.
        //      - value is correct type.
        ret,

        // Synopsis: getVar Kt(A) Lw(B)
        // StkFx: -- result->Lw(B)
        // Gets var Kt(A).
        // Verif. Rules:
        //      - Kt(A) is valid.
        //      - Kt(A) is a var type.
        //      - Lw(B) is valid.
        getVar,

        // Synopsis: setVar Kt(A)
        // StkFx: value --
        // Sets value to var Kt(A).
        // Verif. Rules:
        //      - Kt(A) is valid.
        //      - Kt(A) is a var type.
        //      - Kt(A) has an assigner.
        //      - value is present.
        //      - value is correct type.
        setVar,

        // Synopsis: getProp Kt(A) Lw(B)
        // StkFx: subject -- result->Lw(B)
        // Gets property Kt(A) in subject.
        // Verif. Rules:
        //      - Kt(A) is valid.
        //      - Kt(A) is a property type.
        //      - Lw(B) is valid.
        //      - subject is present.
        //      - subject is correct type.
        getProp,

        // Synopsis: setProp Kt(A)
        // StkFx: subject value --
        // Sets value to property Kt(A) in subject.
        // Verif. Rules:
        //      - Kt(A) is valid.
        //      - Kt(A) is a property type.
        //      - Kt(A) has an assigner.
        //      - subject and value are present.
        //      - subject is correct type.
        //      - value is correct type.
        setProp,

        // TODO: But... what if conversion fails? We haven't really added
        //		 panicking yet.

        // NOTE: The distinction between explicit/implicit conversions isn't
        //		 important at bcode static verif level. So all conversions at
        //		 this level are 'explicit'.
        //
        //		 For 'dynamic' converions to/from/between protocols, where
        //		 we can't 100% know until runtime if it'll succeed, such
        //		 conversion are legal at static verif level.

        // Synopsis: Kt(A) Lw(B)
        // StkFx: top -- result->Lw(B)
        // Converts top to Kt(A).
        // Verif. Rules:
        //      - Kt(A) is valid.
        //      - Lw(B) is valid.
        //      - top is present.
        //      - top is statically known to convert to Kt(A).
        conv,

        // Synopsis: jump sBx
        // StkFx: --
        // Adds sBx to program counter.
        // Program counter is updated after instr read incrs it.
        // Verif. Rules:
        //      - sBx jump is in-bounds.
        jump,

        // Synopsis: jumpTrue sBx
        // StkFx: cond --
        // Adds sBx to program counter if Bool cond is true, falling through otherwise.
        // Program counter is updated after instr read incrs it.
        // Verif. Rules:
        //      - sBx jump is in-bounds.
        //      - cond is present.
        //      - cond is type Bool.
        jumpTrue,

        // Synopsis: jumpFalse sBx
        // StkFx: cond --
        // Adds sBx to program counter if Bool cond is false, falling through otherwise.
        // Program counter is updated after instr read incrs it.
        // Verif. Rules:
        //      - sBx jump is in-bounds.
        //      - cond is present.
        //      - cond is type Bool.
        jumpFalse,

        num, // Not a valid Opcode.
    };

    constexpr YmUInt8 Opcodes = (YmUInt8)Opcode::num;

    constexpr const YmChar* fmt(Opcode opc) {
        static_assert(Opcodes == 17);
        switch (opc) {
        case Opcode::noop:			return "noop";
        case Opcode::pop:			return "pop";
        case Opcode::putNone:		return "putNone";
        case Opcode::putConst:		return "putConst";
        case Opcode::putArg:		return "putArg";
        case Opcode::copy:			return "copy";
        case Opcode::defaultInit:	return "defaultInit";
        //case Opcode::structInit:	return "structInit";
        case Opcode::pcall:			return "pcall";
        //case Opcode::pncall:		return "pncall";
        case Opcode::ret:			return "ret";
        case Opcode::getVar:		return "getVar";
        case Opcode::setVar:		return "setVar";
        case Opcode::getProp:		return "getProp";
        case Opcode::setProp:		return "setProp";
        case Opcode::conv:			return "conv";
        case Opcode::jump:			return "jump";
        case Opcode::jumpTrue:		return "jumpTrue";
        case Opcode::jumpFalse:		return "jumpFalse";
        default:					return "<Unknown>";
        }
    }
}

template<>
struct std::formatter<_ym::Opcode> : std::formatter<std::string> {
    auto format(const _ym::Opcode& x, format_context& ctx) const {
        return formatter<string>::format(_ym::fmt(x), ctx);
    }
};
namespace std {
    inline std::ostream& operator<<(std::ostream& stream, const _ym::Opcode& x) {
        return stream << _ym::fmt(x);
    }
}

namespace _ym {


    // TODO: If we invest in an immutable string type again, look into using
    //		 it to avoid the current 'bloat' of BCodeDbgSym::origin.

    struct BCodeDbgSym final {
        size_t		index;  // The index of the instr of this symbol.
        std::string origin; // The string (eg. a file path) describing where the contents referenced by the symbol resides.
        size_t		ln;     // The line number (indexes from 1.)
        size_t		ch;     // The character number (indexes from 1.)


        bool operator==(const BCodeDbgSym&) const noexcept = default;

        std::string fmt() const;
    };

    class BCodeDbgSyms final {
    public:
        BCodeDbgSyms() = default;
        ~BCodeDbgSyms() noexcept = default;
        BCodeDbgSyms(const BCodeDbgSyms&) = default;
        BCodeDbgSyms(BCodeDbgSyms&&) noexcept = default;
        BCodeDbgSyms& operator=(const BCodeDbgSyms&) = default;
        BCodeDbgSyms& operator=(BCodeDbgSyms&&) noexcept = default;


        size_t size() const noexcept;
        const BCodeDbgSym* symbol(size_t index) const noexcept;

        void add(BCodeDbgSym symbol);


    private:
        std::unordered_map<size_t, BCodeDbgSym> _symbols;
    };


    // The bcode oprand equiv of YM_PUSH.
    constexpr YmUInt8 bPush = YmUInt8(-1);
    // The bcode oprand equiv of YM_DISCARD.
    constexpr YmUInt8 bDiscard = YmUInt8(-2);

    struct BCodeInstr final {
        Opcode opc;
        YmUInt8 A;
        union {
            struct {
                YmUInt8 B;
                YmUInt8 C;
            };
            YmInt16 sBx;
        };


        static inline BCodeInstr mk1(Opcode opc, YmUInt8 A = 0, YmUInt8 B = 0, YmUInt8 C = 0) noexcept {
            BCodeInstr result{ .opc = opc, .A = A };
            result.B = B;
            result.C = C;
            return result;
        }
        static inline BCodeInstr mk2(Opcode opc, YmUInt8 A, YmInt16 sBx) noexcept {
            BCodeInstr result{ .opc = opc, .A = A };
            result.sBx = sBx;
            return result;
        }
    };

    static_assert(sizeof(BCodeInstr) == sizeof(YmUInt32));

    class BCode final {
    public:
        BCode() = default;
        ~BCode() noexcept = default;
        BCode(const BCode&) = default;
        BCode(BCode&&) noexcept = default;
        BCode& operator=(const BCode&) = default;
        BCode& operator=(BCode&&) noexcept = default;


        bool operator==(const BCode&) const noexcept = default;

        size_t size() const noexcept;
        BCodeInstr& operator[](size_t index) noexcept;
        const BCodeInstr& operator[](size_t index) const noexcept;

        std::vector<BCodeInstr>::const_iterator begin() const noexcept;
        std::vector<BCodeInstr>::const_iterator end() const noexcept;

        // Returns the instr which the instr at index branches to, if any.
        std::optional<size_t> branchDest(size_t index) const noexcept;

        // Provides reasonable fmt string for both indices w/ and w/out
        // associated symbols.
        std::string fmtSymbol(size_t index, const BCodeDbgSyms* symbols = nullptr) const;
        std::string fmtInstr(size_t index, const BCodeDbgSyms* symbols = nullptr) const;
        std::string fmtDisassembly(const BCodeDbgSyms* symbols = nullptr) const;

        void push(BCodeInstr instr);
        void shrinkToFit();


    private:
        std::vector<BCodeInstr> _instrs;


        std::string _fmtInstr(size_t index) const;
    };


    // BCodeWriter provides flexible and composable way of creating bcode objects by
    // letting branch instrs be written as stubs up front, which then get automatically
    // resolved later on using 'labels'.
    // BCodeWriter makes the process of complex code generation easier by decoupling
    // branch instrs from the exact sBx offsets they use.
    // Take note that 'labels' are identified by ID values, w/ the values used for these
    // being otherwise arbitrary, being only important in-so-far as they're all unique.
    class BCodeWriter final {
    public:
        using LabelID = YmUInt32;


        BCodeWriter(BCodeDbgSyms* symbols = nullptr);

        ~BCodeWriter() noexcept = default;
        BCodeWriter(const BCodeWriter&) = delete;
        BCodeWriter(BCodeWriter&&) noexcept = default;
        BCodeWriter& operator=(const BCodeWriter&) = delete;
        BCodeWriter& operator=(BCodeWriter&&) noexcept = default;


        size_t size() const noexcept;

        static_assert(Opcodes == 17);
        BCodeWriter& addNoop();
        BCodeWriter& addPop(YmUInt8 A);
        BCodeWriter& addPutNone(YmUInt8 LwA);
        BCodeWriter& addPutConst(YmUInt8 KoA, YmUInt8 LwB);
        BCodeWriter& addPutArg(YmUInt8 ArgA, YmUInt8 LwB);
        BCodeWriter& addCopy(YmUInt8 LA, YmUInt8 LwB);
        BCodeWriter& addDefaultInit(YmUInt8 KtA, YmUInt8 LwB);
        //BCodeWriter& addStructInit(YmUInt8 KtA, YmUInt8 KnlB, YmUInt8 LwC);
        BCodeWriter& addPCall(YmUInt8 KtA, YmUInt8 LwB);
        //BCodeWriter& addPNCall(YmUInt8 KtA, YmUInt8 KnlB, YmUInt8 LwC);
        BCodeWriter& addRet();
        BCodeWriter& addGetVar(YmUInt8 KtA, YmUInt8 LwB);
        BCodeWriter& addSetVar(YmUInt8 KtA);
        BCodeWriter& addGetProp(YmUInt8 KtA, YmUInt8 LwB);
        BCodeWriter& addSetProp(YmUInt8 KtA);
        BCodeWriter& addConv(YmUInt8 KtA, YmUInt8 LwB);
        BCodeWriter& addJump(LabelID label);
        BCodeWriter& addJumpTrue(LabelID label);
        BCodeWriter& addJumpFalse(LabelID label);

        // addLabel maps id to the current code write position, overwriting
        // any existing mapping.
        // id may be any arbitrary integer.
        BCodeWriter& addLabel(LabelID id);

        // NOTE: These set the symbol which will be written for each instruction added.

        // Index field is replaced during symbol output.
        BCodeWriter& setSymbol(BCodeDbgSym&& symbol);
        BCodeWriter& setSymbol(std::string origin, size_t ln, size_t ch);
        BCodeWriter& unsetSymbol() noexcept;

        // done finishes bcode writing, returning the final code object created,
        // or std::nullopt if creation failed.
        // Resets BCodeWriter state if successful.
        // Fail if a branch instr's label_id didn't correspond to any label.
        // Fail if a branch instr's sBx field value cannot handle the offset
        // resolved for it.
        // If done fails, and label_not_found != nullptr, set *label_not_found to true
        // if the issue was a missing label, and false if the issue was a sBx field
        // value offset overflow/underflow.
        std::optional<BCode> done(bool* labelNotFound = nullptr);


    private:
        BCodeDbgSyms* _symbols = nullptr;
        std::optional<BCodeDbgSym> _current;
        BCode _bcode;
        // Maps label IDs to their instrs.
        std::unordered_map<LabelID, size_t> _labelMap;
        // Maps label using instrs to their label IDs.
        std::unordered_map<size_t, LabelID> _labelUserMap;


        size_t _writePos() const noexcept;
        std::optional<size_t> _instrIndex(LabelID id) const noexcept;

        void _bindLabel(LabelID id);
        void _bindLabelUser(LabelID id);
        void _outputSymbol();
        void _reset();

        bool _resolveStubs(bool* labelNotFound);
        bool _resolveStub(size_t instrToPatch, LabelID instrLabel, bool* labelNotFound);
        std::optional<size_t> _labelInstrIndex(LabelID instrLabel, bool* labelNotFound) const noexcept;
        struct _JumpInfo final {
            size_t low, high, diff = 0;
            bool isBackwardsJump;
        };
        std::optional<YmInt16> _sBxOf(std::optional<_JumpInfo> info, bool* labelNotFound) const noexcept;
        std::optional<YmInt16> _sBxOfForwardJump(const _JumpInfo& info, bool* labelNotFound) const noexcept;
        std::optional<YmInt16> _sBxOfBackwardJump(const _JumpInfo& info, bool* labelNotFound) const noexcept;
        static std::optional<_JumpInfo> _jumpInfo(size_t instrToPatch, std::optional<size_t> labelInstrIndex) noexcept;
        static void _labelNotFoundError(bool* labelNotFound) noexcept;
        static void _sBxOverflowOrUnderflowError(bool* labelNotFound) noexcept;
    };
}

