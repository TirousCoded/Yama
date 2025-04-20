

#pragma once


#include <cstdint>
#include <string>
#include <format>
#include <memory>
#include <array>

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

        general             = 1 << 0,           // otherwise uncategorized
        domain              = 1 << 1,           // domain behaviour trace
        compile             = 1 << 2,           // compiler behaviour trace
        compile_error       = 1 << 3,           // compiler error
        verif               = 1 << 4,           // static verification behaviour trace
        verif_error         = 1 << 5,           // static verification error
        verif_warning       = 1 << 6,           // static verification warning
        install             = 1 << 7,           // (parcel) install behaviour trace
        install_error       = 1 << 8,           // (parcel) install error
        import              = 1 << 9,           // (module) import behaviour trace
        import_error        = 1 << 10,          // (module) import error
        load                = 1 << 11,          // (type) loading behaviour trace
        load_error          = 1 << 12,          // (type) loading error
        ctx_panic           = 1 << 13,          // context panic
        ctx_llcmd           = 1 << 14,          // low-level command behaviour trace
        bcode_exec          = 1 << 15,          // bcode execution trace

        none                = 0,
        all                 = uint32_t(-1),

        errors              = compile_error
                            | verif_error
                            | install_error
                            | import_error
                            | load_error
                            | ctx_panic,

        warnings            = verif_warning,

        defaults            = general
                            | errors
                            | warnings
                            | ctx_panic,
    };


    // these are used to summarize usage of yama::dcat

    constexpr auto general_c            = dcat::general;
    constexpr auto domain_c             = dcat::domain;
    constexpr auto compile_c            = dcat::compile;
    constexpr auto compile_error_c      = dcat::compile_error;
    constexpr auto verif_c              = dcat::verif;
    constexpr auto verif_error_c        = dcat::verif_error;
    constexpr auto verif_warning_c      = dcat::verif_warning;
    constexpr auto install_c            = dcat::install;
    constexpr auto install_error_c      = dcat::install_error;
    constexpr auto import_c             = dcat::import;
    constexpr auto import_error_c       = dcat::import_error;
    constexpr auto load_c               = dcat::load;
    constexpr auto load_error_c         = dcat::load_error;
    constexpr auto ctx_panic_c          = dcat::ctx_panic;
    constexpr auto ctx_llcmd_c          = dcat::ctx_llcmd;
    constexpr auto bcode_exec_c         = dcat::bcode_exec;

    constexpr auto none_c               = dcat::none;
    constexpr auto all_c                = dcat::all;
    constexpr auto errors_c             = dcat::errors;
    constexpr auto warnings_c           = dcat::warnings;
    constexpr auto defaults_c           = dcat::defaults;
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

    // some quick-n'-dirty tests

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

        compile_file_not_found, // TODO: reimpl use of this in yama::domain
        compile_impl_internal, // internal error
        compile_impl_limits, // impl-defined limits exceeded
        compile_syntax_error,
        compile_name_conflict,
        compile_undeclared_name,
        compile_ambiguous_name,
        compile_undeclared_qualifier,
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
        compile_malformed_literal,
        compile_numeric_overflow,
        compile_numeric_underflow,
        compile_illegal_unicode,
        compile_invalid_operation,
        compile_wrong_arg_count,
        compile_invalid_import,
        compile_misplaced_import,
        compile_invalid_env,

        verif_param_name_count_mismatch,
        verif_type_callsig_invalid,
        verif_constsym_qualified_name_invalid,
        verif_constsym_callsig_invalid,
        verif_callsig_param_type_out_of_bounds,
        verif_callsig_return_type_out_of_bounds,
        verif_callsig_param_type_not_type_const,
        verif_callsig_return_type_not_type_const,
        verif_binary_is_empty,
        verif_RTop_does_not_exist,
        verif_RTop_wrong_type,
        verif_RA_out_of_bounds,
        verif_RA_wrong_type,
        verif_RB_out_of_bounds,
        verif_RB_wrong_type,
        verif_KoB_out_of_bounds,
        verif_KoB_not_object_const,
        verif_ArgB_out_of_bounds,
        verif_RA_and_RB_types_differ,
        verif_RA_and_KoB_types_differ,
        verif_RA_and_ArgB_types_differ,
        verif_ArgRs_out_of_bounds,
        verif_ArgRs_zero_objects,
        verif_ArgRs_illegal_callobj,
        verif_ParamArgRs_wrong_number,
        verif_ParamArgRs_wrong_types,
        verif_puts_PC_out_of_bounds,
        verif_fallthrough_puts_PC_out_of_bounds,
        verif_pushing_overflows,
        verif_violates_register_coherence,

        install_invalid_parcel,
        install_install_name_conflict,
        install_missing_dep_mapping,
        install_invalid_dep_mapping,
        install_dep_graph_cycle,

        import_module_not_found,
        import_invalid_module,

        load_type_not_found,
        load_kind_mismatch,
        load_callsig_mismatch,

        num, // not a valid dsignal
    };

    constexpr auto dsignals = (size_t)dsignal::num;


    inline std::string fmt_dsignal(dsignal sig) {
        static_assert(dsignals == 67);
        std::string result{};
#define _YAMA_ENTRY_(x) case dsignal:: x : result = #x ; break
        switch (sig) {

            _YAMA_ENTRY_(compile_file_not_found);
            _YAMA_ENTRY_(compile_impl_internal);
            _YAMA_ENTRY_(compile_impl_limits);
            _YAMA_ENTRY_(compile_syntax_error);
            _YAMA_ENTRY_(compile_name_conflict);
            _YAMA_ENTRY_(compile_undeclared_name);
            _YAMA_ENTRY_(compile_ambiguous_name);
            _YAMA_ENTRY_(compile_undeclared_qualifier);
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
            _YAMA_ENTRY_(compile_malformed_literal);
            _YAMA_ENTRY_(compile_numeric_overflow);
            _YAMA_ENTRY_(compile_numeric_underflow);
            _YAMA_ENTRY_(compile_illegal_unicode);
            _YAMA_ENTRY_(compile_invalid_operation);
            _YAMA_ENTRY_(compile_wrong_arg_count);
            _YAMA_ENTRY_(compile_invalid_import);
            _YAMA_ENTRY_(compile_misplaced_import);
            _YAMA_ENTRY_(compile_invalid_env);

            _YAMA_ENTRY_(verif_param_name_count_mismatch);
            _YAMA_ENTRY_(verif_type_callsig_invalid);
            _YAMA_ENTRY_(verif_constsym_qualified_name_invalid);
            _YAMA_ENTRY_(verif_constsym_callsig_invalid);
            _YAMA_ENTRY_(verif_callsig_param_type_out_of_bounds);
            _YAMA_ENTRY_(verif_callsig_return_type_out_of_bounds);
            _YAMA_ENTRY_(verif_callsig_param_type_not_type_const);
            _YAMA_ENTRY_(verif_callsig_return_type_not_type_const);
            _YAMA_ENTRY_(verif_binary_is_empty);
            _YAMA_ENTRY_(verif_RTop_does_not_exist);
            _YAMA_ENTRY_(verif_RTop_wrong_type);
            _YAMA_ENTRY_(verif_RA_out_of_bounds);
            _YAMA_ENTRY_(verif_RA_wrong_type);
            _YAMA_ENTRY_(verif_RB_out_of_bounds);
            _YAMA_ENTRY_(verif_RB_wrong_type);
            _YAMA_ENTRY_(verif_KoB_out_of_bounds);
            _YAMA_ENTRY_(verif_KoB_not_object_const);
            _YAMA_ENTRY_(verif_ArgB_out_of_bounds);
            _YAMA_ENTRY_(verif_RA_and_RB_types_differ);
            _YAMA_ENTRY_(verif_RA_and_KoB_types_differ);
            _YAMA_ENTRY_(verif_RA_and_ArgB_types_differ);
            _YAMA_ENTRY_(verif_ArgRs_out_of_bounds);
            _YAMA_ENTRY_(verif_ArgRs_zero_objects);
            _YAMA_ENTRY_(verif_ArgRs_illegal_callobj);
            _YAMA_ENTRY_(verif_ParamArgRs_wrong_number);
            _YAMA_ENTRY_(verif_ParamArgRs_wrong_types);
            _YAMA_ENTRY_(verif_puts_PC_out_of_bounds);
            _YAMA_ENTRY_(verif_fallthrough_puts_PC_out_of_bounds);
            _YAMA_ENTRY_(verif_pushing_overflows);
            _YAMA_ENTRY_(verif_violates_register_coherence);

            _YAMA_ENTRY_(install_invalid_parcel);
            _YAMA_ENTRY_(install_install_name_conflict);
            _YAMA_ENTRY_(install_missing_dep_mapping);
            _YAMA_ENTRY_(install_invalid_dep_mapping);
            _YAMA_ENTRY_(install_dep_graph_cycle);

            _YAMA_ENTRY_(import_module_not_found);
            _YAMA_ENTRY_(import_invalid_module);

            _YAMA_ENTRY_(load_type_not_found);
            _YAMA_ENTRY_(load_kind_mismatch);
            _YAMA_ENTRY_(load_callsig_mismatch);

        default: YAMA_DEADEND; break;
        }
#undef _YAMA_ENTRY
        return result;
    }


    // TODO: this class hasn't been unit tested

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


