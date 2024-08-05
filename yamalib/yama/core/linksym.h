

#pragma once


#include "general.h"
#include "kind.h"
#include "callsig_info.h"


namespace yama {


    // IMPORTANT:
    //      this code is provided to allow for the end-user to define their own
    //      types, which they can then push to a yama::domain
    //
    //      this is part of the frontend, but being technical and niche, should
    //      avoid being liberally exposed on the interfaces of things like 
    //      yama::type, yama::callsig, etc.


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

                3) (optionally) a callsig string detailing the expected type call signature
                    * this uses a yama::str formatted string of the expected yama::callsig_info
    */


    struct linksym final {
        str                         fullname;   // the fullname of the type
        kind                        kind;       // the expected kind of type
        std::optional<callsig_info> callsig;    // the expected call signature, if any


        // TODO: operator== has not been unit tested

        inline bool operator==(const linksym& other) const noexcept {
            return
                fullname == other.fullname &&
                kind == other.kind &&
                callsig == other.callsig;
        }


        inline std::string fmt(std::span<const linksym> linksyms) const {
            return
                callsig
                ? std::format("({}) {} [{}]", kind, fullname, callsig->fmt(linksyms))
                : std::format("({}) {}", kind, fullname);
        }
    };


    // TODO: make_linksym has not been unit tested

    inline linksym make_linksym(str fullname, kind kind) {
        return linksym{ std::move(fullname), kind, std::nullopt };
    }

    inline linksym make_linksym(str fullname, kind kind, callsig_info callsig) {
        return linksym{ std::move(fullname), kind, std::make_optional(std::move(callsig)) };
    }


    // TODO: linksyms_factory has not been unit tested

    class linksyms_factory final {
    public:

        linksyms_factory() = default;
        linksyms_factory(const linksyms_factory&) = delete;
        linksyms_factory(linksyms_factory&&) noexcept = default;
        ~linksyms_factory() noexcept = default;
        linksyms_factory& operator=(const linksyms_factory&) = delete;
        linksyms_factory& operator=(linksyms_factory&&) noexcept = default;


        static_assert(kinds == 2);

        // these are used to add new linksym objects to the linksyms vector

        linksyms_factory& primitive(str fullname);
        linksyms_factory& function(str fullname, callsig_info callsig);


        // done returns the linksyms vector produced

        // linksyms_factory behaviour is undefined after calling done

        std::vector<linksym> done();


    private:

        std::vector<linksym> _result;
    };
}

