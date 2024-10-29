

#pragma once


#include <cstdint>
#include <string>
#include <format>
#include <memory>

#include "macros.h"
#include "asserts.h"


namespace yama {


    // Yama uses 'debug layers' to report diagnostics to the end-user

    // debug layers are placed *in between* API components and the end-user,
    // and they moniter interactions between them, as well as outputting
    // diagnostic logs generally

    // debug layers filter what to output w/ 'categories', which specify
    // what they should, and shouldn't, be sensitive to

    // quality-of-life macros are provided to help perform outputs to the
    // debug layer in a way that helps avoid unneeded code being executed
    // when a debug layer is not sensitive to a given category


    // enum of debug layer categories

    enum class dcat : uint32_t {
        // this enum is intended to define bit flags

        none            = 0,
        all             = uint32_t(-1),
        defaults        = all,

        general         = 1 << 0,           // general is for all otherwise uncategorized debug logs
        compile         = 1 << 1,           // compiler debug logs
        verify          = 1 << 2,           // static verification debug logs
        instantiate     = 1 << 3,           // type instantiation debug logs
        ctx_panic       = 1 << 4,           // context panic debug logs
        ctx_llcmd       = 1 << 5,           // low-level command behaviour debug logs
        bcode_exec      = 1 << 6,           // bcode execution trace debug logs
    };


    // these are used to summarize usage of yama::dcat

    constexpr auto none_c           = dcat::none;
    constexpr auto all_c            = dcat::all;
    constexpr auto defaults_c       = dcat::defaults;

    constexpr auto general_c        = dcat::general;
    constexpr auto compile_c        = dcat::compile;
    constexpr auto verify_c         = dcat::verify;
    constexpr auto instantiate_c    = dcat::instantiate;
    constexpr auto ctx_panic_c      = dcat::ctx_panic;
    constexpr auto ctx_llcmd_c      = dcat::ctx_llcmd;
    constexpr auto bcode_exec_c     = dcat::bcode_exec;
}

// TODO: the below have not been unit tested

constexpr yama::dcat operator&(yama::dcat lhs, yama::dcat rhs) noexcept {
    return yama::dcat(uint32_t(lhs) & uint32_t(rhs));
}

constexpr yama::dcat operator|(yama::dcat lhs, yama::dcat rhs) noexcept {
    return yama::dcat(uint32_t(lhs) | uint32_t(rhs));
}

constexpr yama::dcat operator^(yama::dcat lhs, yama::dcat rhs) noexcept {
    return yama::dcat(uint32_t(lhs) ^ uint32_t(rhs));
}

constexpr yama::dcat operator~(yama::dcat rhs) noexcept {
    return yama::dcat(~uint32_t(rhs));
}

constexpr yama::dcat& operator&=(yama::dcat& lhs, yama::dcat rhs) noexcept {
    lhs = lhs & rhs;
    return lhs;
}

constexpr yama::dcat& operator|=(yama::dcat& lhs, yama::dcat rhs) noexcept {
    lhs = lhs | rhs;
    return lhs;
}

constexpr yama::dcat& operator^=(yama::dcat& lhs, yama::dcat rhs) noexcept {
    lhs = lhs ^ rhs;
    return lhs;
}

namespace yama {


    // check returns if cat contains the flags of expect

    constexpr bool check(dcat cat, dcat expect) noexcept {
        return (cat & expect) == expect;
    }

    // TODO: maybe update the below later to be properly comprehensive

    // NOTE: some quick-n'-dirty (and notably non-comprehensive) static_assert-based
    //       unit tests of yama::check

    static_assert(check(none_c, none_c));
    static_assert(check(all_c, none_c));
    static_assert(check(all_c, all_c));

    static_assert(!check(none_c, all_c));

    static_assert(check(all_c, general_c));


    // debug 'signals' are used to signal to the debug layer that a certain event has
    // occurred within the system

    // no additional details are provided except a dsignal signal code

