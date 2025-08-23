

#pragma once


#include "callsig.h"
#include "const_table_ref.h"

#include "../internals/safeptr.h"


namespace yama {


    // callsig_ref encapsulates a non-owning view of a callsig associated w/ a const_table_ref.
    class callsig_ref final {
    public:
        // Behaviour is undefined if consts is not appropriate to be used w/ info provided.
        callsig_ref(const callsig& info, const_table_ref consts);

        callsig_ref() = delete;
        callsig_ref(const callsig_ref&) = default;
        callsig_ref(callsig_ref&&) noexcept = default;
        ~callsig_ref() noexcept = default;
        callsig_ref& operator=(const callsig_ref&) = default;
        callsig_ref& operator=(callsig_ref&&) noexcept = default;


        const callsig& info() const noexcept;
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

        std::optional<item_ref> param_type(size_t index) const noexcept;
        std::optional<item_ref> return_type() const noexcept;

        // Compares by value.
        bool operator==(const callsig_ref& other) const noexcept;

        std::string fmt() const;


    private:
        internal::safeptr<const callsig> _info;
        const_table_ref _consts;
    };
}

YAMA_SETUP_FORMAT(yama::callsig_ref, x.fmt());

