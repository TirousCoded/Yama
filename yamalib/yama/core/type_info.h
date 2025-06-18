

#pragma once


#include <vector>
#include <optional>
#include <variant>

#include "res.h"
#include "kind.h"
#include "ptype.h"
#include "call_fn.h"
#include "bcode.h"
#include "callsig_info.h"
#include "const_table_info.h"


namespace yama {
    

    class type_info;

    namespace internal {
        class base_info;
        template<std::derived_from<internal::base_info> T, typename... Args>
        inline type_info make_type_info(Args&&... args);
    }


    class type_info final {
    public:
        type_info() = delete;
        type_info(const type_info& other);
        type_info(type_info&&) noexcept = default;
        ~type_info() noexcept = default;
        type_info& operator=(const type_info& other);
        type_info& operator=(type_info&&) noexcept = default;


        // TODO: we don't really unit test owner_name/member_name for scenarios where
        //       owner/member name division is opposite of what is expected (ie. method
        //       expects name in form '<owner>.<member>', but name is instead just '<name>')

        // IMPORTANT: do keep in mind that most of the below methods involve a virtual
        //            method call

        str& unqualified_name() noexcept;
        const str& unqualified_name() const noexcept;
        const_table_info& consts() noexcept;
        const const_table_info& consts() const noexcept;
        kind kind() const noexcept;
        std::optional<ptype> ptype() const noexcept;
        const callsig_info* callsig() const noexcept;
        size_t max_locals() const noexcept; // returns 0 if the type is not callable
        std::optional<call_fn> call_fn() const noexcept;
        const bc::code* bcode() const noexcept;
        const bc::syms* bsyms() const noexcept;
        bool uses_bcode() const noexcept;
        str owner_name() const noexcept;
        str member_name() const noexcept;

        // TODO: operator== has not been unit tested

        // equality compares by value

        bool operator==(const type_info& other) const noexcept;
        
        // change_# methods fail quietly upon fail

        void change_unqualified_name(str x) noexcept;
        void change_consts(const_table_info x) noexcept;
        void change_ptype(yama::ptype x) noexcept;
        void change_callsig(callsig_info x) noexcept;
        void change_call_fn(yama::call_fn x) noexcept;
        void change_max_locals(size_t x) noexcept;
        void change_bcode(bc::code x) noexcept;
        void change_bsyms(bc::syms x) noexcept;

        std::string fmt(const char* tab = default_tab) const;
        std::string fmt_sym(size_t index) const;


    private:
        template<std::derived_from<internal::base_info> T, typename... Args>
        friend type_info yama::internal::make_type_info(Args&&... args);


        res<internal::base_info> _info;


        type_info(res<internal::base_info>&& info);
    };
}

YAMA_SETUP_FORMAT(yama::type_info, x.fmt());

namespace yama {


    // IMPORTANT: when updating below, be sure to update module_factory::add_# methods

    type_info make_primitive(
        const str& unqualified_name,
        const_table_info consts,
        ptype ptype);

    type_info make_function(
        const str& unqualified_name,
        const_table_info consts,
        callsig_info callsig,
        size_t max_locals,
        yama::call_fn call_fn);

    type_info make_function(
        const str& unqualified_name,
        const_table_info consts,
        callsig_info callsig,
        size_t max_locals,
        bc::code bcode,
        bc::syms bsyms = bc::syms{});
    
    type_info make_method(
        const str& unqualified_name,
        const_table_info consts,
        callsig_info callsig,
        size_t max_locals,
        yama::call_fn call_fn);

    type_info make_method(
        const str& unqualified_name,
        const_table_info consts,
        callsig_info callsig,
        size_t max_locals,
        bc::code bcode,
        bc::syms bsyms = bc::syms{});

    type_info make_struct(
        const str& unqualified_name,
        const_table_info consts);
}

namespace yama::internal {


    template<std::derived_from<internal::base_info> T, typename... Args>
    inline type_info make_type_info(Args&&... args) {
        return type_info(make_res<T>(std::forward<Args>(args)...));
    }


    class base_info {
    public:
        str                             unqualified_name;   // the unqualified name of the type
        const_table_info                consts;             // the constant table symbols


        base_info(
            const str& unqualified_name,
            const_table_info consts);

        virtual ~base_info() noexcept = default;


        inline bool uses_bcode() const noexcept { return get_call_fn() == bcode_call_fn; }
        std::string fmt(const char* tab) const;

        virtual res<base_info> clone() const = 0;
        virtual kind get_kind() const noexcept = 0;
        virtual std::string fmt_extension(const const_table_info& consts, const char* tab) const = 0;

