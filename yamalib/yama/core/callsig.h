

#pragma once


#include "callsig_info.h"
#include "links_view.h"


namespace yama {


    // TODO: I've chosen to not make link index info available to yama::callsig
    //       in case I in the future want to decouple yama::callsig from yama::type,
    //       and allow for the definition of yama::callsig w/out underlying 
    //       yama::callsig_info associations


    // NOTE: yama::callsig encapsulates a yama::callsig_info extended
    //       by yama::type linkage information


    class callsig final {
    public:

        // behaviour is undefined if links is not appropriate to be
        // used w/ info provided

        callsig(const callsig_info& info, links_view links);

        callsig() = delete;
        callsig(const callsig&) = default;
        callsig(callsig&&) noexcept = default;

        ~callsig() noexcept = default;

        callsig& operator=(const callsig&) = default;
        callsig& operator=(callsig&&) noexcept = default;


        // params returns the number of params in the callsig

        size_t params() const noexcept;

        // param_type returns the param type at index in the callsig, if any

        // param_type returns std::nullopt if index is out-of-bounds

        // param_type returns std::nullopt if the link table index associated
        // w/ the param index has no associated link table data (due to
        // index out-of-bounds or due to entry being a stub)

        std::optional<type> param_type(size_t index) const noexcept;

        // return_type returns the return type of the callsig, if any

        // return_type returns std::nullopt if the link table index associated
        // for the return type has no associated link table data (due to
        // index out-of-bounds or due to entry being a stub)

        std::optional<type> return_type() const noexcept;


        // yama::callsig are compared by value

        // TODO: rewrite the below to be less verbose

        // param_type/return_type usages which resolve to std::nullopt are 
        // considered equal if their corresponding usages in the other callsig
        // also resolve to std::nullopt, w/out regard for why they both resulted
        // in std::nullopt (ie. they can be for different reasons and still be
        // considered equal)

        bool operator==(const callsig& other) const noexcept;


        std::string fmt() const;


    private:

        const callsig_info* _info;
        links_view _links;


        inline const callsig_info& _get_info() const noexcept {
            YAMA_ASSERT(_info);
            return *_info;
        }
    };
}


YAMA_SETUP_FORMAT(yama::callsig, x.fmt());

