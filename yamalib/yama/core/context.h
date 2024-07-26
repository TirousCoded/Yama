

#pragma once


#include "res.h"
#include "api_component.h"
#include "mas.h"
#include "domain.h"
#include "command_api.h"


namespace yama {


    class context final : public api_component {
    public:

        context(
            res<domain> dm, 
            std::shared_ptr<debug> dbg = nullptr);


        // get_domain returns the domain this context is associated w/

        res<domain> get_domain() const noexcept;

        // dm provides summary access to the domain of the context

        domain& dm() const noexcept;


        // get_mas returns the MAS used internally by this context
        
        // this MAS is provided by the domain upon context init

        res<mas> get_mas() const noexcept;


        // TODO: while the methods of command_api will be unit tested, the
        //       yama::context::cmd method itself hasn't really been

        // cmd returns a command API object used to query/manipulate
        // the state of the context at a low-level

        command_api cmd() noexcept;


    private:

        res<domain> _dm;
        res<mas> _mas;
    };
}

