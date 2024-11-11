

#pragma once


#include "scalars.h"
#include "res.h"
#include "api_component.h"
#include "domain.h"
#include "object_ref.h"

#include "const_table.h"


namespace yama {


    // TODO: in the future, we're gonna need to impl a way to allow for things
    //       like destructors to be able to run when panicking, w/ them likewise
    //       having a notion of they themselves being able to panic

    // TODO: we're also gonna have to have cleanup semantics for other forms of
    //       object state, like *global* objects, when we add things like that

    // IMPORTANT:
    //      (call stack & call frames)
    // 
    //      at the heart of the Yama context is the 'call stack', which is
    //      composed of 'call frames'
    // 
    //      each call frame encapsulates a 'local register table', or simply
    //      its 'local registers', or simply its 'locals'
    // 
    //      local register tables encapsulate the local state of a call, 
    //      including things such as local variables, arguments, etc.
    // 
    //      (the 'user' call frame)
    // 
    //      at the base of the call stack is a special 'user' call frame,
    //      which exists to provide a local register table to use when operating
    //      outside of an actual call
    // 
    //      the user call frame is part of a pseudo-call which never *ends*,
    //      and is not actually a real call
    // 
    //      (call objects)
    // 
    //      Yama not only allows for first-class functions, but in fact 
    //      requires all calls to be associated w/ an object encapsulating
    //      the callable type
    // 
    //      for regular functions, these come in the form of stateless objects
    //      which are of the function's type
    // 
    //      (register-based vs. stack-based VMs)
    //
    //      Yama uses a 'register-based' VM, rather than a 'stack-based' one
    // 
    //      this means that Yama uses 'local register tables' to operate, which
    //      are fixed-size arrays of Yama object references which the system
    //      uses to store intermediate state
    //
    //      instead of operations popping from, and pushing to, the end of a
    //      stack for each operation, operations specify table indices which
    //      define the locations of objects which are used as the inputs/outputs
    //      of the operation, operating on these objects directly, avoiding
    //      having to copy as much as a stack machine would
    // 
    //      local register table registers start off initialized to object
    //      references to the None object
    // 
    //      local register table registers are weakly typed, and as such the
    //      type of object they hold can change upon reassignment
    //
    //      (panicking)
    //
    //      Yama contexts 'panic' when they are irrecoverably in error
    //
    //      panicking results in Yama-related behaviour being aborted, and the
    //      entire object state, call stack, etc. of the context being reset
    // 
    //      the user call frame is also reset when panicking
    // 
    //      methods of the context's low-level command interface which return
    //      cmd_status will, if they return a *bad* status object, be indicating
    //      that command failure has resulted in a panic, either due to improper
    //      command method usage, or for other reasons
    // 
    //      C++ functions defining call behaviour are expected to do the following
    //      if by cmd_status or other means they detect a panic:
    //
    //          1) cease low-level command interface usage <- TODO: not 100% sure on this one
    //
    //          2) perform any behaviour necessary to ensure that any relevant
    //             invariants are maintained
    //
    //          3) return immediately after #2


    // cmd_status encapsulates whether a low-level context command 
    // succeeded or failed

    struct cmd_status final {
        bool status;


        inline bool good() const noexcept { return status; }
        inline bool bad() const noexcept { return !good(); }


        static inline cmd_status init(bool is_good) noexcept {
            return cmd_status{ is_good };
        }
    };


    using arg_t     = size_t; // arg_t encapsulates an index into a call argument slice
    using local_t   = size_t; // local_t encapsulates an index into a local register table


    // ctx_config defines init config options for contexts

    struct ctx_config final {
        // TODO: maybe in the future replace below undefined behaviour w/ something *safer*

        // IMPORTANT: undefined behaviour if max_call_frames == 0 (ie. if can't have user call frame)

        size_t max_call_frames  = 32; // max call frames before overflow
        size_t user_locals      = 32; // user call frame's local register table size


        bool operator==(const ctx_config&) const noexcept = default;
    };

    const ctx_config default_ctx_config = ctx_config{};


    class context final : public api_component {
    public:

        context(
            res<domain> dm,
            ctx_config config = default_ctx_config,
            std::shared_ptr<debug> dbg = nullptr);


        // get_domain returns the domain this context is associated w/

        res<domain> get_domain() const noexcept;

        // dm provides summary access to the domain of the context

        domain& dm() const noexcept;


        // get_config returns the config details of this context

        const ctx_config& get_config() const noexcept;


        // NOTE: try not to get the below load methods confused w/ the notion
        //       of *loading* when it comes to object registers

        // load returns the type under fullname, if any

        std::optional<type> load(const str& fullname);

        // load_# methods below provide quick access to preloaded 
        // built-in Yama types, w/ as little overhead as possible

