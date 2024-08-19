

#pragma once


#include "../core/api_component.h"
#include "../core/type_info.h"


namespace yama::dm {


    // static_verifier performs static verification of type_data

    // static verification is performed upon the pushing of type_data,
    // to determine if it's valid for use

    // static verification occurs in the absence of linkage info, which
    // is established later during instantiation


    class static_verifier final : public api_component {
    public:

        static_verifier(std::shared_ptr<debug> dbg = nullptr);


        // verify returns if subject passes static verification

        bool verify(const type_info& subject);


    private:

        struct callsig_report final {
            bool indices_are_in_bounds          = true;
            bool indices_specify_type_consts    = true;
        };


        bool _verify(const type_info& subject);
        void _begin_verify(const type_info& subject);
        void _end_verify(bool success);

        bool _verify_type(const type_info& subject);
        bool _verify_type_callsig(const type_info& subject);

        bool _verify_constant_symbols(const type_info& subject);
        bool _verify_constant_symbol(const type_info& subject, const_t index);
        bool _verify_constant_symbol_callsig(const type_info& subject, const_t index);

        callsig_report gen_callsig_report(const type_info& subject, const callsig_info* callsig);
    };
}