namespace yama {


    class null_debug final : public debug {
    public:
        inline null_debug(dcat cats = defaults_c) : debug(cats) {}
    protected:
        inline void do_log(dcat, const std::string&) override final {}
    };

    class stderr_debug final : public debug {
    public:
        inline stderr_debug(dcat cats = defaults_c) : debug(cats) {}
    protected:
        inline void do_log(dcat, const std::string& msg) override final { std::cerr << msg << '\n'; }
    };

    class stdout_debug final : public debug {
    public:
        inline stdout_debug(dcat cats = defaults_c) : debug(cats) {}
    protected:
        inline void do_log(dcat, const std::string& msg) override final { std::cout << msg << '\n'; }
    };


    // TODO: maybe rename to 'filter_debug'

    // proxy_debug is a general-purpose proxy for debug impls, used to provide
    // a way to restrict the categories of logs allowed upstream

    class proxy_debug final : public debug {
    public:
        const std::shared_ptr<debug> base = nullptr;


        // the final cats field of the proxy will equal base->cats & cats_mask

        proxy_debug(std::shared_ptr<debug> base, dcat cats_mask = all_c);


    protected:
        void do_log(dcat cat, const std::string& msg) override final;
        void do_raise(dsignal sig) override final;
    };

    // proxy_dbg returns a proxy_debug layer for base, w/ cats, or nullptr if base == nullptr