        type load_none();
        type load_int();
        type load_uint();
        type load_float();
        type load_bool();
        type load_char();


        // TODO: fmt_stacktrace has not been unit tested

        // fmt_stacktrace returns a formatted string of a trace of the current
        // state of the call stack of the context

        // skip tells fmt_stacktrace how many of the top call frames of the
        // call stack should be ignored

        std::string fmt_stacktrace(size_t skip = 0, const char* tab = "    ") const;


        // LOW-LEVEL COMMAND INTERFACE
        
        // IMPORTANT:
        //      for the methods below, it's always undefined behaviour to
        //      use a yama::object_ref, yama::type, etc. which is associated
        //      w/ another context, or w/ a domain other than the one this
        //      context is associated w/

        // TODO: the ref counting methods below haven't been unit tested due
        //       to the MVP of Yama not providing enough nuances to really
        //       be able to do so yet

        // TODO: if later on ll_remove_ref will fire destructors, what happens
        //       if doing so causes ll_remove_ref to *panic*
        //
        //       should ll_remove_ref return cmd_status?

        // these perform ref count incr/decr for x, w/ associated cleanup 
        // of x in the event of it's ref count reaching 0

        // behaviour is undefined if x is used improperly w/ these methods

        void ll_add_ref(const object_ref& x);
        void ll_remove_ref(const object_ref& x);

        // these methods are used to simplify the process of incr/decr of
        // object ref counts in common circumstances

        // ll_drop_ref is for dissociating a object_ref from an object, 
        // putting the object_ref in a semantically indeterminate state
        // (sorta like if you move-assigned its contents away)

        // ll_copy_ref, ll_move_ref, and ll_swap_ref are for assisting
        // in copy/move assignment and swapping, respectively

        // ll_clone_ref is for creating a object_ref which is a clone
        // of another object_ref

        // ll_clone_ref overload taking an std::optional is monadic,
        // returning std::nullopt if argument was std::nullopt

        // behaviour is undefined if x is used improperly w/ these methods

        void ll_drop_ref(object_ref& x);
        void ll_copy_ref(const object_ref& src, object_ref& dest);
        void ll_move_ref(object_ref& src, object_ref& dest);
        void ll_swap_ref(object_ref& a, object_ref& b);

        object_ref ll_clone_ref(borrowed_ref x);
        std::optional<object_ref> ll_clone_ref(std::optional<borrowed_ref> x);

        // IMPORTANT:
        //      for all 'll_#' methods below, all ref count incr/decr 
        //      relating to the passing of passing of objects into, or
        //      out from, the methods, will be handled automatically
        //      by said methods
        //
        //      this means that if a returned object_ref is used to
        //      move-construct another object_ref variable, the variable's 
        //      object's ref count is guaranteed to be correct

        // these ll_new_# methods are used for the quick creation of objects

        canonical_ref ll_new_none();
        canonical_ref ll_new_int(int_t v);
        canonical_ref ll_new_uint(uint_t v);
        canonical_ref ll_new_float(float_t v);
        canonical_ref ll_new_bool(bool_t v);
        canonical_ref ll_new_char(char_t v);

        std::optional<canonical_ref> ll_new_fn(type f); // ll_new_fn returns std::nullopt if f is not a function type

        // it's okay to use ll_panicking and ll_panics while panicking,
        // as the whole point of it is to detect it

        size_t ll_panics() noexcept;                       // ll_panics returns the number of times the context has panicked
        bool ll_panicking() noexcept;                        // ll_panicking queries if the context is panicking
        bool ll_is_user() noexcept;                         // ll_is_user returns if the current call frame is the user call frame
        size_t ll_max_call_frames() noexcept;               // ll_max_call_frames returns the max call stack height allowed
        size_t ll_call_frames() noexcept;                   // ll_call_frames returns the number of call frames on the call stack

        // ll_consts behaviour is undefined if in the user call frame

        const_table ll_consts() noexcept;                   // ll_consts returns the constant table of the current call

        size_t ll_args() noexcept;                          // ll_args returns the number of args available
        size_t ll_locals() noexcept;                        // ll_locals returns the number of objects on the local register table
        std::optional<object_ref> ll_arg(arg_t x);          // ll_args returns the object, if any, at x in the arg slice
        std::optional<object_ref> ll_local(local_t x);      // ll_local returns the object, if any, at x in the local register table

        void ll_panic();                                    // ll_panic induces a panic, failing quietly if already panicking

        // ll_load loads v into the local object register at x

        // ll_load does not care what type the x object is, as it overwrites it

        // ll_load panics if x is out-of-bounds

        cmd_status ll_load(local_t x, borrowed_ref v);

        // these methods wrap ll_load calls composed w/ ll_new_# method calls

