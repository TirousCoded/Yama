

#pragma once


#include <vector>
#include <optional>
#include <variant>

#include "kind.h"
#include "ptype.h"
#include "call_fn.h"
#include "linksym.h"
#include "callsig_info.h"


namespace yama {
    

    // type_info encapsulates details defining a Yama type (pre-instantiation)
    
    // these are meant to be really clean and nice to use, so as to be put in
    // the Yama API frontend for end-users thereof to use to define types

    struct primitive_info final {
        ptype                           ptype;          // the type of primitive this is
    };

    struct function_info final {
        callsig_info                    callsig;        // the call signature
        call_fn                         call_fn;        // the call_fn encapsulating call behaviour
        size_t                          locals;         // the call frame's local register table size
    };

    struct type_info final {
        using info_t = std::variant<
            primitive_info, 
            function_info>;

        str                             fullname;       // the fullname of the type
        std::vector<linksym>            linksyms;       // the link symbol vector
        info_t                          info;           // the kind-specific details of the type


        kind kind() const noexcept;

        std::optional<ptype> ptype() const noexcept;
        const callsig_info* callsig() const noexcept;
        std::optional<call_fn> call_fn() const noexcept;
        size_t locals() const noexcept; // returns 0 if the type is not callable
    };
}

