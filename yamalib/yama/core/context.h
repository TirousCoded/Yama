

#pragma once


#include "scalars.h"
#include "res.h"
#include "api_component.h"
#include "mas.h"
#include "domain.h"
#include "object_ref.h"


namespace yama {


    // TODO: in the future, we're gonna need to impl a way to allow for things
    //       like destructors to be able to run when crashing, w / them likewise
    //       having a notion of they themselves being able to crash

    // TODO: we're also gonna have to have cleanup semantics for other forms of
    //       object state, like* global* objects, when we add things like that

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
    //      (crashing)
    //
    //      Yama contexts 'crash' when they are irrecoverably in error
    //
    //      crashing results in Yama-related behaviour being aborted, and the
    //      entire object state, call stack, etc. of the context being reset
    // 
    //      the user call frame is also reset when crashing
    // 
    //      methods of the context's low-level command interface which return
    //      cmd_status will, if they return a *bad* status object, be indicating
    //      that command failure has resulted in a crash, either due to improper
    //      command method usage, or for other reasons
    // 
    //      C++ functions defining call behaviour are expected to do the following
    //      if by cmd_status or other means they detect a crash:
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

    struct no_result_t final {};

    constexpr auto no_result = no_result_t{};


    // ctx_config defines init config options for contexts

    struct ctx_config final {
        // TODO: maybe in the future replace below undefined behaviour w/ something *safer*

        // IMPORTANT:
        //      undefined behaviour if max_call_frames == 0 (ie. can't have user call frame)

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


        // get_mas returns the MAS used internally by this context
        
        // this MAS is provided by the domain upon context init

        res<mas> get_mas() const noexcept;


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
        //       if doing so causes ll_remove_ref to *crash* the context?
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

        // it's okay to use ll_crashing and ll_crashes while crashing,
        // as the whole point of it is to detect it

        size_t ll_crashes() noexcept;                       // ll_crashes returns the number of times the context has crashed
        bool ll_crashing() noexcept;                        // ll_crashing queries if the context is crashing
        bool ll_is_user() noexcept;                         // ll_is_user returns if the current call frame is the user call frame
        size_t ll_max_call_frames() noexcept;               // ll_max_call_frames returns the max call stack height allowed
        size_t ll_call_frames() noexcept;                   // ll_call_frames returns the number of call frames on the call stack

        size_t ll_args() noexcept;                          // ll_args returns the number of args available
        size_t ll_locals() noexcept;                        // ll_locals returns the number of objects on the local register table
        std::optional<object_ref> ll_arg(arg_t x);          // ll_args returns the object, if any, at x in the arg slice
        std::optional<object_ref> ll_local(local_t x);      // ll_local returns the object, if any, at x in the local register table

        void ll_crash();                                    // ll_crash induces a crash, failing quietly if already crashing

        // ll_load loads v into the local object register at x

        // ll_load does not care what type the x object is, as it overwrites it

        // ll_load crashes if x is out-of-bounds

        cmd_status ll_load(local_t x, borrowed_ref v);

        // these methods wrap ll_load calls composed w/ ll_new_# method calls

        cmd_status ll_load_none(local_t x);
        cmd_status ll_load_int(local_t x, int_t v);
        cmd_status ll_load_uint(local_t x, uint_t v);
        cmd_status ll_load_float(local_t x, float_t v);
        cmd_status ll_load_bool(local_t x, bool v);
        cmd_status ll_load_char(local_t x, char_t v);

        // ll_load_fn crashes if f is not a function type

        cmd_status ll_load_fn(local_t x, type f);

        // ll_load_const loads the object constant at c, in the constant table
        // of the current call, into the local object register at x

        // ll_load_const does not care what type the x object is, as it overwrites it

        // ll_load_const crashes if in the user call frame

        // ll_load_const crashes if x is out-of-bounds

        // ll_load_const crashes if c is out-of-bounds

        // ll_load_const crashes if the constant at c is not an object constant

        cmd_status ll_load_const(local_t x, const_t c);

