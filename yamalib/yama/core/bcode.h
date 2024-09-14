

#pragma once


#include <string>
#include <vector>

#include "general.h"
#include "macros.h"


namespace yama::bc {


    // IMPORTANT:
    //      we're gonna employ a modified version of the bytecode
    //      static verif procedure outlined in our bytecode.txt
    //      design doc:
    // 
    //          1) instead of each basic block having a *stack* of
    //             object registers, they'll have a fixed-size arrays 
    //             of registers, w/ the entrypoint block's array's
    //             registers being initialized to the None object
    // 
    //          2) by default, when symbol execution occurs, it occurs
    //             as normal, w/ instructions being checked to see that
    //             the types involved in their inputs/outputs are in line
    //             w/ the instruction's semantics
    // 
    //          3) instructions may be marked as 'reinitializing', which
    //             means that the rules in #2 are augmented to allow for
    //             the instruction to CHANGE the type of registers output
    //             to by *reinitializing* them
    //
    //          4) the rule where a block which is branched to by some set
    //             of incoming blocks must all have the same object register
    //             type state is gonna be called 'register coherence', and
    //             thus a violation of this is a 'register coherence violation'
    // 
    //          5) unreachable, and thus dead, basic blocks will be tolerated
    //             (and thus should raise *warnings*)
    //              * TODO: what if there's illegal instrs in dead code blocks?
    //
    //          6) entrypoint block always begins at instruction 0 (pretty obvious)
    //
    //          7) the final block the bcode binary is partitioned into must
    //             not allow for control to fallthrough to the out-of-bounds
    //             instruction indices following the block
    //              * TODO: what should we do for dead code final blocks?
    //
    //          8) control-flow graphs composed of nothing but cyclical control
    //             paths (ie. no 'exitpoint' blocks) are legal, as the infinite
    //             loop they create means there won't be any issue of the wrong
    //             type of object being returned
    //
    //             this might seem silly, and while undesirable to have no
    //             exitpoints, our system nevertheless does nothing to prevent
    //             any other sort of infinite loop from forming, so these don't
    //             pose any kind of unique problem to our system (as code which
    //             can never return need-not worry about what it's nonexistent
    //             ret instr returns, lol)


    // each instruction is 32-bit, w/ the first 8 bits encoding
    // the opcode, and the remaining 24 encoding oprand fields

    // instructions may use the following oprand fields:
    //      A       : unsigned, 8-bit
    //      B       : unsigned, 8-bit
    //      C       : unsigned, 8-bit
    //      sBx     : signed,   16-bit (uses bits of B and C)

    // in opcode descs below, the following notation is used
    // to describe intent, w/ X specifying an oprand field:
    //      R(X)    : local register X
    //      Arg(X)  : argument X
    //      Ko(X)   : object constant at X
    //      Kt(X)   : type constant at X

    enum class opcode : uint8_t {
        // noop

        // noop is the no-op instruction

        // crash conditions:
        //      n/a

        // static verif requires:
        //      n/a

        noop,

        // load_none R(A)

        // load_none performs ll_load_none(R(A))

        // crash conditions:
        //      n/a

        // static verif requires:
        //      - R(A) must be in-bounds
        //      - R(A) must be of type None (unless reinit)

        load_none,

        // load_const R(A) Ko(B)

        // load_const performs ll_load_const(R(A), Ko(B))

        // crash conditions:
        //      n/a

        // static verif requires:
        //      - R(A) must be in-bounds
        //      - Ko(B) must be in-bounds
        //      - Ko(B) must be an object constant
        //      - R(A) and Ko(B) must agree on type (unless reinit)

        load_const,

        // load_arg R(A) Arg(B)

        // load_arg performs ll_load_arg(R(A), Arg(B))

        // crash conditions:
        //      n/a

        // static verif requires:
        //      - R(A) must be in-bounds
        //      - Arg(B) must be in-bounds
        //      - R(A) and Arg(B) must agree on type (unless reinit)

        load_arg,

        // copy R(A) R(B)

        // copy performs ll_copy(R(A), R(B))

        // crash conditions:
        //      n/a

        // static verif requires:
        //      - R(A) must be in-bounds
        //      - R(B) must be in-bounds
        //      - R(A) and R(B) must agree on type (unless reinit)

        copy,

        // call R(A) B R(C)

        // call performs ll_call(R(A), B, R(C))

        // R(A) defines the 'call object'
        // [R(A), R(A+B-1)] defines the 'arg registers' of the call
        // [R(A+1), R(A+B-1)] defines the 'param arg registers' of the call

        // crash conditions:
        //      - if call behaviour crashes

        // static verif requires:
        //      - arg registers must be in-bounds
        //      - arg registers must include at least one object (ie. B must be >=1)
        //      - R(A) must be legal to use as call object
        //      - param arg registers, if any, must be of number and param types expected by call object
        //      - R(C) must be in-bounds
        //      - R(C) must be of return type expected by call object (unless reinit)

        call,

        // call_nr R(A) B

        // call_nr performs ll_call(R(A), B, no_result)

        // R(A) defines the 'call object'
        // [R(A), R(A+B-1)] defines the 'args' of the call
        // [R(A+1), R(A+B-1)] defines the 'param args' of the call

        // crash conditions:
        //      - if call behaviour crashes

        // static verif requires:
        //      - arg registers must be in-bounds
        //      - arg registers must include at least one object (ie. B must be >=1)
        //      - R(A) must be legal to use as call object
        //      - param arg registers, if any, must be of number and param types expected by call object

        call_nr,

        // ret R(A)

        // ret performs ll_ret(R(A)), then instructs the interpreter to halt

        // crash conditions:
        //      n/a