        inline virtual std::optional<ptype> get_ptype() const noexcept { return std::nullopt; }
        inline virtual const callsig_info* get_callsig() const noexcept { return nullptr; }
        inline virtual size_t get_max_locals() const noexcept { return 0; }
        inline virtual std::optional<call_fn> get_call_fn() const noexcept { return std::nullopt; }
        inline virtual const bc::code* get_bcode() const noexcept { return nullptr; }
        inline virtual const bc::syms* get_bsyms() const noexcept { return nullptr; }

        inline virtual void set_ptype(yama::ptype x) noexcept {}
        inline virtual void set_callsig(callsig_info x) noexcept {}
        inline virtual void set_max_locals(size_t x) noexcept {}
        inline virtual void set_call_fn(yama::call_fn x) noexcept {}
        inline virtual void set_bcode(bc::code x) noexcept {}
        inline virtual void set_bsyms(bc::syms x) noexcept {}
    };

    class primitive_info final : public base_info {
    public:
        yama::ptype                     ptype;              // the type of primitive this is


        primitive_info(
            const str& unqualified_name,
            const_table_info consts,
            yama::ptype ptype);


        res<base_info> clone() const override final;
        inline yama::kind get_kind() const noexcept override final { return kind::primitive; }
        std::string fmt_extension(const const_table_info& consts, const char* tab) const override final;

        inline std::optional<yama::ptype> get_ptype() const noexcept override final { return ptype; }

        inline void set_ptype(yama::ptype x) noexcept override final { ptype = x; }
    };

    class callable_type_info : public base_info {
    public:
        callsig_info                    callsig;            // the call signature
        size_t                          max_locals;         // the max local object stack height
        yama::call_fn                   call_fn;            // the call_fn encapsulating call behaviour
        bc::code                        bcode;              // the bytecode (no static verif check if call_fn != bcode_call_fn)
        bc::syms                        bsyms;              // the bytecode symbol info


        callable_type_info(
            const str& unqualified_name,
            const_table_info consts,
            callsig_info callsig,
            size_t max_locals,
            yama::call_fn call_fn);
        callable_type_info(
            const str& unqualified_name,
            const_table_info consts,
            callsig_info callsig,
            size_t max_locals,
            bc::code bcode,
            bc::syms bsyms);


        std::string fmt_extension(const const_table_info& consts, const char* tab) const override final;

        inline const callsig_info* get_callsig() const noexcept override final { return &callsig; }
        inline size_t get_max_locals() const noexcept override final { return max_locals; }
        inline std::optional<yama::call_fn> get_call_fn() const noexcept override final { return call_fn; }
        inline const bc::code* get_bcode() const noexcept override final { return &bcode; }
        inline const bc::syms* get_bsyms() const noexcept override final { return &bsyms; }

        inline void set_callsig(callsig_info x) noexcept override final { callsig = std::move(x); }
        inline void set_max_locals(size_t x) noexcept override final { max_locals = x; }
        inline void set_call_fn(yama::call_fn x) noexcept override final { call_fn = x; }
        inline void set_bcode(bc::code x) noexcept override final { bcode = std::move(x); }
        inline void set_bsyms(bc::syms x) noexcept override final { bsyms = std::move(x); }
    };
    
    class function_info final : public callable_type_info {
    public:
        function_info(
            const str& unqualified_name,
            const_table_info consts,
            callsig_info callsig,
            size_t max_locals,
            yama::call_fn call_fn);
        function_info(
            const str& unqualified_name,
            const_table_info consts,
            callsig_info callsig,
            size_t max_locals,
            bc::code bcode,
            bc::syms bsyms);


        res<base_info> clone() const override final;
        inline yama::kind get_kind() const noexcept override final { return kind::function; }
    };

    class method_info final : public callable_type_info {
    public:
        method_info(
            const str& unqualified_name,
            const_table_info consts,
            callsig_info callsig,
            size_t max_locals,
            yama::call_fn call_fn);
        method_info(
            const str& unqualified_name,
            const_table_info consts,
            callsig_info callsig,
            size_t max_locals,
            bc::code bcode,
            bc::syms bsyms);


        res<base_info> clone() const override final;
        inline yama::kind get_kind() const noexcept override final { return kind::method; }
    };

    class struct_info final : public base_info {
    public:
        struct_info(
            const str& unqualified_name,
            const_table_info consts);


        res<base_info> clone() const override final;
        inline yama::kind get_kind() const noexcept override final { return kind::struct0; }
        std::string fmt_extension(const const_table_info& consts, const char* tab) const override final;
    };
}

