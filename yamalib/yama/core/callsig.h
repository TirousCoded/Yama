

#pragma once


#include "callsig_info.h"
#include "const_table.h"

#include "../internals/safeptr.h"


namespace yama {


    // callsig encapsulates a non-owning view of a callsig_info associated w/ a const_table

    class callsig final {
    public:
        // behaviour is undefined if consts is not appropriate to be used w/ info provided

        callsig(const callsig_info& info, const_table consts);

        callsig() = delete;
        callsig(const callsig&) = default;
        callsig(callsig&&) noexcept = default;
        ~callsig() noexcept = default;
        callsig& operator=(const callsig&) = default;
        callsig& operator=(callsig&&) noexcept = default;


        const callsig_info& info() const noexcept;
        size_t params() const noexcept;

        // TODO: when we add types w/ stubs in their otherwise resolved constant table, try
        //       to update our unit tests to cover the below behaviour
        //
        //       right now our unit tests don't really cover this behaviour

        // NOTE: param_type and return_type return std::nullopt if queried type's constant
        //       table index refers to a stub (including if it's out-of-bounds)
        //
        //       when equality comparing two callsigs, these std::nullopt(s) are considered,
        //       w/ equality being established even if the two callsigs have std::nullopt
        //       at a particular index for two different reasons

        std::optional<type> param_type(size_t index) const noexcept;
        std::optional<type> return_type() const noexcept;

        bool operator==(const callsig& other) const noexcept; // compares by value

        std::string fmt() const;


    private:
        internal::safeptr<const callsig_info> _info;
        const_table _consts;
    };
}


YAMA_SETUP_FORMAT(yama::callsig, x.fmt());

