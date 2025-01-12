

#pragma once


#include <string>
#include <vector>

#include "general.h"
#include "macros.h"
#include "newtop.h"


namespace yama::bc {


    // IMPORTANT:
    //      we're gonna employ a modified version of the bytecode
    //      static verif procedure outlined in our bytecode.txt
    //      design doc:
    // 
    //          1) by default, when symbol execution occurs, it occurs
    //             as normal, w/ instructions being checked to see that
    //             the types involved in their inputs/outputs are in line
    //             w/ the instruction's semantics
    // 
    //          2) instructions may be marked as 'reinitializing', which
    //             means that the rules in #2 are augmented to allow for
    //             the instruction to CHANGE the type of registers output
    //             to by *reinitializing* them
    //                  * instrs which use yama::newtop need-not be marked 'reinit',
    //                    but are allowed to be
    //
    //          3) the rule where a block which is branched to by some set
    //             of incoming blocks must all have the same object register
    //             type state is gonna be called 'register coherence', and
    //             thus a violation of this is a 'register coherence violation'
    // 
    //          4) unreachable, and thus dead, basic blocks will be tolerated
    //             (and thus should raise *warnings*)
    //              * TODO: what if there's illegal instrs in dead code blocks?
    //
    //          5) entrypoint block always begins at instruction 0 (pretty obvious)
    //
    //          6) the final block the bcode binary is partitioned into must
    //             not allow for control to fallthrough to the out-of-bounds
    //             instruction indices following the block
    //              * TODO: what should we do for dead code final blocks?
    //
    //          7) control-flow graphs composed of nothing but cyclical control
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
    //      A           : unsigned, 8-bit
    //      B           : unsigned, 8-bit
    //      C           : unsigned, 8-bit
    //      sBx         : signed,   16-bit (uses bits of B and C)

    // in opcode descs below, the following notation is used
    // to describe intent, w/ X specifying an oprand field:
    //      R(X)        : local register X
    //      R(top)      : top register on local object stack
    //      Arg(X)      : argument X
    //      Ko(X)       : object constant at X
    //      Kt(X)       : type constant at X

    enum class opcode : uint8_t {
        // noop

        // noop is the no-op instruction

        // panic conditions:
        //      n/a

        // static verif requires:
        //      n/a

        noop,

        // pop A

        // pop performs pop(A)

        // panic conditions:
        //      n/a

        // static verif requires:
        //      n/a

        pop,

        // put_none R(A)

        // put_none performs put_none(R(A))

        // panic conditions:
        //      n/a

        // static verif requires:
        //      - R(A) must be in-bounds (unless newtop)
        //      - R(A) must be of type None (unless reinit) (unless newtop)
        //      - pushing must not overflow (unless not newtop)

        put_none,

        // put_const R(A) Ko(B)

        // put_const performs put_const(R(A), Ko(B))

        // panic conditions:
        //      n/a

        // static verif requires:
        //      - R(A) must be in-bounds (unless newtop)
        //      - Ko(B) must be in-bounds
        //      - Ko(B) must be an object constant
        //      - R(A) and Ko(B) must agree on type (unless reinit) (unless newtop)
        //      - pushing must not overflow (unless not newtop)

        put_const,

        // put_arg R(A) Arg(B)

        // put_arg performs put_arg(R(A), Arg(B))

        // panic conditions:
        //      n/a

        // static verif requires:
        //      - R(A) must be in-bounds (unless newtop)
        //      - Arg(B) must be in-bounds
        //      - R(A) and Arg(B) must agree on type (unless reinit) (unless newtop)
        //      - pushing must not overflow (unless not newtop)

        put_arg,

        // copy R(A) R(B)

        // copy performs copy(R(A), R(B))

        // panic conditions:
        //      n/a

        // static verif requires:
        //      - R(A) must be in-bounds
        //      - R(B) must be in-bounds (unless newtop)
        //      - R(A) and R(B) must agree on type (unless reinit) (unless newtop)
        //      - pushing must not overflow (unless not newtop)

