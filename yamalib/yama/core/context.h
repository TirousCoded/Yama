

#pragma once


#include "scalars.h"
#include "res.h"
#include "api_component.h"
#include "mas.h"
#include "domain.h"
#include "object_ref.h"

#include "../internals/call_frame.h"


namespace yama {


    // TODO: in the future, we're gonna need to impl a way to allow for things
    //       like destructors to be able to run when crashing, w/ them likewise 
    //       having a notion of they themselves being able to crash

    // IMPORTANT:
    //      (call stack & call frames)
    // 
    //      at the heart of the Yama context is the 'call stack', which is
    //      composed of 'call frames'
    // 
    //      each call frame encapsulates a 'local object stack', or simply
    //      its 'object stack', or just its 'stack'
    // 
    //      local object stacks encapsulate the local state of a call, 
    //      including things such as local variables, arguments, etc.
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
    //      (call frame layout)
    // 
    //      w/ exception of user call frames, Yama call frames are mandated
    //      to always start off respecting a particular object layout
    // 
    //      growing left-to-right, this layout is as follows:
    // 
    //          [ CALL OBJ. ][ ARGUMENTS ][ OTHERS ]
    // 
    //      where:
    // 
    //          CALL OBJ. is the call object of the call
    // 
    //          ARGUMENTS are argument objects of the call
    // 
    //          OTHERS is a catch-all for all other objects used in the call
    // 
    //      (return value objects)
    // 
    //      Yama requires all calls to return something, w/ unit typed objects
    //      of type None being returned for when *nothing* is returned
    // 
    //      at the end of a call, after its call behaviour has ended, the top
    //      object of the call frame's local object stack is taken as the return
    //      value object returned by the call
    // 
    //      (register-based vs. stack-based VMs)
    //
    //      Yama uses a 'register-based' VM, rather than a 'stack-based' one
    //
    //      this means that while Yama uses 'local object stacks' to operate,
    //      how it transacts data to/from stacks differs from how it would be 
    //      done in a stack machine
    //
    //      instead of operations popping from, and pushing to, the end of the
    //      stack for each operation, operations specify stack indices which
    //      define the locations of objects which are used as the inputs/outputs
    //      of the operation, operating on these objects directly, avoiding
    //      having to copy as much as a stack machine would
    //
    //      (transactions)
    //
    //      each operation upon a Yama local object stack is defined, in part, 
    //      by a 'transaction', which is a signature detailing the objects
    //      involved in the operation
    //
    //      the following syntax is used to define a transaction:
    //
    //          transaction: <operation-name> <term-0> <term-1> ... <term-N>
    //
    //      where:
    // 
    //          <operation-name> is the name of the operation
    //
    //          <term-0>, <term-1>, <term-N>, etc. are the 'terms' of the
    //          operation's transaction
    // 
    //      the above 'terms' take either the form 'in(x)' or 'out(x)', where
    //      x is some symbol specifying a Yama object, and detail either an
    //      input/output object read-from/written-to by the operation
    // 
    //      the alternative form 'in(x)...' may be used for scenarios in which 
    //      the input symbol x refers to an array of Yama objects
    // 
    //      in yama::context's low-level command interface, the aforementioned
    //      'symbols' take the form of either local object stack indices, or
    //      an lvalue to a particular yama::object_ref
    // 
    //      examples:
    //
    //          transaction: plus in(v0) in(v1) out(r)
    // 
    //          transaction: sum in(vs)... out(r)
    //
    //      (newtop & forget)
    // 
    //      in order to push new objects to a local object stack, transaction
    //      terms expressing output objects may be used w/ a special 'newtop'
    //      pseudo-index, which tells the Yama context to write the object
    //      output to a new object pushed to the stack, rather than onto an 
    //      existing object
    // 
    //      for output terms of an operation's transaction, the special 
    //      pseudo-objects 'newtop' and 'forget' may be used
    // 
    //      newtop specifies that the output object should be pushed to the
    //      end of the local object stack, instead of being written to an
    //      existing object
    // 
    //      newtop is Yama's mechanism for *pushing* to the local object stack
    // 
    //      forget specifies that no output should occur, instead just *forgetting*
    //      about the output object altogether, which can be useful for scenarios
    //      where, for example, a return value object from a call is unimportant
    //
    //      (crashing)
    //
    //      Yama VMs 'crash' when they are declared to be irrecoverably in error
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


