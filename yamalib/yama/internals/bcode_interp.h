

#pragma once


#include "../core/bcode.h"
#include "../core/const_table.h"
#include "../core/context.h"


namespace yama::internal {


    struct bcode_interp final {
        context* ctx_ptr;
        const_table consts;

        const bc::code* bcode_ptr;
        bc::instr curr = bc::instr{};
        size_t pc = 0;
        bool should_halt = false;


        context& ctx();
        const bc::code& bcode();

        void halt();
        void jump(int16_t offset);


        void fire();

        void setup();
        void acquire_bcode();
        void fetch_instr();
        void exec_instr();
        void halt_if_crashing();
    };
}

