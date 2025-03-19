

#pragma once


#include <vector>


namespace yama::internal {


    // cfg_analyzer performs control-flow analyzer upon the (implied) control-flow graph
    // of a translation unit of Yama code, which communicates this CFG via immediate-mode
    // method calls, over a single pass of the AST

    // analysis only considers control paths reachable from the entrypoint

    // analysis is performed using a scopal stack, w/ each entry encapsulating info about
    // the 'control path' which goes through a block of code, or a statement

    // paths are hierarchical and may have nested subpaths

    // example: path A w/ subpaths B and C
    
    //        |-- B -- B --|
    // -- A --|            |-- A --
    //        |-- C -- C --|


    class cfg_analyzer final {
    public:
        enum class path_type : uint8_t {
            fn,
            block,
            if_stmt,
            loop_stmt,
        };

        struct path final {
            path_type   type;

            // the number of subpaths of this path which are *guaranteed* to exit via
            // a break/continue/return stmt
            size_t      block_exits = 0;

            // the number of subpaths of this path which are *guaranteed* to exit via
            // a return stmt, or enter an infinite loop (which herein is treated as
            // equiv to an exit of the fn, in that block code thereafter is dead code)
            size_t      fn_exits    = 0;

            // the number of subpaths of this path which *might* exit via a break stmt
            size_t      breaks      = 0;


            inline bool is(path_type t) const noexcept { return type == t; }
            inline bool is_fn() const noexcept { return is(path_type::fn); }
            inline bool is_block() const noexcept { return is(path_type::block); }
            inline bool is_if_stmt() const noexcept { return is(path_type::if_stmt); }
            inline bool is_loop_stmt() const noexcept { return is(path_type::loop_stmt); }
        };


        cfg_analyzer() = default;


        bool is_in_fn() const noexcept; // returns if currently analyzing the CFG of a fn
        bool is_in_loop() const noexcept; // returns if code at this point in the CFG is nested in a loop stmt
        bool is_in_dead_code() const noexcept; // returns if code at this point in the CFG is *definitely* dead code

        path& current() noexcept;
        const path& current() const noexcept; // returns current path at this point in the CFG

        // check_fn returns if can *guarantee* all paths of current fn reach return stmt, or
        // enter infinite loop

        bool check_fn() const noexcept;

        // begin_fn/end_fn designate the begin/end of the CFG of a fn

        // these are designed to allow for easily analysis of lambda fns by letting end-user
        // nest the lambda fn's method call sequence inside that of the fn it's nested within

        // when used like this, the lambda fn's CFG analysis is *logically seperate* from
        // that of the fn it's nested within, w/ analysis of said fn essentially *pausing*

        void begin_fn();
        void end_fn();

        void break_stmt();
        void continue_stmt();
        void return_stmt();

        void begin_block();
        void end_block();

        void begin_if_stmt();
        void end_if_stmt();

        void begin_loop_stmt();
        void end_loop_stmt();


    private:
        std::vector<path> _stk;
        size_t _loop_scopes = 0;


        void _assert_top() const noexcept;
        void _assert_top(path_type t) const noexcept;

        void _push(path_type t);
        path _pop();

        void _push_loop();
        void _pop_loop();
    };
}