    // local_t encapsulates an index into a local object stack

    using local_t = size_t;

    // newtop and forget pseudo-objects

    struct newtop_t final {};
    struct forget_t final {};

    constexpr auto newtop = newtop_t{};
    constexpr auto forget = forget_t{};


    // TODO: replace these later w/ more proper config options

    constexpr size_t max_call_frames = 32;
    constexpr size_t user_max_locals = 32;


    // the terms 'context' and 'Yama VM' may be used interchangeably

    class context final : public api_component {
    public:

        context(
            res<domain> dm, 
            std::shared_ptr<debug> dbg = nullptr);


        // get_domain returns the domain this context is associated w/

        res<domain> get_domain() const noexcept;

        // dm provides summary access to the domain of the context

        domain& dm() const noexcept;


        // get_mas returns the MAS used internally by this context
        
        // this MAS is provided by the domain upon context init

        res<mas> get_mas() const noexcept;


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

        object_ref ll_new_none();
        object_ref ll_new_int(int_t v);
        object_ref ll_new_uint(uint_t v);
        object_ref ll_new_float(float_t v);
        object_ref ll_new_bool(bool_t v);
        object_ref ll_new_char(char_t v);

        // ll_new_fn returns std::nullopt if f is not a function type

        std::optional<object_ref> ll_new_fn(type f);

        // it's okay to use ll_is_crashing and ll_crashes while crashing,
        // as the whole point of it is to detect it

        size_t ll_crashes() noexcept;                   // ll_crashes returns the number of times the context has crashed
        bool ll_is_crashing() noexcept;                 // ll_is_crashing queries if the context is crashing
        bool ll_is_user() noexcept;                     // ll_is_user returns if the current call frame is the user call frame
        size_t ll_max_call_frames() noexcept;           // ll_max_call_frames returns the max call stack height allowed
        size_t ll_call_frames() noexcept;               // ll_call_frames returns the number of call frames on the call stack
        size_t ll_max_locals() noexcept;                // ll_max_locals returns the max objects allowed on the local object stack
        size_t ll_locals() noexcept;                    // ll_locals returns the number of objects on the local object stack
        std::optional<object_ref> ll_local(local_t x);  // ll_local returns the object, if any, at x in the local object stack

        void ll_crash();                                // ll_crash induces a crash, failing quietly if already crashing

        // transaction: ll_call in(x) in(args)... out(ret)

        // ll_call performs a Yama call of call object x over argument objects 
        // args, writing the return value object to object ret

        // ll_call does not care what type ret is, as it overwrites ret's state

        // ll_call overloads w/ param 'f' specify a function type f, creating an 
        // object of type f directly in the new call frame to use for x

        // ll_call does not check if args types are correct to use for a call to x

        // ll_call does not check if the type of the return value object returned
        // by the call is correct for a call to x

        // ll_call crashes if x is out-of-bounds

        // ll_call crashes if indices in args are out-of-bounds

        // ll_call crashes if ret is out-of-bounds

        // ll_call crashes if the type of x is not a callable type

        // ll_call crashes if f is not a callable type

        // ll_call crashes if args.size() is not equal to the expected argument count

        // ll_call crashes for newtop_t overloads if pushing to the local object
        // stack would cause it to overflow

        // ll_call crashes if the call stack would overflow from pushing a new
        // call frame to it

        // ll_call crashes if no return value object is provided on the call's 
        // local object stack after call behaviour has completed execution

        // TODO: add overloads for specifying args via a pair of iterators

