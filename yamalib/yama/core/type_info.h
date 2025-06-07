

#pragma once


#include <vector>
#include <optional>
#include <variant>

#include "kind.h"
#include "ptype.h"
#include "call_fn.h"
#include "bcode.h"
#include "callsig_info.h"
#include "const_table_info.h"


namespace yama {
    

    // type_info encapsulates details defining a (non-loaded) Yama type
    
    // these are meant to be really clean and nice to use, so as to be put in
    // the Yama API frontend for end-users thereof to use to define types

    struct primitive_info final {
        ptype                           ptype;          // the type of primitive this is


        bool operator==(const primitive_info&) const noexcept = default;

        std::string fmt(const char* tab = "    ") const;


        // TODO: create has not been unit tested

        static primitive_info create(yama::ptype ptype);
    };

    struct function_info final {
        callsig_info                    callsig;        // the call signature
        call_fn                         call_fn;        // the call_fn encapsulating call behaviour
        size_t                          max_locals;     // the max local object stack height
        bc::code                        bcode;          // the bytecode (no static verif check if call_fn != bcode_call_fn)
        bc::syms                        bsyms;          // the bytecode symbol info


        // TODO: this has not been unit tested

        bool uses_bcode() const noexcept;


        bool operator==(const function_info&) const noexcept = default;

        std::string fmt(const const_table_info& consts, const char* tab = "    ") const;


        // TODO: create has not been unit tested

        static function_info create(callsig_info callsig, size_t max_locals, yama::call_fn call_fn);
        static function_info create(callsig_info callsig, size_t max_locals, bc::code bcode, bc::syms bsyms);
    };

    struct struct_info final {
        //


        bool operator==(const struct_info&) const noexcept = default;

        std::string fmt(const char* tab = "    ") const;


        // TODO: create has not been unit tested

        static struct_info create();
    };

    struct type_info final {
        using info_t = std::variant<
            primitive_info,
            function_info,
            struct_info>;

        static_assert(std::variant_size_v<info_t> == kinds);


        str                             unqualified_name;   // the unqualified name of the type
        const_table_info                consts;             // the constant table symbols
        info_t                          info;               // the kind-specific details of the type


        kind kind() const noexcept;

        std::optional<ptype> ptype() const noexcept;
        const callsig_info* callsig() const noexcept;
        std::optional<call_fn> call_fn() const noexcept;
        size_t max_locals() const noexcept; // returns 0 if the type is not callable
        const bc::code* bcode() const noexcept;
        const bc::syms* bsyms() const noexcept;

        // TODO: this has not been unit tested

        bool uses_bcode() const noexcept;

        bool operator==(const type_info&) const noexcept = default;

        std::string fmt(const char* tab = "    ") const;
        std::string fmt_sym(size_t index) const;


        // TODO: create has not been unit tested

        static type_info create(const str& unqualified_name, const_table_info consts, info_t info);
    };
}

YAMA_SETUP_FORMAT(yama::primitive_info, x.fmt());
YAMA_SETUP_FORMAT(yama::struct_info, x.fmt());
YAMA_SETUP_FORMAT(yama::type_info, x.fmt());

namespace yama {


    // IMPORTANT: when updating below, be sure to update module_factory::add_# methods

    // TODO: these have not been unit tested

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

    type_info make_struct(
        const str& unqualified_name,
        const_table_info consts);
}