    // debug signals are not affected by enabling/disabling debug categories, but are
    // intended to each *belong to* a dcat

    // the main purpose of this system is to expose certain forms of expected internal
    // behaviour to the frontend in order to better unit test it

    enum class dsignal : uint32_t {
        // all dsignal values should be prefixed by the name of the dcat they belong to
        
        compile_file_not_found,
        compile_impl_internal, // internal error
        compile_impl_limits, // impl-defined limits exceeded
        compile_syntax_error,
        compile_name_conflict,
        compile_undeclared_name,
        compile_type_mismatch,
        compile_not_a_type,
        compile_not_an_expr,
        compile_nonlocal_var,
        compile_local_fn,
        compile_invalid_local_var,
        compile_invalid_param_list,
        compile_nonassignable_expr,
        compile_not_in_loop,
        compile_no_return_stmt,
        compile_numeric_overflow,
        compile_numeric_underflow,
        compile_illegal_unicode,
        compile_invalid_operation,
        compile_wrong_arg_count,

        verify_type_callsig_invalid,
        verify_constsym_callsig_invalid,
        verify_callsig_param_type_out_of_bounds,
        verify_callsig_return_type_out_of_bounds,
        verify_callsig_param_type_not_type_const,
        verify_callsig_return_type_not_type_const,
        verify_binary_is_empty,
        verify_RA_out_of_bounds,
        verify_RA_wrong_type,
        verify_RA_illegal_callobj,
        verify_RB_out_of_bounds,
        verify_RC_out_of_bounds,
        verify_RC_wrong_type,
        verify_KoB_out_of_bounds,
        verify_KoB_not_object_const,
        verify_ArgB_out_of_bounds,
        verify_RA_and_RB_types_differ,
        verify_RA_and_KoB_types_differ,
        verify_RA_and_ArgB_types_differ,
        verify_ArgRs_out_of_bounds,
        verify_ArgRs_zero_objects,
        verify_ParamArgRs_wrong_number,
        verify_ParamArgRs_wrong_types,
        verify_puts_PC_out_of_bounds,
        verify_fallthrough_puts_PC_out_of_bounds,
        verify_violates_register_coherence,

        instantiate_type_already_instantiated,
        instantiate_type_not_found,
        instantiate_kind_mismatch,
        instantiate_callsig_mismatch,

        num, // not a valid dsignal
    };

    constexpr auto dsignals = (size_t)dsignal::num;