        cmd_status ll_load_none(local_t x);
        cmd_status ll_load_int(local_t x, int_t v);
        cmd_status ll_load_uint(local_t x, uint_t v);
        cmd_status ll_load_float(local_t x, float_t v);
        cmd_status ll_load_bool(local_t x, bool v);
        cmd_status ll_load_char(local_t x, char_t v);

        // ll_load_fn panics if f is not a function type

        cmd_status ll_load_fn(local_t x, type f);

        // ll_load_const loads the object constant at c, in the constant table
        // of the current call, into the local object register at x

        // ll_load_const does not care what type the x object is, as it overwrites it

        // ll_load_const panics if in the user call frame

        // ll_load_const panics if x is out-of-bounds

        // ll_load_const panics if c is out-of-bounds

        // ll_load_const panics if the constant at c is not an object constant

        cmd_status ll_load_const(local_t x, const_t c);

        // ll_load_arg loads the argument at index arg into the local object
        // register at x

        // ll_load_arg does not care what type the x object is, as it overwrites it

        // ll_load_arg panics if in the user call frame

        // ll_load_arg panics if x is out-of-bounds
        
        // ll_load_arg panics if arg is out-of-bounds

        cmd_status ll_load_arg(local_t x, arg_t arg);

        // ll_copy copies object at src onto object at dest

        // ll_copy does not care what type the dest object is, as it overwrites it

        // ll_copy panics if src is out-of-bounds

        // ll_copy panics if dest is out-of-bounds

        cmd_status ll_copy(local_t src, local_t dest);

        // below, let
        //      - 'args' refer to the exclusive object range [args_start, args_start+args_n)
        //      - 'callobj' refer to the first object in args
        //      - 'param args' refer to args excluding callobj

        // ll_call performs a Yama call of callobj using args, writing the
        // return value object to object ret

        // ll_call does not care what type the ret object is, as it overwrites it

        // ll_call does not check if args types are correct to use in a callobj call

        // ll_call panics if args provides no callobj (ie. if args_n == 0)

        // ll_call panics if args index range is out-of-bounds

        // ll_call panics if ret is out-of-bounds

        // ll_call panics if callobj is not of a callable type

        // ll_call panics if args_n-1 is not equal to the argument count expected
        // for a call to callobj (here, the -1 excludes callobj from argument count)

        // ll_call panics if the call stack would overflow

        // ll_call panics if, after call behaviour has completed execution, no 
        // return value object has been provided

        cmd_status ll_call(local_t args_start, size_t args_n, local_t ret);

        // ll_call_nr performs a Yama call of callobj using args, discarding the
        // return value object

        // the '_nr' in ll_call_nr stands for 'no result'

        // ll_call_nr does not check if args types are correct to use in a callobj call

        // ll_call_nr panics if args provides no callobj (ie. if args_n == 0)

        // ll_call_nr panics if args index range is out-of-bounds

        // ll_call_nr panics if callobj is not of a callable type

        // ll_call_nr panics if args_n-1 is not equal to the argument count expected
        // for a call to callobj (here, the -1 excludes callobj from argument count)

        // ll_call_nr panics if the call stack would overflow

        // ll_call_nr panics if, after call behaviour has completed execution, no 
        // return value object has been provided

        cmd_status ll_call_nr(local_t args_start, size_t args_n);

        // ll_ret is used within calls to perform the act of returning object x
        // as the return value object of the call

        // ll_ret does not check if the type of the return value object returned
        // is correct for a call to the call object in question

        // ll_ret panics if in the user call frame

        // ll_ret panics if x is out-of-bounds

        // ll_ret panics if called multiple times

        cmd_status ll_ret(local_t x);


    private:

        res<domain> _dm;
        ctx_config _config;

        std::allocator<void> _al;

        size_t _panics = 0;
        bool _panicking = false;

        // NOTE: _cf_t must use indices, as reallocs in _registers will invalidate pointers

        struct _cf_t final {
            std::optional<type>         t;              // the type associated w/ this call (or std::nullopt if user call frame)
            size_t                      args_offset;    // offset into _registers where call args begin
            size_t                      args_count;     // number of registers in call args
            size_t                      locals_offset;  // offset into _registers where local register table begins
            size_t                      locals_count;   // number of registers in local register table
            std::optional<object_ref>   returned;       // cache for returned object, if any

            // bcode exec fields

            // TODO: the memory locality improvements of bcode_ptr have not actually been proven yet

            const bc::code*             bcode_ptr;      // pointer to bcode (to improve memory locality)
            size_t                      pc;             // program counter
            bool                        should_halt;    // if instructed to halt
        };
        
        std::vector<object_ref>     _registers; // storage for local register tables for call frames
        std::vector<_cf_t>          _callstk;   // the call stack

