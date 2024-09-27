

#pragma once


#include <array>

#include "../core/debug.h"


namespace yama {


    // debug impl which provides mechanism to *count* number of times a particular
    // dsignal has been raised (w/ this being useful in unit testing)

    // this impl can also act as a proxy for another debug object injected into it


    // TODO: this class hasn't been unit tested

    class dsignal_debug final : public debug {
    public:

        std::array<size_t, dsignals> counts = { 0 };
        std::shared_ptr<debug> base = nullptr;


        // ctor doesn't provide explicit control over cats as all this
        // debug impl does w/ it is forward logs as a proxy

        // ctor sets cats == base->cats if base != nullptr

        // ctor sets cats == none_c if base == nullptr

        dsignal_debug(std::shared_ptr<debug> base);


        size_t count(dsignal sig) const noexcept;

        void reset() noexcept;


    protected:

        void do_log(dcat cat, const std::string& msg) override final;
        void do_raise(dsignal sig) override final;
    };
}

