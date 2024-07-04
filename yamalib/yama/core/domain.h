

#pragma once


#include "res.h"
#include "mas.h"

#include "../query-systems/system.h"


namespace yama {


    class context;


    class domain : public qs::system {
    public:

        friend class yama::context;


        domain(
            res<mas> mas,
            std::shared_ptr<debug> dbg = nullptr);


        // get_mas returns the MAS used internally by this domain

        res<mas> get_mas() const noexcept;


    protected:

        // get_mas_for_context creates an MAS for a yama::context
        // associated w/ this domain to use

        // this MAS is expected to be fast and w/out synchronization,
        // outside of things like chunk allocs for pool MAS impls

        // if applicable, this MAS should be upstream to the MAS of
        // the domain

        virtual res<mas> get_mas_for_context() = 0;


    private:

        res<mas> _mas;
    };
}