    // the final cats field of the proxy, if any, will equal base->cats & cats_mask

    // proxy_dbg exists due to the fact that YAMA_LOG only skips (potentially costly) formatted
    // string preparation if there is no debug layer provided to it (ie. it's provided nullptr),
    // meaning that situations where a proxy_debug is used, but where there is no upstream debug
    // layer, YAMA_LOG won't skip formatted string preparation, leading to unwanted allocations

    // proxy_dbg addresses this problem by acting as a ctor for proxy_debug when there is an
    // upstream debug layer, but returning nullptr instead if there isn't, which allows the
    // lack up upstream debug layer to *propagate* downstream

    // proxy_dbg by extension also helps avoid the heap allocation of unneeded proxy_debug objects

    std::shared_ptr<proxy_debug> proxy_dbg(std::shared_ptr<debug> base, dcat cats_mask = all_c);


    // TODO: this class hasn't been unit tested

    // debug impl which provides mechanism to *count* number of times a particular
    // dsignal has been raised (w/ this being useful in unit testing)

    // this impl can also act as a proxy for another debug object injected into it

    class dsignal_debug final : public debug {
    public:
        std::array<size_t, dsignals> counts = { 0 };
        const std::shared_ptr<debug> base = nullptr;


        // ctor doesn't provide explicit control over cats as all this
        // debug impl does w/ it is forward logs as a proxy

        // ctor sets cats == base->cats if base != nullptr

        // ctor sets cats == none_c if base == nullptr

        // TODO: I'm not 100% sure about this last part, but we need it if we
        //       want to make usage of this transparent (ie. so end-user can
        //       easily change categories w/out having to access underlying)

        // base->cats is set to dcat::all in order to allow for end-user
        // to modify debug categories properly (and transparently)

        dsignal_debug(std::shared_ptr<debug> base);


        size_t count(dsignal sig) const noexcept;
        void reset() noexcept;


    protected:
        void do_log(dcat cat, const std::string& msg) override final;
        void do_raise(dsignal sig) override final;
    };
}