        // ll_load_arg loads the argument at index arg into the local object
        // register at x

        // ll_load_arg does not care what type the x object is, as it overwrites it

        // ll_load_arg crashes if in the user call frame

        // ll_load_arg crashes if x is out-of-bounds
        
        // ll_load_arg crashes if arg is out-of-bounds

        cmd_status ll_load_arg(local_t x, arg_t arg);

        // ll_copy copies object at src onto object at dest

        // ll_copy does not care what type the dest object is, as it overwrites it

        // ll_copy crashes if src is out-of-bounds

        // ll_copy crashes if dest is out-of-bounds

        cmd_status ll_copy(local_t src, local_t dest);

        // below, let
        //      - 'args' refer to the exclusive object range [args_start, args_start+args_n)
        //      - 'callobj' refer to the first object in args
        //      - 'param args' refer to args excluding callobj

        // ll_call performs a Yama call of callobj using args, writing the
        // return value object to object ret

        // ll_call, for its no_result_t overload, does not output any return value object

        // ll_call does not care what type the ret object is, as it overwrites it

        // ll_call does not check if args types are correct to use in a callobj call

        // ll_call crashes if args provides no callobj (ie. if args_n == 0)

        // ll_call crashes if args index range is out-of-bounds

        // ll_call crashes if ret is out-of-bounds

        // ll_call crashes if callobj is not of a callable type

        // ll_call crashes if args_n-1 is not equal to the argument count expected
        // for a call to callobj (here, the -1 excludes callobj from argument count)

        // ll_call crashes if the call stack would overflow

        // ll_call crashes if, after call behaviour has completed execution, no 
        // return value object has been provided

        cmd_status ll_call(local_t args_start, size_t args_n, local_t ret);
        cmd_status ll_call(local_t args_start, size_t args_n, no_result_t);

        // ll_ret is used within calls to perform the act of returning object x
        // as the return value object of the call

        // ll_ret does not check if the type of the return value object returned
        // is correct for a call to the call object in question

        // ll_ret crashes if in the user call frame

        // ll_ret crashes if x is out-of-bounds

        // ll_ret crashes if called multiple times

        cmd_status ll_ret(local_t x);


    private:

        res<domain> _dm;
        ctx_config _config;
        res<mas> _mas;

        std::allocator<void> _al;

        size_t _crashes = 0;
        bool _crashing = false;

        // NOTE: _cf_t must used indices, as reallocs in _registers will invalidate pointers

        struct _cf_t final {
            size_t                      args_offset;    // offset into _registers where call args begin
            size_t                      args_count;     // number of registers in call args
            size_t                      locals_offset;  // offset into _registers where local register table begins
            size_t                      locals_count;   // number of registers in local register table
            std::optional<object_ref>   returned;       // cache for returned object, if any
        };
        
        std::vector<object_ref> _registers; // storage for local register tables for call frames
        std::vector<_cf_t>      _callstk;   // the call stack

        const_table*            _consts;    // the constant table available to ll_load_const

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

        // _push_cf returns if successful, w/ it failing if pushing
        // would overflow the call stack

        // _push_cf will handle converting [args_start, args_start+args_n) into
        // the equiv _registers index range

        bool _push_cf(local_t args_start, size_t args_n, size_t locals);
        void _pop_cf();

        void _push_user_cf();

        // _add_registers is used to add new registers, initializing each one
        // to an object reference to the None object

        // _remove_registers is used to remove registers when popping call frames, 
        // w/ us having to *unwind* each one to ensure their object references are
        // deinitialized properly

        void _add_registers(size_t n);
        void _remove_registers(size_t n);


        // this needs to be called in ll_crash and ll_call (after call 
        // behaviour) in order to handle crash behaviour once call stack 
        // unwinding reaches the user call frame, which needs to be popped, 
        // and a new one pushed

        void _try_handle_crash_for_user_cf();


        cmd_status _load(local_t x, borrowed_ref v);

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
    };
}

