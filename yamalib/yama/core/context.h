

#pragma once


#include "scalars.h"
#include "res.h"
#include "api_component.h"
#include "domain.h"
#include "object_ref.h"

#include "newtop.h"
#include "const_table_ref.h"


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
    //      (local object stacks)
    // 
    //      each call frame encapsulates a 'local object stack'
    // 
    //      these objects are also known as 'registers'
    // 
    //      local object stack encapsulate the local state of a call, 
    //      including things such as local variables, arguments, etc.
    // 
    //      indices into a local object stack only refer to the current call
    //      frame's local object stack, w/ each existing in relative isolation
    // 
    //      (the 'user' call frame)
    // 
    //      at the base of the call stack is a special 'user' call frame,
    //      which exists to provide a local object stack to use when operating
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
    //      stack machines transact from the end of an object stack, w/ this
    //      involving a lot of copying of data to/from the end of the stack
    // 
    //      the Yama VM instead has a stack of mutable registers which are
    //      pushed/popped, but w/ operations transacting mainly by specifying
    //      indices to put/get data to/from (though w/ exceptions where stack
    //      machine-like transacting is done)
    // 
    //      local object stack registers are weakly typed, and as such the
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
    //          1) cease low-level command interface usage (w/ exceptions)
    //
    //          2) perform any behaviour necessary to ensure that any relevant
    //             invariants are maintained
    //
    //          3) return immediately after #2


    class token_exception final {};

    // cmd_status encapsulates whether a low-level context command 
    // succeeded or failed

    struct cmd_status final {
        bool status;


        inline bool good() const noexcept { return status; }
        inline bool bad() const noexcept { return !good(); }

        // TODO: or_throw has not been unit tested

        // or_throw is alternative to good for situations where cleanup needs to occur
        // before exiting a call_fn which detected a panic, w/ or_throw throwing an
        // exception upon fail so end-user can have all the cleanup code required on
        // panic in one place, rather than duplicating it for every context command
        // in the try-block

        inline void or_throw() const {
            if (bad()) throw token_exception{};
        }


        static inline cmd_status init(bool is_good) noexcept {
            return cmd_status{ is_good };
        }
    };


    using arg_t     = size_t; // arg_t encapsulates an index into a call argument slice
    using local_t   = size_t; // local_t encapsulates an index into a local object stack


    // TODO: figure out how Yama's gonna have below be able to be configured by end-user

    constexpr size_t max_call_frames = 32; // max call frames before overflow
    constexpr size_t user_max_locals = 32; // user call frame's max local object stack height

    static_assert(max_call_frames >= 1); // need >=1 so can have user call frame


    class context final : public api_component {
    public:
        context(res<domain> dm, std::shared_ptr<debug> dbg = nullptr);


        res<domain> get_domain() const noexcept;
        domain& dm() const noexcept;

        std::optional<item_ref> load(const str& fullname);

        item_ref none_type();
        item_ref int_type();
        item_ref uint_type();
        item_ref float_type();
        item_ref bool_type();
        item_ref char_type();
        item_ref type_type();

        // TODO: fmt_stacktrace has not been unit tested.

        // Returns a formatted string of a trace of the current state of the call stack of the context.
        // Skip tells fmt_stacktrace how many of the top call frames of the call stack should be ignored.
        std::string fmt_stacktrace(size_t skip = 0, const char* tab = default_tab) const;


        // LOW-LEVEL COMMAND INTERFACE
        
        // IMPORTANT:
        //      For these methods, it's always undefined behaviour to use a
        //      yama::object_ref, yama::item_ref, etc. which is associated
        //      w/ another context, or w/ a domain other than the one this
        //      context is associated w/.

        // TODO: The ref counting methods below haven't been unit tested due
        //       to the MVP of Yama not providing enough nuances to really
        //       be able to do so yet.

        // TODO: If later on remove_ref will fire destructors, what happens
        //       if doing so causes remove_ref to *panic*?
        //
        //       Should remove_ref return cmd_status?

        // TODO: If we have finalizers instead of destructors, I don't think
        //       above TODO should be an issue.

        // Incrs ref count for x.
        // Behaviour is undefined if x is improper to use here.
        void add_ref(const object_ref& x);
        // Decrs ref count for x, performing cleanup upon reaching 0.
        // Behaviour is undefined if x is improper to use here.
        void remove_ref(const object_ref& x);

        // These methods automate the incrs/decrs needed for common operations, as well
        // as more generally helping communicate intent.

        // Performs ref count decr to dissociate x from its object, dropping its reference.
        // Puts x in an indeterminate state (ie. like when move-assigning its contents away.)
        // Behaviour is undefined if x is improper to use here.
        void drop_ref(object_ref& x);
        // Performs ref count incrs/decrs needed to copy-assign src onto dest.
        // Behaviour is undefined if src/dest is improper to use here.
        void copy_ref(const object_ref& src, object_ref& dest);
        // Performs ref count incrs/decrs needed to move-assign src onto dest.
        // Behaviour is undefined if src/dest is improper to use here.
        void move_ref(object_ref& src, object_ref& dest);
        // Performs ref count incrs/decrs needed to swap a and b.
        // Behaviour is undefined if a/b is improper to use here.
        void swap_ref(object_ref& a, object_ref& b);

        // Clones a borrowed reference to create a new owned one.
        // Behaviour is undefined if x is improper to use here.
        object_ref clone_ref(borrowed_ref x);
        // Clones a borrowed reference to create a new owned one.
        // This overload is monadic.
        // Behaviour is undefined if x is improper to use here.
        std::optional<object_ref> clone_ref(std::optional<borrowed_ref> x);

        // IMPORTANT:
        //      For all methods below, all ref count incr/decr relating to
        //      the passing of passing of objects into, or out from, the
        //      methods, will be handled automatically by said methods.
        //
        //      This means that if a returned object_ref is used to
        //      move-construct another object_ref variable, the variable's 
        //      object's ref count is guaranteed to be correct.

        // These new_# methods are used for the quick creation of objects.

        // Returns a new None object.
        canonical_ref new_none();
        // Returns a new Int object.
        canonical_ref new_int(int_t v);
        // Returns a new UInt object.
        canonical_ref new_uint(uint_t v);
        // Returns a new Float object.
        canonical_ref new_float(float_t v);
        // Returns a new Bool object.
        canonical_ref new_bool(bool_t v);
        // Returns a new Char object.
        canonical_ref new_char(char_t v);
        // Returns a new Type object.
        canonical_ref new_type(item_ref v);
        // Returns a new fn type object.
        // Fails if f is not a callable type.
        std::optional<canonical_ref> new_fn(item_ref f);

        // IMPORTANT:
        //      It's okay to use panicking and panics while panicking,
        //      as the whole point of it is to detect it.

        // Returns the number of times the context has panicked.
        size_t panics() noexcept;
        // Returns if the context is panicking.
        bool panicking() noexcept;
        // Returns if the current call frame is the user call frame.
        bool is_user() noexcept;
        // Returns the call stack height.
        size_t call_frames() noexcept;
        // Returns the max call stack height.
        size_t max_call_frames() noexcept;

        // Returns the constant table of the current call.
        // Behaviour is undefined if in the user call frame.
        const_table_ref consts() noexcept;

        // Returns the number of args available.
        size_t args() noexcept;
        // Returns the height of the current local object stack.
        size_t locals() noexcept;
        // Returns the max height of the current local object stack.
        size_t max_locals() noexcept;
        // Returns the object, if any, at x in the arg slice.
        std::optional<borrowed_ref> arg(arg_t x);
        // Returns clone_ref(arg(x)).
        std::optional<object_ref> arg_c(arg_t x);
        // Returns the object, if any, at x in the local object stack.
        std::optional<borrowed_ref> local(local_t x);
        // Returns clone_ref(local(x)).
        std::optional<object_ref> local_c(arg_t x);

        // Induces a panic.
        void panic();

        // pop pops the top n local object stack objects.
        // 
        // pop stops prematurely if the stack is emptied.
        cmd_status pop(size_t n = 1);

        // put loads v into the local object register at x (x may be newtop.)
        // 
        // put does not care what type the x object is, as it overwrites it.
        // 
        // Panic Conditions:
        //      - If x is out-of-bounds (unless newtop.)
        //      - If (x == newtop, but) pushing would overflow.
        cmd_status put(local_t x, borrowed_ref v);

        inline cmd_status push(borrowed_ref v) { return put(newtop, v); }

        // These methods wrap put calls composed w/ new_# method calls.

        cmd_status put_none(local_t x);
        cmd_status put_int(local_t x, int_t v);
        cmd_status put_uint(local_t x, uint_t v);
        cmd_status put_float(local_t x, float_t v);
        cmd_status put_bool(local_t x, bool_t v);
        cmd_status put_char(local_t x, char_t v);
        cmd_status put_type(local_t x, item_ref v);

        inline cmd_status push_none() { return put_none(newtop); }
        inline cmd_status push_int(int_t v) { return put_int(newtop, v); }
        inline cmd_status push_uint(uint_t v) { return put_uint(newtop, v); }
        inline cmd_status push_float(float_t v) { return put_float(newtop, v); }
        inline cmd_status push_bool(bool_t v) { return put_bool(newtop, v); }
        inline cmd_status push_char(char_t v) { return put_char(newtop, v); }
        inline cmd_status push_type(item_ref v) { return put_type(newtop, v); }

        // put_fn loads fn object of type f into the local object register at x (x may be newtop.)
        //
        // Panic Conditions:
        //      - If x is out-of-bounds (unless newtop.)
        //      - If (x == newtop, but) pushing would overflow.
        //      - If f is not a callable type.
        cmd_status put_fn(local_t x, item_ref f);

        inline cmd_status push_fn(item_ref f) { return put_fn(newtop, f); }

        // put_const loads the object constant at c, in the constant table
        // of the current call, into the local object register at x (x may be newtop)
        //
        // put_const does not care what type the x object is, as it overwrites it
        //
        // Panic Conditions:
        //      - If in the user call frame.
        //      - If x is out-of-bounds (unless newtop.)
        //      - If (x == newtop, but) pushing would overflow.
        //      - If c is out-of-bounds.
        //      - If the constant at c is not an object constant.
        cmd_status put_const(local_t x, const_t c);

        inline cmd_status push_const(const_t c) { return put_const(newtop, c); }

        // put_type_const loads a yama:Type object of the type constant at c, in the
        // constant table of the current call, into the local object register at x
        // (x may be newtop.)
        //
        // put_type_const does not care what type the x object is, as it overwrites it.
        //
        // Panic Conditions:
        //      - If in the user call frame.
        //      - If x is out-of-bounds (unless newtop.)
        //      - If (x == newtop, but) pushing would overflow.
        //      - If c is out-of-bounds.
        //      - If the constant at c is not a type constant.
        cmd_status put_type_const(local_t x, const_t c);

        inline cmd_status push_type_const(const_t c) { return put_type_const(newtop, c); }

        // put_arg loads the argument at index arg into the local object
        // register at x (x may be newtop.)
        //
        // put_arg does not care what type the x object is, as it overwrites it.
        //
        // Panic Conditions:
        //      - If in the user call frame.
        //      - If x is out-of-bounds (unless newtop.)
        //      - If (x == newtop, but) pushing would overflow.
        //      - If arg is out-of-bounds.
        cmd_status put_arg(local_t x, arg_t arg);

        inline cmd_status push_arg(arg_t arg) { return put_arg(newtop, arg); }

        // copy copies object at src onto object at dest (dest may be newtop.)
        //
        // copy does not care what type the dest object is, as it overwrites it.
        //
        // Panic Conditions:
        //      - If src is out-of-bounds.
        //      - If dest is out-of-bounds (unless newtop.)
        //      - If (dest == newtop, but) pushing would overflow.
        cmd_status copy(local_t src, local_t dest = newtop);

        // TODO: Add a 'explicit_init' command method later on when we add the ability to
        //       construct things like structs w/ stored properties which need to be initialized.

        // TODO: Be sure to, when we add types which cannot be default initialized, make it
        //       so default_init panics.

        // default_init loads a default initialized object of type t into the local
        // object register at x (x may be newtop.)
        //
        // The const_t overload initializes w/ type from the type constant at c.
        //
        // Panic Conditions:
        //      - If in the user call frame.
        //          * const_t overload.
        //      - If x is out-of-bounds (unless newtop.)
        //      - If (x == newtop, but) pushing would overflow.
        //      - If c is out-of-bounds.
        //          * const_t overload.
        //      - If the constant at c is not a type constant.
        //          * const_t overload.
        cmd_status default_init(local_t x, item_ref t);
        cmd_status default_init(local_t x, const_t c);
        
        // IMPORTANT:
        //      This details some important notes about conversions and unit testing.
        //
        //      context::conv and the conv instr are unit tested, w/ EACH AND EVERY legal and
        //      illegal conversion case being intended to be covered.
        //
        //      Static verification DOES NOT verify the validity of conversions. The reason for
        //      this is that any illegal conversions will cleanly panic at runtime, w/out
        //      compromising the system.
        //          * We'd need to introduce static verif for conversion if this ever changes.
        //
        //      Compiler unit tests (ie. in compilation-tests.cpp) cover EACH AND EVERY legal
        //      and illegal conversion case, JUST LIKE w/ 'conv'.
        //          * Keep these tests up-to-date (they're under 'Conversions'.)
        //
        //      Compiler unit tests raise compilation errors for certain conversions which are
        //      known at compile-time to be illegal, meaning these are guaranteed to never be
        //      able cause a panic.
        // 
        //      Compiler unit tests also introduce the notion of *constexpr* conversions, which
        //      are resolved during compilation.
        //          * Keep these tests up-to-date (they're under 'Conversions'.)

        // IMPORTANT:
        //      The following conversions are defined:
        //          - T -> T, where T is any type.
        //              * Identity Convs
        //          - T -> U, where T != U, and T and U are Int, UInt, Float, Bool, or Char.
        //              * Primitive Type Convs
        //          - T -> U, where T is any fn or method type, and U is Type.
        //              * Fn/Method Type Narrowed To yama:Type Conv
        //
        //      All other conversions are illegal.

        // TODO: Right now, all primitive conversions are *unsafe*, meaning it's possible to
        //       do things like creating Char objects for *illegal* Unicode codepoints!
        // 
        //       We also lack comprehensive testing of things like conversion between primitive
        //       values *extremes*. Nor do we properly test NaNs and subnormals.
        //
        //       Do we want this? Or should these conversions always be safe, and panicking
        //       otherwise?

        // TODO: The specifics of 'Primitive Type Convs', both in terms of semantics, and
        //       our UNIT TESTS, are NOT well defined and can be VASTLY improved!
        // 
        //       This is to say that we don't really specifically assert what kinds of values
        //       should be mapped to specific other values during conversion, w/ us just broadly
        //       testing that it does *something*, and we presume it's a C++ conversion.
        //
        //       This critique also goes for our compilation-tests.cpp 'Conversions' section
        //       unit tests also.

        // TODO: The unit test 'ConversionExpr_ConditionallyConstExpr_ExprIsConstExprButConversionItselfIsNot'
        //       is currently a stub until we add support for conversions which are themselves
        //       not able to be constexpr, even if the converted expr is constexpr.

        // TODO: At present, the stmt 'return;' DOES NOT attempt to perform an implicit conv
        //       None() to the return type, meaning that it does NOT 100% line up w/ the behaviour
        //       of a (supposedly equivalent) 'return None();' stmt.
        //
        //       Our unit tests don't assert either way w/ regards to whether an implicit conv is/isn't
        //       expected.
        //
        //       At some point try and update our unit tests to cover this edgecase. I'm not 100%
        //       sure if we at the moment are able to properly unit test this though, at least w/
        //       regards to expecting an implicit conv to occur (as that would require us to be able
        //       to coerce None *into* something else.)

        // conv loads an object into register dest (dest may be newtop) produced by converting
        // the value of the object at src into type t.
        //
        // The const_t overload converts to the type of the type constant at c.
        //
        // Panic Conditions:
        //      - If the target cannot be converted to t.
        //          * Or type constant at c.
        //      - If in the user call frame.
        //          * const_t overload.
        //      - If src is out-of-bounds.
        //      - If dest is out-of-bounds (unless newtop.)
        //      - If c is out-of-bounds.
        //          * const_t overload.
        //      - If the constant at c is not a type constant.
        //          * const_t overload.
        cmd_status conv(local_t src, local_t dest, item_ref t);
        cmd_status conv(local_t src, local_t dest, const_t c);

        // NOTE: Due to the existence of the callobj, plus the limit to only one return value
        //       object, it's not possible for callobj to overflow when ret == newtop.

        // Below, let
        //      - 'args' refer to the inclusive object range [R(top-args_n), R(top)];
        //      - 'callobj' refer to the first object in args; and
        //      - 'param args' refer to args excluding callobj.

        // call performs a Yama call of callobj using args, writing the return value
        // object to object ret (ret may be newtop), then consuming args.
        //
        // call does not care what type the ret object is, as it overwrites it.
        //
        // call does not check if args types are correct to use in a callobj call.
        //
        // Panic Conditions:
        //      - If args provides no callobj (ie. if args_n == 0.)
        //      - If args index range is out-of-bounds.
        //      - If ret will be out-of-bounds after the call (unless newtop.)
        //      - If callobj is not of a callable type.
        //      - If args_n-1 is not equal to the argument count expected for a call to callobj (here, the -1
        //        excludes callobj from argument count.)
        //      - If the call stack would overflow.
        //      - If, after call behaviour has completed execution, no return value object has been provided.
        cmd_status call(size_t args_n, local_t ret);

        // call_nr performs a Yama call of callobj using args, discarding the return
        // value object, then consuming args.
        //
        // The '_nr' in call_nr stands for 'no result'.
        //
        // call_nr does not check if args types are correct to use in a callobj call.
        //
        // Panic Conditions:
        //      - If args provides no callobj (ie. if args_n == 0.)
        //      - If args index range is out-of-bounds.
        //      - If callobj is not of a callable type.
        //      - If args_n-1 is not equal to the argument count expected for a call to callobj (here, the -1
        //        excludes callobj from argument count.)
        //      - If the call stack would overflow.
        //      - If, after call behaviour has completed execution, no return value object has been provided.
        cmd_status call_nr(size_t args_n);

        // ret is used within calls to perform the act of returning object x
        // as the return value object of the call.
        //
        // ret does not check if the type of the return value object returned
        // is correct for a call to the call object in question.
        //
        // Panic Conditions:
        //      - If in the user call frame.
        //      - If x is out-of-bounds.
        //      - If called multiple times.
        cmd_status ret(local_t x);


    private:
        res<domain> _dm;
        std::allocator<void> _al;

        size_t _panics = 0;
        bool _panicking = false;

        // NOTE: _cf_t must use indices, as reallocs in _registers will invalidate pointers

        struct _cf_t final {
            std::optional<item_ref>     t;              // the type associated w/ this call (or std::nullopt if user call frame)
            size_t                      args_offset;    // offset into _registers where call args begin
            size_t                      args_count;     // number of registers in call args
            size_t                      locals_offset;  // offset into _registers where local object stack begins
            size_t                      max_locals;     // max locals allowed on this call's local object stack
            std::optional<object_ref>   returned;       // cache for returned object, if any

            // bcode exec fields

            // TODO: the memory locality improvements of bcode_ptr have not actually been proven yet

            const bc::code*             bcode_ptr;      // pointer to bcode (to improve memory locality)
            size_t                      pc;             // program counter
            bool                        should_halt;    // if instructed to halt
        };
        
        std::vector<object_ref>     _registers; // storage for local object stacks for call frames
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

        item_ref _curr_type() noexcept;
        bool _has_curr_type() noexcept;


        template<typename... Args>
        inline void _panic(std::format_string<Args...> msg, Args&&... args);


        // _push_cf returns if successful, w/ it failing if pushing
        // would overflow the call stack

        // _push_cf will handle converting [args_start, args_start+args_n) into
        // the equiv _registers index range

        bool _push_cf(std::optional<item_ref> t, local_t args_start, size_t args_n, size_t max_locals);
        void _pop_cf();

        void _push_user_cf();

        // _push_reg attempts to push a new register to the stack, initializing it

        // _pop_regs pops the first n registers from the stack, deinitializing each
        // one via drop_ref, w/ this being used to *unwind* the stack properly

        // _pop_regs only affects registers of the current call frame

        void _push_reg(stolen_ref x);
        void _pop_regs(size_t n);


        // this needs to be called in panic and call (after call 
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
        std::string _fmt_Kt(const_t x);
        std::string _fmt_Arg(arg_t x);


        cmd_status _put(local_t x, stolen_ref v, const char* verb = "put");

        bool _put_err_pushing_overflows(borrowed_ref v);
        bool _put_err_x_out_of_bounds(local_t x, borrowed_ref v, const char* verb);

        cmd_status _put_fn(local_t x, item_ref f);

        bool _put_fn_err_f_not_callable_type(item_ref f);

        cmd_status _put_const(local_t x, const_t c);

        bool _put_const_err_in_user_call_frame();
        bool _put_const_err_c_out_of_bounds(const_t c);
        bool _put_const_err_c_is_not_object_constant(const_t c);
        
        cmd_status _put_type_const(local_t x, const_t c);

        bool _put_type_const_err_in_user_call_frame();
        bool _put_type_const_err_c_out_of_bounds(const_t c);
        bool _put_type_const_err_c_is_not_type_constant(const_t c);

        cmd_status _put_arg(local_t x, arg_t arg);

        bool _put_arg_err_in_user_call_frame();
        bool _put_arg_err_arg_out_of_bounds(arg_t arg);


        cmd_status _copy(local_t src, local_t dest);

        bool _copy_err_src_out_of_bounds(local_t src);


        cmd_status _default_init(local_t x, stolen_ref obj);
        cmd_status _default_init(local_t x, const_t c);

        object_ref _gen_default_initialized(item_ref t);

        bool _default_init_err_in_user_call_frame();
        bool _default_init_err_c_out_of_bounds(const_t c);
        bool _default_init_err_c_is_not_type_constant(const_t c);


        cmd_status _conv(local_t src, local_t dest, item_ref t);
        cmd_status _conv(local_t src, local_t dest, const_t c);

        std::optional<object_ref> _gen_conv_result(borrowed_ref x, item_ref t);

        bool _conv_err_in_user_call_frame();
        bool _conv_err_src_out_of_bounds(local_t src);
        bool _conv_err_c_out_of_bounds(const_t c);
        bool _conv_err_c_is_not_type_constant(const_t c);


        std::optional<object_ref> _call(size_t args_n);

        cmd_status _call_ret_to_local(std::optional<stolen_ref> ret, local_t target);
        cmd_status _call_ret_to_no_result(std::optional<stolen_ref> ret);

        // turns out the out-of-bounds/overflow checks for call outputting to
        // local need to occur BEFORE anything else, so we'll use these methods 
        // to check these as a dirty little exception to above pattern

        bool _call_ret_to_local_prechecks(size_t args_n, local_t target);

        bool _call_err_no_callobj(size_t args_n);
        bool _call_err_args_out_of_bounds(size_t args_n);
        bool _call_err_callobj_not_callable_type(borrowed_ref callobj);
        bool _call_err_param_arg_count_mismatch(borrowed_ref callobj, size_t param_args);
        bool _call_err_push_cf_would_overflow(borrowed_ref callobj, bool push_cf_result);
        bool _call_err_no_return_value_object(borrowed_ref callobj);

        bool _call_err_ret_out_of_bounds(size_t args_n, local_t ret);


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

        // ensure bcode uint8_t newtop gets translated into size_t version correctly

        size_t _maybe_newtop(uint8_t x) const noexcept;
    };

    template<typename ...Args>
    inline void context::_panic(std::format_string<Args...> msg, Args&&... args) {
        YAMA_LOG(
            dbg(), ctx_panic_c,
            msg, 
            std::forward<Args>(args)...);
        panic();
    }
}