    inline std::string fmt_dsignal(dsignal sig) {
        static_assert(dsignals == 51);
        std::string result{};
#define _YAMA_ENTRY_(x) case dsignal:: x : result = #x ; break
        switch (sig) {

            _YAMA_ENTRY_(compile_file_not_found);
            _YAMA_ENTRY_(compile_impl_internal);
            _YAMA_ENTRY_(compile_impl_limits);
            _YAMA_ENTRY_(compile_syntax_error);
            _YAMA_ENTRY_(compile_name_conflict);
            _YAMA_ENTRY_(compile_undeclared_name);
            _YAMA_ENTRY_(compile_type_mismatch);
            _YAMA_ENTRY_(compile_not_a_type);
            _YAMA_ENTRY_(compile_not_an_expr);
            _YAMA_ENTRY_(compile_nonlocal_var);
            _YAMA_ENTRY_(compile_local_fn);
            _YAMA_ENTRY_(compile_invalid_local_var);
            _YAMA_ENTRY_(compile_invalid_param_list);
            _YAMA_ENTRY_(compile_nonassignable_expr);
            _YAMA_ENTRY_(compile_not_in_loop);
            _YAMA_ENTRY_(compile_no_return_stmt);
            _YAMA_ENTRY_(compile_numeric_overflow);
            _YAMA_ENTRY_(compile_numeric_underflow);
            _YAMA_ENTRY_(compile_illegal_unicode);
            _YAMA_ENTRY_(compile_invalid_operation);
            _YAMA_ENTRY_(compile_wrong_arg_count);

            _YAMA_ENTRY_(verify_type_callsig_invalid);
            _YAMA_ENTRY_(verify_constsym_callsig_invalid);
            _YAMA_ENTRY_(verify_callsig_param_type_out_of_bounds);
            _YAMA_ENTRY_(verify_callsig_return_type_out_of_bounds);
            _YAMA_ENTRY_(verify_callsig_param_type_not_type_const);
            _YAMA_ENTRY_(verify_callsig_return_type_not_type_const);
            _YAMA_ENTRY_(verify_binary_is_empty);
            _YAMA_ENTRY_(verify_RA_out_of_bounds);
            _YAMA_ENTRY_(verify_RA_wrong_type);
            _YAMA_ENTRY_(verify_RA_illegal_callobj);
            _YAMA_ENTRY_(verify_RB_out_of_bounds);
            _YAMA_ENTRY_(verify_RC_out_of_bounds);
            _YAMA_ENTRY_(verify_RC_wrong_type);
            _YAMA_ENTRY_(verify_KoB_out_of_bounds);
            _YAMA_ENTRY_(verify_KoB_not_object_const);
            _YAMA_ENTRY_(verify_ArgB_out_of_bounds);
            _YAMA_ENTRY_(verify_RA_and_RB_types_differ);
            _YAMA_ENTRY_(verify_RA_and_KoB_types_differ);
            _YAMA_ENTRY_(verify_RA_and_ArgB_types_differ);
            _YAMA_ENTRY_(verify_ArgRs_out_of_bounds);
            _YAMA_ENTRY_(verify_ArgRs_zero_objects);
            _YAMA_ENTRY_(verify_ParamArgRs_wrong_number);
            _YAMA_ENTRY_(verify_ParamArgRs_wrong_types);
            _YAMA_ENTRY_(verify_puts_PC_out_of_bounds);
            _YAMA_ENTRY_(verify_fallthrough_puts_PC_out_of_bounds);
            _YAMA_ENTRY_(verify_violates_register_coherence);

            _YAMA_ENTRY_(instantiate_type_already_instantiated);
            _YAMA_ENTRY_(instantiate_type_not_found);
            _YAMA_ENTRY_(instantiate_kind_mismatch);
            _YAMA_ENTRY_(instantiate_callsig_mismatch);

        default: YAMA_DEADEND; break;
        }
#undef _YAMA_ENTRY
        return result;
    }


    // the base class of Yama debug layers

    class debug : public std::enable_shared_from_this<debug> {
    public:

        dcat cats; // categories to filter by


        debug(dcat cats);

        virtual ~debug() noexcept = default;


        bool has_cat(dcat cat) const noexcept;  // has_cat returns if output will occur w/ under categor(ies) cat

        void add_cat(dcat cat) noexcept;        // add_cat adds debug categor(ies) cat
        void remove_cat(dcat cat) noexcept;     // remove_cat removes debug categor(ies) cat


        // log performs a debug log w/ category cat

        // no log is performed if has_cat(cat) == false

        void log(dcat cat, const std::string& msg);

        template<typename... Args>
        inline void log(dcat cat, std::format_string<Args...> fmt, Args&&... args) {
            log(cat, std::format(fmt, std::forward<Args&&>(args)...));
        }

        void raise(dsignal sig); // raises debug signal sig


    protected:

        virtual void do_log(dcat cat, const std::string& msg) = 0;
        virtual void do_raise(dsignal sig) {}
    };
}


// quality-of-life helper macros

// these expect debug_layer_ptr to be some kind of pointer (raw or fancy)

#define YAMA_LOG(debug_layer_ptr, cat, fmt, ...) \
YAMA_COND( \
((debug_layer_ptr) && (debug_layer_ptr)->has_cat((cat))), \
(debug_layer_ptr)->log((cat), (fmt), __VA_ARGS__))

#define YAMA_RAISE(debug_layer_ptr, sig) \
YAMA_COND( \
(debug_layer_ptr), \
(debug_layer_ptr)->raise((sig)))

