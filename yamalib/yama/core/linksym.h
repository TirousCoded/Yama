

#pragma once


#include "general.h"
#include "kind.h"
#include "callsig.h"


namespace yama {


    /*
        -- link (symbol) tables --

            the behaviour of a type (especially a function-like type) is governed
            in part by said type's 'link table', which provides an array of
            links to other types which the type may access in order to operate

            link tables make it so that within their associated types, the
            types the table links can be identified by an index value

            these link tables are generated from 'link symbol tables',
            which get linked into link tables during type instantation

            each link symbol is a decorated fullname string which is used to
            query a corresponding type, according to the semantics of the system
            performing the instantiation

            each link symbol carries three pieces of information:

                1) a decorated fullname string used to query a corresponding yama::type

                2) a yama::kind detailing the expected kind of type

                3) (optionally) a yama::callsig detailing the expected type call signature
    */


    struct linksym final {
        str                     fullname;   // the fullname of the type
        kind                    kind;       // the expected kind of type
        std::optional<callsig>  callsig;    // the expected call signature, if any


        // TODO: operator== has not been unit tested

        inline bool operator==(const linksym& other) const noexcept {
            return
                fullname == other.fullname &&
                kind == other.kind &&
                callsig == other.callsig;
        }


        template<link_formatter Formatter>
        inline std::string fmt(Formatter fmt) const {
            return
                callsig
                ? std::format("({}) {} ( {} )", kind, fullname, callsig->fmt(fmt))
                : std::format("({}) {}", kind, fullname);
        }
    };


    // TODO: make_linksym has not been unit tested

    inline linksym make_linksym(str fullname, kind kind) {
        return linksym{ std::move(fullname), kind, std::nullopt };
    }

    inline linksym make_linksym(str fullname, kind kind, callsig callsig) {
        return linksym{ std::move(fullname), kind, std::make_optional(std::move(callsig)) };
    }
}