        // static verif requires:
        //      - R(A) must be in-bounds
        //      - R(A) must be of return type expected by call object

        ret,

        // jump sBx

        // jump unconditionally jumps, adding sBx to program counter

        // jump operates on the program counter AFTER it's been incremented
        // from reading the instruction itself
        //
        // take note that this ONLY applies when considering sBx offsets,
        // so for describing branches *from* an instr, *to* another, the 
        // branched-from instr reported is expected to be the branching 
        // instr itself (ie. NOT the instr *after* it)

        // crash conditions:
        //      n/a

        // static verif requires:
        //      - program counter must remain in-bounds after sBx offset
        //      - the branch must not violate register coherence

        jump,

        // jump_true R(A) sBx

        // jump_true conditionally jumps, adding sBx to program counter if 
        // Bool in R(A) is true, falling-through otherwise

        // jump_true operates on the program counter AFTER it's been incremented
        // from reading the instruction itself
        //
        // take note that this ONLY applies when considering sBx offsets,
        // so for describing branches *from* an instr, *to* another, the 
        // branched-from instr reported is expected to be the branching 
        // instr itself (ie. NOT the instr *after* it)

        // crash conditions:
        //      n/a

        // static verif requires:
        //      - R(A) must be in-bounds
        //      - R(A) must be of type Bool
        //      - program counter must remain in-bounds after sBx offset
        //      - the branch must not violate register coherence

        jump_true,

        // jump_false R(A) sBx

        // jump_false conditionally jumps, adding sBx to program counter if 
        // Bool in R(A) is false, falling-through otherwise

        // jump_false operates on the program counter AFTER it's been incremented
        // from reading the instruction itself
        //
        // take note that this ONLY applies when considering sBx offsets,
        // so for describing branches *from* an instr, *to* another, the 
        // branched-from instr reported is expected to be the branching 
        // instr itself (ie. NOT the instr *after* it)

        // crash conditions:
        //      n/a

        // static verif requires:
        //      - R(A) must be in-bounds
        //      - R(A) must be of type Bool
        //      - program counter must remain in-bounds after sBx offset
        //      - the branch must not violate register coherence

        jump_false,


        num, // this is not a valid opcode
    };

    constexpr auto opcodes = size_t(opcode::num);

    std::string fmt_opcode(opcode x);
}


YAMA_SETUP_FORMAT(yama::bc::opcode, yama::bc::fmt_opcode(x));


namespace yama::bc {


    // instr encapsulates a single Yama bcode instruction

    struct instr final {
        opcode opc;
        uint8_t A;
        union {
            struct {
                uint8_t B;
                uint8_t C;
            };
            int16_t sBx;
        };
    };

    static_assert(sizeof(instr) == sizeof(int32_t));
    static_assert(alignof(instr) <= alignof(int32_t));


    // code encapsulates a Yama bcode binary

    class code final {
    public:

        code() = default;
        code(const code&) = default;
        code(code&&) noexcept = default;
        ~code() noexcept = default;
        code& operator=(const code&) = default;
        code& operator=(code&&) noexcept = default;


        // count returns instruction count

        size_t count() const noexcept;

        // instr returns the instruction at x

        // behaviour is undefined if x is out-of-bounds

        instr get(size_t x) const noexcept;
        instr operator[](size_t x) const noexcept;

        // reinit_flag returns if the instruction at x has a reinitialize flag

        // behaviour is undefined if x is out-of-bounds

        bool reinit_flag(size_t x) const noexcept;


        std::string fmt_instr(size_t x, const char* tab = "    ") const;
        std::string fmt_disassembly(const char* tab = "    ") const;


        static_assert(opcodes == 11);

        code& add_noop();
        code& add_load_none(uint8_t A, bool reinit = false);
        code& add_load_const(uint8_t A, uint8_t B, bool reinit = false);
        code& add_load_arg(uint8_t A, uint8_t B, bool reinit = false);
        code& add_copy(uint8_t A, uint8_t B, bool reinit = false);
        code& add_call(uint8_t A, uint8_t B, uint8_t C, bool reinit = false);
        code& add_call_nr(uint8_t A, uint8_t B);
        code& add_ret(uint8_t A);
        code& add_jump(int16_t sBx);
        code& add_jump_true(uint8_t A, int16_t sBx);
        code& add_jump_false(uint8_t A, int16_t sBx);


    private:

        std::vector<instr> _instrs;
        std::vector<bool> _reinit_flags;
    };


    struct sym final {
        size_t  index;  // the index of the instr this symbol
        str     origin; // the string (eg. a file path) describing where the contents referenced by the symbol resides
        size_t  ch;     // the character number (indexes from 1)
        size_t  ln;     // the line number (indexes from 1)


        bool operator==(const sym&) const noexcept = default;


        std::string fmt() const;
    };
}


YAMA_SETUP_FORMAT(yama::bc::sym, x.fmt());


namespace yama::bc {


    class syms final {
    public:

        syms() = default;
        syms(const syms&) = default;
        syms(syms&&) noexcept = default;
        ~syms() noexcept = default;
        syms& operator=(const syms&) = default;
        syms& operator=(syms&&) noexcept = default;


        // fetch fetches the symbol for the instr at index, if any

        std::optional<sym> fetch(size_t index) const noexcept;
        std::optional<sym> operator[](size_t index) const noexcept;


        // add adds a new symbol for the instr at index, overwriting
        // any existing association

        // ch and ln index from 1, not 0, however this being respected
        // by the API users is not enforced

        // index does not need to be in-bounds of the code object these
        // symbols are being used w/, as that association is a convention

        syms& add(size_t index, str origin, size_t ch, size_t ln);


    private:

        std::unordered_map<size_t, sym> _syms;
    };
}