        copy,

        // call A R(B)

        // call performs call(A, R(B))

        // R(top-A) defines the 'call object'
        // [R(top-A), R(top)] defines the 'args' of the call
        // [R(top-A+1), R(top)] defines the 'param args' of the call

        // panic conditions:
        //      - if call behaviour panics
        //      - if call behaviour does not return anything
        //      - if call call stack would overflow

        // static verif requires:
        //      - arg registers must be in-bounds
        //      - arg registers must include at least one object (ie. A must be >=1)
        //      - R(top-A) must be legal to use as call object
        //      - param arg registers, if any, must be of number and param types expected by call object
        //      - R(B) must be in-bounds (after args are popped) (unless newtop)
        //      - R(B) must be of return type expected by call object (unless reinit) (unless newtop)
        //      - pushing must not overflow (unless not newtop)

        call,

        // call_nr A

        // call_nr performs call_nr(A)

        // R(top-A) defines the 'call object'
        // [R(top-A), R(top)] defines the 'args' of the call
        // [R(top-A+1), R(top)] defines the 'param args' of the call

        // panic conditions:
        //      - if call behaviour panics
        //      - if call behaviour does not return anything
        //      - if call call stack would overflow

        // static verif requires:
        //      - arg registers must be in-bounds
        //      - arg registers must include at least one object (ie. A must be >=1)
        //      - R(top-A) must be legal to use as call object
        //      - param arg registers, if any, must be of number and param types expected by call object

        call_nr,

        // ret R(A)

        // ret performs ret(R(A)), then instructs the interpreter to halt

        // panic conditions:
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

        // panic conditions:
        //      n/a

        // static verif requires:
        //      - program counter must remain in-bounds after sBx offset
        //      - the branch must not violate register coherence

        jump,

        // jump_true A sBx

        // jump_true conditionally jumps, adding sBx to program counter if the Bool
        // register R(top) is true, falling-through otherwise, and popping A registers
        // from the local object stack (as though via a pop instr) in either case

        // jump_true pops A registers to better ensure register coherence while also
        // playing nicely w/ common usage patterns where the register used to determine
        // where to branch is on the top of the local object stack

        // jump_true operates on the program counter AFTER it's been incremented
        // from reading the instruction itself

        // note that the above ONLY applies when considering sBx offsets, but
        // when describing branches *from* an instr, *to* another, the branched-from
        // instr is expected to be described as the origin point of the branch itself
        // (ie. where the offset is relative to is just a formality)

        // panic conditions:
        //      n/a

        // static verif requires:
        //      - R(top) must exist
        //      - R(top) must be of type Bool
        //      - program counter must remain in-bounds after sBx offset
        //      - the branch must not violate register coherence

        jump_true,

        // jump_false A sBx

        // jump_false conditionally jumps, adding sBx to program counter if the Bool
        // register R(top) is false, falling-through otherwise, and popping A registers
        // from the local object stack (as though via a pop instr) in either case

        // jump_false pops A registers to better ensure register coherence while also
        // playing nicely w/ common usage patterns where the register used to determine
        // where to branch is on the top of the local object stack

        // jump_false operates on the program counter AFTER it's been incremented
        // from reading the instruction itself

        // note that the above ONLY applies when considering sBx offsets, but
        // when describing branches *from* an instr, *to* another, the branched-from
        // instr is expected to be described as the origin point of the branch itself
        // (ie. where the offset is relative to is just a formality)

        // panic conditions:
        //      n/a

        // static verif requires:
        //      - R(top) must exist
        //      - R(top) must be of type Bool
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


        static_assert(opcodes == 12);

