

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
    

    // type_info encapsulates details defining a Yama type (pre-instantiation)
    
    // these are meant to be really clean and nice to use, so as to be put in
    // the Yama API frontend for end-users thereof to use to define types

    struct primitive_info final {
        ptype                           ptype;          // the type of primitive this is


        bool operator==(const primitive_info&) const noexcept = default;

        std::string fmt(const char* tab = "    ") const;
    };

    struct function_info final {
        callsig_info                    callsig;        // the call signature
        call_fn                         call_fn;        // the call_fn encapsulating call behaviour
        size_t                          max_locals;     // the max local object stack height
        bc::code                        bcode;          // the bytecode (no static verif check if call_fn != bcode_call_fn)
        bc::syms                        bcodesyms;      // the bytecode symbol info


        bool operator==(const function_info&) const noexcept = default;

        std::string fmt(const const_table_info& consts, const char* tab = "    ") const;
    };

    struct type_info final {
        using info_t = std::variant<
            primitive_info, 
            function_info>;

        static_assert(std::variant_size_v<info_t> == kinds);


        str                             fullname;       // the fullname of the type
        const_table_info                consts;         // the constant table symbols
        info_t                          info;           // the kind-specific details of the type


        kind kind() const noexcept;

        std::optional<ptype> ptype() const noexcept;
        const callsig_info* callsig() const noexcept;
        std::optional<call_fn> call_fn() const noexcept;
        size_t max_locals() const noexcept; // returns 0 if the type is not callable
        const bc::code* bcode() const noexcept;
        const bc::syms* bcodesyms() const noexcept;

        bool operator==(const type_info&) const noexcept = default;

        std::string fmt(const char* tab = "    ") const;
    };
}

YAMA_SETUP_FORMAT(yama::primitive_info, x.fmt());
YAMA_SETUP_FORMAT(yama::type_info, x.fmt());

