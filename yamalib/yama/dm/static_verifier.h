

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

        bool _verify(const type_info& subject);
        void _begin_verify(const type_info& subject);
        void _end_verify(bool success);

        inline bool _verify_type_callsig_indices(const type_info& subject);
        inline bool _verify_linksym_callsigs(const type_info& subject);
        inline bool _verify_linksym_callsig(const type_info& subject, link_index index);
        inline bool _verify_linksym_callsig_indices(const type_info& subject, link_index index);
    };
}