        cmd_status ll_call(local_t x, std::initializer_list<local_t> args, local_t ret);
        cmd_status ll_call(local_t x, std::initializer_list<local_t> args, newtop_t);
        cmd_status ll_call(local_t x, std::initializer_list<local_t> args, forget_t);
        cmd_status ll_call(borrowed_ref x, std::initializer_list<local_t> args, local_t ret);
        cmd_status ll_call(borrowed_ref x, std::initializer_list<local_t> args, newtop_t);
        cmd_status ll_call(borrowed_ref x, std::initializer_list<local_t> args, forget_t);
        cmd_status ll_call(type f, std::initializer_list<local_t> args, local_t ret);
        cmd_status ll_call(type f, std::initializer_list<local_t> args, newtop_t);
        cmd_status ll_call(type f, std::initializer_list<local_t> args, forget_t);

        // transaction: ll_put in(src) out(dest)

        // ll_put copies object src onto object dest

        // ll_put does not care what type dest is, as it overwrites dest's state

        // ll_put crashes if src is out-of-bounds
        
        // ll_put crashes if dest is out-of-bounds

        // ll_put crashes for newtop_t overloads if pushing to the local object
        // stack would cause it to overflow

        cmd_status ll_put(local_t src, local_t dest);
        cmd_status ll_put(local_t src, newtop_t);
        cmd_status ll_put(local_t src, forget_t);
        cmd_status ll_put(borrowed_ref src, local_t dest);
        cmd_status ll_put(borrowed_ref src, newtop_t);
        cmd_status ll_put(borrowed_ref src, forget_t);

        // ll_pop pops the first n objects from the local object stack

        // ll_pop will stop early if it runs out of objects to pop

        cmd_status ll_pop(size_t n = 1);


    private:

        res<domain> _dm;
        res<mas> _mas;

        std::allocator<void> _al;

        size_t _crashes = 0;
        bool _crashing = false;
        
        std::vector<internal::call_frame<decltype(_al)>> _callstk;


        internal::call_frame<decltype(_al)>& _top_cf();
        internal::call_frame<decltype(_al)>& _below_top_cf(); // returns cf just below the top one

        // _push_cf returns if successful, w/ it failing if pushing
        // would overflow the call stack

        bool _push_cf(size_t max_locals);
        void _pop_cf() noexcept;

        void _push_user_cf();


        // this needs to be called in ll_crash and ll_call (after call 
        // behaviour) in order to handle crash behaviour once call stack 
        // unwinding reaches the user call frame, which needs to be popped, 
        // and a new one pushed

        void _try_handle_crash_for_user_cf();


        // _call defines the *core behaviour* of ll_call, which is common
        // to all overloads

        std::optional<object_ref> _call(std::optional<stolen_ref> callobj, std::initializer_list<local_t> args);

        // overload behaviour is defined by composition of _call w/ below
        // for getting callobj, and handling what to do w/ return value

        std::optional<object_ref> _call_callobj_by_local(local_t x);
        std::optional<object_ref> _call_callobj_by_lvalue(borrowed_ref x);
        std::optional<object_ref> _call_callobj_by_fntype(type f);

        cmd_status _call_ret_to_local(std::optional<stolen_ref> ret, local_t target);
        cmd_status _call_ret_to_newtop(std::optional<stolen_ref> ret);
        cmd_status _call_ret_to_forget(std::optional<stolen_ref> ret);

        // turns out the out-of-bounds/overflow checks for ll_call outputting to
        // local/newtop need to occur BEFORE anything else, so we'll use these
        // methods to check these as a dirty little exception to our pattern

        bool _call_ret_to_local_precheck(local_t target);
        bool _call_ret_to_newtop_precheck();


        // ll_put is defined in a manner akin to how we defined ll_call, except
        // that we have no single *core behaviour* method

        std::optional<object_ref> _put_input_by_local(local_t src);
        std::optional<object_ref> _put_input_by_lvalue(borrowed_ref src);

        cmd_status _put_output_to_local(std::optional<stolen_ref> x, local_t dest);
        cmd_status _put_output_to_newtop(std::optional<stolen_ref> x);
        cmd_status _put_output_to_forget(std::optional<stolen_ref> x);
    };
}

