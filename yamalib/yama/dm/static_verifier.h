

#pragma once


#include "../core/api_component.h"
#include "../core/type_data.h"


namespace yama::dm {


    // static_verifier performs static verification of type_data

    // static verification is performed upon the pushing of type_data,
    // to determine if it's valid for use

    // static verification occurs in the absence of linkage info, which
    // is established later during instantiation

    // once statically verified, type_data will be *marked* as verified,
    // which allows for easy checking of type_data validity


    class static_verifier final : public api_component {
    public:

        static_verifier(std::shared_ptr<debug> dbg = nullptr);


        // verify returns if subject passes static verification

        // verify returns true immediately if subject.verified() == true

        bool verify(const type_data& subject);


    private:

        bool _verify(const type_data& subject);
        void _begin_verify(const type_data& subject);
        void _end_verify(bool success);
        void _mark_as_verified(const type_data& subject);

        inline bool _verify_type_callsig(const type_data& subject);
        inline bool _verify_type_callsig_indices(const type_data& subject);
        inline bool _verify_linksym_callsigs(const type_data& subject);
        inline bool _verify_linksym_callsig(const type_data& subject, link_index index);
        inline bool _verify_linksym_callsig_indices(const type_data& subject, link_index index);
        inline bool _verify_fn_type_max_locals(const type_data& subject);
    };
}

