

#pragma once


#include "res.h"
#include "mas.h"
#include "domain.h"


namespace yama {


    class context final : public qs::system<qtype> {
    public:

        context(
            res<domain> dm, 
            std::shared_ptr<debug> dbg = nullptr);


        // get_domain returns the domain this context is associated w/

        // dm is get_domain, but w/ a shorter name

        res<domain> get_domain() const noexcept;
        res<domain> dm() const noexcept;


        // get_mas returns the MAS used internally by this context
        
        // this MAS is provided by the domain upon context initialization

        res<mas> get_mas() const noexcept;


        //


    protected:

        qs::untyped_provider<qtype>* get_provider(qtype qtype) const noexcept override final;
        void do_discard_all() override final;


    private:

        res<mas> _mas;
        res<domain> _dm;
    };
}