        code& add_noop();
        code& add_pop(uint8_t A);
        code& add_put_none(uint8_t A, bool reinit = false);
        code& add_put_const(uint8_t A, uint8_t B, bool reinit = false);
        code& add_put_arg(uint8_t A, uint8_t B, bool reinit = false);
        code& add_copy(uint8_t A, uint8_t B, bool reinit = false);
        code& add_call(uint8_t A, uint8_t B, bool reinit = false);
        code& add_call_nr(uint8_t A);
        code& add_ret(uint8_t A);
        code& add_jump(int16_t sBx);
        code& add_jump_true(uint8_t A, int16_t sBx);
        code& add_jump_false(uint8_t A, int16_t sBx);


        code& append(const code& x);


        static code concat(const code& a, const code& b);


    private:
        friend class code_writer;


        std::vector<instr> _instrs;
        std::vector<bool> _reinit_flags;


        void _reset();
    };


    // code_writer provides flexible and composable way of creating code objects by
    // letting branch instrs be written as stubs up front, which then get automatically
    // resolved later on using 'labels'

    // code_writer makes the process of complex code generation easier by decoupling
    // branch instrs from the exact sBx offsets they use

    // take note that 'labels' are identified by ID values, w/ the values used for these
    // being otherwise arbitrary, being only important in-so-far as they're all unique

    class code_writer final {
    public:

        using label_id_t = uint32_t;


        code_writer() = default;
        code_writer(const code_writer&) = delete;
        code_writer(code_writer&&) noexcept = default;
        ~code_writer() noexcept = default;
        code_writer& operator=(const code_writer&) = delete;
        code_writer& operator=(code_writer&&) noexcept = default;


        // TODO: count has not been unit tested

        // count returns instruction count

        size_t count() const noexcept;


        static_assert(opcodes == 12);

        code_writer& add_noop();
        code_writer& add_pop(uint8_t A);
        code_writer& add_put_none(uint8_t A, bool reinit = false);
        code_writer& add_put_const(uint8_t A, uint8_t B, bool reinit = false);
        code_writer& add_put_arg(uint8_t A, uint8_t B, bool reinit = false);
        code_writer& add_copy(uint8_t A, uint8_t B, bool reinit = false);
        code_writer& add_call(uint8_t A, uint8_t B, bool reinit = false);
        code_writer& add_call_nr(uint8_t A);
        code_writer& add_ret(uint8_t A);
        code_writer& add_jump(label_id_t label_id); // label_id may specify labels added later
        code_writer& add_jump_true(uint8_t A, label_id_t label_id); // label_id may specify labels added later
        code_writer& add_jump_false(uint8_t A, label_id_t label_id); // label_id may specify labels added later


        // add_label maps label_id to the current code write position, overwriting
        // any existing mapping

        // label_id may be any arbitrary integer

        code_writer& add_label(label_id_t label_id);


        // TODO: the behaviour of done returning std::nullopt due to sBx issues has
        //       not really been unit tested

        // done finishes code writing, returning the final code object created,
        // or std::nullopt if creation failed

        // done resets the code writer's state if successful

        // done will fail if a branch instr's label_id didn't correspond to any label

        // done will fail if a branch instr's sBx field value cannot handle the offset
        // resolved for it

        // done will, if it fails, and label_not_found != nullptr, set *label_not_found
        // to true if the issue was a missing label, and false if the issue was a sBx
        // field value offset overflow/underflow

        std::optional<code> done(bool* label_not_found = nullptr);


    private:
        code _result;
        std::unordered_map<label_id_t, size_t> _label_id_map; // maps label IDs to instr indices
        std::unordered_map<size_t, label_id_t> _label_id_user_map; // maps branch instr indices to IDs of labels they use


        size_t _write_pos() const noexcept;

        std::optional<size_t> _get_instr_index(label_id_t label_id);
        void _bind_label_id(label_id_t label_id);

        void _bind_label_id_user(label_id_t label_id);

        void _reset();
        bool _resolve(bool* label_not_found);
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

        // TODO: this overload has not been unit tested

        syms& add(sym x);


    private:

        std::unordered_map<size_t, sym> _syms;
    };
}