        // _cf_view_t is used to easily view contents of a call frame

        struct _cf_view_t final {
            std::span<object_ref> args, locals;


            bool args_bounds_check(arg_t x) const noexcept;
            bool locals_bounds_check(local_t x) const noexcept;
        };

        _cf_t& _top_cf() noexcept;
        _cf_t& _below_top_cf() noexcept; // cf just below the top one

        _cf_view_t _view_top_cf();
        _cf_view_t _view_below_top_cf(); // cf just below the top one

        type _curr_type() noexcept;
        bool _has_curr_type() noexcept;


        template<typename... Args>
        inline void _panic(std::format_string<Args...> msg, Args&&... args);


        // _push_cf returns if successful, w/ it failing if pushing
        // would overflow the call stack

        // _push_cf will handle converting [args_start, args_start+args_n) into
        // the equiv _registers index range

        bool _push_cf(std::optional<type> t, local_t args_start, size_t args_n, size_t locals);
        void _pop_cf();

        void _push_user_cf();

        // _add_registers is used to add new registers, initializing each one
        // to an object reference to the None object

        // _remove_registers is used to remove registers when popping call frames, 
        // w/ us having to *unwind* each one to ensure their object references are
        // deinitialized properly

        void _add_registers(size_t n);
        void _remove_registers(size_t n);


        // this needs to be called in ll_panic and ll_call (after call 
        // behaviour) in order to handle panic behaviour once call stack 
        // unwinding reaches the user call frame, which needs to be popped, 
        // and a new one pushed

        void _try_handle_panic_for_user_cf();


        // TODO: gonna need a '_fmt_Kt', or others, later when we use constants 
        //       for things other than just object constants

        // special formatter methods used to help w/ low-level command diagnostics

        std::string _fmt_R_no_preview(local_t x);
        std::string _fmt_R(local_t x);
        std::string _fmt_Ko(const_t x);
        std::string _fmt_Arg(arg_t x);


        cmd_status _load(local_t x, stolen_ref v);

        bool _load_err_x_out_of_bounds(local_t x, borrowed_ref v);

        cmd_status _load_fn(local_t x, type f);

        bool _load_fn_err_f_not_callable_type(type f);

        cmd_status _load_const(local_t x, const_t c);

        bool _load_const_err_in_user_call_frame();
        bool _load_const_err_c_out_of_bounds(const_t c);
        bool _load_const_err_c_is_not_object_constant(const_t c);

        cmd_status _load_arg(local_t x, arg_t arg);

        bool _load_arg_err_in_user_call_frame();
        bool _load_arg_err_arg_out_of_bounds(arg_t arg);


        cmd_status _copy(local_t src, local_t dest);

        bool _copy_err_src_out_of_bounds(local_t src);
        bool _copy_err_dest_out_of_bounds(local_t dest, borrowed_ref src_object);


        std::optional<object_ref> _call(local_t args_start, size_t args_n);

        cmd_status _call_ret_to_local(std::optional<stolen_ref> ret, local_t target);
        cmd_status _call_ret_to_no_result(std::optional<stolen_ref> ret);

        // turns out the out-of-bounds/overflow checks for ll_call outputting to
        // local need to occur BEFORE anything else, so we'll use these methods 
        // to check these as a dirty little exception to above pattern

        bool _call_ret_to_local_precheck(local_t target);

        bool _call_err_no_callobj(size_t args_n);
        bool _call_err_args_out_of_bounds(local_t args_start, size_t args_n);
        bool _call_err_callobj_not_callable_type(borrowed_ref callobj);
        bool _call_err_param_arg_count_mismatch(borrowed_ref callobj, size_t param_args);
        bool _call_err_push_cf_would_overflow(borrowed_ref callobj, bool push_cf_result);
        bool _call_err_no_return_value_object(borrowed_ref callobj);

        bool _call_err_ret_ouf_of_bounds(local_t ret);


        cmd_status _ret(local_t x);

        bool _ret_err_in_user_call_frame();
        bool _ret_err_x_out_of_bounds(local_t x);
        bool _ret_err_already_returned_something();


        // bcode exec helpers

        void _bcode_exec();
        void _bcode_setup();
        const bc::code* _bcode_acquire_bcode();
        bc::instr _bcode_fetch_instr();
        void _bcode_exec_instr(bc::instr x);
        void _bcode_halt_if_panicking();
        void _bcode_halt();
        void _bcode_jump(int16_t offset);
    };

    template<typename ...Args>
    inline void context::_panic(std::format_string<Args...> msg, Args&&... args) {
        YAMA_LOG(
            dbg(), ctx_panic_c,
            msg, 
            std::forward<Args>(args)...);
        ll_panic();
    }
}

