

#pragma once


#include <vector>

#include "safeptr.h"
#include "ast.h"
#include "ctypesys.h"


namespace yama::internal {


    class translation_unit;


    // register_stk keeps track of the state of the register stack of Yama code
    // during the compilation process, for sake of register allocation, type
    // checking, and discerning the max locals needed by a fn

    class register_stk final {
    public:
        // IMPORTANT:
        //      we impose a limit of 254 registers allocated at a time, w/ us raising an
        //      error if this limit is exceeded
        //
        //      the rest of the system, however, doesn't see this limit, and will simply
        //      continue to operate w/out regards for this limit
        //
        //      when writing bcode during codegen, writing stack indices into register fields
        //      A, B, and C, which are >254, will result in a mangled value being written
        //      as a result of the overflow from casting to uint8_t
        //
        //      the reason 254 was chosen instead of 255 is to ensure that no issues occur
        //      w/ the yama::newtop constant

        static constexpr size_t reg_limit = size_t((uint8_t)yama::newtop) - 1;


        // the stack holds two types of registers: 'temporaries' and 'local vars'

        struct reg_t final {
            ctype   type;                   // type encapsulated by the register
            bool    localvar    = false;    // if this register is a local var
            size_t  index       = 0;        // the stack index of this register


            inline bool is_temp() const noexcept { return !localvar; }
            inline bool is_localvar() const noexcept { return !is_temp(); }
        };

        struct scope_t final {
            size_t  first_reg   = 0;        // the first reg on stack which belongs to this scope
        };


        safeptr<translation_unit> tu;


        register_stk(translation_unit& tu);


        size_t regs() const noexcept;

        size_t reg_abs_index(ssize_t index); // takes relative index x and return absolute index

        reg_t& reg_abs(size_t index);
        reg_t& reg(ssize_t index); // negative values index from top downward (in Lua style)

        reg_t& top_reg();
        scope_t& top_scope();


        // IMPORTANT: all ssize_t indices below operate using relative indices

        // IMPORTANT: pop_temp can auto-write pop instr, but push_temp does not auto-write
        //            instr pushing the temporary

        void push_temp(const ast_node& x, ctype type);
        void pop_temp(size_t n, bool write_pop_instr); // expects autosym will add correct instr sym

        void push_scope();
        void pop_scope(bool write_pop_instr); // unwinds local vars, expects autosym will add correct instr sym

        void reinit_temp(ssize_t index, ctype new_type); // changes type of existing temporaries/local vars
        void promote_to_localvar(ast_VarDecl& x); // *promotes* top temporary to a local var, updating its symbol table entry
        bool type_check_reg(ssize_t index, ctype expected); // type checks a register


    private:
        std::vector<reg_t> _regstk;
        std::vector<scope_t> _scopestk;


        void _update_max_locals_of_codegen_target();
    };
}

